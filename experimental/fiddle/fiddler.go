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
	"log"
	"os"
	"os/exec"
	"path"
	"strings"
	"syscall"

	"github.com/skia-dev/glog"
)

func setResourceLimits() {
	const maximumTimeInSeconds = 5
	limit := syscall.Rlimit{maximumTimeInSeconds, maximumTimeInSeconds}
	if err := syscall.Setrlimit(syscall.RLIMIT_CPU, &limit); err != nil {
		glog.Fatalf("syscall.Setrlimit(RLIMIT_CPU) error: %v", err)
	}
	const maximumMemoryInBytes = 1 << 28
	limit = syscall.Rlimit{maximumMemoryInBytes, maximumMemoryInBytes}
	if err := syscall.Setrlimit(syscall.RLIMIT_AS, &limit); err != nil {
		glog.Fatalf("syscall.Setrlimit(RLIMIT_CPU) error: %v", err)
	}
}
func trimmedBasePath(fpath string) string {
	basename := path.Base(fpath)
	return strings.TrimSuffix(basename, path.Ext(basename))
}
func fixPath(p string) string {
	sep := fmt.Sprintf("%c", os.PathSeparator)
	return strings.Join([]string{path.Dir(p), path.Base(p)}, sep)
}
func main() {
	setResourceLimits()
	if len(os.Args) < 2 {
		glog.Fatalf("missing argument")
	}
	sourcepath := os.Args[1]
	name := trimmedBasePath(sourcepath)
	binarypath := path.Join("/tmp", name)
	// const FIDDLE_DIR = "/fiddle"
	// const SKIA_SRC = "/fiddle/skia"
	const FIDDLE_DIR = "."
	const SKIA_SRC = "../.."

	compileCmd := exec.Command(
		"c++",
		"--std=c++11",
		"-DSK_RELEASE",
		"-DSK_MESA",
		fmt.Sprintf("-I%s", FIDDLE_DIR),
		fmt.Sprintf("-I%s", path.Join(SKIA_SRC, "include/core")),
		fmt.Sprintf("-I%s", path.Join(SKIA_SRC, "include/gpu")),
		fmt.Sprintf("-I%s", path.Join(SKIA_SRC, "include/effects")),
		fmt.Sprintf("-I%s", path.Join(SKIA_SRC, "include/pathops")),
		fmt.Sprintf("-I%s", path.Join(SKIA_SRC, "include/c")),
		fmt.Sprintf("-I%s", path.Join(SKIA_SRC, "include/utils")),
		fmt.Sprintf("-I%s", path.Join(SKIA_SRC, "include/config")),
		"-o", binarypath,
		sourcepath,
		path.Join(FIDDLE_DIR, "fiddle_main.o"),
		path.Join(FIDDLE_DIR, "libskia.so"),
		"-lOSMesa",
		//"-Wl,-rpath", fmt.Sprintf("-Wl,%s", FIDDLE_DIR)
	)
	var buffer bytes.Buffer
	compileCmd.Stdout = &buffer
	compileCmd.Stderr = &buffer
	if err := compileCmd.Run(); err != nil {
		glog.Fatalf("compile failed:\n\n%s\n[%v]", buffer.String(), err)
	}
	glog.Infof("Successfully compiled %s", name)
	runCmd := exec.Command(fixPath(path.Join(FIDDLE_DIR, "secwrap")), binarypath)
	runCmd.Stdout = os.Stdout
	runCmd.Stderr = &buffer
	if err := runCmd.Run(); err != nil {
		log.Fatalf("execution failed:\n\n%s\n[%v]", buffer.String(), err)
	}
}
