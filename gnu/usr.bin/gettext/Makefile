#	$NetBSD: Makefile,v 1.3 2000/11/08 16:26:46 itojun Exp $

.include <bsd.own.mk>

SUBDIR=	lib gettext msgcmp msgcomm msgfmt msgmerge msgunfmt xgettext \
	info

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif

.include <bsd.subdir.mk>
