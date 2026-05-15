# Photohosting

Photohosting is a small C++ photo hosting service. It accepts an archive with
photos, generates a static web album, and serves the result over HTTP.

The project can be built either as a standalone HTTP server or as a CGI binary.

## Key Feature

Photohosting has no runtime installation dependencies: all third-party
libraries are compiled statically and linked into the final binary.

## Build

Initialize bundled dependencies:

```sh
git submodule init
git submodule update
```

Build the standalone server:

```sh
make sa
```

Build the CGI binary:

```sh
make cgi
```

## Run

Edit `photohosting.ini` if needed, then start the standalone server:

```sh
./server
```

By default, static files are served from `public_html`, uploaded albums are
stored under the same directory, and authentication data is read from the
configured users file.

## Authentication

User credentials are loaded from the configured users file. After a successful
login, the service creates a file-backed session token and sends it to the
client in the `sid` cookie. Uploaded albums are only available to authenticated
users.
