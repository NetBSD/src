#	$NetBSD: Makefile,v 1.2 1997/01/30 09:18:33 thorpej Exp $

AWK=	awk

diodevs.h diodevs_data.h: diodevs devlist2h.awk
	/bin/rm -f diodevs.h diodevs_data.h
	${AWK} -f devlist2h.awk diodevs
