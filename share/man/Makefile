#	$NetBSD: Makefile,v 1.26 2003/07/18 08:26:04 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man3 man4 man5 man6 man7 man8 man9

makedb:
	${TOOL_MAKEWHATIS} ${DESTDIR}/usr/share/man
.if ${MKUNPRIVED} != "no"
	echo "./usr/share/man/whatis.db type=file mode=0444 uname=root gname=wheel" \
	    | ${METALOG.add}
.endif
.else
makedb:
.endif

.include <bsd.subdir.mk>
.include <bsd.sys.mk>
