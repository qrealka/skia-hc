/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#ifndef Slide_DEFINED
#define Slide_DEFINED

#include "SkRefCnt.h"
#include "SkString.h"

class SkCanvas;
class SkAnimTimer;

class Slide : public SkRefCnt {
public:
    virtual ~Slide() {}

    virtual void draw(SkCanvas* canvas) = 0;
    virtual bool animate(const SkAnimTimer&) { return false;  }
    virtual void load() {}
    virtual void unload() {}

    SkString getName() { return fName; }

protected:
    SkString    fName;
};


#endif
