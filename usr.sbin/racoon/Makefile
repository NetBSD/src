#	$NetBSD: Makefile,v 1.2.4.1 2004/04/09 23:07:20 jmc Exp $

.include <bsd.own.mk>

SUBDIR+=libpfkey .WAIT racoon

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif

.include <bsd.subdir.mk>
