#	$NetBSD: Makefile,v 1.16 2001/01/08 04:41:31 simonb Exp $

SUBDIR+= _setjmp db div gen md5sha popen regex setjmp sigsetjmp time
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
