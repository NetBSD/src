#	$NetBSD: Makefile,v 1.6.14.3 2001/03/12 13:29:40 bouyer Exp $

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
