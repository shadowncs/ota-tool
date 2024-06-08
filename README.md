# OTA Tool

A tool for extracting or patching images from Android OTA payloads.

This project was a originally a fork of [payload-dumper-go](https://github.com/ssut/payload-dumper-go) -
it has now been reimplemented in C(++).

## Features

- Incredibly fast decompression. All decompression progresses are executed in parallel.
- Payload checksum verification.
- Support original zip package that contains payload.bin.
- Incremental OTA (delta) payloads can be applied to old images.

### Cautions

- Working on a SSD is highly recommended for performance reasons, a HDD could be a bottle-neck.

## Installation

0. Download the latest binary for your platform from [here][download-page] and extract the contents of the downloaded file to a directory on your system.

[download-page]: https://github.com/EmilyShepherd/ota-tool/releases

### Linux and OSX

1. Make sure the extracted binary file has executable permissions. You can use the following command to set the permissions if necessary:
```
chmod +x ota-tool
```
2. Run the following command to add the directory path to your system's PATH environment variable:
```
export PATH=$PATH:/path/to/ota-tool
```
Note: This command sets the PATH environment variable only for the current terminal session. To make it permanent, you need to add the command to your system's profile file (e.g. .bashrc or .zshrc for Linux/Unix systems).

### Windows

1. Open the Start menu and search for "Environment Variables".
2. Click on "Edit the system environment variables".
3. Click on the "Environment Variables" button at the bottom right corner of the "System Properties" window.
4. Under "System Variables", scroll down and click on the "Path" variable, then click on "Edit".
5. Click "New" and add the path to the directory where the extracted binary is located.
6. Click "OK" on all the windows to save the changes.

## Usage

Run the following command in your terminal:
```
Usage: ./ota-tool [options] [inputfile]
  --concurrency int
        Number of multiple workers to extract (default 4)
  --input string
        Set directory for existing partitions
  --list
        Show list of partitions in payload.bin and exit
  --output string
        Set output directory for partitions
  --partitions string
        Only work with selected partitions (comma-separated)
```

Inputfile can either be a `payload.bin` or an uncompressed zip file containing a `payload.bin`.

## Sources

https://android.googlesource.com/platform/system/update_engine/+/master/update_metadata.proto

## Building

```
git submodule update --init --recursive
make
```

### Build Deps

- liblzma-devel
- libopenssl-devel

### Build Toolchain

- protoc
- make
- cmake
- gcc (or another C compiler)
- g++ (or another C++ compiler)

## License

This source code is licensed under the Apache License 2.0 as described in the LICENSE file.
