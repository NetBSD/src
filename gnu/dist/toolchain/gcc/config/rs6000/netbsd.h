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
%{!mcpu*: -mppc} \
%{mcpu=403: -m403} \
%{mcpu=601: -m601} \
%{mcpu=603e: -mppc} \
%{mcpu=604e: -mppc} \
%{V} %{v:%{!V:-V}} %{Qy:} %{!Qn:-Qy} %{n} %{T} %{Ym,*} %{Yd,*} %{Wa,*:%*} \
%{mrelocatable} \
%{mlittle} %{mlittle-endian} %{mbig} %{mbig-endian}"

/* The `multiple' instructions may not be universally implemented.
   We avoid their use here.
   The 403 ports cannot access unaligned data.
   Force strict alignment. */
#undef	CC1_SPEC
#define	CC1_SPEC	"-mno-multiple -mstrict-align"

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
%{pthread:-D_REENTRANT -D_PTHREADS} \
%{msoft-float:-D_SOFT_FLOAT} \
%{mcall-sysv: -D_CALL_SYSV} %{mcall-aix: -D_CALL_AIX} %{!mcall-sysv: %{!mcall-aix: -D_CALL_SYSV}}"
