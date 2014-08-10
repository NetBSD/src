/*	Id: ccconfig.h,v 1.18 2012/08/05 14:35:00 ragge Exp 	*/	
/*	$NetBSD: ccconfig.h,v 1.1.1.4.18.1 2014/08/10 07:10:08 tls Exp $	*/

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
