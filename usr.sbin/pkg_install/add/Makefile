# $NetBSD: Makefile,v 1.2 1997/06/05 12:59:22 agc Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add

CFLAGS+=	${DEBUG} -I${.CURDIR}/../lib

LDADD+=		-L${INSTALLOBJDIR} -linstall
DPADD+=		${INSTALLOBJDIR}/libinstall.a

BINDIR=		/usr/sbin

SRCS= main.c perform.c futil.c extract.c

.include <bsd.prog.mk>
