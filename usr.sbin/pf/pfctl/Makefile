#	$NetBSD: Makefile,v 1.6 2023/06/03 09:09:22 lukem Exp $
#	$OpenBSD: Makefile,v 1.17 2004/07/16 23:44:24 frantzen Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	pfctl
SRCS=	pfctl.c parse.y pfctl_parser.c pf_print_state.c pfctl_altq.c
SRCS+=	pfctl_osfp.c pfctl_radix.c pfctl_table.c pfctl_qstats.c
SRCS+=	pfctl_optimize.c pf_ruleset.c
CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/sbin/pfctl
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/pf
YFLAGS=
MAN=	pfctl.8

# Ruleset and Anchor handling
.PATH: ${NETBSDSRCDIR}/sys/dist/pf/net

LDADD+=	-lm
DPADD+=	${LIBM}

BINDIR=		/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.include <bsd.prog.mk>

COPTS.pfctl_osfp.c+=	${CC_WNO_FORMAT_TRUNCATION}

.PATH: ${NETBSDSRCDIR}/dist/pf/sbin/pfctl
