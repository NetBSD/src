# $NetBSD: Makefile,v 1.3 1997/10/17 12:04:11 lukem Exp $
# Original from FreeBSD, no rcs id

PROG= pkg_info
CFLAGS+=	${DEBUG} -I${.CURDIR}/../lib

LDADD+=		-L${INSTALLOBJDIR} -linstall
DPADD+=		${INSTALLOBJDIR}/libinstall.a

SRCS= main.c perform.c show.c

.include <bsd.prog.mk>
