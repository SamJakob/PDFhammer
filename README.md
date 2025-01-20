# PDFhammerCore

This is an extremely minimal version of [Chromium's PDFium](https://pdfium.googlesource.com/pdfium/)
for Linux x64, specifically (code and build scripts, etc., for other architectures and platforms
have been stripped out).

This version of the library is intended to be easy to configure and build as a static library,
meaning that non-essential features have been stripped out and the build scripts have been
modified to facilitate this.

The build has been modified to support Clang 19+ (rather than Clang 20+ from the upstream library).
This allows using the system distribution of Clang from the `apt` repository under Debian (i.e.,
you can build this with what `apt install clang` gives you). The build scripts will automatically
detect a newer version of Clang and opt-in to any flag(s) supported in that version where they are
not already supported by Clang 19.

## Dependencies

This list may not yet be complete. You will also need to install all of the build libraries and
dependencies described in PDFium's documentation.

```
sudo apt install clang nasm ninja-build lld
```

## Building

```bash
# Automatically configures the "out/Release" directory
# ...and builds the library. Re-invoking the script will
# trigger a clean followed by a rebuild.
./build.sh
```

- The build output can then be found in [out/Release](./out/Release).

## Releasing

```bash
# Copies all public interfaces to the staging directory,
# followed by the static library archive.
./release.sh
```

- The release can then be found in [out/Release/staging](./out/Release/staging).

## Usage

```yaml
prefix=/path/to/staging_or_installed_pdfhammer
lib_dir=${prefix}/libs
include_dir=${prefix}/include

Name: PDFhammerCore
Description: PDF parsing library based on Chromium's PDFium.
Version: 9999
Requires:

Libs: -L${lib_dir} -lpdfium -lstdc++ -llcms2 -lm -ljpeg -ltiff -lz -lopenjp2 -licuuc
Cflags: -I${include_dir}
```
