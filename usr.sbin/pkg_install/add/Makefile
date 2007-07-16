# $NetBSD: Makefile,v 1.8 2007/07/16 19:33:02 joerg Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c futil.c extract.c verify.c

.include <bsd.init.mk>

.PATH:	${DIST}/add

CPPFLAGS+=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
