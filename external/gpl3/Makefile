#	$NetBSD: Makefile,v 1.7.2.1 2014/08/19 23:52:24 tls Exp $

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
