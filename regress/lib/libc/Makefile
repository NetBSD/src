#	$NetBSD: Makefile,v 1.26 2002/03/22 00:09:35 fredette Exp $

SUBDIR+= _setjmp clone db div divrem 				\
	gen hsearch int_fmtio md5sha popen regex rpc		\
	setjmp sigsetjmp string sys time
.if (${MACHINE_ARCH} != "arm32" && ${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ldexp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
