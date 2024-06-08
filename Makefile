CFLAGS=\
	-I lib \
	-I src \
	-I $(BUILD_DIR)/src \
	-I $(BUILD_DIR)/lib \
	-I $(BUILD_DIR) \
	-I lib/brotli/c/include \
	-I lib/bsdiff/include \
	-I lib/puffin/src/include \
	-I lib/zucchini/aosp/include \
	-I lib/bzip2 \
	-I lib/libchrome \
	-I lib/protobuf/src \
	-I lib/protobuf/third_party/abseil-cpp \
	-flto

CXXFLAGS=-std=gnu++17 $(CFLAGS)
LDFLAGS=-Wl,--copy-dt-needed-entries
BUILD_DIR ?= .build

libs: $(BUILD_DIR)/libprotobuf.a $(BUILD_DIR)/third_party.a

SRC=$(wildcard src/*.cc)

BSPATCH_SRC=\
	lib/bsdiff/brotli_decompressor.cc \
	lib/bsdiff/bspatch.cc \
	lib/bsdiff/bz2_decompressor.cc \
	lib/bsdiff/buffer_file.cc \
	lib/bsdiff/decompressor_interface.cc \
	lib/bsdiff/extents.cc \
	lib/bsdiff/extents_file.cc \
	lib/bsdiff/file.cc \
	lib/bsdiff/logging.cc \
	lib/bsdiff/memory_file.cc \
	lib/bsdiff/patch_reader.cc \
	lib/bsdiff/sink_file.cc \
	lib/bsdiff/utils.cc

BZ2_SRC=\
	lib/bzip2/blocksort.c \
	lib/bzip2/huffman.c \
	lib/bzip2/crctable.c \
	lib/bzip2/randtable.c \
	lib/bzip2/compress.c \
	lib/bzip2/decompress.c \
	lib/bzip2/bzlib.c

PUFFPATCH_SRC=\
	lib/puffin/src/bit_reader.cc \
	lib/puffin/src/bit_writer.cc \
	lib/puffin/src/brotli_util.cc \
	lib/puffin/src/huffer.cc \
	lib/puffin/src/huffman_table.cc \
	lib/puffin/src/memory_stream.cc \
	lib/puffin/src/puff_reader.cc \
	lib/puffin/src/puff_writer.cc \
	lib/puffin/src/puffer.cc \
	lib/puffin/src/puffin_stream.cc \
	lib/puffin/src/puffpatch.cc \
	lib/puffin/src/puffin.pb.cc

ZUCCHINI_SRC=\
	lib/zucchini/abs32_utils.cc \
	lib/zucchini/address_translator.cc \
	lib/zucchini/arm_utils.cc \
	lib/zucchini/binary_data_histogram.cc \
	lib/zucchini/buffer_sink.cc \
	lib/zucchini/buffer_source.cc \
	lib/zucchini/crc32.cc \
	lib/zucchini/disassembler.cc \
	lib/zucchini/disassembler_dex.cc \
	lib/zucchini/disassembler_elf.cc \
	lib/zucchini/disassembler_no_op.cc \
	lib/zucchini/disassembler_win32.cc \
	lib/zucchini/disassembler_ztf.cc \
	lib/zucchini/element_detection.cc \
	lib/zucchini/encoded_view.cc \
	lib/zucchini/ensemble_matcher.cc \
	lib/zucchini/equivalence_map.cc \
	lib/zucchini/heuristic_ensemble_matcher.cc \
	lib/zucchini/image_index.cc \
	lib/zucchini/imposed_ensemble_matcher.cc \
	lib/zucchini/io_utils.cc \
	lib/zucchini/patch_reader.cc \
	lib/zucchini/patch_writer.cc \
	lib/zucchini/reference_bytes_mixer.cc \
	lib/zucchini/reference_set.cc \
	lib/zucchini/rel32_finder.cc \
	lib/zucchini/rel32_utils.cc \
	lib/zucchini/reloc_elf.cc \
	lib/zucchini/reloc_win32.cc \
	lib/zucchini/target_pool.cc \
	lib/zucchini/targets_affinity.cc \
	lib/zucchini/zucchini_apply.cc \
	lib/zucchini/zucchini_gen.cc \
	lib/zucchini/zucchini_tools.cc

BROTLI_SRC=$(wildcard \
	lib/brotli/c/common/*.c \
	lib/brotli/c/dec/*.c \
	lib/brotli/c/enc/*.c \
)

CHROME_SRC=$(wildcard \
	lib/libchrome/base/third_party/*/*.cc \
	lib/libchrome/base/*/*.cc \
	lib/libchrome/base/*.cc \
)

ALL_SRC=\
	$(SRC) \
	$(BSPATCH_SRC) \
	$(BZ2_SRC) \
	$(PUFFPATCH_SRC) \
	$(ZUCCHINI_SRC) \
	$(BROTLI_SRC)

ALL_OBJ=$(addprefix $(BUILD_DIR)/,$(patsubst %.cc,%.o,$(patsubst %.c,%.o, $(ALL_SRC))))
CHROME_OBJ=$(addprefix $(BUILD_DIR)/,$(CHROME_SRC:.cc=.o))

$(BUILD_DIR)/libchrome.a: $(CHROME_OBJ)
	@ echo "[AR]\t$@"
	@ $(AR) -rc $@ $^

ota-tool: $(ALL_OBJ) $(BUILD_DIR)/third_party.a $(BUILD_DIR)/libprotobuf.a $(BUILD_DIR)/libchrome.a
	@ echo "[LD]\t$@"
	@ $(CXX) -flto -w -s -static -o $@ $(ALL_OBJ) -L$(BUILD_DIR) -llzma -l:libprotobuf.a -l:third_party.a -levent -l:libchrome.a -lcrypto

# Need to replace the LOG statement with a single ; so that a statement
# still exists. This is because some LOG calls are in an if/else block
# without brackets.
$(BUILD_DIR)/%.cc: %.cc
	@ mkdir -p $(dir $@)
	@ sed -z 's/\(\n *\)[A-Z_]*LOG[^;]*;\n/\1;\n/g' $< > $@

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.cc
	@ echo "[CPP]\t$@"
	@ $(CXX) $(CXXFLAGS) -c -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@ echo "[CC]\t$@"
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

$(addprefix $(BUILD_DIR)/, $(SRC)): $(BUILD_DIR)/src/update_metadata.pb.cc
$(BUILD_DIR)/lib/puffin/src/puffpatch.cc: $(BUILD_DIR)/lib/puffin/src/puffin.pb.cc
$(BUILD_DIR)/%.pb.cc: %.proto
	@ echo "[PROTO]\t$^"
	@ protoc --cpp_out=$(BUILD_DIR) $^

$(BUILD_DIR)/third_party.a: $(BUILD_DIR)/lib/protobuf/libprotobuf.a
	@ echo "[AR]\t$@"
	@ find $(BUILD_DIR)/lib/protobuf/third_party -name '*.a' | xargs $(AR) -rcT $@

$(BUILD_DIR)/libprotobuf.a: $(BUILD_DIR)/lib/protobuf/libprotobuf.a
	cp $< $@

$(BUILD_DIR)/lib/protobuf/libprotobuf.a: $(BUILD_DIR)/lib/protobuf/Makefile
	make -C $(BUILD_DIR)/lib/protobuf libprotobuf

$(BUILD_DIR)/lib/protobuf/Makefile:
	cmake -S lib/protobuf -B $(BUILD_DIR)/lib/protobuf

clean:
	rm -rf $(BUILD_DIR)
