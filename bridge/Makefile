INCLUDE_DIR = include
LIBRARY_DIR = lib
EXTERNALS_BASEDIR = externals

WEBALBUMCREATOR_DIR = ../$(EXTERNALS_BASEDIR)/WebAlbumCreator
WEBALBUMCREATOR_STATIC = libwac.a

PICOHTTPPARSER_DIR = ../$(EXTERNALS_BASEDIR)/picohttpparser
PICOHTTPPARSER_STATIC = libpico.a

MULTIPARTPARSER_DIR = ../$(EXTERNALS_BASEDIR)/multipart-parser-c
MULTIPARTPARSER_STATIC = libmultipart.a

INIPARSER_DIR = ../$(EXTERNALS_BASEDIR)/iniparser
INIPARSER_STATIC = libiniparser.a

CCGI_DIR = ../$(EXTERNALS_BASEDIR)/ccgi-1.2
CCGI_STATIC = libccgi.a


WebAlbumCreator:
	@echo "Compiling WebAlbumCreator"
	make -C $(WEBALBUMCREATOR_DIR) -f Makefile static
	cp $(WEBALBUMCREATOR_DIR)/$(WEBALBUMCREATOR_STATIC) $(LIBRARY_DIR)
	cp $(WEBALBUMCREATOR_DIR)/include/WebAlbumCreator.h $(INCLUDE_DIR)
	cp $(WEBALBUMCREATOR_DIR)/include/Exceptions.h $(INCLUDE_DIR)

picohttpparser:
	@echo "Compiling picohttpparser"
	make -C $(PICOHTTPPARSER_DIR) -f Makefile static
	cp $(PICOHTTPPARSER_DIR)/$(PICOHTTPPARSER_STATIC) $(LIBRARY_DIR)
	cp $(PICOHTTPPARSER_DIR)/picohttpparser.h $(INCLUDE_DIR)

multipart_parser:
	@echo "Compiling multipart_parser"
	make -C $(MULTIPARTPARSER_DIR) -f Makefile static
	cp $(MULTIPARTPARSER_DIR)/$(MULTIPARTPARSER_STATIC) $(LIBRARY_DIR)
	cp $(MULTIPARTPARSER_DIR)/multipart_parser.h $(INCLUDE_DIR)

iniparser:
	@echo "Compiling iniparser"
	make -C $(INIPARSER_DIR) -f Makefile static
	cp $(INIPARSER_DIR)/$(INIPARSER_STATIC) $(LIBRARY_DIR)
	cp $(INIPARSER_DIR)/src/iniparser.h $(INCLUDE_DIR)
	cp $(INIPARSER_DIR)/src/dictionary.h $(INCLUDE_DIR)

ccgi:
	@echo "Compiling ccgi"
	make -C $(CCGI_DIR) -f Makefile static
	cp $(CCGI_DIR)/$(CCGI_STATIC) $(LIBRARY_DIR)
	cp $(CCGI_DIR)/ccgi.h $(INCLUDE_DIR)


clean:
	make -C $(PICOHTTPPARSER_DIR) -f Makefile clean
	make -C $(WEBALBUMCREATOR_DIR) -f Makefile clean
	make -C $(MULTIPARTPARSER_DIR) -f Makefile clean
	make -C $(INIPARSER_DIR) -f Makefile clean
	make -C $(CCGI_DIR) -f Makefile clean

.PHONY: WebAlbumCreator picohttpparser multipart_parser iniparser ccgi clean

