#	$NetBSD: Makefile,v 1.1 2004/11/11 11:54:51 yamt Exp $
#	$OpenBSD: Makefile,v 1.10 2003/11/20 23:23:09 avsm Exp $

PROG=	authpf
MAN=	authpf.8
BINOWN= root
BINGRP= authpf
BINMODE= 6555
SRCS=	authpf.c parse.y pfctl_parser.c pf_print_state.c pfctl_altq.c
SRCS+=	pfctl_radix.c pfctl_osfp.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/sbin/pfctl

LDADD+= -lm
DPADD+= ${LIBM}

CLEANFILES+=y.tab.h

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/sbin/pfctl
.PATH: ${NETBSDSRCDIR}/dist/pf/usr.sbin/authpf
