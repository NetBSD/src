// Test sanitizers ld flags.

// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target i386-unknown-linux -fsanitize=address \
// RUN:     -resource-dir=%S/Inputs/resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-LINUX %s
//
// CHECK-ASAN-LINUX: "{{(.*[^-.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-ASAN-LINUX-NOT: "-lc"
// CHECK-ASAN-LINUX: libclang_rt.asan-i386.a"
// CHECK-ASAN-LINUX: "-lpthread"
// CHECK-ASAN-LINUX: "-lrt"
// CHECK-ASAN-LINUX: "-ldl"
// CHECK-ASAN-LINUX-NOT: "-export-dynamic"
// CHECK-ASAN-LINUX: "--dynamic-list={{.*}}libclang_rt.asan-i386.a.syms"

// RUN: %clangxx -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target i386-unknown-linux -fsanitize=address \
// RUN:     -resource-dir=%S/Inputs/empty_resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-LINUX-CXX %s
//
// CHECK-ASAN-LINUX-CXX: "{{(.*[^-.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-ASAN-LINUX-CXX-NOT: "-lc"
// CHECK-ASAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.asan-i386.a" "-no-whole-archive"
// CHECK-ASAN-LINUX-CXX: "-lpthread"
// CHECK-ASAN-LINUX-CXX: "-lrt"
// CHECK-ASAN-LINUX-CXX: "-ldl"
// CHECK-ASAN-LINUX-CXX: "-export-dynamic"
// CHECK-ASAN-LINUX-CXX-NOT: "--dynamic-list"
// CHECK-ASAN-LINUX-CXX: stdc++

// RUN: %clang -no-canonical-prefixes %s -### -o /dev/null -fsanitize=address \
// RUN:     -target i386-unknown-linux --sysroot=%S/Inputs/basic_linux_tree \
// RUN:     -lstdc++ -static 2>&1 \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-LINUX-CXX-STATIC %s
//
// CHECK-ASAN-LINUX-CXX-STATIC: "{{(.*[^-.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-ASAN-LINUX-CXX-STATIC-NOT: stdc++
// CHECK-ASAN-LINUX-CXX-STATIC: "-whole-archive" "{{.*}}libclang_rt.asan-i386.a" "-no-whole-archive"
// CHECK-ASAN-LINUX-CXX-STATIC: stdc++

// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target arm-linux-gnueabi -fsanitize=address \
// RUN:     --sysroot=%S/Inputs/basic_android_tree/sysroot \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-ARM %s
//
// CHECK-ASAN-ARM: "{{(.*[^.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-ASAN-ARM-NOT: "-lc"
// CHECK-ASAN-ARM: libclang_rt.asan-arm.a"
//
// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target armv7l-linux-gnueabi -fsanitize=address \
// RUN:     --sysroot=%S/Inputs/basic_android_tree/sysroot \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-ARMv7 %s
//
// CHECK-ASAN-ARMv7: "{{(.*[^.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-ASAN-ARMv7-NOT: "-lc"
// CHECK-ASAN-ARMv7: libclang_rt.asan-arm.a"
//

// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target arm-linux-androideabi -fsanitize=address \
// RUN:     --sysroot=%S/Inputs/basic_android_tree/sysroot \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-ANDROID %s
//
// CHECK-ASAN-ANDROID: "{{(.*[^.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-ASAN-ANDROID-NOT: "-lc"
// CHECK-ASAN-ANDROID: libclang_rt.asan-arm-android.so"
// CHECK-ASAN-ANDROID-NOT: "-lpthread"
//
// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target arm-linux-androideabi -fsanitize=address \
// RUN:     --sysroot=%S/Inputs/basic_android_tree/sysroot \
// RUN:     -shared \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-ANDROID-SHARED %s
//
// CHECK-ASAN-ANDROID-SHARED: "{{(.*[^.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-ASAN-ANDROID-SHARED-NOT: "-lc"
// CHECK-ASAN-ANDROID-SHARED: libclang_rt.asan-arm-android.so"
// CHECK-ASAN-ANDROID-SHARED-NOT: "-lpthread"

// RUN: %clangxx -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target x86_64-unknown-linux -lstdc++ -fsanitize=thread \
// RUN:     -resource-dir=%S/Inputs/resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-TSAN-LINUX-CXX %s
//
// CHECK-TSAN-LINUX-CXX: "{{(.*[^-.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-TSAN-LINUX-CXX-NOT: stdc++
// CHECK-TSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.tsan-x86_64.a" "-no-whole-archive"
// CHECK-TSAN-LINUX-CXX: "-lpthread"
// CHECK-TSAN-LINUX-CXX: "-lrt"
// CHECK-TSAN-LINUX-CXX: "-ldl"
// CHECK-TSAN-LINUX-CXX-NOT: "-export-dynamic"
// CHECK-TSAN-LINUX-CXX: "--dynamic-list={{.*}}libclang_rt.tsan-x86_64.a.syms"
// CHECK-TSAN-LINUX-CXX: stdc++

// RUN: %clangxx -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target x86_64-unknown-linux -lstdc++ -fsanitize=memory \
// RUN:     -resource-dir=%S/Inputs/resource_dir \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-MSAN-LINUX-CXX %s
//
// CHECK-MSAN-LINUX-CXX: "{{(.*[^-.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-MSAN-LINUX-CXX-NOT: stdc++
// CHECK-MSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.msan-x86_64.a" "-no-whole-archive"
// CHECK-MSAN-LINUX-CXX: "-lpthread"
// CHECK-MSAN-LINUX-CXX: "-lrt"
// CHECK-MSAN-LINUX-CXX: "-ldl"
// CHECK-MSAN-LINUX-CXX-NOT: "-export-dynamic"
// CHECK-MSAN-LINUX-CXX: "--dynamic-list={{.*}}libclang_rt.msan-x86_64.a.syms"
// CHECK-MSAN-LINUX-CXX: stdc++

// RUN: %clang -fsanitize=undefined %s -### -o %t.o 2>&1 \
// RUN:     -target i386-unknown-linux \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-UBSAN-LINUX %s
// CHECK-UBSAN-LINUX: "{{.*}}ld{{(.exe)?}}"
// CHECK-UBSAN-LINUX-NOT: libclang_rt.asan
// CHECK-UBSAN-LINUX: "-whole-archive" "{{.*}}libclang_rt.san-i386.a" "-no-whole-archive"
// CHECK-UBSAN-LINUX-NOT: libclang_rt.asan
// CHECK-UBSAN-LINUX: "-whole-archive" "{{.*}}libclang_rt.ubsan-i386.a" "-no-whole-archive"
// CHECK-UBSAN-LINUX-NOT: libclang_rt.ubsan_cxx
// CHECK-UBSAN-LINUX: "-lpthread"
// CHECK-UBSAN-LINUX-NOT: "-lstdc++"

// RUN: %clangxx -fsanitize=undefined %s -### -o %t.o 2>&1 \
// RUN:     -target i386-unknown-linux \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-UBSAN-LINUX-CXX %s
// CHECK-UBSAN-LINUX-CXX: "{{.*}}ld{{(.exe)?}}"
// CHECK-UBSAN-LINUX-CXX-NOT: libclang_rt.asan
// CHECK-UBSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.san-i386.a" "-no-whole-archive"
// CHECK-UBSAN-LINUX-CXX-NOT: libclang_rt.asan
// CHECK-UBSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.ubsan-i386.a" "-no-whole-archive"
// CHECK-UBSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.ubsan_cxx-i386.a" "-no-whole-archive"
// CHECK-UBSAN-LINUX-CXX: "-lpthread"
// CHECK-UBSAN-LINUX-CXX: "-lstdc++"

// RUN: %clang -fsanitize=address,undefined %s -### -o %t.o 2>&1 \
// RUN:     -target i386-unknown-linux \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-UBSAN-LINUX %s
// CHECK-ASAN-UBSAN-LINUX: "{{.*}}ld{{(.exe)?}}"
// CHECK-ASAN-UBSAN-LINUX-NOT: libclang_rt.san
// CHECK-ASAN-UBSAN-LINUX: "-whole-archive" "{{.*}}libclang_rt.asan-i386.a" "-no-whole-archive"
// CHECK-ASAN-UBSAN-LINUX-NOT: libclang_rt.san
// CHECK-ASAN-UBSAN-LINUX: "-whole-archive" "{{.*}}libclang_rt.ubsan-i386.a" "-no-whole-archive"
// CHECK-ASAN-UBSAN-LINUX-NOT: libclang_rt.ubsan_cxx
// CHECK-ASAN-UBSAN-LINUX: "-lpthread"
// CHECK-ASAN-UBSAN-LINUX-NOT: "-lstdc++"

// RUN: %clangxx -fsanitize=address,undefined %s -### -o %t.o 2>&1 \
// RUN:     -target i386-unknown-linux \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-ASAN-UBSAN-LINUX-CXX %s
// CHECK-ASAN-UBSAN-LINUX-CXX: "{{.*}}ld{{(.exe)?}}"
// CHECK-ASAN-UBSAN-LINUX-CXX-NOT: libclang_rt.san
// CHECK-ASAN-UBSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.asan-i386.a" "-no-whole-archive"
// CHECK-ASAN-UBSAN-LINUX-CXX-NOT: libclang_rt.san
// CHECK-ASAN-UBSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.ubsan-i386.a" "-no-whole-archive"
// CHECK-ASAN-UBSAN-LINUX-CXX: "-whole-archive" "{{.*}}libclang_rt.ubsan_cxx-i386.a" "-no-whole-archive"
// CHECK-ASAN-UBSAN-LINUX-CXX: "-lpthread"
// CHECK-ASAN-UBSAN-LINUX-CXX: "-lstdc++"

// RUN: %clang -fsanitize=undefined %s -### -o %t.o 2>&1 \
// RUN:     -target i386-unknown-linux \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:     -shared \
// RUN:   | FileCheck --check-prefix=CHECK-UBSAN-LINUX-SHARED %s
// CHECK-UBSAN-LINUX-SHARED: "{{.*}}ld{{(.exe)?}}"
// CHECK-UBSAN-LINUX-SHARED: libclang_rt.ubsan-i386.a"

// RUN: %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:     -target x86_64-unknown-linux -fsanitize=leak \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-LSAN-LINUX %s
//
// CHECK-LSAN-LINUX: "{{(.*[^-.0-9A-Z_a-z])?}}ld{{(.exe)?}}"
// CHECK-LSAN-LINUX-NOT: "-lc"
// CHECK-LSAN-LINUX: libclang_rt.lsan-x86_64.a"
// CHECK-LSAN-LINUX: "-lpthread"
// CHECK-LSAN-LINUX: "-ldl"

// RUN: %clang -fsanitize=leak,undefined %s -### -o %t.o 2>&1 \
// RUN:     -target x86_64-unknown-linux \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-LSAN-UBSAN-LINUX %s
// CHECK-LSAN-UBSAN-LINUX: "{{.*}}ld{{(.exe)?}}"
// CHECK-LSAN-UBSAN-LINUX-NOT: libclang_rt.san
// CHECK-LSAN-UBSAN-LINUX: "-whole-archive" "{{.*}}libclang_rt.lsan-x86_64.a" "-no-whole-archive"
// CHECK-LSAN-UBSAN-LINUX-NOT: libclang_rt.san
// CHECK-LSAN-UBSAN-LINUX: "-whole-archive" "{{.*}}libclang_rt.ubsan-x86_64.a" "-no-whole-archive"
// CHECK-LSAN-UBSAN-LINUX-NOT: libclang_rt.ubsan_cxx
// CHECK-LSAN-UBSAN-LINUX: "-lpthread"
// CHECK-LSAN-UBSAN-LINUX-NOT: "-lstdc++"

// RUN: %clang -fsanitize=leak,address %s -### -o %t.o 2>&1 \
// RUN:     -target x86_64-unknown-linux \
// RUN:     --sysroot=%S/Inputs/basic_linux_tree \
// RUN:   | FileCheck --check-prefix=CHECK-LSAN-ASAN-LINUX %s
// CHECK-LSAN-ASAN-LINUX: "{{.*}}ld{{(.exe)?}}"
// CHECK-LSAN-ASAN-LINUX-NOT: libclang_rt.lsan
// CHECK-LSAN-ASAN-LINUX: libclang_rt.asan-x86_64
// CHECK-LSAN-ASAN-LINUX-NOT: libclang_rt.lsan
