# $NetBSD: Makefile.inc,v 1.3 2022/09/03 20:29:30 thorpej Exp $
#

.PATH:	${.CURDIR}/../../../../netmpls

SRCS+=	mpls_ttl.c if_mpls.c mpls_proto.c
