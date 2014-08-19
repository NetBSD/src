#	$NetBSD: Makefile,v 1.4.18.1 2014/08/20 00:03:00 tls Exp $

.include <bsd.own.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
