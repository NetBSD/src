#	$NetBSD: Makefile,v 1.8 2001/02/04 08:57:22 jmc Exp $

SA_PROG= ufsboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= ufs.c

PROG=	${SA_PROG} ${SA_PROG}.bin
MKMAN= no
NOMAN= yes

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa
DEFS+=  -DCOMPAT_UFS
