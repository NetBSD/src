#	$NetBSD: Makefile,v 1.6.2.2 2014/05/22 16:55:08 yamt Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
SUBDIR+=	binutils
.endif

.if ${MKGCC} != "no"
.if ${HAVE_GCC} >= 45
.if ${MKGCCCMDS} != "no"
SUBDIR+=	${EXTERNAL_GCC_SUBDIR}
.endif
.endif
.endif

.if ${MKGDB} != "no"
.if ${HAVE_GDB} == "7"
SUBDIR+=        gdb
.endif
.endif

.include <bsd.subdir.mk>
