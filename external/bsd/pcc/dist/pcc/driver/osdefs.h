/*	Id: osdefs.h	*/	
/*	$NetBSD: osdefs.h,v 1.1.1.1 2016/02/09 20:29:11 plunky Exp $	*/

#define CPPADD			\
	"-D__NetBSD__",

#define CPPADD_amd64		\
	"-D__x86_64__",		\
	"-D__x86_64",		\
	"-D__amd64__",		\
	"-D__amd64",		\
	"-D__LP64__",		\
	"-D_LP64",

#define CPPADD_arm		\
	"-D__arm__",

#define CPPADD_i386		\
	"-D__i386__",

#define CPPADD_mips		\
	"-D__mips__",

#define CPPADD_pdp10		\
	"-D__pdp10__",

#define CPPADD_powerpc		\
	"-D__ppc__",

#define CPPADD_vax		\
	"-D__vax__",

#define CPPADD_sparc64		\
	"-D__sparc64__",

/* host-dependent */
#define CRT0FILE LIBDIR "crt0.o"
#define CRT0FILE_PROFILE LIBDIR "gcrt0.o"

#if TARGOSVER == 1
#define STARTFILES { LIBDIR "crtbegin.o", NULL }
#define	ENDFILES { LIBDIR "crtend.o", NULL }
#else
#define STARTFILES { LIBDIR "crti.o", LIBDIR "crtbegin.o", NULL }
#define	ENDFILES { LIBDIR "crtend.o", LIBDIR "crtn.o", NULL }
#endif

/* shared libraries linker files */
#if TARGOSVER == 1
#define STARTFILES_S { LIBDIR "crtbeginS.o", NULL }
#define	ENDFILES_S { LIBDIR "crtendS.o", NULL }
#else
#define STARTFILES_S { LIBDIR "crti.o", LIBDIR "crtbeginS.o", NULL }
#define	ENDFILES_S { LIBDIR "crtendS.o", LIBDIR "crtn.o", NULL }
#endif

#ifdef LANG_F77
#define F77LIBLIST { "-L/usr/local/lib", "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

/* host-independent */
#define	DYNLINKER { "-dynamic-linker", "/usr/libexec/ld.elf_so", NULL }

#elif defined(mach_i386)
#define	SIZE_TYPE_i386		UNSIGNED
#define	PTRDIFF_TYPE_i386	INT
#elif defined(mach_powerpc)
#define STARTLABEL "_start"

#define WCHAR_TYPE		INT

#define	WINT_TYPE		INT

#define	SIZE_TYPE		ULONG
#define	SIZE_TYPE_i386		UNSIGNED

#define	PTRDIFF_TYPE		LONG
#define	PTRDIFF_TYPE_i386	INT

#define ELFABI
