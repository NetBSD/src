/* netbsd sparc64 ELF configuration */

#include <sparc/sp64-elf.h>

/* remove unix and maybe __sparc__ ? */
#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dunix -D__sparc__ -D__sparc64__ -D__NetBSD__ -D__KPRINTF_ATTRIBUTE__"

/* clean up after <sparc/sp64-elf.h> */
#undef CPP_SUBTARGET_SPEC
#define CPP_SUBTARGET_SPEC ""

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned long"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long"

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
#define ASM_SPEC "\
%%{fpic:-K PIC} %{fPIC:-K PIC} \
%{mlittle-endian:-EL} \
%(asm_cpu) %(asm_arch) \
"

#undef STDC_0_IN_SYSTEM_HEADERS

#define NETBSD_ELF
#include <netbsd.h>

/* Name the port. */
#undef TARGET_NAME
#define TARGET_NAME     "sparc64-netbsd"

/* XXX Redefine this; <sparc/sparc.h> mucks with it. */
#undef TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (%s)", TARGET_NAME);
