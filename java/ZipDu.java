import java.io.*;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.Path;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;

import javax.servlet.MultipartConfigElement;
import javax.servlet.http.Part;

import static spark.Spark.*;
import spark.utils.IOUtils;
import com.google.gson.*; 

public class ZipDu {
  public static Gson GSON = new Gson();
  public static String uploadsFolder = "uploads";

  public static void main(String[] args) {
    // check if uploads directory exists and exit if not
    Path uploadsDirectory = Paths.get(uploadsFolder);
    if (!Files.exists(uploadsDirectory)) {
      System.err.println("Could not find the `uploads` directory in the folder you executed zipdu in. Exitting.");
      System.exit(1);
    }

    // health route
    get("/health", (req, res) -> {
      res.type("application/json");

      Map<String, Object> resMap = new HashMap<String, Object>();
      resMap.put("ok", true);
      return GSON.toJson(resMap);
    });

    // zipstats route
    post("/zipstats", (req, res) -> {
      res.type("application/json");

      req.attribute("org.eclipse.jetty.multipartConfig", new MultipartConfigElement(System.getProperty("java.io.tmpdir")));
      Part filePart = req.raw().getPart("file");
      if (filePart == null) {
        res.status(400);
        return "";
      }

      ZipInputStream zis = null;
      try {
        zis = new ZipInputStream(filePart.getInputStream());
      } catch(FileNotFoundException e) {}
      if (zis == null) {
        res.status(400);
        return "";
      }

      // create directory inside the uploads directory
      String directoryName = UUID.randomUUID().toString();
      Path directoryPath = Paths.get(uploadsFolder, directoryName);
      File outputDirectory = new File(directoryPath.toString());
      if (outputDirectory.exists()) {
        try { zis.close(); } catch (IOException e) {}
        res.status(400);
        return "";
      }
      if (!outputDirectory.mkdir()) {
        try { zis.close(); } catch (IOException e) {}
        res.status(400);
        return "";
      }

      // actual unzip operation
      int numberOfFiles = 0;
      int totalSize = 0;
      try {
        byte[] buffer = new byte[2048];
        ZipEntry zipEntry = zis.getNextEntry();
        while (zipEntry != null) {
          File destinationFile = new File(directoryPath.toString(), zipEntry.getName());
          if (zipEntry.isDirectory()) {
            destinationFile.mkdirs();
            zipEntry = zis.getNextEntry();
            continue;
          }
          FileOutputStream outStream = new FileOutputStream(destinationFile);
          int written;
          while ((written = zis.read(buffer)) > 0) {
            outStream.write(buffer, 0, written);
            totalSize += written;
          }
          outStream.close();
          numberOfFiles += 1;

          zipEntry = zis.getNextEntry();
        }
      } catch (IOException e) {
        res.status(400);
        return "";
      } finally {
        try {
          zis.closeEntry();
          zis.close();
        } catch (IOException e) {
          res.status(400);
          return e.toString();
        }
      }

      // write response
      Map<String, Object> resMap = new HashMap<String, Object>();
      resMap.put("uuid", directoryName);
      resMap.put("totalSize", totalSize);
      resMap.put("numberOfFiles", numberOfFiles);
      return GSON.toJson(resMap);
    });

    // notFound route
    notFound((req, res) -> {
      res.status(404);
      return "";
    });
  }
}
