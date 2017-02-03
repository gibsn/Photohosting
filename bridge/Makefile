INCLUDE_DIR = include
LIBRARY_DIR = lib
EXTERNALS_BASEDIR = externals

WEBALBUMCREATOR_DIR = ../$(EXTERNALS_BASEDIR)/WebAlbumCreator
WEBALBUMCREATOR_STATIC = libwac.a

PICOHTTPPARSER_DIR = ../$(EXTERNALS_BASEDIR)/picohttpparser
PICOHTTPPARSER_STATIC = libpico.a

MULTIPARTPARSER_DIR = ../$(EXTERNALS_BASEDIR)/multipart-parser-c
MULTIPARTPARSER_STATIC = libmultipart.a


WebAlbumCreator:
	@echo "Compiling WebAlbumCreator"
	make -C $(WEBALBUMCREATOR_DIR) -f Makefile static
	cp $(WEBALBUMCREATOR_DIR)/$(WEBALBUMCREATOR_STATIC) $(LIBRARY_DIR)
	cp $(WEBALBUMCREATOR_DIR)/include/WebAlbumCreator.h $(INCLUDE_DIR)

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


clean:
	make -C $(PICOHTTPPARSER_DIR) -f Makefile clean
	make -C $(WEBALBUMCREATOR_DIR) -f Makefile clean
	make -C $(MULTIPARTPARSER_DIR) -f Makefile clean

.PHONY: WebAlbumCreator picohttpparser multipart_parser clean

