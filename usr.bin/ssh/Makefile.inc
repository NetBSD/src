#	$NetBSD: Makefile.inc,v 1.1 2000/07/25 16:32:24 thorpej Exp $

#SSHDIST=${.CURDIR}/../../../crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST}
.PATH: ${SSHDIST}

.if defined(PROG)
LIBSSHOBJDIR != cd ${.CURDIR}/../libssh && make print-objdir
LDADD+= -L${LIBSSHOBJDIR} -lssh
DPADD+= ${LIBSSHOBJDIR}/libssh.a
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
