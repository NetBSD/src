# $NetBSD: Makefile,v 1.2.46.2 2020/04/21 19:37:56 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kinit
SRCS= kinit.c

CPPFLAGS+= -I${HEIMDIST}/lib/ntlm

LDADD+= -lkafs -lsl -lheimntlm
DPADD+= ${LIBKAFS} ${LIBSL} ${LIBHEIMNTLM}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
