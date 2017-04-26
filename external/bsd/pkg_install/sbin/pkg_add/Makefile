# $NetBSD: Makefile,v 1.2.34.1 2017/04/26 02:52:51 pgoyette Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c

.include <bsd.init.mk>

.PATH:	${DIST}/add

CPPFLAGS+=	-DPKGSRC_MACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
