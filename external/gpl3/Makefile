#	$NetBSD: Makefile,v 1.13 2015/08/16 09:17:18 christos Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
SUBDIR+=	binutils
.endif

.if ${MKGCC} != "no"
SUBDIR+=	${EXTERNAL_GCC_SUBDIR}
.endif

.if ${MKGDB} != "no"
.	if ${HAVE_GDB} == "79"
SUBDIR+=        gdb
.	elif ${HAVE_GDB} == "7"
SUBDIR+=        gdb
.	endif
.endif

.include <bsd.subdir.mk>
