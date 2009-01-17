#	$NetBSD: Makefile,v 1.2.162.1 2009/01/17 13:28:00 mjf Exp $

.include <bsd.sys.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
