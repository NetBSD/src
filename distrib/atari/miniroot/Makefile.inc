#	$NetBSD: Makefile.inc,v 1.5.8.1 2002/02/09 20:54:17 he Exp $

VND=		vnd0c
IMAGESIZE?=	2880 # 512 byte blocks
NEWFSOPTS?=	-i 3052 -c 60 -B be
DISKTYPE?=	floppyhd

#
# Using libhack:opendir() causes linking errors
#
NOLIBHACKOPENDIR=1

${CBIN}: libhack.o

HACKSRC=../utils/libhack

.include "${HACKSRC}/Makefile.inc"
