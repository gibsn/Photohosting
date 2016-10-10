CXX = clang++
CXXFLAGS = -g -Wall
CXXFLAGS += -Wno-unused-variable -Wno-unused-value -std=c++98 -fpermissive
#Needed for proper libarchive work
#CXXFLAGS += -D_FILE_OFFSET_BITS=64

SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
DEPS_DIR = deps
BRIDGE_DIR = bridge

SRC_MODULES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_MODULES = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC_MODULES:.cpp=.o)))
DEPS_MODULES = $(addprefix $(DEPS_DIR)/, $(notdir $(SRC_MODULES:.cpp=.d)))

BRIDGE_TARGETS = picohttpparser WebAlbumCreator

CXXFLAGS += -I$(INCLUDE_DIR)
CXXFLAGS += -I$(BRIDGE_DIR)/include

LDFLAGS = -L $(BRIDGE_DIR)/lib/
LDFLAGS += -l wac -l pico

src_to_obj = $(addprefix $(OBJ_DIR)/, $(notdir $(1:.cpp=.o)))

server: $(OBJ_MODULES)
	$(CXX) $(CXXFLAGS) $(OBJ_MODULES) -o $@ $(LDFLAGS)

ifneq ($(MAKECMDGOALS), clean)
-include bridge.touch
endif

$(DEPS_DIR)/%.d: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -E -MM -MT $(call src_to_obj, $<) -MT $@ -MF $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bridge.touch:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(DEPS_DIR)
	mkdir -p $(BRIDGE_DIR)/include
	mkdir -p $(BRIDGE_DIR)/lib
	mkdir -p $(BRIDGE_DIR)/bin
	make -C $(BRIDGE_DIR) -f Makefile $(BRIDGE_TARGETS)
	@echo "-include $(DEPS_MODULES)" > $@

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(DEPS_DIR)
	rm -rf $(BRIDGE_DIR)/include
	rm -rf $(BRIDGE_DIR)/lib
	rm -rf $(BRIDGE_DIR)/bin
	rm -f bridge.touch
	make -C $(dir $(BRIDGE_DIR)/Makefile) -f Makefile clean

clangcomp:
	@echo $(CXXFLAGS) | tr ' ' '\n' | grep -v 'Werror' > .clang_complete

.PHONY: clangcomp clean


