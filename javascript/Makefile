INSTALL_DIR = ../
INDEX_JS_OUT = dist/index.js

.PHONY: build
build:
	@yarn install
	@yarn makeDist

.PHONY: install
install: build
	@mv $(INDEX_JS_OUT) $(INSTALL_DIR)/zipdu-dist.js
