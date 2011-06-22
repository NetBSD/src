# $NetBSD: Makefile,v 1.13 2011/06/22 02:49:44 mrg Exp $

USE_FORT?= yes	# setuid
PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.ping6.c+=	-fno-strict-aliasing -Wno-error
.endif
