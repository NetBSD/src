#	$NetBSD: Makefile,v 1.27 2002/04/21 16:24:19 martin Exp $

SUBDIR+= _setjmp clone db div divrem 				\
	gen hsearch int_fmtio md5sha popen regex rpc		\
	setjmp sigsetjmp string sys time
.if (${MACHINE_ARCH} != "arm" && ${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ldexp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
