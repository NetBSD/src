#	$NetBSD: Makefile,v 1.2.12.1 1999/11/30 13:34:21 itojun Exp $

# use 'make -f Makefile.pcmciadevs' to make pcmciadevs.h and pcmciadevs_data.h

INCSDIR= /usr/include/dev/pcmcia

INCS=	pcmciachip.h pcmciareg.h pcmciavar.h

# if_wi.c
INCS+=	if_wi_ieee.h

# if_cnw.c
INCS+=	if_cnwioctl.h

.include <bsd.kinc.mk>
