#	$NetBSD: Makefile,v 1.10.2.1 2002/01/10 19:49:50 thorpej Exp $

SA_PROG= ufsboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= ufs.c

PROG=	${SA_PROG}
NOMAN= # defined

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa
DEFS+=  -DCOMPAT_UFS
