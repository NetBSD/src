#	$NetBSD: Makefile,v 1.2 1999/11/27 23:57:43 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

KERNELSIZE!=		expr 400 \* 1024

SUBDIR=			setnetimage

SECONDARY_PROG=		smallnet
SECONDARY_LOAD_ADDRESS=	0x80300000	# 3MB - should work on 4MB machines
SRCS=			start.S smallnet.c
SRCS+=			bootinfo.c callvec.c putchar.c

CPPFLAGS+=		-DBOOT_TYPE_NAME='"Compressed Kernel Network"' \
			-DRELOC=${SECONDARY_LOAD_ADDRESS} \
			-DKERNELSIZE=${KERNELSIZE}

.include <bsd.subdir.mk>
.include <bsd.obj.mk>
.include "../Makefile.booters"
