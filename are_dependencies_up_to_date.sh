#!/bin/sh
# Lockbox: Encrypted File System
# Copyright (C) 2013 Rian Hunter <rian@alum.mit.edu>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

DEPS_OUT_OF_DATE_RETURN_CODE=1
DEPS_UP_TO_DATE_RETURN_CODE=0

APPDIR="$1"
if [ -z "$APPDIR" ]; then
    APPDIR="."
fi

# just check libs for now
# TODO: check headers as well!

ANYBOTAN_FILE=$(find "$APPDIR/encfs-dependencies/botan" \
    -type f -and \
    -newer "$APPDIR/out/deps/lib/libbotan-1.10.a" 2>/dev/null | \
    head -n 1)
if [ `uname` != "Darwin" ] && ( [ ! -e "$APPDIR/out/deps/lib/libbotan-1.10.a" ] || [ "$ANYBOTAN_FILE" ] ); then
    echo "botan is out of date" >> /dev/stderr
    exit $DEPS_OUT_OF_DATE_RETURN_CODE
fi

ANYPROTOBUF_FILE=$(find "$APPDIR/../protobuf" \
    -type f -and \
    -newer "$APPDIR/out/deps/lib/libprotobuf.a" 2>/dev/null | \
    grep -v "^$APPDIR/../protobuf/.hg/" | \
    head -n 1)
if [ ! -e "$APPDIR/out/deps/lib/libprotobuf.a" ] || [ "$ANYPROTOBUF" ]; then
    echo "protobuf is out of date" >> /dev/stderr
    exit $DEPS_OUT_OF_DATE_RETURN_CODE
fi

ANYTINYXML_FILE=$(find "$APPDIR/encfs-dependencies/tinyxml" \
    -type f -and \
    -newer "$APPDIR/out/deps/lib/libtinyxml.a" 2>/dev/null | \
    head -n 1)
if [ ! -e "$APPDIR/out/deps/lib/libtinyxml.a" ] || [ "$ANYTINYXML_FILE" ]; then
    echo "tinyxml is out of date" >> /dev/stderr
    exit $DEPS_OUT_OF_DATE_RETURN_CODE
fi

ANYENCFS_FILE=$(find "$APPDIR/../encfs" \
    -type f -and \
    -newer "$APPDIR/out/deps/lib/libencfs.a" 2>/dev/null | \
    grep -v "^$APPDIR/../encfs/.hg/" | \
    head -n 1)
if [ ! -e "$APPDIR/out/deps/lib/libencfs.a" ] || [ "$ANYENCFS_FILE" ]; then
    echo "encfs is out of date" >> /dev/stderr
    exit $DEPS_OUT_OF_DATE_RETURN_CODE
fi

ANYWEBDAV_SERVER_FS_FILE=$(find "$APPDIR/../davfuse" \
    -type f -and \
    -newer "$APPDIR/out/deps/lib/libwebdav_server_fs.a" 2>/dev/null | \
    grep -v "^$APPDIR/../davfuse/.hg/" | \
    head -n 1)
if [ ! -e "$APPDIR/out/deps/lib/libwebdav_server_fs.a" ] || [ "$ANYWEBDAV_SERVER_FS_FILE" ]; then
    echo "davfuse is out of date" >> /dev/stderr
    exit $DEPS_OUT_OF_DATE_RETURN_CODE
fi

exit $DEPS_UP_TO_DATE_RETURN_CODE
