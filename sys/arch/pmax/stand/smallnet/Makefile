#	$NetBSD: Makefile,v 1.1 1999/05/13 08:38:05 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

KERNELSIZE!=	expr 400 \* 1024

SUBDIR=		setnetimage

PROG=		smallnet
RELOC=		80300000		# 3MB - should work on 4MB machines
SRCS=		start.S smallnet.c
BOOTDEFADD=	-DBOOT_TYPE_NAME='"Compressed Kernel Network"' \
		-DRELOC=0x${RELOC} -DKERNELSIZE=${KERNELSIZE}
VERSIONFILE=	${.CURDIR}/version
.PATH:  	${.CURDIR}/../lib

.include <bsd.subdir.mk>
.include <bsd.obj.mk>
.include "../Makefile.booters"
