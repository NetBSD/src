# $NetBSD: Makefile,v 1.3.4.1 2000/10/18 02:02:48 tv Exp $

PROG=	traceroute6
MAN=	traceroute6.8

BINMODE=4555
BINOWN=	root

CPPFLAGS+=-DINET6 -DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

# it seems that, if we use poll(2), the timing changes.  commented out for now
#CPPFLAGS+=-DHAVE_POLL

.include <bsd.prog.mk>
