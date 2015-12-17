/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef ProtocolRemoteEncoder_DEFINED
#define ProtocolRemoteEncoder_DEFINED

#include "SkRemote.h"

class SkStream;
class SkWStream;

namespace ProtocolRemoteEncoder {
/**
 *  Return a new text encoder that encodes the SkRemote commands.
 */
SkRemote::Encoder* CreateEncoder(SkWStream*);

/**
 *  Decode the encoded SkRemote commands from CreateTextEncoder.
 */
bool Decode(SkRemote::Encoder* dst, SkStream* src);
}  // namespace SkRemote


#endif  // ProtocolRemoteEncoder_DEFINED
