#	$NetBSD: Makefile,v 1.1 1996/10/14 23:16:07 mark Exp $

AWK=	awk

podules.h podule_data.h: podules podulelist2h.awk
	/bin/rm -f podules.h podule_data.h
	${AWK} -f podulelist2h.awk podules
