# $NetBSD: Makefile,v 1.4 2023/06/20 17:23:02 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kinit
SRCS= kinit.c

CPPFLAGS+= -I${HEIMDIST}/lib/ntlm

PROGDPLIBS+= ${DPLIBKAFS} ${DPLIBSL} ${DPLIBHEIMNTLM}
LDADD+= -lcrypto
DPSDD+= ${LIBCRYPTO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
