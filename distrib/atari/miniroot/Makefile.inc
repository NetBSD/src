#	$NetBSD: Makefile.inc,v 1.4 1999/03/23 10:22:31 leo Exp $

VND=		vnd0c
IMAGESIZE?=	2160 # 512 byte blocks
NEWFSOPTS?=	-i 3052 -c 60 -B be
DISKTYPE?=	floppybt

${CBIN}: libhack.o

HACKSRC=../utils/libhack

.include "${HACKSRC}/Makefile.inc"
