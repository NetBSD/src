#	$NetBSD: Makefile,v 1.19 2001/05/22 18:57:13 christos Exp $

SUBDIR+= _setjmp db div gen hsearch md5sha popen regex rpc setjmp sigsetjmp \
	string time
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
