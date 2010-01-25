#	$NetBSD: Makefile,v 1.1 2010/01/25 22:21:28 pooka Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	bpf
# XXX: bpf_filter.c maybe needs to be its own module (or made
# inseparable from bpfilter?)
SRCS=	bpf.c bpf_filter.c

.include <bsd.kmodule.mk>
