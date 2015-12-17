#!/bin/sh
# Copyright 2015 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
cd "$(dirname "$0")"
tdir=$(mktemp -d "tmp.XXXXXXXXXX")
protoc --cpp_out="$tdir" skia.proto
for file in skia.pb.h skia.pb.cc; do
    if ! diff "$tdir/$file" "$file" > /dev/null; then
        mv "$tdir/$file" "$file"
    fi
done
rm "$tdir"/*; rmdir "$tdir"
