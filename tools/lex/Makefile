#	$NetBSD: Makefile,v 1.1 2001/08/14 11:02:00 tv Exp $

HOST_SRCDIR=	usr.bin/lex

.include "${.CURDIR}/../Makefile.host"

${TIMESTAMP}: ${SRCS:Nscan.c:Nskel.c} initscan.c

scan.c:
	echo '#include <initscan.c>' >$@
