
BUILD_DIR ?= .build
PROG      ?= ota-tool
PROTOC    ?= $(shell which protoc)

.DEFAULT_GOAL := $(PROG)

include lib/libs.mk


###
# ota-tool
#

INCLUDE += src $(BUILD_DIR)/src

SRC += $(wildcard src/*.cc) src/update_metadata.pb.cc
OBJ = $(addprefix $(BUILD_DIR)/,$(SRC:=.o))

$(PROG): $(OBJ) $(addprefix $(BUILD_DIR)/, $(LIBS))
	@ echo "[LD]\t$@"
	@ $(CXX) -flto -w -s -static -o $@ $(OBJ) -L$(BUILD_DIR) -llzma -lcrypto $(addprefix -l:,$(LIBS))

clean:
	rm -rf $(BUILD_DIR) $(PROG)


###
# Protoc generation
#

$(addprefix $(BUILD_DIR)/, $(wildcard src/*.cc)): $(BUILD_DIR)/src/update_metadata.pb.cc
$(BUILD_DIR)/%.pb.cc: %.proto
	@ echo "[PROTO]\t$<"
	@ mkdir -p $(dir $@)
	@ $(PROTOC) --cpp_out=$(BUILD_DIR) $<


###
# General C(++)
#

INCLUDE += $(BUILD_DIR)
CFLAGS   = $(addprefix -I,$(INCLUDE)) -flto
CXXFLAGS = -std=gnu++17 $(CFLAGS)
LDFLAGS  = -Wl,--copy-dt-needed-entries

# Need to replace the LOG statement with a single ; so that a statement
# still exists. This is because some LOG calls are in an if/else block
# without brackets.
$(BUILD_DIR)/%.cc: %.cc
	@ mkdir -p $(dir $@)
	@ sed -z 's/\(\n *\)[A-Z_]*LOG[^;]*;\n/\1;\n/g' $< > $@

$(BUILD_DIR)/%.cc.o: $(BUILD_DIR)/%.cc
	@ echo "[CPP]\t$@"
	@ mkdir -p $(dir $@)
	@ $(CXX) $(CXXFLAGS) -c -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	@ echo "[CC]\t$@"
	@ mkdir -p $(dir $@)
	@ $(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)
