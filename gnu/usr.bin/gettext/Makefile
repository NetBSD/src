#	$NetBSD: Makefile,v 1.4 2001/01/09 08:52:58 enami Exp $

.include <bsd.own.mk>

SUBDIR=	lib info .WAIT \
	gettext msgcmp msgcomm msgfmt msgmerge msgunfmt xgettext

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif

.include <bsd.subdir.mk>
