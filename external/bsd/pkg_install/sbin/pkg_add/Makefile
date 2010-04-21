# $NetBSD: Makefile,v 1.1.8.1.2.1 2010/04/21 05:23:10 matt Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c

.include <bsd.init.mk>

.PATH:	${DIST}/add

CPPFLAGS+=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
