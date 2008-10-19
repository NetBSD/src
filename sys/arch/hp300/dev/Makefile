#	$NetBSD: Makefile,v 1.3 2008/10/19 22:05:21 apb Exp $

.include <bsd.sys.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
