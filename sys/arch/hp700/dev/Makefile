#	$NetBSD: Makefile,v 1.1.112.1 2009/01/17 13:28:01 mjf Exp $

#	$OpenBSD: Makefile,v 1.3 1999/04/20 20:19:37 mickey Exp $

.include <bsd.sys.mk>

cpudevs.h cpudevs_data.h: cpudevs devlist2h.awk
	/bin/rm -f cpudevs.h cpudevs_data.h
	${TOOL_AWK} -f devlist2h.awk cpudevs

