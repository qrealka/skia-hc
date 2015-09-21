/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
package main

import (
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"text/template"

	"github.com/skia-dev/glog"
)

/*
	Expected directory heirarchy (everything but /tmp can be read-only):

	/
	├── tmp
	├── usr
	│   ├── include
	│   └── lib
	├── webtryimages
	├── webtrybinaries
	└── fiddle
	    ├── fiddle_main.h
	    ├── fiddle_main.o
	    ├── libskia.so
	    ├── skia
	    │   └── include
	    ├── skia.h
	    └── skia.h.gch
*/

// // template.cpp
// #include "fiddle_main.h"
// DrawOptions GetDrawOptions() {
//     return DrawOptions({{.Width}}, {{.Height}},
//                        {{.Raster}}, {{.Gpu}}, {{.Pdf}}, {{.Source}});
// }
// {{.Code}}
type CodeData struct {
	Filename string
	Width    int
	Height   int
	Raster   bool
	Gpu      bool
	Pdf      bool
	Source   string
	Code     string
	Name     string
}

func f(codeData CodeData, codeTemplate *template.Template) error {
	if len(os.Args) < 2 {
		return fmt.Errorf("missing argument")
	}
	actualsourcepath := fmt.Sprintf("/home/webtry/sources/%s.cpp", codeData.Name)
	f, err := os.Create(actualsourcepath)
	if err != nil {
		return err
	}
	if err := codeTemplate.Execute(f, codeData); err != nil {
		if err2 := f.Close(); err2 != nil {
			glog.Errorf("Failed to Close(): %v", err2)
		}
		return err
	}
	sourcepath := fmt.Sprintf("/webtrysources/%s.cpp", codeData.Name)
	binarypath := fmt.Sprintf("/webtrybinaries/%s", codeData.Name)
	compileCmd := exec.Command(
		"schroot", "-c", "webtry", "-d", "/", "--",
		"c++",
		"--std=c++11",
		"-DSK_RELEASE",
		"-DSK_MESA",
		"-I/fiddle",
		"-I/fiddle/skia/include/core",
		"-I/fiddle/skia/include/gpu",
		"-I/fiddle/skia/include/effects",
		"-I/fiddle/skia/include/pathops",
		"-I/fiddle/skia/include/c",
		"-I/fiddle/skia/include/utils",
		"-I/fiddle/skia/include/config",
		"-o", binarypath,
		sourcepath,
		"/fiddle/fiddle_main.o",
		"/fiddle/libskia.so",
		"-lOSMesa",
		"-Wl,-rpath", "-Wl,/fiddle")
	var buffer bytes.Buffer
	compileCmd.Stdout = &buffer
	compileCmd.Stderr = &buffer
	if err := compileCmd.Run(); err != nil {
		glog.Fatalf("compile failed:\n\n%s\n[%v]", buffer.String(), err)
	}
	runCmd := exec.Command(
		"schroot", "-c", "webtry", "-d", "/", "--",
		"/fiddle/secwrap", binarypath)
	runCmd.Stdout = os.Stdout
	runCmd.Stderr = &buffer
	if err := runCmd.Run(); err != nil {
		glog.Fatalf("execution failed:\n\n%s\n[%v]", buffer.String(), err)
	}
	// actualbinarypath := fmt.Sprintf("/home/webtry/binaries/%s", codeData.Name)
	// rm actualbinarypath
}

////////////////////////////////////////////////////////////////////////////////
