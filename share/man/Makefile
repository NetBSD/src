#	$NetBSD: Makefile,v 1.25 2003/07/10 10:34:35 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man3 man4 man5 man6 man7 man8 man9

makedb:
	${TOOL_MAKEWHATIS} ${DESTDIR}/usr/share/man
.if defined(UNPRIVED)
	echo "./usr/share/man/whatis.db type=file mode=0444 uname=root gname=wheel" \
	    | ${METALOG.add}
.endif
.else
makedb:
.endif

.include <bsd.subdir.mk>
.include <bsd.sys.mk>
