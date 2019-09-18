#	$NetBSD: Makefile,v 1.2.10.1 2019/09/18 17:30:05 martin Exp $

PROG=		bootia32.efi
OBJFMT=		pei-i386

EXTRA_SOURCES=	efibootia32.c startprog32.S multiboot32.S

CPUFLAGS=	-march=i686 -mtune=i686
GNUEFIARCH=	ia32
LIBKERN_ARCH=	i386
LIBGNUEFI_ARCH=	i386
KLINK_MACHINE=	i386

CFLAGS+=	-m32 -DACPI_32BIT_PHYSICAL_ADDRESS
AFLAGS+=	-m32

.include "${.CURDIR}/../Makefile.efiboot"
