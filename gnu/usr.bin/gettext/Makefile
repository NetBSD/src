#	$NetBSD: Makefile,v 1.1.1.1 2005/04/29 14:59:30 christos Exp $

.include <bsd.own.mk>

SUBDIR=	libgrep libnlsut libnlspr info .WAIT \
	gettext msgattrib msgcat msgcmp msgcomm msgconv msgen msgfilter \
	msgfmt msginit msgmerge msgunfmt msguniq xgettext

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif

.include <bsd.subdir.mk>
