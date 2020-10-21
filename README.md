## zipdu

zipdu is a web service which exposes an HTTP endpoint that accepts a zip file upload as input and returns its inflated byte size and number of contained files in a JSON-formatted response.
zipdu is vulnerable to zip bombs and directory traversal attacks.

----------

### Setup

**First**, follow the README instructions in the language directory of your choice to prepare `zipdu` for execution.

**Second**, run `zipdu` in a terminal session from the root folder of this repository:

```bash
// for native binaries [go, cplusplus]
$ ./zipdu
Starting server on port 8000
// ----------------------------------------

// for a jar [java, scala]
$ java -jar zipdu-0.0.1-all.jar
// ...trimmed output
2020-10-18 16:19:48.518:INFO:oejs.Server:Thread-0: Started @154ms
// ----------------------------------------

// for a javascript file
$ node zipdu-dist.js
// ...trimmed output
Serving at http://localhost:8000
// ----------------------------------------
```

**Third**, using a tool like `curl`, confirm that your process responds to HTTP requests against the `/health` endpoint:

```bash
// second terminal session
$ curl http://localhost:8000/health
{"ok":true}
```

----------

### How to execute the zip bomb attack

**First**, set up `zipdu` for your language of choice, and run it from the root folder of this repository.

**Afterwards**, send a POST request to the `/zipstats` endpoint using the `philkatz.zip` file found in the `bombs` directory (compressed size ~971KB, expands to ~1GB):

```bash
$ curl -XPOST -F file=@bombs/philkatz.zip http://localhost:8000/zipstats
```

If you _really_ don't care about messing up the system `zipdu` is running on, the `bombs` directory also contains a file named `42.zip` which has a compressed size of ~42KB and expands to ~4500TB.


----------

### How to execute the directory traversal attack

**First**, set up `zipdu` for your language of choice, and run it from the root folder of this repository.

**Second**, check the contents of the `execution.sh` script found in the root folder of this repository:

```bash
// this file will be overwritten if the directory traversal attack is successfull
$ cat execution.sh 
#!/usr/bin/env bash

echo "Harmless NOOP executed successfully."
```

**Third**, execute the directory traversal attack by sending the specially crafted zip archive found under `slips/slipwell.zip`:

```bash
$ curl -XPOST -F file=@slips/slipwell.zip http://localhost:8000/zipstats
```

**Finally**, check that the contents of `execution.sh` have been overwritten:

```bash
$ cat execution.sh 
#!/usr/bin/env bash

readonly UPLOAD_FOLDER=./uploads

rm -rf "${UPLOAD_FOLDER}"
echo "This script slipped through and the upload folder is now gone."
```

----------

### Notes

The `42.zip` zip bomb was taken from [https://www.unforgettable.dk/](https://www.unforgettable.dk/) and introduced in [https://www.usenix.org/system/files/woot19-paper_fifield_0.pdf](https://www.usenix.org/system/files/woot19-paper_fifield_0.pdf).

### Further reading

https://www.bamsoftware.com/hacks/zipbomb/

