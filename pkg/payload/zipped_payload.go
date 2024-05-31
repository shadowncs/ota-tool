package payload

import (
	"archive/zip"
	"fmt"
	"io"
	"os"
)

func NewPayloadFromZipFile(path string) (*Payload, error) {
	zipFile, err := os.Open(path)
	if err != nil {
		return nil, err
	}

	stat, err := zipFile.Stat()
	if err != nil {
		return nil, err
	}

	return NewPayloadFromZip(zipFile, stat.Size())
}

func NewPayloadFromZip(data io.ReaderAt, size int64) (*Payload, error) {
	zipReader, err := zip.NewReader(data, size)
	if err != nil {
		return nil, fmt.Errorf("Not a valid zip archive")
	}

	for _, file := range zipReader.File {
		if file.Name == "payload.bin" && file.UncompressedSize64 > 0 {
			size, _ := file.DataOffset()
			return NewPayload(io.NewSectionReader(data, size, int64(file.UncompressedSize64))), nil
		}
	}

	return nil, fmt.Errorf("Archive did not contain payload.bin")
}
