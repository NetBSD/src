#	$NetBSD: Makefile,v 1.5 2008/08/29 00:02:23 gmcgarry Exp $

PROG=	dkctl
MAN=	dkctl.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.dkctl.c+=	-Wno-pointer-sign
.endif
