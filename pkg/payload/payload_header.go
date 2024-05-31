package payload

import (
	"encoding/binary"
	"fmt"
	"io"
)

const payloadHeaderMagic = "CrAU"
const brilloMajorPayloadVersion = 2
const blockSize = 4096

type PayloadHeader struct {
	Version              uint64
	ManifestLen          uint64
	MetadataSignatureLen uint32
	Size                 uint64
}

func (header *PayloadHeader) ReadPayloadHeader(data io.Reader) error {
	buf := make([]byte, 4)
	if _, err := data.Read(buf); err != nil {
		return err
	}
	if string(buf) != payloadHeaderMagic {
		return fmt.Errorf("Invalid payload magic: %s", buf)
	}

	// Read Version
	buf = make([]byte, 8)
	if _, err := data.Read(buf); err != nil {
		return err
	}
	header.Version = binary.BigEndian.Uint64(buf)

	if header.Version != brilloMajorPayloadVersion {
		return fmt.Errorf("Unsupported payload version: %d", header.Version)
	}

	// Read Manifest Len
	buf = make([]byte, 8)
	if _, err := data.Read(buf); err != nil {
		return err
	}
	header.ManifestLen = binary.BigEndian.Uint64(buf)

	header.Size = 24

	// Read Manifest Signature Length
	buf = make([]byte, 4)
	if _, err := data.Read(buf); err != nil {
		return err
	}
	header.MetadataSignatureLen = binary.BigEndian.Uint32(buf)

	return nil
}
