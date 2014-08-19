#	$NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $

#	$OpenBSD: Makefile,v 1.3 1999/04/20 20:19:37 mickey Exp $

.include <bsd.own.mk>

cpudevs.h cpudevs_data.h: cpudevs devlist2h.awk
	/bin/rm -f cpudevs.h cpudevs_data.h
	${TOOL_AWK} -f devlist2h.awk cpudevs

