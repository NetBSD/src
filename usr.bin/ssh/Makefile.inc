#	$NetBSD: Makefile.inc,v 1.6 2002/09/18 14:00:41 lukem Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

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
