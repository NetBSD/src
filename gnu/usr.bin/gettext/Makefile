#	$NetBSD: Makefile,v 1.2 2000/11/06 15:35:25 itojun Exp $

.include <bsd.own.mk>

SUBDIR=	lib gettext msgcmp msgcomm msgfmt msgmerge msgunfmt xgettext \
	gettextize info

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif

.include <bsd.subdir.mk>
