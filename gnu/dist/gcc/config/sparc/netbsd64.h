/* netbsd sparc64 ELF configuration */

/* Name the target CPU (this must be before <sparc/sparc.h>) */
#ifndef TARGET_CPU_DEFAULT
#define TARGET_CPU_DEFAULT    TARGET_CPU_ultrasparc
#endif

#include <sparc/sp64-elf.h>

/* maybe remove __sparc__ ? */
#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__sparc__ -D__sparc64__ -D__arch64__ -D__sparcv9__ -D__NetBSD__ -D__ELF__ -D__KPRINTF_ATTRIBUTE__"

/* clean up after <sparc/sp64-elf.h> */
#undef CPP_SUBTARGET_SPEC
#define CPP_SUBTARGET_SPEC ""

#define NETBSD_ELF
#include <netbsd.h>

#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

#undef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0

#undef PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG

/* This is the char to use for continuation (in case we need to turn
   continuation back on).  */
#undef DBX_CONTIN_CHAR
#define DBX_CONTIN_CHAR '?'

/*#undef ASM_OUTPUT_SKIP*/

#undef DBX_REGISTER_NUMBER
#define DBX_REGISTER_NUMBER(REGNO) \
  (TARGET_FLAT && REGNO == FRAME_POINTER_REGNUM ? 31 : REGNO)

/* This is how to output a definition of an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.  */

#undef  ASM_OUTPUT_INTERNAL_LABEL
#define ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM)	\
  fprintf (FILE, ".L%s%d:\n", PREFIX, NUM)

/* This is how to store into the string LABEL
   the symbol_ref name of an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.
   This is suitable for output with `assemble_name'.  */

#undef  ASM_GENERATE_INTERNAL_LABEL
#define ASM_GENERATE_INTERNAL_LABEL(LABEL,PREFIX,NUM)	\
  sprintf ((LABEL), "*.L%s%ld", (PREFIX), (long)(NUM))

#undef ASM_SPEC
#define ASM_SPEC "%{fpic:-K PIC} %{fPIC:-K PIC} \
%{mlittle-endian:-EL} \
%(asm_cpu) \
"

#undef LINK_SPEC
#define LINK_SPEC \
 "-m elf64_sparc \
  %{assert*} %{R*} \
  %{shared:-shared} \
  %{!shared: \
    -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

#undef STDC_0_IN_SYSTEM_HEADERS

/* Name the port. */
#undef TARGET_NAME
#define TARGET_NAME     "sparc64-netbsd"

/* XXX Redefine this; <sparc/sparc.h> mucks with it. */
#undef TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (%s)", TARGET_NAME);

/* Use sjlj exceptions. */
#define DWARF2_UNWIND_INFO 0
