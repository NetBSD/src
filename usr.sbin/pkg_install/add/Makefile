# $NetBSD: Makefile,v 1.6 2004/01/13 08:30:34 grant Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c futil.c extract.c verify.c

CPPFLAGS+=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>
