package main

import (
	"archive/zip"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"time"

	"github.com/google/uuid"
	"github.com/gorilla/mux"
)

type zipstatsHandler struct {
	outputDirectory string
}

func (h zipstatsHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Error(w, "Only POST is allowed", http.StatusNotFound)
		return
	}

	// max file size of 10MB
	r.ParseMultipartForm(10 << 20)

	file, _, err := r.FormFile("file")
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	defer file.Close()

	// open file stream
	fileBuffer, err := ioutil.ReadAll(file)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// read the file from the stream
	zipReader, err := zip.NewReader(bytes.NewReader(fileBuffer), int64(len(fileBuffer)))
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// create directory to hold the files
	directoryName := uuid.New().String()
	directoryPath := filepath.Join(h.outputDirectory, directoryName)
	if err = os.Mkdir(directoryPath, os.ModePerm); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	numberOfFiles := 0
	totalSize := int64(0)
	zipExtractionError := ""
	for _, zf := range zipReader.File {
		filePath := filepath.Join(directoryPath, zf.Name)

		// for directories, the extraction is straightforward
		if zf.FileInfo().IsDir() {
			os.MkdirAll(filePath, zf.Mode())
			continue
		}

		// create subdirectories if necessary
		if err = os.MkdirAll(filepath.Dir(filePath), os.ModePerm); err != nil {
			zipExtractionError = err.Error()
			break
		}

		// open stream to write extracted file
		destinationFile, err := os.OpenFile(filePath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, zf.Mode())
		if err != nil {
			zipExtractionError = err.Error()
			break
		}

		// open current zip file for reading
		sourceFile, err := zf.Open()
		if err != nil {
			zipExtractionError = err.Error()
			destinationFile.Close()
			break
		}

		// copy contents of sourceFile to destinationFile
		written, err := io.Copy(destinationFile, sourceFile)

		// count stats of the current file
		numberOfFiles += 1
		totalSize += written

		destinationFile.Close()
		sourceFile.Close()
		if err != nil {
			zipExtractionError = err.Error()
			break
		}
	}

	if len(zipExtractionError) > 0 {
		http.Error(w, zipExtractionError, http.StatusInternalServerError)
		return
	}

	json.NewEncoder(w).Encode(map[string]interface{}{
		"uuid":          directoryName,
		"numberOfFiles": numberOfFiles,
		"totalSize":     totalSize,
	})
}

func main() {
	router := mux.NewRouter()

	router.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		json.NewEncoder(w).Encode(map[string]bool{"ok": true})
	})

	uploadsDirectoryPath := "uploads"
	_, err := os.Stat(uploadsDirectoryPath)
	if os.IsNotExist(err) {
		log.Fatal("Could not find the `uploads` directory in the folder you executed zipdu in. Exitting.")
	}

	zh := zipstatsHandler{outputDirectory: uploadsDirectoryPath}
	router.PathPrefix("/zipstats").Handler(zh)

	bindAddress := "127.0.0.1:8000"
	srv := &http.Server{
		Handler:      router,
		Addr:         bindAddress,
		WriteTimeout: 60 * time.Second,
		ReadTimeout:  60 * time.Second,
	}

	fmt.Println("Starting server on port 8000")
	log.Fatal(srv.ListenAndServe())
}
