/*	Id: ccconfig.h,v 1.19 2014/12/24 12:35:38 plunky Exp 	*/	
/*	$NetBSD: ccconfig.h,v 1.1.1.6 2016/02/09 20:29:21 plunky Exp $	*/

/*
 * Currently only supports console applications.
 */

#if defined(MSLINKER)
/* requires microsoft toolkit headers and linker, and pcc-libs */
#define CPPADD		{ "-DWIN32", "-D_WIN32", NULL }
#define DEFLIBS		{ "/subsystem:console", "msvcrt.lib", "libpcc.a", NULL }
#else
/* common cpp predefines */
#define CPPADD		{ "-DWIN32", "-D_WIN32", "-D__MSVCRT__", "-D__MINGW32__", NULL }

/* host-dependent */
/* requires w32api-3.2.tar.gz and mingw-runtime-3.14.tar.gz */
#define CRT0		"crt2.o"
#define STARTLABEL	"_mainCRTStartup"
#define CRT0_S		"dllcrt2.o"
#define STARTLABEL_S	"_DllMainCRTStartup@12"
#define GCRT0		"gcrt2.o"	/* in _addition_ to either crt2.o or dllcrt2.o */

#define CRTBEGIN	0	/* No constructor/destructor support for now */
#define CRTEND		0
#define CRTBEGIN_S	0
#define CRTEND_S	0
#define CRTBEGIN_T	0	/* Note: MingW cannot do -static linking */
#define CRTEND_T	0
#define CRTI		0
#define CRTN		0

#define WIN32_LIBC	"-lmsvcrt"
#define MINGW_RTLIBS	"-lmoldname", "-lmingwex", "-lmingw32"
#define MINGW_SYSLIBS	"-luser32", "-lkernel32" /* ,"-ladvapi32", "-lshell32" */
#define DEFLIBS		{ MINGW_RTLIBS, "-lpcc", WIN32_LIBC, MINGW_SYSLIBS, MINGW_RTLIBS, WIN32_LIBC, 0 }
#define DEFPROFLIBS	{ MINGW_RTLIBS, "-lpcc", WIN32_LIBC, "-lgmon", MINGW_SYSLIBS, MINGW_RTLIBS, WIN32_LIBC, 0 }
#define DEFCXXLIBS	{ "-lp++", MINGW_RTLIBS, "-lpcc", WIN32_LIBC, MINGW_SYSLIBS, MINGW_RTLIBS, WIN32_LIBC, 0 }

#endif

#define CPPMDADD { "-D__i386__", NULL }
