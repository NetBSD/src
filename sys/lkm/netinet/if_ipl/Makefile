#	$NetBSD: Makefile,v 1.15 2002/01/24 13:50:32 martti Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

CPPFLAGS+= -DIPFILTER_LOG -I$S/netinet -DUSE_INET6 -DIPFILTER_LKM
KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c

.include <bsd.kmod.mk>
