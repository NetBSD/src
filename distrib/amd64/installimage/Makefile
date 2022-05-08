#	$NetBSD: Makefile,v 1.22 2022/05/08 08:12:45 mrg Exp $

.include <bsd.own.mk>

INSTIMGBASE=	NetBSD-${DISTRIBVER}-amd64-install	# gives ${IMGBASE}.img

.include "installimage.mk"

EFIBOOT=		${WORKDIR}/usr/mdec/bootx64.efi
EFIBOOT+=		${WORKDIR}/usr/mdec/bootia32.efi
#EFIBOOT=		${.OBJDIR}/../../../sys/arch/i386/stand/efiboot/bootx64/bootx64.efi
#EFIBOOT+=		${.OBJDIR}/../../../sys/arch/i386/stand/efiboot/bootia32/bootia32.efi

USE_MBR=		yes
USE_GPT=		yes
USE_GPTMBR=		yes
