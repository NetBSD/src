# $NetBSD: Makefile,v 1.3 1997/10/17 12:03:52 lukem Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add

CFLAGS+=	${DEBUG} -I${.CURDIR}/../lib

LDADD+=		-L${INSTALLOBJDIR} -linstall
DPADD+=		${INSTALLOBJDIR}/libinstall.a

SRCS= main.c perform.c futil.c extract.c

.include <bsd.prog.mk>
