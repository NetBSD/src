#	$NetBSD: Makefile,v 1.6 2011/06/20 07:43:59 mrg Exp $

PROG=	dkctl
MAN=	dkctl.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.dkctl.c+=	-Wno-pointer-sign
.endif
