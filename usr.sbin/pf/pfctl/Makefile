#	$NetBSD: Makefile,v 1.2 2004/11/11 12:03:54 yamt Exp $
#	$OpenBSD: Makefile,v 1.15 2004/03/10 18:49:49 mcbride Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	pfctl
SRCS=	pfctl.c parse.y pfctl_parser.c pf_print_state.c pfctl_altq.c
SRCS+=	pfctl_osfp.c pfctl_radix.c pfctl_table.c pfctl_qstats.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/sbin/pfctl
YFLAGS=
MAN=	pfctl.8

LDADD+=	-lm
DPADD+=	${LIBM}

BINDIR=		/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/sbin/pfctl
