BIN_OUT = ./build/bin/zipdu
INSTALL_DIR = ../

.PHONY: build
build:
	@./build.sh
	@cp $(BIN_OUT) $(INSTALL_DIR)

.PHONY: install
install: build
	@mv $(BIN_OUT) $(INSTALL_DIR)
