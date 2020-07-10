import java.io._
import java.net._

import java.nio.file.{Files, Paths, Path}

import java.util.{Map, UUID}
import java.util.zip.{ZipEntry, ZipFile, ZipInputStream}

import javax.servlet.MultipartConfigElement
import javax.servlet.http.Part

import scala.collection.mutable.HashMap
import scala.collection.JavaConverters._
import scala.util.control.Breaks._

import spark.Spark._
import spark.{Request, Response, Route}
import spark.utils.IOUtils
import com.google.gson._

object ZipDu {
  val GSON = new Gson()
  val uploadsFolder = "uploads"

  def main(args: Array[String]) = {
    // check if uploads directory exists and exit if not
    val uploadsDirectory = Paths.get(uploadsFolder)
    if (!Files.exists(uploadsDirectory)) {
      System.err.println("Could not find the `uploads` directory in the folder you executed zipdu in. Exitting.")
      System.exit(1)
    }

    // health route
    get("/health", new Route {
      override def handle(request: Request, response: Response): String = {
        val resMap = HashMap("ok" -> true)
        return GSON.toJson(resMap)
      }
    })

    // zipstats route
    post("/zipstats", new Route {
      override def handle(req: Request, res: Response): String = {
          req.attribute("org.eclipse.jetty.multipartConfig", new MultipartConfigElement(System.getProperty("java.io.tmpdir")))
          val filePart = req.raw().getPart("file")
          if (filePart == null) {
            res.status(400)
            return ""
          }

          var zis : ZipInputStream = null
          try {
            zis = new ZipInputStream(filePart.getInputStream())
          } catch {
            case e: FileNotFoundException => {}
          }
          if (zis == null) {
            res.status(400)
            return ""
          }

          // create directory inside the uploads directory
          val directoryName = UUID.randomUUID().toString()
          val directoryPath = Paths.get(uploadsFolder, directoryName)
          val outputDirectory = new File(directoryPath.toString())
          if (outputDirectory.exists()) {
            try { zis.close() } catch { case e: IOException => {} }
            res.status(400)
            return ""
          }
          if (!outputDirectory.mkdir()) {
            try { zis.close() } catch { case e: IOException => {} }
            res.status(400)
            return ""
          }

          // actual unzip operation
          var numberOfFiles: Int = 0
          var totalSize: Int = 0
          try {
            var buffer = new Array[Byte](2048)
            var zipEntry = zis.getNextEntry()
            while (zipEntry != null) {
              breakable {
                val destinationFile = new File(directoryPath.toString(), zipEntry.getName())
                if (zipEntry.isDirectory()) {
                  destinationFile.mkdirs()
                  zipEntry = zis.getNextEntry()
                  break
                }
                val outStream = new FileOutputStream(destinationFile)
                var written: Int = zis.read(buffer)
                while (written > 0) {
                  outStream.write(buffer, 0, written)
                  totalSize += written
                  written = zis.read(buffer)
                }
                outStream.close()
                numberOfFiles += 1

                zipEntry = zis.getNextEntry()
              }
            }
          } catch {
            case e: IOException => {
              res.status(400)
              return ""
            }
          } finally {
            try {
              zis.closeEntry()
              zis.close()
            } catch {
              case e: IOException => {
                res.status(400)
                return e.toString()
              }
            }
          }

          // write response
          val resMap = HashMap(
            "uuid" -> directoryName,
            "totalSize" -> totalSize,
            "numberOfFiles" -> numberOfFiles)
          return GSON.toJson(resMap.asJava)
       }
    })

    notFound(new Route {
      override def handle(req: Request, res: Response): String = {
        res.status(404)
        return ""
      }
    })
  }
}
