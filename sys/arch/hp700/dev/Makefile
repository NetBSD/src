#	$NetBSD: Makefile,v 1.1.2.2 2002/06/23 17:36:19 jdolecek Exp $

#	$OpenBSD: Makefile,v 1.3 1999/04/20 20:19:37 mickey Exp $

AWK?=	awk

cpudevs.h cpudevs_data.h: cpudevs devlist2h.awk
	/bin/rm -f cpudevs.h cpudevs_data.h
	${AWK} -f devlist2h.awk cpudevs

