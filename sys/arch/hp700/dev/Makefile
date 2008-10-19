#	$NetBSD: Makefile,v 1.2 2008/10/19 22:05:21 apb Exp $

#	$OpenBSD: Makefile,v 1.3 1999/04/20 20:19:37 mickey Exp $

.include <bsd.sys.mk>

cpudevs.h cpudevs_data.h: cpudevs devlist2h.awk
	/bin/rm -f cpudevs.h cpudevs_data.h
	${TOOL_AWK} -f devlist2h.awk cpudevs

