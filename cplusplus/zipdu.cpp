#include "Poco/Exception.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/MultipartReader.h"
#include "Poco/Net/PartHandler.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Path.h"
#include "Poco/StreamCopier.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/UUIDGenerator.h"
#include "Poco/Zip/ParseCallback.h"
#include "Poco/Zip/ZipArchive.h"
#include "Poco/Zip/ZipCommon.h"
#include "Poco/Zip/ZipException.h"
#include "Poco/Zip/ZipLocalFileHeader.h"
#include "Poco/Zip/ZipStream.h"

#include <iostream>
#include <sstream>

using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::ServerSocket;
using Poco::Path;
using Poco::Util::Application;
using Poco::Util::ServerApplication;
using Poco::UUIDGenerator;
using Poco::Zip::ParseCallback;
using Poco::Zip::ZipArchive;
using Poco::Zip::ZipCommon;
using Poco::Zip::ZipInputStream;
using Poco::Zip::ZipLocalFileHeader;

class ZipParseCallback: public Poco::Zip::ParseCallback {
public:
  std::string outputDirectory;
  int numberOfFiles = 0;
  int totalSize = 0;

  ZipParseCallback(std::string outputDirectory) {
    this->outputDirectory = outputDirectory;
  }

  bool handleZipEntry(std::istream& zipStream, const ZipLocalFileHeader& header) {
    if (header.isDirectory()) {
      std::string dirName = header.getFileName();
      Path dir(Path(this->outputDirectory), dirName);
      dir.makeDirectory();
      Poco::File aFile(dir);
      aFile.createDirectories();
      return true;
    }

    try {
      std::string fileName = header.getFileName();
      Path file(this->outputDirectory, fileName);
      file.makeFile();

      Poco::FileOutputStream out(file.toString());
      Poco::Zip::ZipInputStream inp(zipStream, header, false);
      Poco::StreamCopier::copyStream(inp, out);
      out.close();
      Poco::File aFile(file.toString());
      if (!inp.crcValid()) {
        throw std::runtime_error("CRC invalid.");
        return false;
      }
      if (aFile.getSize() != header.getUncompressedSize()) {
        throw std::runtime_error("Decompressed file has invalid size.");
        return false;
      }
      std::pair<const ZipLocalFileHeader, const Path> tmp = std::make_pair(header, file);
    } catch (const std::exception& e) {
      throw std::runtime_error(e.what());
      return false;
    }
    return true;
  }
};


class ZipStatsHandler: public HTTPRequestHandler {
  public:
  std::string uploadsDirectoryPath;
  ZipStatsHandler(const std::string uploadsDirectoryPath) {
    this->uploadsDirectoryPath = uploadsDirectoryPath;
  }

  void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    response.setContentType("application/json");

    const unsigned int contentLength = request.getContentLength();
    if (contentLength == 0) {
      response.setStatus(HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST);
      response.send();
    }

    // copy request body into char buffer
    char buffer[contentLength];
    int size = 0;
    char ch;
    static const int eof = std::char_traits<char>::eof();
    while((ch = request.stream().get()) != eof) {
      buffer[size] = ch;
      size += 1;
    }
    std::istringstream requestBody(std::string(buffer, size));

    // create directory inside uploads folder
    auto uuid_str = UUIDGenerator().createRandom().toString();
    std::cout << this->uploadsDirectoryPath;
    Path outputPath;
    outputPath.pushDirectory(this->uploadsDirectoryPath);
    outputPath.pushDirectory(uuid_str);
    Poco::File outputDirectory(outputPath.toString());
    if (outputDirectory.exists()) {
      response.setStatus(HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST);
      response.send();
      return;
    }
    if (!outputDirectory.createDirectory()) {
      response.setStatus(HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST);
      response.send();
      return;
    }

    // actual unzip operation
    int numberOfFiles = 0;
    int totalSize = 0;
    try {
      ZipParseCallback parseCallback = ZipParseCallback(outputDirectory.path());
      Poco::Zip::ZipArchive archive(requestBody, parseCallback);
    } catch (const std::exception& e) {
      response.setStatus(HTTPResponse::HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
      response.send();
      return;
    }

    response.setStatus(HTTPResponse::HTTPStatus::HTTP_OK);
    std::ostream& ostr = response.send();
    ostr << "{\"uuid\": \""<< uuid_str << "\", \"totalSize\": "<< totalSize << ", \"numberOfFiles\": " << numberOfFiles <<"}";
  }
};

class EmptyHandler: public HTTPRequestHandler {
public:
  void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    response.setStatus(HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
    std::ostream& ostr = response.send();
  }
};

class HealthHandler: public HTTPRequestHandler {
public:
  void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    response.setContentType("application/json");
    std::ostream& ostr = response.send();
    ostr << "{\"ok\": true}";
  }
};

class HandlerFactory: public HTTPRequestHandlerFactory {
public:
  std::string uploadsDirectoryPath;
  HandlerFactory(const std::string uploadsDirectoryPath) {
    this->uploadsDirectoryPath = uploadsDirectoryPath;
  }

  HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) {
    if (request.getURI() == "/zipstats" && request.getMethod() == "POST") {
      return new ZipStatsHandler(this->uploadsDirectoryPath);
    } else if (request.getURI() == "/health" && request.getMethod() == "GET") {
      return new HealthHandler();
    } else {
      return new EmptyHandler();
    }
  }
};

class Server: public Poco::Util::ServerApplication {
  public:
    std::string uploadsDirectoryPath;
    Server(const std::string uploadsDirectoryPath) {
      this->uploadsDirectoryPath = uploadsDirectoryPath;
    }

  protected:
  int main(const std::vector<std::string>& args) {
    HTTPServer srv(new HandlerFactory(this->uploadsDirectoryPath), ServerSocket(8000), new HTTPServerParams);
    srv.start();
    waitForTerminationRequest();
    srv.stop();
    return Application::EXIT_OK;
  }
};

int main(int argc, char** argv) {
  const std::string uploadsDirectoryPath = "uploads";
  Poco::File uploadsDirectory(uploadsDirectoryPath);
  bool exists = uploadsDirectory.exists();
  if (!exists) {
    std::cerr << "Could not find the `uploads` directory in the folder you executed zipdu in. Exitting." << std::endl;
    exit(1);
  }

  std::cout << "Starting up the server." << std::endl;
  Server app(uploadsDirectoryPath);
  return app.run(argc, argv);
}
