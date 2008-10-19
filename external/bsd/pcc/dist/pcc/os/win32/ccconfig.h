#ifndef LIBDIR
#define LIBDIR "/usr/lib/"
#endif

/*
 * Currently only supports console applications.
 */

#ifdef WIN32
#define	CPPADD { "-DWIN32", NULL }
#define LIBCLIBS { "/subsystem:console", "msvcrt.lib", NULL }
#else
/* requires w32api-3.2.tar.gz and mingw-runtime-3.14.tar.gz */
#define	CPPADD { "-DWIN32", "-D__MINGW32__", NULL }
#define STARTFILES { LIBDIR "crt2.o", NULL }
#define ENDFILES { NULL }
#define STARTFILES_S { LIBDIR "dllcrt2.o", NULL }
#define ENDFILES_S { NULL }
#define LIBCLIBS { "-lmoldname", "-lmsvcrt", "-lmingw32", "-luser32", "-lkernel32", "-lpcc", "-lmsvcrt", NULL }
#endif

#define CPPMDADD { "-D__i386__", NULL }
