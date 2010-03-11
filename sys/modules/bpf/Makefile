#	$NetBSD: Makefile,v 1.1.4.2 2010/03/11 15:04:24 yamt Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	bpf
# XXX: bpf_filter.c maybe needs to be its own module (or made
# inseparable from bpfilter?)
SRCS=	bpf.c bpf_filter.c

.include <bsd.kmodule.mk>
