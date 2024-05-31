package chromeos_update_engine

//go:generate make -j libs

/*
#cgo CXXFLAGS: -Ibsdiff/include -Ipuffin/src/include -Izucchini/aosp/include -Ilibchrome -Iprotobuf/third_party/abseil-cpp
#cgo LDFLAGS: -Lbuild -l:libpuffpatch.a -l:libzucchini.a  -l:libbspatch.a -l:libbz2.a -l:libbrotli.a -l:libchrome.a -l:libprotobuf-lite.a -l:third_party.a
#include "update_engine.h"
*/
import "C"
import (
	"fmt"
	"unsafe"
)

func ExecuteSourceBsdiffOperation(data []byte, patch []byte, size int64) ([]byte, error) {
	output := make([]byte, size)
	result := int(C.ExecuteSourceBsdiffOperation(
		unsafe.Pointer(&data[0]), C.ulong(len(data)),
		unsafe.Pointer(&patch[0]), C.ulong(len(patch)),
		unsafe.Pointer(&output[0]), C.ulong(len(output)),
	))

	if result < 0 {
		return nil, fmt.Errorf("C++ ExecuteSourceBsdiffOperation call failed (returned %d)", result)
	}

	if result != len(output) {
		realloc := make([]byte, result)
		copy(realloc, output)
		return realloc, nil
	}

	return output, nil
}

func ExecuteSourcePuffDiffOperation(data []byte, patch []byte, size int64) ([]byte, error) {
	output := make([]byte, size)
	result := int(C.ExecuteSourcePuffDiffOperation(
		unsafe.Pointer(&data[0]), C.ulong(len(data)),
		unsafe.Pointer(&patch[0]), C.ulong(len(patch)),
		unsafe.Pointer(&output[0]), C.ulong(len(output)),
	))

	if result < 0 {
		return nil, fmt.Errorf("C++ ExecuteSourcePuffDiffOperation call failed (returned %d)", result)
	}

	if result != len(output) {
		realloc := make([]byte, result)
		copy(realloc, output)
		return realloc, nil
	}

	return output, nil
}

func ExecuteSourceZucchiniOperation(data []byte, patch []byte, size int64) ([]byte, error) {
	output := make([]byte, size)
	result := int(C.ExecuteSourceZucchiniOperation(
		unsafe.Pointer(&data[0]), C.ulong(len(data)),
		unsafe.Pointer(&patch[0]), C.ulong(len(patch)),
		unsafe.Pointer(&output[0]), C.ulong(len(output)),
	))

	if result != 0 {
		return nil, fmt.Errorf("C++ ExecuteSourcePuffDiffOperation call failed (returned %d)", result)
	}

	return output, nil
}
