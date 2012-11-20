#	$NetBSD: Makefile,v 1.3.2.1 2012/11/20 03:02:46 tls Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	bpf
# XXX: bpf_filter.c maybe needs to be its own module (or made
# inseparable from bpfilter?)
SRCS=	bpf.c bpf_filter.c

.include <bsd.kmodule.mk>
