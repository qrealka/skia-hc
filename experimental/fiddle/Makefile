# Copyright 2015 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

.PHONEY: all
all: draw
clean:
	git clean -fXd

CXXFLAGS += --std=c++11

LIBSKIA := build/libskia.so
ifeq ($(shell uname), Darwin)
  LIBSKIA := build/libskia.dylib
endif

$(LIBSKIA):
	cmake -Bbuild -H../../cmake -G Ninja && ninja -C build skia

skia.h: headers.sh $(LIBSKIA)
	sh headers.sh > skia.h

SKIA_SRC = ../..

SKIA_FLAGS := \
  -DSK_RELEASE -DSK_MESA \
  -I$(SKIA_SRC)/include/core \
  -I$(SKIA_SRC)/include/gpu \
  -I$(SKIA_SRC)/include/effects \
  -I$(SKIA_SRC)/include/pathops \
  -I$(SKIA_SRC)/include/c \
  -I$(SKIA_SRC)/include/utils \
  -I$(SKIA_SRC)/include/config

fiddle_main.o: fiddle_main.cpp fiddle_main.h skia.h
	$(CXX) $(CXXFLAGS) $(SKIA_FLAGS) skia.h
	$(CXX) $(CXXFLAGS) $(SKIA_FLAGS) -c -o $@ $<

################################################################################

FIDDLE_LD := -lOSMesa -Wl,-rpath -Wl,"$(PWD)/build"

FIDDLE_SRCS := draw.cpp fiddle_main.o $(LIBSKIA)

draw: $(FIDDLE_SRCS) skia.h fiddle_main.h
	$(CXX) $(CXXFLAGS) $(SKIA_FLAGS) -o $@ $(FIDDLE_SRCS) $(FIDDLE_LD)
