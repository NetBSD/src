#	$NetBSD: Makefile,v 1.5.16.1 2011/06/23 14:18:41 cherry Exp $

PROG=	dkctl
MAN=	dkctl.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.dkctl.c+=	-Wno-pointer-sign
.endif
