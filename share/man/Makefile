#	$NetBSD: Makefile,v 1.29.56.2 2014/05/22 11:37:49 yamt Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man2 man3 man3lua man4 man5 man6 man7 man8 man9 man9lua

WHATISDBDIR?=	/usr/share/man

.if ${MKMAKEMANDB} == "no"
makedb:
	${TOOL_MAKEWHATIS} ${DESTDIR}${WHATISDBDIR}
.if ${MKUNPRIVED} != "no"
	echo ".${WHATISDBDIR}/whatis.db type=file mode=0444 uname=${BINOWN} gname=${BINGRP}" \
	    | ${METALOG.add}
.endif
.else
makedb:
.endif

.else
makedb:
.endif

.include <bsd.subdir.mk>
