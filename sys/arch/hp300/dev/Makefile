#	$NetBSD: Makefile,v 1.2.166.1 2009/05/04 08:11:05 yamt Exp $

.include <bsd.sys.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
