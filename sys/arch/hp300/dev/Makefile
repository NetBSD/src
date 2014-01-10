#	$NetBSD: Makefile,v 1.5 2014/01/10 16:35:59 christos Exp $

.include <bsd.own.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
