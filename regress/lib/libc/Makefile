#	$NetBSD: Makefile,v 1.41 2003/12/11 22:45:06 matt Exp $

SUBDIR+= _setjmp atexit basename clone convfp db dirname div divrem \
	gen getaddrinfo hsearch int_fmtio locale md5sha popen randomid \
	regex rpc setjmp sigsetjmp stdio stdlib string sys time

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ldexp
.endif

HAVE_SIGINFO != if (echo "\#include <signal.h>" && echo "SA_SIGINFO") | ${CC} -E -I${DESTDIR}/usr/include - | grep -sq 0x0040; then echo yes; else echo no; fi

.if make(obj) || (${HAVE_SIGINFO} == "yes")
SUBDIR+= siginfo
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
