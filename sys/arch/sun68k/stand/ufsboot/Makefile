#	$NetBSD: Makefile,v 1.3 2003/03/30 00:17:29 kristerw Exp $

SA_PROG= ufsboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= ufs.c

PROG=	${SA_PROG}
NOMAN= # defined

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa
DEFS+=  -DCOMPAT_UFS -D__daddr_t=int32_t
