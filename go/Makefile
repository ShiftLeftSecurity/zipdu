GO_FILE = zipdu.go
GO_OUT = zipdu
INSTALL_DIR = ../

.PHONY: getDeps
getDeps:
	@go get "github.com/google/uuid"
	@go get "github.com/gorilla/mux"

.PHONY: build
build: getDeps
	@go build -o $(GO_OUT) $(GO_FILE)

.PHONY: install
install: build
	@mv $(GO_OUT) $(INSTALL_DIR)

