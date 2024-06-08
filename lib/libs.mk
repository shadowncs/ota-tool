
INCLUDE += lib $(BUILD_DIR)/lib

###
# BSDiff
#

INCLUDE += lib/bsdiff/include

SRC += \
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


###
# bzip2
#

INCLUDE += lib/bzip2

SRC += \
	lib/bzip2/blocksort.c \
	lib/bzip2/huffman.c \
	lib/bzip2/crctable.c \
	lib/bzip2/randtable.c \
	lib/bzip2/compress.c \
	lib/bzip2/decompress.c \
	lib/bzip2/bzlib.c


###
# PuffDiff
#

$(BUILD_DIR)/lib/puffin/src/puffpatch.cc: $(BUILD_DIR)/lib/puffin/src/puffin.pb.cc

INCLUDE += lib/puffin/src/include

SRC += \
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


###
# Zucchini
#

INCLUDE += lib/zucchini/aosp/include

SRC += \
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


###
# Brotli
#

INCLUDE += lib/brotli/c/include

SRC += $(wildcard \
	lib/brotli/c/common/*.c \
	lib/brotli/c/dec/*.c \
	lib/brotli/c/enc/*.c \
)


###
# LibChrome
#

INCLUDE += lib/libchrome

LIBS += libchrome.a

CHROME_SRC=$(wildcard \
	lib/libchrome/base/third_party/*/*.cc \
	lib/libchrome/base/*/*.cc \
	lib/libchrome/base/*.cc \
)

CHROME_OBJ=$(addprefix $(BUILD_DIR)/,$(CHROME_SRC:=.o))

$(BUILD_DIR)/libchrome.a: $(CHROME_OBJ)
	@ echo "[AR]\t$@"
	@ $(AR) -rc $@ $^


###
# Protobuf
#

INCLUDE += lib/protobuf/src lib/protobuf/third_party/abseil-cpp

LIBS += libprotobuf.a third_party.a

$(BUILD_DIR)/third_party.a: $(BUILD_DIR)/lib/protobuf/libprotobuf.a
	@ echo "[AR]\t$@"
	@ find $(BUILD_DIR)/lib/protobuf/third_party -name '*.a' | xargs $(AR) -rcT $@

$(BUILD_DIR)/libprotobuf.a: $(BUILD_DIR)/lib/protobuf/libprotobuf.a
	cp $< $@

$(BUILD_DIR)/lib/protobuf/libprotobuf.a: $(BUILD_DIR)/lib/protobuf/Makefile
	make -C $(BUILD_DIR)/lib/protobuf libprotobuf

$(BUILD_DIR)/lib/protobuf/Makefile:
	cmake -S lib/protobuf -B $(BUILD_DIR)/lib/protobuf

