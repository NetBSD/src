#	$NetBSD: Makefile.inc,v 1.2.2.2 2001/02/26 20:57:30 he Exp $

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
