#	$NetBSD: Makefile,v 1.1 2004/06/22 15:16:30 itojun Exp $
#	$OpenBSD: Makefile,v 1.15 2004/03/10 18:49:49 mcbride Exp $

PROG=	pfctl
SRCS=	pfctl.c parse.y pfctl_parser.c pf_print_state.c pfctl_altq.c
SRCS+=	pfctl_osfp.c pfctl_radix.c pfctl_table.c pfctl_qstats.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/sbin/pfctl
YFLAGS=
MAN=	pfctl.8

WARNS?=	1

LDADD+=	-lm
DPADD+=	${LIBM}

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/sbin/pfctl
