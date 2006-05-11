# $NetBSD: Makefile,v 1.7 2006/05/11 23:16:30 mrg Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_add
SRCS= main.c perform.c futil.c extract.c verify.c

CPPFLAGS+=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.extract.c+=	-Wno-pointer-sign
.endif

