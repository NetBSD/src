#	$NetBSD: Makefile,v 1.18 1999/02/13 02:54:36 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man3 man4 man5 man6 man7 man8 man9

makedb:
	${DESTDIR}/usr/libexec/makewhatis ${DESTDIR}/usr/share/man
.else
makedb:
.endif

.include <bsd.subdir.mk>
