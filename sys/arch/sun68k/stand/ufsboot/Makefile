#	$NetBSD: Makefile,v 1.4 2003/11/30 05:32:39 tsutsui Exp $

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
