INSTALL_DIR = ../
JAR_OUT = build/libs/zipdu-0.0.1-all.jar

.PHONY: build
build:
	@gradle shadowJar

.PHONY: install
install: build
	@mv $(JAR_OUT) $(INSTALL_DIR)
