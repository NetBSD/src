#	$NetBSD: Makefile.inc,v 1.3 2000/10/28 15:36:58 garbled Exp $

SSHDIST?= ${.CURDIR}/../../../crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST}
.PATH: ${SSHDIST}

.if defined(PROG)
LIBSSHOBJDIR != cd ${.CURDIR}/../libssh && ${MAKE} print-objdir
LDADD+= -L${LIBSSHOBJDIR} -lssh
DPADD+= ${LIBSSHOBJDIR}/libssh.a
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
