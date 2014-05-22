#	$NetBSD: Makefile,v 1.4.8.1 2014/05/22 11:39:46 yamt Exp $

.include <bsd.own.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
