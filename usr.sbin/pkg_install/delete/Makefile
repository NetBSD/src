# $NetBSD: Makefile,v 1.3 1997/10/17 12:04:04 lukem Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_delete
CFLAGS+=	${DEBUG} -I${.CURDIR}/../lib

LDADD+=		-L${INSTALLOBJDIR} -linstall
DPADD+=		${INSTALLOBJDIR}/libinstall.a

SRCS= main.c perform.c

.include <bsd.prog.mk>
