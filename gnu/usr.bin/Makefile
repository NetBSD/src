#	$NetBSD: Makefile,v 1.140 2016/01/10 22:21:44 christos Exp $

.include <bsd.own.mk>

SUBDIR+=	bc
SUBDIR+=	c89 c99
SUBDIR+=	dc diffutils
SUBDIR+=	rcs send-pr texinfo

.if ${MKGROFF} != "no"
SUBDIR+=	groff
.endif

SUBDIR+=	gettext

.include <bsd.subdir.mk>
