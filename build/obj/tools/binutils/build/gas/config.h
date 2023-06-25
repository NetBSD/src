/* config.h.  Generated from config.in by configure.  */
/* config.in.  Generated from configure.ac by autoheader.  */

/* Check that config.h is #included before system headers
   (this works only for glibc, but that should be enough).  */
#if defined(__GLIBC__) && !defined(__FreeBSD_kernel__) && !defined(__CONFIG_H__)
#  error config.h must be #included before system headers
#endif
#define __CONFIG_H__ 1

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define if using AIX 5.2 value for C_WEAKEXT. */
/* #undef AIX_WEAK_SUPPORT */

/* assert broken? */
#define BROKEN_ASSERT 1

/* Compiling cross-assembler? */
#define CROSS_COMPILE 1

/* Default architecture. */
/* #undef DEFAULT_ARCH */

/* Default CRIS architecture. */
/* #undef DEFAULT_CRIS_ARCH */

/* Default emulation. */
#define DEFAULT_EMULATION ""

/* Define if you want compressed debug sections by default. */
/* #undef DEFAULT_FLAG_COMPRESS_DEBUG */

/* Define to 1 if you want to generate GNU Build attribute notes by default,
   if none are contained in the input. */
#define DEFAULT_GENERATE_BUILD_NOTES 0

/* Define to 1 if you want to generate ELF common symbols with the STT_COMMON
   type by default. */
#define DEFAULT_GENERATE_ELF_STT_COMMON 0

/* Define to 1 if you want to generate x86 relax relocations by default. */
#define DEFAULT_GENERATE_X86_RELAX_RELOCATIONS 1

/* Define to 1 if you want to fix Loongson3 LLSC Errata by default. */
#define DEFAULT_MIPS_FIX_LOONGSON3_LLSC 0

/* Define default value for RISC-V -march. */
/* #undef DEFAULT_RISCV_ARCH_WITH_EXT */

/* Define to 1 if you want to generate RISC-V arch attribute by default. */
#define DEFAULT_RISCV_ATTR 1

/* Define default value for RISC-V -misa-spec. */
/* #undef DEFAULT_RISCV_ISA_SPEC */

/* Define default value for RISC-V -mpriv-spec */
/* #undef DEFAULT_RISCV_PRIV_SPEC */

/* Define to 1 if you want to generate GNU x86 used ISA and feature properties
   by default. */
#define DEFAULT_X86_USED_NOTE 0

/* Supported emulations. */
#define EMULATIONS 

/* Define if you want run-time sanity checks. */
/* #undef ENABLE_CHECKING */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Define to 1 if you have the declaration of `asprintf', and to 0 if you
   don't. */
#define HAVE_DECL_ASPRINTF 1

/* Is the prototype for getopt in <unistd.h> in the expected format? */
#define HAVE_DECL_GETOPT 1

/* Define to 1 if you have the declaration of `mempcpy', and to 0 if you
   don't. */
#define HAVE_DECL_MEMPCPY 0

/* Define to 1 if you have the declaration of `stpcpy', and to 0 if you don't.
   */
#define HAVE_DECL_STPCPY 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if your <locale.h> file defines LC_MESSAGES. */
#define HAVE_LC_MESSAGES 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strsignal' function. */
#define HAVE_STRSIGNAL 1

/* Define if <sys/stat.h> has struct stat.st_mtim.tv_nsec */
/* #undef HAVE_ST_MTIM_TV_NSEC */

/* Define if <sys/stat.h> has struct stat.st_mtim.tv_sec */
/* #undef HAVE_ST_MTIM_TV_SEC */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if <time.h> has struct tm.tm_gmtoff. */
/* #undef HAVE_TM_GMTOFF */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Using i386 COFF? */
/* #undef I386COFF */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Default CPU for MIPS targets. */
/* #undef MIPS_CPU_STRING_DEFAULT */

/* Generate 64-bit code by default on MIPS targets. */
/* #undef MIPS_DEFAULT_64BIT */

/* Choose a default ABI for MIPS targets. */
/* #undef MIPS_DEFAULT_ABI */

/* Define value for nds32_arch_name */
/* #undef NDS32_DEFAULT_ARCH_NAME */

/* Define default value for nds32_audio_ext */
/* #undef NDS32_DEFAULT_AUDIO_EXT */

/* Define default value for nds32_dsp_ext */
/* #undef NDS32_DEFAULT_DSP_EXT */

/* Define default value for nds32_dx_regs */
/* #undef NDS32_DEFAULT_DX_REGS */

/* Define default value for nds32_perf_ext */
/* #undef NDS32_DEFAULT_PERF_EXT */

/* Define default value for nds32_perf_ext2 */
/* #undef NDS32_DEFAULT_PERF_EXT2 */

/* Define default value for nds32_string_ext */
/* #undef NDS32_DEFAULT_STRING_EXT */

/* Define default value for nds32_zol_ext */
/* #undef NDS32_DEFAULT_ZOL_EXT */

/* Define default value for nds32_linux_toolchain */
/* #undef NDS32_LINUX_TOOLCHAIN */

/* Define if environ is not declared in system header files. */
#define NEED_DECLARATION_ENVIRON 1

/* Define if ffs is not declared in system header files. */
/* #undef NEED_DECLARATION_FFS */

/* a.out support? */
/* #undef OBJ_MAYBE_AOUT */

/* COFF support? */
/* #undef OBJ_MAYBE_COFF */

/* ECOFF support? */
/* #undef OBJ_MAYBE_ECOFF */

/* ELF support? */
/* #undef OBJ_MAYBE_ELF */

/* generic support? */
/* #undef OBJ_MAYBE_GENERIC */

/* SOM support? */
/* #undef OBJ_MAYBE_SOM */

/* Name of package */
#define PACKAGE "gas"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "gas"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "gas 2.39"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "gas"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.39"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Using strict COFF? */
/* #undef STRICTCOFF */

/* Target alias. */
#define TARGET_ALIAS "m68k--netbsdelf"

/* Define as 1 if big endian. */
/* #undef TARGET_BYTES_BIG_ENDIAN */

/* Canonical target. */
#define TARGET_CANONICAL "m68k--netbsdelf"

/* Target CPU. */
#define TARGET_CPU "m68k"

/* Target OS. */
#define TARGET_OS "netbsdelf"

/* Define if default target is PowerPC Solaris. */
/* #undef TARGET_SOLARIS_COMMENT */

/* Target vendor. */
#define TARGET_VENDOR ""

/* Target specific CPU. */
/* #undef TARGET_WITH_CPU */

/* Use b modifier when opening binary files? */
/* #undef USE_BINARY_FOPEN */

/* Use emulation support? */
/* #undef USE_EMULATIONS */

/* Allow use of E_MIPS_ABI_O32 on MIPS targets. */
/* #undef USE_E_MIPS_ABI_O32 */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Using cgen code? */
/* #undef USING_CGEN */

/* Version number of package */
#define VERSION "2.39"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */
