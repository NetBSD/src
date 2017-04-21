# $NetBSD: Makefile,v 1.2.38.1 2017/04/21 16:52:34 bouyer Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c

.include <bsd.init.mk>

.PATH:	${DIST}/add

CPPFLAGS+=	-DPKGSRC_MACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
