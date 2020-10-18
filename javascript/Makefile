INSTALL_DIR = ../
INDEX_JS_OUT = dist/index.js

.PHONY: build
build:
	@yarnpkg install
	@yarnpkg makeDist

.PHONY: install
install: build
	@mv $(INDEX_JS_OUT) $(INSTALL_DIR)/zipdu-dist.js
