#	$NetBSD: Makefile,v 1.6 2015/06/03 14:42:51 christos Exp $

.include <bsd.own.mk>

SUBDIR=	libgrep libnlsut libnlspr info .WAIT \
	msgattrib msgcat msgcmp msgcomm msgconv msgen msgfilter \
	msgfmt msginit msgmerge msgunfmt msguniq xgettext

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif

.include <bsd.subdir.mk>
