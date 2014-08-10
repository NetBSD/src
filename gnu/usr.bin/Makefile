#	$NetBSD: Makefile,v 1.137.6.1 2014/08/10 06:51:31 tls Exp $

.include <bsd.own.mk>

SUBDIR+=	bc
SUBDIR+=	c89 c99
SUBDIR+=	dc diffutils
SUBDIR+=	rcs send-pr texinfo

.if ${MKBSDGREP} == "no"
SUBDIR+=	grep
.endif

.if ${MKGROFF} != "no"
SUBDIR+=	groff
.endif

SUBDIR+=	gettext

.include <bsd.subdir.mk>
