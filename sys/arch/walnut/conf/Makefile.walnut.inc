#	$NetBSD: Makefile.walnut.inc,v 1.1.4.3 2002/01/10 19:50:09 thorpej Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403

netbsd.img: netbsd
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; ${MKIMG} netbsd netbsd.img

