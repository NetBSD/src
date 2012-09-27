#	$NetBSD: Makefile,v 1.4 2012/09/27 18:28:56 alnsn Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	bpf
# XXX: bpf_filter.c maybe needs to be its own module (or made
# inseparable from bpfilter?)
SRCS=	bpf.c bpf_filter.c

.include <bsd.kmodule.mk>
