#	$NetBSD: Makefile,v 1.12.8.2 2002/02/11 20:10:26 jdolecek Exp $

.include "../Makefile.inc"

.PATH: $S/netinet

CPPFLAGS+= -DIPFILTER_LOG -I$S/netinet -DUSE_INET6 -DIPFILTER_LKM
KMOD=	if_ipl
SRCS=	ip_fil.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c ip_proxy.c \
	ip_auth.c ip_log.c

.include <bsd.kmod.mk>
