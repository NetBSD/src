# $NetBSD: Makefile,v 1.3 2019/04/06 10:29:13 pgoyette Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_create
SRCS= main.c perform.c pl.c util.c build.c

COPTS.main.c+=	-Wno-error=implicit-fallthrough

.include <bsd.init.mk>

.PATH:	${DIST}/create

.include <bsd.prog.mk>
