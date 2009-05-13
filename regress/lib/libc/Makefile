#	$NetBSD: Makefile,v 1.55.12.1 2009/05/13 19:18:44 jym Exp $

SUBDIR+= _setjmp atexit basename citrus clone context convfp db dirname \
	div divrem gen gdtoa getaddrinfo hsearch inet int_fmtio locale md5sha \
	nsdispatch popen pty randomid regex rpc servent setjmp sigsetjmp \
	stdio stdlib string strptime sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if (${HAS_SSP} == "yes")
SUBDIR+= ssp
.endif
.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ldexp
.endif

.if !make(obj) && !make(cleandir)
HAVE_SIGINFO != if (echo "\#include <signal.h>" && echo "SA_SIGINFO") | ${CC} -E -I${DESTDIR}/usr/include - | grep -sq 0x0040; then echo yes; else echo no; fi
.endif

.if make(obj) || make(cleandir) || (${HAVE_SIGINFO} == "yes")
SUBDIR+= siginfo
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
