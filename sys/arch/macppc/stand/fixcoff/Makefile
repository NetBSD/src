#	$NetBSD: Makefile,v 1.5 2003/12/10 23:45:25 matt Exp $

NOMAN=  # defined

.include <bsd.own.mk>

HOSTPROG=   fixcoff
SRCS=    fixcoff.c nb_progname.c

.include <bsd.hostprog.mk>

COMPATOBJ!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
.-include	"${COMPATOBJ}/defs.mk"

