#	$NetBSD: Makefile,v 1.2.172.1 2008/12/13 01:13:11 haad Exp $

.include <bsd.sys.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
