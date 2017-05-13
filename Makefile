-include Makefile.local

CXXFLAGS += -g -Wall
CXXFLAGS += -Wno-unused-variable -Wno-unused-value -std=c++98 -fpermissive

SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
DEPS_DIR = deps
BRIDGE_DIR = bridge

STANDALONE_SRC_MODULES = \
	auth.cpp             \
	cfg.cpp              \
	common.cpp           \
	exceptions.cpp       \
	http_request.cpp     \
	http_response.cpp    \
	http_server.cpp      \
	http_session.cpp     \
	log.cpp              \
	standalone_main.cpp  \
	multipart.cpp        \
	photohosting.cpp     \
	tcp_server.cpp       \
	tcp_session.cpp      \
	web_album_creator_helper.cpp

STANDALONE_OBJ_MODULES = $(addprefix $(OBJ_DIR)/, $(notdir $(STANDALONE_SRC_MODULES:.cpp=.o)))

CGI_SRC_MODULES =    \
	auth.cpp         \
	cfg.cpp          \
	common.cpp       \
	exceptions.cpp   \
	log.cpp          \
	cgi.cpp          \
	cgi_main.cpp     \
	photohosting.cpp \
	web_album_creator_helper.cpp

CGI_OBJ_MODULES = $(addprefix $(OBJ_DIR)/, $(notdir $(CGI_SRC_MODULES:.cpp=.o)))

TMP = $(addprefix $(DEPS_DIR)/, $(notdir $(wildcard $(SRC_DIR)/*.cpp)))
DEPS_MODULES = $(TMP:.cpp=.d)

BRIDGE_TARGETS = picohttpparser WebAlbumCreator multipart_parser iniparser ccgi

CXXFLAGS += -I$(INCLUDE_DIR)
CXXFLAGS += -I$(BRIDGE_DIR)/include

LDFLAGS = -L $(BRIDGE_DIR)/lib/
LDFLAGS += -l wac -l pico -l multipart -l iniparser -l ccgi

src_to_obj = $(addprefix $(OBJ_DIR)/, $(notdir $(1:.cpp=.o)))

STANDALONE = server
CGI = photohosting.cgi


sa: $(STANDALONE)

$(STANDALONE): $(STANDALONE_OBJ_MODULES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

cgi: $(CGI)

$(CGI): $(CGI_OBJ_MODULES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: cgi sa


ifneq ($(MAKECMDGOALS), clean)
-include .bridge.touch
endif

$(DEPS_DIR)/%.d: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -E -MM -MT $(call src_to_obj, $<) -MT $@ -MF $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.bridge.touch:
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
	rm -f .bridge.touch
	rm -f $(STANDALONE)
	rm -f $(CGI)
	make -C $(dir $(BRIDGE_DIR)/Makefile) -f Makefile clean

clangcomp:
	@echo $(CXXFLAGS) | tr ' ' '\n' | grep -v 'Werror' > .clang_complete

.PHONY: clangcomp clean
