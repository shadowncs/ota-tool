CFLAGS=\
	-I . \
	-I build \
	-I brotli/c/include \
	-I bsdiff/include \
	-I puffin/src/include \
	-I zucchini/aosp/include \
	-I bzip2 \
	-I libchrome \
	-I protobuf/src \
	-I protobuf/third_party/abseil-cpp

CXXFLAGS=-std=gnu++17 $(CFLAGS)
LDFLAGS=-Wl,--copy-dt-needed-entries

libs: build/combined.a build/libprotobuf-lite.a build/third_party.a

BSPATCH_SRC=\
	bsdiff/brotli_decompressor.cc \
	bsdiff/bspatch.cc \
	bsdiff/bz2_decompressor.cc \
	bsdiff/buffer_file.cc \
	bsdiff/decompressor_interface.cc \
	bsdiff/extents.cc \
	bsdiff/extents_file.cc \
	bsdiff/file.cc \
	bsdiff/logging.cc \
	bsdiff/memory_file.cc \
	bsdiff/patch_reader.cc \
	bsdiff/sink_file.cc \
	bsdiff/utils.cc

BZ2_SRC=\
	bzip2/blocksort.c \
	bzip2/huffman.c \
	bzip2/crctable.c \
	bzip2/randtable.c \
	bzip2/compress.c \
	bzip2/decompress.c \
	bzip2/bzlib.c

PUFFPATCH_SRC=\
	puffin/src/bit_reader.cc \
	puffin/src/bit_writer.cc \
	puffin/src/brotli_util.cc \
	puffin/src/huffer.cc \
	puffin/src/huffman_table.cc \
	puffin/src/memory_stream.cc \
	puffin/src/puff_reader.cc \
	puffin/src/puff_writer.cc \
	puffin/src/puffer.cc \
	puffin/src/puffin_stream.cc \
	puffin/src/puffpatch.cc \
	puffin/src/puffin.pb.cc

ZUCCHINI_SRC=\
	zucchini/abs32_utils.cc \
	zucchini/address_translator.cc \
	zucchini/arm_utils.cc \
	zucchini/binary_data_histogram.cc \
	zucchini/buffer_sink.cc \
	zucchini/buffer_source.cc \
	zucchini/crc32.cc \
	zucchini/disassembler.cc \
	zucchini/disassembler_dex.cc \
	zucchini/disassembler_elf.cc \
	zucchini/disassembler_no_op.cc \
	zucchini/disassembler_win32.cc \
	zucchini/disassembler_ztf.cc \
	zucchini/element_detection.cc \
	zucchini/encoded_view.cc \
	zucchini/ensemble_matcher.cc \
	zucchini/equivalence_map.cc \
	zucchini/heuristic_ensemble_matcher.cc \
	zucchini/image_index.cc \
	zucchini/imposed_ensemble_matcher.cc \
	zucchini/io_utils.cc \
	zucchini/patch_reader.cc \
	zucchini/patch_writer.cc \
	zucchini/reference_bytes_mixer.cc \
	zucchini/reference_set.cc \
	zucchini/rel32_finder.cc \
	zucchini/rel32_utils.cc \
	zucchini/reloc_elf.cc \
	zucchini/reloc_win32.cc \
	zucchini/target_pool.cc \
	zucchini/targets_affinity.cc \
	zucchini/zucchini_apply.cc \
	zucchini/zucchini_gen.cc \
	zucchini/zucchini_tools.cc

BROTLI_SRC=$(wildcard \
	brotli/c/common/*.c \
	brotli/c/dec/*.c \
	brotli/c/enc/*.c \
)

CHROME_SRC=$(wildcard \
	libchrome/base/third_party/*/*.cc \
	libchrome/base/*/*.cc \
	libchrome/base/*.cc \
)

ALL_SRC=\
	$(BSPATCH_SRC) \
	$(BZ2_SRC) \
	$(PUFFPATCH_SRC) \
	$(ZUCCHINI_SRC) \
	$(BROTLI_SRC) \
	$(CHROME_SRC)

ALL_OBJ=$(addprefix build/,$(patsubst %.cc,%.o,$(patsubst %.c,%.o, $(ALL_SRC))))

# Need to replace the LOG statement with a single ; so that a statement
# still exists. This is because some LOG calls are in an if/else block
# without brackets.
build/%.cc: %.cc
	@ mkdir -p $(dir $@)
	@ sed -z 's/\(\n *\)[A-Z_]*LOG[^;]*;\n/\1;\n/g' $< > $@

build/%.o: build/%.cc
	@ echo "[CPP]\t$@"
	@ $(CXX) $(CXXFLAGS) -c -o $@ $< $(LDFLAGS)

build/%.o: %.c
	@ echo "[CC]\t$@"
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

build/combined.a: $(ALL_OBJ)
	@ echo "[AR]\t$@"
	@ $(AR) rcs $@ $^

build/puffin/src/puffin.pb.cc: puffin/src/puffin.proto
	@ echo "[PROTO]\t$^"
	@ protoc -I=puffin/src/ --cpp_out=build/puffin/src/ $^

build/third_party.a: build/protobuf/libprotobuf-lite.a
	@ echo "[AR]\t$@"
	@ find build/protobuf/third_party -name '*.a' | xargs $(AR) -rcT $@

build/libprotobuf-lite.a: build/protobuf/libprotobuf-lite.a
	cp $< $@

build/protobuf/libprotobuf-lite.a: build/protobuf/Makefile
	make -C build/protobuf libprotobuf-lite

build/protobuf/Makefile:
	cmake -S protobuf -B build/protobuf

ALL_OBJ=build/
clean:
	rm -rf $(ALL_OBJ)
