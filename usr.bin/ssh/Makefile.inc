#	$NetBSD: Makefile.inc,v 1.4 2000/12/30 14:54:40 sommerfeld Exp $

SSHDIST?= ${.CURDIR}/../../../crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST}
.PATH: ${SSHDIST}

.if defined(PROG)
LIBSSHOBJDIR != cd ${.CURDIR}/../libssh && ${PRINTOBJDIR}
LDADD+= -L${LIBSSHOBJDIR} -lssh
DPADD+= ${LIBSSHOBJDIR}/libssh.a
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
