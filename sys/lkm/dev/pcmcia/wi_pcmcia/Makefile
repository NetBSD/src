# $NetBSD: Makefile,v 1.1.2.2 2004/08/25 13:23:42 skrll Exp $

S= ${.CURDIR}/../../../..

KMOD= wi_pcmcia
SRCS= wi_pcmcia_lkm.c
NOMAN=	#yes
WARNS=	2

#CPPFLAGS+=	-DDIAGNOSTIC -DLOCKDEBUG
CPPFLAGS+=	-DDIAGNOSTIC -DDEBUG

.PATH:	$S/dev/pcmcia $S/dev/ic
SRCS+=	if_wi_pcmcia.c

# if the "wi" driver backend is already in the
# kernel (as "wi at pci"), comment out / uncomment
SRCS+=	wi.c
#CPPFLAGS+=	-DWI_DRIVER_IS_PRESENT

.include <bsd.kmod.mk>
