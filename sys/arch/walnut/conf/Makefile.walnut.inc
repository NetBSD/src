MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403

netbsd.img: netbsd
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; ${MKIMG} netbsd netbsd.img

