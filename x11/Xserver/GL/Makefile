#	$NetBSD: Makefile,v 1.4 2004/03/06 10:30:26 rtr Exp $

SUBDIR=	glx GLcore

.include <bsd.own.mk>

.if ${X11DRI} != "no"
SUBDIR+=	dri
.endif

.include <bsd.subdir.mk>
