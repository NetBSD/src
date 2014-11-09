#	$NetBSD: Makefile,v 1.33 2014/11/09 17:39:37 pooka Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man2 man3 man3lua man4 man5 man6 man7 man8 man9 man9lua
SUBDIR+=${.CURDIR}/../../sys/rump/share/man

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
