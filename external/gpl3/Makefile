#	$NetBSD: Makefile,v 1.9 2014/02/26 09:54:32 mrg Exp $

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
