
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

CHROME_SRC = \
	lib/libchrome/base/at_exit.cc \
	lib/libchrome/base/base_switches.cc \
	lib/libchrome/base/callback_helpers.cc \
	lib/libchrome/base/callback_internal.cc \
	lib/libchrome/base/command_line.cc \
	lib/libchrome/base/files/file.cc \
	lib/libchrome/base/files/file_enumerator.cc \
	lib/libchrome/base/files/file_enumerator_posix.cc \
	lib/libchrome/base/files/file_path.cc \
	lib/libchrome/base/files/file_path_constants.cc \
	lib/libchrome/base/files/file_posix.cc \
	lib/libchrome/base/files/file_tracing.cc \
	lib/libchrome/base/files/file_util.cc \
	lib/libchrome/base/files/file_util_posix.cc \
	lib/libchrome/base/files/scoped_file.cc \
	lib/libchrome/base/lazy_instance_helpers.cc \
	lib/libchrome/base/location.cc \
	lib/libchrome/base/memory/ref_counted.cc \
	lib/libchrome/base/memory/weak_ptr.cc \
	lib/libchrome/base/pickle.cc \
	lib/libchrome/base/run_loop.cc \
	lib/libchrome/base/sequence_checker_impl.cc \
	lib/libchrome/base/sequenced_task_runner.cc \
	lib/libchrome/base/sequence_token.cc \
	lib/libchrome/base/strings/string16.cc \
	lib/libchrome/base/strings/string_number_conversions.cc \
	lib/libchrome/base/strings/string_piece.cc \
	lib/libchrome/base/strings/stringprintf.cc \
	lib/libchrome/base/strings/string_split.cc \
	lib/libchrome/base/strings/string_util.cc \
	lib/libchrome/base/strings/string_util_constants.cc \
	lib/libchrome/base/strings/sys_string_conversions_posix.cc \
	lib/libchrome/base/strings/utf_string_conversions.cc \
	lib/libchrome/base/strings/utf_string_conversion_utils.cc \
	lib/libchrome/base/synchronization/condition_variable_posix.cc \
	lib/libchrome/base/synchronization/lock.cc \
	lib/libchrome/base/synchronization/lock_impl_posix.cc \
	lib/libchrome/base/task_runner.cc \
	lib/libchrome/base/third_party/icu/icu_utf.cc \
	lib/libchrome/base/third_party/nspr/prtime.cc \
	lib/libchrome/base/threading/platform_thread_internal_posix.cc \
	lib/libchrome/base/threading/platform_thread_linux.cc \
	lib/libchrome/base/threading/platform_thread_posix.cc \
	lib/libchrome/base/threading/post_task_and_reply_impl.cc \
	lib/libchrome/base/threading/sequenced_task_runner_handle.cc \
	lib/libchrome/base/threading/sequence_local_storage_map.cc \
	lib/libchrome/base/threading/thread_checker_impl.cc \
	lib/libchrome/base/threading/thread_id_name_manager.cc \
	lib/libchrome/base/threading/thread_local_storage.cc \
	lib/libchrome/base/threading/thread_local_storage_posix.cc \
	lib/libchrome/base/threading/thread_restrictions.cc \
	lib/libchrome/base/threading/thread_task_runner_handle.cc \
	lib/libchrome/base/time/time.cc \
	lib/libchrome/base/time/time_conversion_posix.cc \
	lib/libchrome/base/time/time_exploded_posix.cc \
	lib/libchrome/base/time/time_now_posix.cc \
	lib/libchrome/base/time/time_override.cc

CHROME_OBJ=$(addprefix $(BUILD_DIR)/,$(CHROME_SRC:=.o))

$(BUILD_DIR)/libchrome.a: $(CHROME_OBJ)
	@ echo "[AR]\t$@"
	@ $(AR) -rc $@ $^


###
# Protobuf
#

INCLUDE += lib/protobuf/src lib/protobuf/third_party/abseil-cpp

LIBS += libprotobuf.a third_party.a

PB_BUILD_DIR  = $(BUILD_DIR)/lib/protobuf
PB_MAKEFILE   = $(PB_BUILD_DIR)/Makefile
PB_LIB_OUT    = $(PB_BUILD_DIR)/libprotobuf.a
PB_PROTOC_OUT = $(PB_BUILD_DIR)/protoc
PROTOC       ?= $(PB_PROTOC_OUT)
PB_MAKE       = $(MAKE) -C $(PB_BUILD_DIR)

$(BUILD_DIR)/third_party.a: $(PB_LIB_OUT)
	@ echo "[AR]\t$@"
	@ find $(PB_BUILD_DIR)/third_party -name '*.a' | xargs $(AR) -rcT $@

$(BUILD_DIR)/libprotobuf.a: $(PB_LIB_OUT)
	cp $< $@

$(PB_LIB_OUT): $(PB_MAKEFILE)
	$(PB_MAKE) libprotobuf

$(PB_MAKEFILE):
	cmake -S lib/protobuf -B $(PB_BUILD_DIR)

$(PB_PROTOC_OUT): $(PB_MAKEFILE)
	$(PB_MAKE) protoc
