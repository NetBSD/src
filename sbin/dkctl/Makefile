#	$NetBSD: Makefile,v 1.4.24.1 2008/09/18 04:28:22 wrstuden Exp $

PROG=	dkctl
MAN=	dkctl.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.dkctl.c+=	-Wno-pointer-sign
.endif
