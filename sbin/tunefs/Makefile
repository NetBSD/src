#	$NetBSD: Makefile,v 1.9.4.1 1999/10/19 20:01:06 fvdl Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	tunefs
MAN=	tunefs.8
SRCS=	tunefs.c ffs_bswap.c
#
# XXX should go away when brought into the mainline
#
CPPFLAGS+=-I${.CURDIR}/../../sys
.PATH:  ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
