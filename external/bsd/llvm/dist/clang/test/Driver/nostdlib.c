// RUN: %clang -target i686-pc-linux-gnu -### -nostdlib %s 2> %t
// RUN: FileCheck < %t %s
//
// CHECK-NOT: start-group

// Most of the toolchains would check for -nostartfiles and -nostdlib
// in a short-circuiting boolean expression, so if both of the preceding
// options were present, the second would warn about being unused.
// RUN: %clang -### -nostartfiles -nostdlib -target i386-apple-darwin %s \
// RUN:   2>&1 | FileCheck %s -check-prefix=ARGSCLAIMED
// ARGSCLAIMED-NOT: warning:

// In the presence of -nostdlib, the standard libraries should not be
// passed down to link line
// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target i686-pc-linux-gnu -nostdlib --rtlib=compiler-rt \
// RUN:     -resource-dir=%S/Inputs/resource_dir -lclang_rt.builtins-i686 \
// RUN:   | FileCheck --check-prefix=CHECK-LINUX-NOSTDLIB %s
//
// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target i686-pc-linux-gnu --rtlib=compiler-rt -nostdlib \
// RUN:     -resource-dir=%S/Inputs/resource_dir -lclang_rt.builtins-i686 \
// RUN:   | FileCheck --check-prefix=CHECK-LINUX-NOSTDLIB %s
//
// CHECK-LINUX-NOSTDLIB: warning: argument unused during compilation: '--rtlib=compiler-rt'
// CHECK-LINUX-NOSTDLIB: "{{(.*[^.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-LINUX-NOSTDLIB-NOT: "{{.*}}/Inputs/resource_dir{{/|\\\\}}lib{{/|\\\\}}linux{{/|\\\\}}libclang_rt.builtins-i686.a"
