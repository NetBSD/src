#	$NetBSD: Makefile.walnut.inc,v 1.2 2001/12/09 05:02:22 atatat Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403

netbsd.img: netbsd
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; ${MKIMG} netbsd netbsd.img

