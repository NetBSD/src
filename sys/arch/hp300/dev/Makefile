#	$NetBSD: Makefile,v 1.3.16.1 2011/03/05 20:50:20 rmind Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${TOOL_AWK} -f devlist2h.awk diodevs
