#	$NetBSD: Makefile,v 1.2 2011/06/22 02:49:45 mrg Exp $

PROG=	iopctl
MAN=	iopctl.8

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.iopctl.c+=	-fno-strict-aliasing
.endif
