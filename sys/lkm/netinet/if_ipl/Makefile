#	$NetBSD: Makefile,v 1.12.6.1 2002/01/08 00:33:19 nathanw Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

CPPFLAGS+= -DIPFILTER_LOG -I$S/netinet -DUSE_INET6
KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c

.include <bsd.kmod.mk>
