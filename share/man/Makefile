#	$NetBSD: Makefile,v 1.19.4.1 1999/12/27 18:30:36 wrstuden Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man3 man4 man5 man6 man7 man8 man9
MAKEWHATIS?=	/usr/libexec/makewhatis

makedb:
	${MAKEWHATIS} ${DESTDIR}/usr/share/man
.else
makedb:
.endif

.include <bsd.subdir.mk>
