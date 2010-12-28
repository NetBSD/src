#	$NetBSD: Makefile,v 1.4 2010/12/28 10:15:49 tsutsui Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
