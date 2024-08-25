# $NetBSD: Makefile,v 1.1.1.1 2024/08/25 06:38:36 wiz Exp $
# Original from FreeBSD, no rcs id

PROG= pkg_info
SRCS= main.c perform.c show.c

.include <bsd.init.mk>

.PATH:	${DIST}/info

CWARNFLAGS.clang+=	-Wno-format-security

.include <bsd.prog.mk>
