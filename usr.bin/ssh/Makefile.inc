#	$NetBSD: Makefile.inc,v 1.5 2001/02/07 17:05:36 itojun Exp $

SSHDIST?= ${.CURDIR}/../../../crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST} -DHAVE_LOGIN_CAP
.PATH: ${SSHDIST}

.if defined(PROG)
LIBSSHOBJDIR != cd ${.CURDIR}/../libssh && ${PRINTOBJDIR}
LDADD+= -L${LIBSSHOBJDIR} -lssh
DPADD+= ${LIBSSHOBJDIR}/libssh.a
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
