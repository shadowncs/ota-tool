package payload

import (
	"bytes"
	"compress/bzip2"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"sort"
	"sync"

	"github.com/EmilyShepherd/ota-tool/chromeos_update_engine"
	humanize "github.com/dustin/go-humanize"
	"github.com/golang/protobuf/proto"
	xz "github.com/spencercw/go-xz"
	"github.com/vbauerster/mpb/v5"
	"github.com/vbauerster/mpb/v5/decor"
)

type FullReader interface {
	io.Reader
	io.ReaderAt
}

type request struct {
	partition       *PartitionUpdate
	sourceDirectory string
	targetDirectory string
}

// Payload is a new format for the Android OTA/Firmware update files since Android Oreo
type Payload struct {
	file                 FullReader
	header               *payloadHeader
	deltaArchiveManifest *DeltaArchiveManifest
	signatures           *Signatures

	concurrency int

	metadataSize int64
	dataOffset   int64
	initialized  bool

	requests chan *request
	workerWG sync.WaitGroup
	progress *mpb.Progress
}

const payloadHeaderMagic = "CrAU"
const brilloMajorPayloadVersion = 2
const blockSize = 4096

type payloadHeader struct {
	Version              uint64
	ManifestLen          uint64
	MetadataSignatureLen uint32
	Size                 uint64

	payload *Payload
}

func (ph *payloadHeader) ReadFromPayload() error {
	buf := make([]byte, 4)
	if _, err := ph.payload.file.Read(buf); err != nil {
		return err
	}
	if string(buf) != payloadHeaderMagic {
		return fmt.Errorf("Invalid payload magic: %s", buf)
	}

	// Read Version
	buf = make([]byte, 8)
	if _, err := ph.payload.file.Read(buf); err != nil {
		return err
	}
	ph.Version = binary.BigEndian.Uint64(buf)
	fmt.Printf("Payload Version: %d\n", ph.Version)

	if ph.Version != brilloMajorPayloadVersion {
		return fmt.Errorf("Unsupported payload version: %d", ph.Version)
	}

	// Read Manifest Len
	buf = make([]byte, 8)
	if _, err := ph.payload.file.Read(buf); err != nil {
		return err
	}
	ph.ManifestLen = binary.BigEndian.Uint64(buf)
	fmt.Printf("Payload Manifest Length: %d\n", ph.ManifestLen)

	ph.Size = 24

	// Read Manifest Signature Length
	buf = make([]byte, 4)
	if _, err := ph.payload.file.Read(buf); err != nil {
		return err
	}
	ph.MetadataSignatureLen = binary.BigEndian.Uint32(buf)
	fmt.Printf("Payload Manifest Signature Length: %d\n", ph.MetadataSignatureLen)

	return nil
}

// NewPayload creates a new Payload struct
func NewPayload(file FullReader) Payload {
	payload := Payload{
		file:        file,
		concurrency: 4,
	}

	return payload
}

// SetConcurrency sets number of workers
func (p *Payload) SetConcurrency(concurrency int) {
	p.concurrency = concurrency
}

// GetConcurrency returns number of workers
func (p *Payload) GetConcurrency() int {
	return p.concurrency
}

func (p *Payload) readManifest() (*DeltaArchiveManifest, error) {
	buf := make([]byte, p.header.ManifestLen)
	if _, err := p.file.Read(buf); err != nil {
		return nil, err
	}
	deltaArchiveManifest := &DeltaArchiveManifest{}
	if err := proto.Unmarshal(buf, deltaArchiveManifest); err != nil {
		return nil, err
	}

	return deltaArchiveManifest, nil
}

func (p *Payload) readMetadataSignature() (*Signatures, error) {
	buf := make([]byte, p.header.MetadataSignatureLen)
	if _, err := p.file.ReadAt(buf, int64(p.header.Size+p.header.ManifestLen)); err != nil {
		return nil, err
	}
	signatures := &Signatures{}
	if err := proto.Unmarshal(buf, signatures); err != nil {
		return nil, err
	}

	return signatures, nil
}

func (p *Payload) Init() error {
	// Read Header
	p.header = &payloadHeader{
		payload: p,
	}
	if err := p.header.ReadFromPayload(); err != nil {
		return err
	}

	// Read Manifest
	deltaArchiveManifest, err := p.readManifest()
	if err != nil {
		return err
	}
	p.deltaArchiveManifest = deltaArchiveManifest

	fmt.Printf("Payload Minor Version: %d\n", deltaArchiveManifest.GetMinorVersion())

	// Read Signatures
	signatures, err := p.readMetadataSignature()
	if err != nil {
		return err
	}
	p.signatures = signatures

	// Update sizes
	p.metadataSize = int64(p.header.Size + p.header.ManifestLen)
	p.dataOffset = p.metadataSize + int64(p.header.MetadataSignatureLen)

	fmt.Println("Found partitions:")
	for i, partition := range p.deltaArchiveManifest.Partitions {
		fmt.Printf("%s (%s)", partition.GetPartitionName(), humanize.Bytes(*partition.GetNewPartitionInfo().Size))

		if i < len(deltaArchiveManifest.Partitions)-1 {
			fmt.Printf(", ")
		} else {
			fmt.Printf("\n")
		}
	}
	p.initialized = true

	return nil
}

func (p *Payload) Extract(partition *PartitionUpdate, out *os.File, in *os.File) error {
	name := partition.GetPartitionName()
	info := partition.GetNewPartitionInfo()
	isDelta := in != nil
	totalOperations := len(partition.Operations)
	barName := fmt.Sprintf("%s (%s)", name, humanize.Bytes(info.GetSize()))
	bar := p.progress.AddBar(
		int64(totalOperations),
		mpb.PrependDecorators(
			decor.Name(barName, decor.WCSyncSpaceR),
		),
		mpb.AppendDecorators(
			decor.Percentage(),
		),
	)
	defer bar.SetTotal(0, true)

	for _, operation := range partition.Operations {
		if len(operation.DstExtents) == 0 {
			return fmt.Errorf("Invalid operation.DstExtents for the partition %s", name)
		}
		bar.Increment()

		e := operation.DstExtents[0]
		dataOffset := p.dataOffset + int64(operation.GetDataOffset())
		dataLength := int64(operation.GetDataLength())
		_, err := out.Seek(int64(e.GetStartBlock())*blockSize, 0)
		if err != nil {
			return err
		}
		expectedUncompressedBlockSize := int64(e.GetNumBlocks() * blockSize)

		bufSha := sha256.New()
		teeReader := io.TeeReader(io.NewSectionReader(p.file, dataOffset, dataLength), bufSha)

		var reader io.Reader

		switch operation.GetType() {
		case InstallOperation_REPLACE_XZ:
			read := xz.NewDecompressionReader(teeReader)
			reader = &read
		case InstallOperation_REPLACE_BZ:
			reader = bzip2.NewReader(teeReader)
		case InstallOperation_ZERO:
			reader = bytes.NewReader(make([]byte, expectedUncompressedBlockSize))
		default:
			reader = teeReader
		}

		switch operation.GetType() {
		case InstallOperation_REPLACE,
			InstallOperation_ZERO,
			InstallOperation_REPLACE_XZ,
			InstallOperation_REPLACE_BZ:

			n, err := io.Copy(out, reader)
			if err != nil {
				return err
			}

			if closer, ok := reader.(io.Closer); ok {
				closer.Close()
			}

			if int64(n) != expectedUncompressedBlockSize {
				return fmt.Errorf("Verify failed (Unexpected bytes written): %s (%d != %d)", name, n, expectedUncompressedBlockSize)
			}

		case InstallOperation_SOURCE_BSDIFF,
			InstallOperation_BSDIFF,
			InstallOperation_BROTLI_BSDIFF,
			InstallOperation_PUFFDIFF,
			InstallOperation_ZUCCHINI,
			InstallOperation_SOURCE_COPY:

			if !isDelta {
				return fmt.Errorf("%s: %s is only supported for delta", name, operation.GetType().String())
			}

			buf := make([]byte, 0)

			srcSha := sha256.New()

			for _, e := range operation.SrcExtents {
				_, err := in.Seek(int64(e.GetStartBlock())*blockSize, 0)
				if err != nil {
					return err
				}

				expectedInputBlockSize := int64(e.GetNumBlocks()) * blockSize

				data := make([]byte, expectedInputBlockSize)
				n, err := in.Read(data)

				if err != nil {
					fmt.Printf("%s: %s error: %s (read %d)\n", name, operation.GetType().String(), err, n)
					return err
				}

				if int64(n) != expectedInputBlockSize {
					return fmt.Errorf("%s: %s expected %d bytes, but got %d", name, operation.GetType().String(), expectedInputBlockSize, n)
				}

				srcSha.Write(data)
				buf = append(buf, data...)
			}

			// verify hash
			hash := hex.EncodeToString(srcSha.Sum(nil))
			expectedHash := hex.EncodeToString(operation.GetSrcSha256Hash())
			if expectedHash != "" && hash != expectedHash {
				return fmt.Errorf("Verify failed (Source Checksum mismatch): %s (%s != %s)", name, hash, expectedHash)
			}

			size := int64(0)
			for _, e := range operation.DstExtents {
				size += int64(e.GetNumBlocks() * blockSize)
			}

			dataBuf := make([]byte, dataLength)
			teeReader.Read(dataBuf)
			var err error

			switch operation.GetType() {
			case InstallOperation_PUFFDIFF:
				buf, err = chromeos_update_engine.ExecuteSourcePuffDiffOperation(buf, dataBuf, size)
			case InstallOperation_ZUCCHINI:
				buf, err = chromeos_update_engine.ExecuteSourceZucchiniOperation(buf, dataBuf, size)
			case InstallOperation_SOURCE_COPY:
				// No processing is done for source copies - just a straight copy paste
				break
			default:
				buf, err = chromeos_update_engine.ExecuteSourceBsdiffOperation(buf, dataBuf, size)
			}

			if err != nil {
				return err
			}

			n := uint64(0)

			for _, e := range operation.DstExtents {
				_, err := out.Seek(int64(e.GetStartBlock())*blockSize, 0)
				if err != nil {
					return err
				}

				data := make([]byte, e.GetNumBlocks()*blockSize)
				copy(data, buf[n*blockSize:])
				if _, err := out.Write(data); err != nil {
					return err
				}
				n += e.GetNumBlocks()
			}
		default:
			return fmt.Errorf("%s: Unhandled operation type: %s", name, operation.GetType().String())
		}

		// verify hash
		hash := hex.EncodeToString(bufSha.Sum(nil))
		expectedHash := hex.EncodeToString(operation.GetDataSha256Hash())
		if expectedHash != "" && hash != expectedHash {
			return fmt.Errorf("Verify failed (Data Checksum mismatch): %s (%s != %s)", name, hash, expectedHash)
		}
	}

	return nil
}

func (p *Payload) worker() {
	for req := range p.requests {
		partition := req.partition
		targetDirectory := req.targetDirectory
		sourceDirectory := req.sourceDirectory
		isDelta := sourceDirectory != ""

		name := fmt.Sprintf("%s.img", partition.GetPartitionName())
		filepath := fmt.Sprintf("%s/%s", targetDirectory, name)
		file, err := os.OpenFile(filepath, os.O_TRUNC|os.O_CREATE|os.O_WRONLY, 0755)
		if err != nil {
		}

		sourcepath := fmt.Sprintf("%s/%s", sourceDirectory, name)
		sourcefile, err := os.OpenFile(sourcepath, os.O_RDONLY, 0755)
		if isDelta {
			if err != nil {
				fmt.Println(err.Error())
			}
		} else {
			sourcefile = nil
		}

		if err := p.Extract(partition, file, sourcefile); err != nil {
			fmt.Println(err.Error())
		}

		p.workerWG.Done()
	}
}

func (p *Payload) spawnExtractWorkers(n int) {
	for i := 0; i < n; i++ {
		go p.worker()
	}
}

func (p *Payload) ExtractSelected(sourceDirectory string, targetDirectory string, partitions []string) error {
	if !p.initialized {
		return errors.New("Payload has not been initialized")
	}
	p.progress = mpb.New()

	p.requests = make(chan *request, 100)
	p.spawnExtractWorkers(p.concurrency)

	sort.Strings(partitions)

	for _, partition := range p.deltaArchiveManifest.Partitions {
		if len(partitions) > 0 {
			idx := sort.SearchStrings(partitions, *partition.PartitionName)
			if idx == len(partitions) || partitions[idx] != *partition.PartitionName {
				continue
			}
		}

		p.workerWG.Add(1)
		p.requests <- &request{
			partition:       partition,
			sourceDirectory: sourceDirectory,
			targetDirectory: targetDirectory,
		}
	}

	p.workerWG.Wait()
	close(p.requests)

	return nil
}

func (p *Payload) ExtractAll(sourceDirectory string, targetDirectory string) error {
	return p.ExtractSelected(sourceDirectory, targetDirectory, nil)
}
