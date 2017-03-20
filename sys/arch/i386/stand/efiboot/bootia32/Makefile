#	$NetBSD: Makefile,v 1.2.2.2 2017/03/20 06:57:15 pgoyette Exp $

PROG=		bootia32.efi
OBJFMT=		pei-i386

EXTRA_SOURCES=	efibootia32.c startprog32.S

CPUFLAGS=	-march=i686 -mtune=i686
GNUEFIARCH=	ia32
LIBKERN_ARCH=	i386
LIBGNUEFI_ARCH=	i386
KLINK_MACHINE=	i386

CFLAGS+=	-m32
AFLAGS+=	-m32

.include "${.CURDIR}/../Makefile.efiboot"
