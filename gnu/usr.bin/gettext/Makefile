#	$NetBSD: Makefile,v 1.5 2005/04/29 15:05:01 christos Exp $

.include <bsd.own.mk>

SUBDIR=	libgrep libnlsut libnlspr info .WAIT \
	gettext msgattrib msgcat msgcmp msgcomm msgconv msgen msgfilter \
	msgfmt msginit msgmerge msgunfmt msguniq xgettext

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif

.include <bsd.subdir.mk>
