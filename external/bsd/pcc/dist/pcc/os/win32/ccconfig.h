/*	Id: ccconfig.h,v 1.16 2011/06/05 08:54:43 plunky Exp 	*/	
/*	$NetBSD: ccconfig.h,v 1.1.1.3.2.1 2012/04/17 00:04:06 yamt Exp $	*/

#ifndef LIBDIR
#define LIBDIR "/usr/lib/"
#endif

/*
 * Currently only supports console applications.
 */

#if defined(MSLINKER)
/* requires microsoft toolkit headers and llnker */
#define	CPPADD { "-DWIN32", NULL }
#define LIBCLIBS { "/subsystem:console", "msvcrt.lib", "libpcc.a", NULL }
#else
/* requires w32api-3.2.tar.gz and mingw-runtime-3.14.tar.gz */
#define	CPPADD { "-DWIN32", "-D__MSVCRT__", "-D__MINGW32__", NULL }
#define STARTFILES { LIBDIR "crt2.o", NULL }
#define ENDFILES { NULL }
#define STARTFILES_S { LIBDIR "dllcrt2.o", NULL }
#define ENDFILES_S { NULL }
#define LIBCLIBS { "-lmoldname", "-lmingwex", "-lmingw32", "-lpcc", "-lmsvcrt", "-luser32", "-lkernel32", NULL }
#endif

#define CPPMDADD { "-D__i386__", NULL }
