# Copyright 2015 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# GYP file to build experimental directory.
{
  'targets': [
    {
      'target_name': 'experimental',
      'type': 'static_library',
      'include_dirs': [
        '../include/config',
        '../include/core',
      ],
      'sources': [
        '../experimental/SkSetPoly3To3.cpp',
        '../experimental/SkSetPoly3To3_A.cpp',
        '../experimental/SkSetPoly3To3_D.cpp',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../experimental',
        ],
      },
    },
    {
      'target_name': 'protobuff',
      'type': 'executable',
      'include_dirs': [
        #TODO(halcanary): fix this
        '<!@(printf %s "$HOME")/local/include',
        '../experimental/protobuff',
        '../src/core',
        '../include/private',
      ],
      'dependencies': [
        'etc1.gyp:libetc1',
        'libpng.gyp:libpng',
        'skia_lib.gyp:skia_lib',
      ],
      'libraries' : [
        #TODO(halcanary): fix this
        '<!@(printf %s "$HOME")/local/lib/libprotobuf.a'
      ],
      'sources': [
        '../experimental/protobuff/main.cpp',
        '../experimental/protobuff/ProtocolRemoteEncoder.cpp',
        '../experimental/protobuff/skia.pb.cc',
      ],
    },
  ],
}
