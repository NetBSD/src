#	$NetBSD: Makefile,v 1.3.2.1 2004/08/03 10:42:21 skrll Exp $

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
