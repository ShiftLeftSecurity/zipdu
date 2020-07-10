const fs = require('fs');
const path = require('path');
const process = require('process');

const express = require('express');
const multer  = require('multer');
const admzip = require('adm-zip');
const uuid = require('uuid').v4;

const storage = multer.memoryStorage();
const upload = multer({ storage: storage });
const app = express();

const port = 3000;

const uploadsFolder = "uploads";
// check if uploads directory exists and exit if not
if (!fs.existsSync(uploadsFolder)) {
  console.error(new Error('Could not find the `uploads` directory in the folder you executed zipdu in. Exitting.'));
  process.exit(1);
}

app.get('/health', (req, res) => res.json({'ok': true}));

app.post('/zipstats', upload.single('file'), (req, res) => {
  if (req.file == null) {
    return res.status(400).send()
  }

  const zis = new admzip(req.file.buffer);
  if (zis == null) {
    return res.status(400).send()
  }

  // create directory inside the uploads directory
  const directoryName = uuid();
  const directoryPath = path.join(uploadsFolder, directoryName);
  if (fs.existsSync(directoryPath)) {
    return res.status(400).send()
  }
  try {
    fs.mkdirSync(directoryPath);
  } catch(e) {
    return res.status(400).send()
  }

  // actual unzip operation
  var errored = false;
  var numberOfFiles = 0;
  var totalSize = 0;
  const zipEntries = zis.getEntries();
  for (var i = 0; i < zipEntries.length; i++) {
    const zipEntry = zipEntries[i];
    const destinationPath = path.join(directoryPath, zipEntry.entryName);

    if (zipEntry.isDirectory === true) {
      try {
        fs.mkdirSync(destinationPath);
        continue;
      } catch (e) {
        return res.status(400).send();
      }
    }

    var destinationFd = null;
    try {
        destinationFd = fs.openSync(destinationPath, 'w+');
    } catch (err) {
        return res.status(400).send();
    }

    if (destinationFd === null) {
      return res.status(400).send();
    }

    const data = zipEntry.getData();
    const writeStream = fs.createWriteStream(null, { fd: destinationFd });
    writeStream.write(data.toString('utf-8'), 'utf-8');
    writeStream.on('finish', () => { fs.closeSync(destinationFd); });
    writeStream.on('error', (e) => {
      errored = true;
      fs.closeSync(destinationFd);
    });
    if (errored === true) {
      return res.status(400).send()
    }

    numberOfFiles += 1;
    totalSize += data.length;
  }

  res.json({
    'uuid': directoryName,
    'totalSize': totalSize,
    'numberOfFiles': numberOfFiles
  })
});

app.listen(port, () => console.log(`Serving at http://localhost:${port}`));
