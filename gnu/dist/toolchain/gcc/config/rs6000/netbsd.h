/* Start with SVR4 defaults. */
#include <rs6000/sysv4.h>

/* Get generic NetBSD definitions.  */

#define NETBSD_ELF
#include <netbsd.h>

#undef SDB_DEBUGGING_INFO
#define SDB_DEBUGGING_INFO
#undef DBX_DEBUGGING_INFO
#define DBX_DEBUGGING_INFO

#undef PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG

/* Names to predefine in the preprocessor for this target machine.  */
#undef	CPP_PREDEFINES
#define CPP_PREDEFINES "\
-D__powerpc__ -D__NetBSD__ -D__ELF__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(powerpc) -Amachine(powerpc)"

/* Make gcc agree with <machine/ansi.h> */

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/* Don't default to pcc-struct-return, because gcc is the only compiler, and
   we want to retain compatibility with older gcc versions.  */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* XXX Duplicated from sysv4.h  --thorpej@netbsd.org
   Pass -mppc to the assembler, since that is what powerpc.h currently
   implies */
#undef ASM_SPEC
#define ASM_SPEC \
  "-u \
%{mcpu=601: -m601} %{!mcpu=601: -mppc} \
%{V} %{v:%{!V:-V}} %{Qy:} %{!Qn:-Qy} %{n} %{T} %{Ym,*} %{Yd,*} %{Wa,*:%*} \
%{mrelocatable} \
%{mlittle} %{mlittle-endian} %{mbig} %{mbig-endian}"

/* The `multiple' instructions may not be universally implemented.
   We avoid their use here. */
#undef	CC1_SPEC
#define	CC1_SPEC	"-mno-multiple"

/* Provide a LINK_SPEC approriate for NetBSD. */
#undef	LINK_SPEC
#define LINK_SPEC " \
  %{O*:-O3} %{!O*:-O1}						\
  %{assert*} %{R*}						\
  %{shared:-shared}						\
  %{!shared:							\
    -dc -dp							\
    %{!nostdlib:%{!r*:%{!e*:-e _start}}}			\
    %{!static:							\
      %{rdynamic:-export-dynamic}				\
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

/* XXX Sort of a mix of ../netbsd.h and sysv4.h  --thorpej@netbsd.org
   Provide a CPP_SPEC appropriate for NetBSD.  Currently we just deal with
   the GCC option `-posix' and the calling convention definition.  */

#undef CPP_SPEC
#define CPP_SPEC "\
%{posix:-D_POSIX_SOURCE} \
%{msoft-float:-D_SOFT_FLOAT} \
%{mcall-sysv: -D_CALL_SYSV} %{mcall-aix: -D_CALL_AIX} %{!mcall-sysv: %{!mcall-aix: -D_CALL_SYSV}}"

/* <netbsd.h> redefined a bunch of these things, but we actually want the
   <rs6000/sysv4.h> versions.  */

#undef  ASM_DECLARE_FUNCTION_NAME
#define ASM_DECLARE_FUNCTION_NAME(FILE, NAME, DECL)			\
  do {									\
    char *orig_name;							\
    char *init_ptr = (TARGET_64BIT) ? ".quad" : ".long";		\
    STRIP_NAME_ENCODING (orig_name, NAME);				\
									\
    if (TARGET_RELOCATABLE && (get_pool_size () != 0 || profile_flag))	\
      {									\
	char buf[256], *buf_ptr;					\
									\
	ASM_OUTPUT_INTERNAL_LABEL (FILE, "LCL", rs6000_pic_labelno);	\
									\
	ASM_GENERATE_INTERNAL_LABEL (buf, "LCTOC", 1);			\
	STRIP_NAME_ENCODING (buf_ptr, buf);				\
	fprintf (FILE, "\t%s %s-", init_ptr, buf_ptr);			\
									\
	ASM_GENERATE_INTERNAL_LABEL (buf, "LCF", rs6000_pic_labelno);	\
	fprintf (FILE, "%s\n", buf_ptr);				\
      }									\
									\
    fprintf (FILE, "\t%s\t %s,", TYPE_ASM_OP, orig_name);		\
    fprintf (FILE, TYPE_OPERAND_FMT, "function");			\
    putc ('\n', FILE);							\
    ASM_DECLARE_RESULT (FILE, DECL_RESULT (DECL));			\
									\
    if (DEFAULT_ABI == ABI_AIX || DEFAULT_ABI == ABI_NT)		\
      {									\
	char *desc_name = orig_name;					\
									\
	while (*desc_name == '.')					\
	  desc_name++;							\
									\
	if (TREE_PUBLIC (DECL))						\
	  fprintf (FILE, "\t.globl %s\n", desc_name);			\
									\
	fprintf (FILE, "%s\n", MINIMAL_TOC_SECTION_ASM_OP);		\
	fprintf (FILE, "%s:\n", desc_name);				\
	fprintf (FILE, "\t%s %s\n", init_ptr, orig_name);		\
	fprintf (FILE, "\t%s _GLOBAL_OFFSET_TABLE_\n", init_ptr);	\
	if (DEFAULT_ABI == ABI_AIX)					\
	  fprintf (FILE, "\t%s 0\n", init_ptr);				\
	fprintf (FILE, "\t.previous\n");				\
      }									\
    fprintf (FILE, "%s:\n", orig_name);					\
  } while (0)
