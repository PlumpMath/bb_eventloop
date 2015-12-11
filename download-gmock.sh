#!/bin/sh -e
#
# Script for downloading gmock source code package from remote repository into
# "3rdparty" directory. Please note that gmock is used only for unit testing,
# it MUST NOT be shipped to customers.
#

GMOCK_TARGET_DIR="3rdparty"
GMOCK_VERSION="1.7.0"
GMOCK_DIR="gmock-$GMOCK_VERSION"
GMOCK_PACKAGE="$GMOCK_DIR.zip"
if test -z "$THIRDPARTY_PKG_URL" ; then
    THIRDPARTY_PKG_URL="http://luxembourg.fp.nsn-rdnet.net/3rdparty/packages/"
fi

wget "$THIRDPARTY_PKG_URL$GMOCK_PACKAGE"
unzip "$GMOCK_PACKAGE"
mkdir -p "$GMOCK_TARGET_DIR"
mv "$GMOCK_DIR" "$GMOCK_TARGET_DIR/gmock"
rm -rf "$GMOCK_DIR"
rm -f "$GMOCK_PACKAGE"
