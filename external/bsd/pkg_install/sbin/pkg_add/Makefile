# $NetBSD: Makefile,v 1.1 2008/09/30 19:19:56 joerg Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c futil.c extract.c verify.c

.include <bsd.init.mk>

.PATH:	${DIST}/add

CPPFLAGS+=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
