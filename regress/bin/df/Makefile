#	$NetBSD: Makefile,v 1.2 2004/07/07 01:57:35 enami Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

PROG=	df
.PATH:  ${NETBSDSRCDIR}/bin/df
.PATH:  ${NETBSDSRCDIR}/bin/csh
SRCS=	df.c strpct.c getmntinfo.c

regress:
	./df | cmp ${.CURDIR}/df.out -
	./df -h | cmp ${.CURDIR}/df-h.out -

.include <bsd.prog.mk>
