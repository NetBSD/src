# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:43 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kpasswd ${HEIMDIST}/lib/hcrypto

CPPFLAGS+=-I${HEIMDIST}/lib

PROG= kpasswd
SRCS= kpasswd.c ui.c

LDADD+= -lkafs -lsl -lheimntlm
DPADD+= ${LIBKAFS} ${LIBSL} ${LIBHEIMNTLM}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
