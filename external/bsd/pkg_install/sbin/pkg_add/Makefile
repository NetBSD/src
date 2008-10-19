# $NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:40:52 haad Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c futil.c extract.c verify.c

.include <bsd.init.mk>

.PATH:	${DIST}/add

CPPFLAGS+=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
