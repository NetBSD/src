#	$NetBSD: Makefile,v 1.1 2000/01/20 19:22:10 wrstuden Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_overlay
SRCS=	mount_overlay.c getmntopts.c
MAN=	mount_overlay.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
