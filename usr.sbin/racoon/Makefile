#	$NetBSD: Makefile,v 1.2.4.2 2004/06/17 12:38:10 tron Exp $

.include <bsd.own.mk>

SUBDIR+=libpfkey .WAIT racoon

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif

.include <bsd.subdir.mk>
