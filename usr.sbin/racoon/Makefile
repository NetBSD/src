#	$NetBSD: Makefile,v 1.3 2004/04/09 22:23:14 he Exp $

.include <bsd.own.mk>

SUBDIR+=libpfkey .WAIT racoon

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif

.include <bsd.subdir.mk>
