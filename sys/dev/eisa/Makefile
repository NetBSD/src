#	$NetBSD: Makefile,v 1.1 1996/02/26 23:46:17 cgd Exp $

AWK=	awk

eisadevs.h eisadevs_data.h: eisadevs devlist2h.awk
	/bin/rm -f eisadevs.h eisadevs_data.h
	${AWK} -f devlist2h.awk eisadevs
