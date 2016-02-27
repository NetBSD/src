// RUN: %clang -no-canonical-prefixes -### -target sparc-myriad-rtems-elf %s \
// RUN: --gcc-toolchain=%S/Inputs/basic_myriad_tree 2>&1 | FileCheck %s -check-prefix=LINK_WITH_RTEMS
// LINK_WITH_RTEMS: Inputs{{.*}}crti.o
// LINK_WITH_RTEMS: Inputs{{.*}}crtbegin.o
// LINK_WITH_RTEMS: "-L{{.*}}Inputs/basic_myriad_tree/lib/gcc/sparc-myriad-elf/4.8.2/../../..{{/|\\\\}}../sparc-myriad-elf/lib"
// LINK_WITH_RTEMS: "-L{{.*}}Inputs/basic_myriad_tree/lib/gcc/sparc-myriad-elf/4.8.2"
// LINK_WITH_RTEMS: "--start-group" "-lc" "-lrtemscpu" "-lrtemsbsp" "--end-group" "-lgcc"
// LINK_WITH_RTEMS: Inputs{{.*}}crtend.o
// LINK_WITH_RTEMS: Inputs{{.*}}crtn.o

// RUN: %clang -c -no-canonical-prefixes -### -target sparc-myriad-rtems-elf -x c++ %s \
// RUN: --gcc-toolchain=%S/Inputs/basic_myriad_tree 2>&1 | FileCheck %s -check-prefix=COMPILE_CXX
// COMPILE_CXX: "-internal-isystem" "{{.*}}/Inputs/basic_myriad_tree/lib/gcc/sparc-myriad-elf/4.8.2/../../../../sparc-myriad-elf/include/c++/4.8.2"
// COMPILE_CXX: "-internal-isystem" "{{.*}}/Inputs/basic_myriad_tree/lib/gcc/sparc-myriad-elf/4.8.2/../../../../sparc-myriad-elf/include/c++/4.8.2/sparc-myriad-elf"
// COMPILE_CXX: "-internal-isystem" "{{.*}}/Inputs/basic_myriad_tree/lib/gcc/sparc-myriad-elf/4.8.2/../../../../sparc-myriad-elf/include/c++/4.8.2/backward"

// RUN: %clang -### -E -target sparc-myriad --sysroot=/yow %s 2>&1 \
// RUN:   | FileCheck %s -check-prefix=SLASH_INCLUDE
// SLASH_INCLUDE: "-isysroot" "/yow" "-internal-isystem" "/yow/include"

// RUN: %clang -### -E -target sparc-myriad --sysroot=/yow %s -nostdinc 2>&1 \
// RUN:   | FileCheck %s -check-prefix=NO_SLASH_INCLUDE
// NO_SLASH_INCLUDE: "-isysroot" "/yow"
// NO_SLASH_INCLUDE-NOT: "-internal-isystem" "/yow/include"

// RUN: %clang -### -target what-myriad %s 2>&1 | FileCheck %s -check-prefix=BAD_ARCH
// BAD_ARCH: the target architecture 'what' is not supported by the target 'myriad'

// Ensure that '-target shave' picks a different compiler.
// Also check that '-I' is turned into '-i:' for the assembler.

// Note that since we don't know where movi tools are installed,
// the driver may or may not find a full path to them.
// That is, the 0th argument will be "/path/to/my/moviCompile"
// or just "moviCompile" depending on whether moviCompile is found.
// As such, we test only for a trailing quote in its rendering.
// The same goes for "moviAsm".

// RUN: %clang -target shave-myriad -c -### %s -isystem somewhere -Icommon -Wa,-yippee 2>&1 \
// RUN:   | FileCheck %s -check-prefix=MOVICOMPILE
// MOVICOMPILE: moviCompile{{(.exe)?}}" "-S" "-fno-exceptions" "-mcpu=myriad2" "-DMYRIAD2" "-isystem" "somewhere" "-I" "common"
// MOVICOMPILE: moviAsm{{(.exe)?}}" "-no6thSlotCompression" "-cv:myriad2" "-noSPrefixing" "-a"
// MOVICOMPILE: "-yippee" "-i:somewhere" "-i:common" "-elf"

// RUN: %clang -target shave-myriad -c -### %s -DEFINE_ME -UNDEFINE_ME 2>&1 \
// RUN:   | FileCheck %s -check-prefix=DEFINES
// DEFINES: "-D" "EFINE_ME" "-U" "NDEFINE_ME"

// RUN: %clang -target shave-myriad -c -### %s -Icommon -iquote quotepath -isystem syspath 2>&1 \
// RUN:   | FileCheck %s -check-prefix=INCLUDES
// INCLUDES: "-iquote" "quotepath" "-isystem" "syspath"

// RUN: %clang -target shave-myriad -c -### %s -g -fno-inline-functions \
// RUN: -fno-inline-functions-called-once -Os -Wall -MF dep.d \
// RUN: -ffunction-sections 2>&1 | FileCheck %s -check-prefix=PASSTHRU_OPTIONS
// PASSTHRU_OPTIONS: "-g" "-fno-inline-functions" "-fno-inline-functions-called-once"
// PASSTHRU_OPTIONS: "-Os" "-Wall" "-MF" "dep.d" "-ffunction-sections"

// RUN: %clang -target shave-myriad -c %s -o foo.o -### -MD -MF dep.d 2>&1 \
// RUN:   | FileCheck %s -check-prefix=MDMF
// MDMF: "-S" "-fno-exceptions" "-mcpu=myriad2" "-DMYRIAD2" "-MD" "-MF" "dep.d" "-MT" "foo.o"

// RUN: %clang -target shave-myriad -std=gnu++11 -S %s -o foo.o -### 2>&1 \
// RUN:   | FileCheck %s -check-prefix=STDEQ
// STDEQ: "-S" "-fno-exceptions" "-mcpu=myriad2" "-DMYRIAD2" "-std=gnu++11"

// RUN: %clang -target shave-myriad -E -Ifoo %s -o foo.i -### 2>&1 \
// RUN:   | FileCheck %s -check-prefix=PREPROCESS
// PREPROCESS: "-E" "-mcpu=myriad2" "-DMYRIAD2" "-I" "foo"

// RUN: %clang -target sparc-myriad -### --driver-mode=g++ %s 2>&1 | FileCheck %s --check-prefix=STDLIBCXX
// STDLIBCXX: "-lstdc++" "-lc" "-lgcc"

// RUN: %clang -target sparc-myriad -### -nostdlib %s 2>&1 | FileCheck %s --check-prefix=NOSTDLIB
// NOSTDLIB-NOT: crtbegin.o
// NOSTDLIB-NOT: "-lc"

// RUN: %clang -### -c -g %s -target sparc-myriad 2>&1 | FileCheck -check-prefix=G_SPARC %s
// G_SPARC: "-debug-info-kind=limited" "-dwarf-version=2"
