package main

import (
	"archive/zip"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"runtime"
	"strings"
	"time"
)

func extractPayloadBin(data io.ReaderAt, size int64) FullReader {
	zipReader, err := zip.NewReader(data, size)
	if err != nil {
		log.Fatalf("Not a valid zip archive\n")
	}

	for _, file := range zipReader.File {
		if file.Name == "payload.bin" && file.UncompressedSize64 > 0 {
			size, _ := file.DataOffset()
			return io.NewSectionReader(data, size, int64(file.UncompressedSize64))
		}
	}

	return nil
}

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())

	var (
		list            bool
		partitions      string
		outputDirectory string
		inputDirectory  string
		concurrency     int
	)

	flag.IntVar(&concurrency, "concurrency", 4, "Number of multiple workers to extract")
	flag.BoolVar(&list, "list", false, "Show list of partitions in payload.bin and exit")
	flag.StringVar(&outputDirectory, "output", "", "Set output directory for partitions")
	flag.StringVar(&inputDirectory, "input", "", "Set directory for existing partitions")
	flag.StringVar(&partitions, "partitions", "", "Only work with selected partitions (comma-separated)")
	flag.Parse()

	if flag.NArg() == 0 {
		usage()
	}
	filename := flag.Arg(0)

	if _, err := os.Stat(filename); os.IsNotExist(err) {
		log.Fatalf("File does not exist: %s\n", filename)
	}

	log.Printf("Delta: %s, partitions: %s\n", inputDirectory, partitions)

	payloadBin, _ := os.Open(filename)
	var payloadReader FullReader = payloadBin
	if strings.HasSuffix(filename, ".zip") {
		stat, _ := payloadBin.Stat()
		payloadReader = extractPayloadBin(payloadBin, stat.Size())
		if payloadReader == nil {
			log.Fatal("Failed to find payload.bin from the archive.")
		}
	}

	payload := NewPayload(payloadReader)
	payload.Init()

	if list {
		return
	}

	now := time.Now()

	var targetDirectory = outputDirectory
	if targetDirectory == "" {
		targetDirectory = fmt.Sprintf("extracted_%d%02d%02d_%02d%02d%02d", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second())
	}
	if _, err := os.Stat(targetDirectory); os.IsNotExist(err) {
		if err := os.Mkdir(targetDirectory, 0755); err != nil {
			log.Fatal("Failed to create target directory")
		}
	}
	var sourceDirectory = inputDirectory
	payload.SetConcurrency(concurrency)
	fmt.Printf("Number of workers: %d\n", payload.GetConcurrency())

	if partitions != "" {
		if err := payload.ExtractSelected(sourceDirectory, targetDirectory, strings.Split(partitions, ",")); err != nil {
			log.Fatal(err)
		}
	} else {
		if err := payload.ExtractAll(sourceDirectory, targetDirectory); err != nil {
			log.Fatal(err)
		}
	}
}

func usage() {
	fmt.Fprintf(os.Stderr, "Usage: %s [options] [inputfile]\n", os.Args[0])
	flag.PrintDefaults()
	os.Exit(2)
}
