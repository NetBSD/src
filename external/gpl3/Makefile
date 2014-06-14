#	$NetBSD: Makefile,v 1.10 2014/06/14 20:49:36 mrg Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
SUBDIR+=	binutils
.endif

.if ${MKGCC} != "no"
.if ${MKGCCCMDS} != "no"
SUBDIR+=	${EXTERNAL_GCC_SUBDIR}
.endif
.endif

.if ${MKGDB} != "no"
.if ${HAVE_GDB} == "7"
SUBDIR+=        gdb
.endif
.endif

.include <bsd.subdir.mk>
