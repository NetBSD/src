/* NetBSD/sparc64 ELF configuration */

/*
 * Pull in generic SPARC64 ELF configuration, and then clean up
 * afterwards
 */

/* Let us output 32 bit code as well */
#define SPARC_BI_ARCH

/* Name the target CPU.  This must be before <sparc/sparc.h>. */
#ifndef TARGET_CPU_DEFAULT
#define TARGET_CPU_DEFAULT    TARGET_CPU_ultrasparc
#endif

#include <sparc/sp64-elf.h>

#undef TARGET_DEFAULT
#define TARGET_DEFAULT \
(MASK_V9 + MASK_PTR64 + MASK_64BIT + /* MASK_HARD_QUAD */ \
 + MASK_APP_REGS + MASK_EPILOGUE + MASK_FPU + MASK_STACK_BIAS)

#undef SPARC_DEFAULT_CMODEL
#define SPARC_DEFAULT_CMODEL CM_MEDANY

#undef INIT_SUBTARGET_OPTABS  
#define INIT_SUBTARGET_OPTABS  

#include <sparc/netbsd-elf-common.h>

#undef CPP_SUBTARGET_SPEC
#define CPP_SUBTARGET_SPEC "-D__sparc64__"

#undef LINK_SPEC64
#define LINK_SPEC64 \
 "-m elf64_sparc \
  %{assert*} %{R*} \
  %{shared:-shared} \
  %{!shared: \
    -dy -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

#undef LINK_SPEC
#define LINK_SPEC LINK_SPEC64

#ifdef SPARC_BI_ARCH

#undef STARTFILE_SPEC64
#define	STARTFILE_SPEC64 \
 "%{!shared: \
     %{pg:gcrt0%O%s} \
     %{!pg: \
        %{p:gcrt0%O%s} \
        %{!p:crt0%O%s}}} \
   %:if-exists(crti%O%s) \
   %{!shared:crtbegin%O%s} %{shared:crtbeginS%O%s}"

#undef STARTFILE_SPEC32
#define	STARTFILE_SPEC32 \
 "%{!shared: \
     %{pg:/usr/lib/32/gcrt0%O%s} \
     %{!pg: \
        %{p:/usr/lib/32/gcrt0%O%s} \
        %{!p:/usr/lib/32/crt0%O%s}}} \
   %:if-exists(/usr/lib/32/crti%O%s) \
   %{!shared:/usr/lib/32/crtbegin%O%s} %{shared:/usr/lib/32/crtbeginS%O%s}"

#undef STARTFILE_SPEC
#if DEFAULT_ARCH32_P
#define STARTFILE_SPEC "\
%{m32:" STARTFILE_SPEC32 "} \
%{m64:" STARTFILE_SPEC64 "} \
%{!m32:%{!m64:" STARTFILE_SPEC32 "}}"
#else
#define STARTFILE_SPEC "\
%{m32:" STARTFILE_SPEC32 "} \
%{m64:" STARTFILE_SPEC64 "} \
%{!m32:%{!m64:" STARTFILE_SPEC64 "}}"
#endif

#undef ENDFILE_SPEC64
#define	ENDFILE_SPEC64 \
 "%{!shared:crtend%O%s} %{shared:crtendS%O%s} \
  %:if-exists(crtn%O%s)"

#undef ENDFILE_SPEC32
#define	ENDFILE_SPEC32 \
 "%{!shared:/usr/lib/32/crtend%O%s} %{shared:/usr/lib/32/crtendS%O%s} \
  %:if-exists(/usr/lib/32/crtn%O%s)"

#undef ENDFILE_SPEC
#if DEFAULT_ARCH32_P
#define ENDFILE_SPEC "\
%{m32:" ENDFILE_SPEC32 "} \
%{m64:" ENDFILE_SPEC64 "} \
%{!m32:%{!m64:" ENDFILE_SPEC32 "}}"
#else
#define ENDFILE_SPEC "\
%{m32:" ENDFILE_SPEC32 "} \
%{m64:" ENDFILE_SPEC64 "} \
%{!m32:%{!m64:" ENDFILE_SPEC64 "}}"
#endif

#undef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS \
  { "link_arch32",       LINK_ARCH32_SPEC },              \
  { "link_arch64",       LINK_ARCH64_SPEC },              \
  { "link_arch_default", LINK_ARCH_DEFAULT_SPEC },	  \
  { "link_arch",	 LINK_ARCH_SPEC },

#undef LINK_ARCH32_SPEC
#define LINK_ARCH32_SPEC \
 "-m elf32_sparc \
  -Y P,/usr/lib/32 \
  %{assert*} %{R*} \
  %{shared:-shared} \
  %{!shared: \
    -dy -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

#undef LINK_ARCH64_SPEC
#define LINK_ARCH64_SPEC LINK_SPEC64

#define LINK_ARCH_SPEC "\
%{m32:%(link_arch32)} \
%{m64:%(link_arch64)} \
%{!m32:%{!m64:%(link_arch_default)}} \
"

#define LINK_ARCH_DEFAULT_SPEC \
(DEFAULT_ARCH32_P ? LINK_ARCH32_SPEC : LINK_ARCH64_SPEC)

#undef  LINK_SPEC
#define LINK_SPEC "\
%(link_arch) \
%{mlittle-endian:-EL} \
"

#undef	CC1_SPEC
#if DEFAULT_ARCH32_P
#define CC1_SPEC "\
%{sun4:} %{target:} \
%{mcypress:-mcpu=cypress} \
%{msparclite:-mcpu=sparclite} %{mf930:-mcpu=f930} %{mf934:-mcpu=f934} \
%{mv8:-mcpu=v8} %{msupersparc:-mcpu=supersparc} \
%{m64:-mptr64 -mcpu=ultrasparc -mstack-bias %{p*:-mcmodel=medlow} \
%{p:-mcmodel=medlow}} \
"
#else
#define CC1_SPEC "\
%{sun4:} %{target:} \
%{mcypress:-mcpu=cypress} \
%{msparclite:-mcpu=sparclite} %{mf930:-mcpu=f930} %{mf934:-mcpu=f934} \
%{mv8:-mcpu=v8} %{msupersparc:-mcpu=supersparc} \
%{m32:-mptr32 -mcpu=cypress -mno-stack-bias} \
%{!m32:%{p*:-mcmodel=medlow}} \
"
#endif

#if DEFAULT_ARCH32_P
#define MULTILIB_DEFAULTS { "m32" }
#else
#define MULTILIB_DEFAULTS { "m64" }
#endif

#undef CPP_SUBTARGET_SPEC
#define CPP_SUBTARGET_SPEC \
(DEFAULT_ARCH32_P ? "\
%{m64:-D__sparc64__}%{!m64:-D__sparc} \
" : "\
%{!m32:-D__sparc64__}%{m32:-D__sparc} \
")

#endif	/* SPARC_BI_ARCH */

/* Name the port. */
#undef TARGET_NAME
#define TARGET_NAME     "sparc64-netbsd"
