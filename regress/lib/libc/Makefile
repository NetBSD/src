#	$NetBSD: Makefile,v 1.18 2001/02/18 21:57:16 bjh21 Exp $

SUBDIR+= _setjmp db div gen hsearch md5sha popen regex rpc setjmp sigsetjmp \
	time
.if (${MACHINE_ARCH} != "arm32" && ${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ldexp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
