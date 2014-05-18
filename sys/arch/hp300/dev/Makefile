#	$NetBSD: Makefile,v 1.4.22.1 2014/05/18 17:45:07 rmind Exp $

.include <bsd.own.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
