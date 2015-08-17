#	$NetBSD: Makefile,v 1.14 2015/08/17 07:30:39 christos Exp $

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
SUBDIR+=        gdb.old
.	endif
.endif

.include <bsd.subdir.mk>
