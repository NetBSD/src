#	$NetBSD: Makefile,v 1.21 2001/09/20 16:57:43 atatat Exp $

SUBDIR+= _setjmp clone db div gen hsearch md5sha popen regex rpc setjmp \
	sigsetjmp string sys time
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
