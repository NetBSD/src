# $NetBSD: Makefile,v 1.3 1997/10/17 12:03:57 lukem Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_create

CFLAGS+=	${DEBUG} -I${.CURDIR}/../lib

LDADD+=		-L${INSTALLOBJDIR} -linstall
DPADD+=		${INSTALLOBJDIR}/libinstall.a

SRCS= main.c perform.c pl.c

.include <bsd.prog.mk>
