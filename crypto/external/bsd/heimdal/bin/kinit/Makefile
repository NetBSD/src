# $NetBSD: Makefile,v 1.3 2023/06/20 00:30:16 mrg Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kinit
SRCS= kinit.c

CPPFLAGS+= -I${HEIMDIST}/lib/ntlm
CPPFLAGS+= -I${HEIMDIST}/lib

LDADD+= -lkafs -lsl -lheimntlm
DPADD+= ${LIBKAFS} ${LIBSL} ${LIBHEIMNTLM}

# XXX: Pull this in from hcrypto.  Be sure to be last!
SRCS+=	ui.c
.PATH:	${HEIMDIST}/lib/hcrypto
CPPFLAGS.ui.c+= 		\
	-I${HEIMDIST}/lib/hcrypto

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
