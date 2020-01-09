# chromium79
This repository contains the v79 edition of the open source Chromium project used by webOS.

Find more information about open source Chromium in Google's [The Chromium Project](http://www.chromium.org/developers/design-documents/) page.

## How to Build
This repository will be built within OE(Open Embedded) build system of webOS platform.

Note: Currently, chromium79 is not the official webruntime for webOS as its still "Work In Progress".
To enable chromium79 as webruntime when building webOS, you need to follow below steps:
1. checkout build-webos
2. create webos-local.conf file in build-webos with following contents before building build-webos:
PREFERRED_VERSION_webruntime = "79.%"
3. follow instructions on build-webos README on how to build image

## Copyright and License Information
See the file src/LICENSE
