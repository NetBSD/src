#	$NetBSD: Makefile,v 1.47 2004/03/21 01:47:00 he Exp $

SUBDIR+= _setjmp atexit basename clone context convfp db dirname div divrem \
	gen getaddrinfo hsearch int_fmtio locale md5sha popen \
	randomid regex rpc setjmp sigsetjmp stdio stdlib string \
	sys time

.include <bsd.own.mk>

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
