# $NetBSD: Makefile,v 1.4 2024/06/11 10:18:11 wiz Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c parse_cross.c perform.c

.include <bsd.init.mk>

.PATH:	${DIST}/add

CPPFLAGS+=	-DPKGSRC_MACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
