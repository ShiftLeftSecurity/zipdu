INSTALL_DIR = ../
JAR_OUT = target/scala-2.12/zipdu-assembly-0.0.1.jar

.PHONY: build
build:
	@sbt assembly

.PHONY: install
install: build
	@mv $(JAR_OUT) $(INSTALL_DIR)
