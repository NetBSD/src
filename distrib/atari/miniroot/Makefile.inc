#	$NetBSD: Makefile.inc,v 1.5 1999/08/05 15:00:28 leo Exp $

VND=		vnd0c
IMAGESIZE?=	2160 # 512 byte blocks
NEWFSOPTS?=	-i 3052 -c 60 -B be
DISKTYPE?=	floppybt

#
# Using libhack:opendir() causes linking errors
#
NOLIBHACKOPENDIR=1

${CBIN}: libhack.o

HACKSRC=../utils/libhack

.include "${HACKSRC}/Makefile.inc"
