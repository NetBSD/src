#	$NetBSD: Makefile,v 1.9 2001/02/22 07:11:11 chs Exp $

SA_PROG= ufsboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= ufs.c

PROG=	${SA_PROG}
MKMAN= no
NOMAN= yes

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa
DEFS+=  -DCOMPAT_UFS
