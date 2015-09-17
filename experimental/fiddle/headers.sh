#!/bin/sh
# Copyright 2015 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate a single header with skia's main public headers.
cd "$(dirname "$0")/../../include"
printf '#ifndef skia_headers_DEFINED\n'
printf '#define skia_headers_DEFINED\n\n'
for directory in c core effects pathops gpu; do 
    printf '// %s\n' $directory
    find $directory -maxdepth 1 -name "*.h" \
        | sed "s/^.*\/\(.*\)/#include \"\1\"/" \
        | LANG= sort
    printf '\n'
done
printf '#include "gl/GrGLInterface.h"\n'
printf '\n#endif  // skia_headers_DEFINED\n'
