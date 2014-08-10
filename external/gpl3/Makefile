#	$NetBSD: Makefile,v 1.9.2.1 2014/08/10 07:13:27 tls Exp $

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
