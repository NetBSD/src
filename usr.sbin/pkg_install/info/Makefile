# $NetBSD: Makefile,v 1.2 1997/06/05 12:59:38 agc Exp $
# Original from FreeBSD, no rcs id

PROG= pkg_info
CFLAGS+=	${DEBUG} -I${.CURDIR}/../lib

LDADD+=		-L${INSTALLOBJDIR} -linstall
DPADD+=		${INSTALLOBJDIR}/libinstall.a

BINDIR=		/usr/sbin

SRCS= main.c perform.c show.c

.include <bsd.prog.mk>
