#	$NetBSD: Makefile,v 1.5 2000/06/25 16:39:07 scw Exp $

RELOC=0x3F0000

S=	${.CURDIR}/../../../..
DEFS= -DSUN_BOOTPARAMS
INCPATH=-I${.CURDIR} -I${.CURDIR}/../libsa -I${.CURDIR}/../libbug \
	-I${S} -I${S}/lib/libsa
CFLAGS=	-O2 ${INCPATH} ${DEFS} ${COPTS}
CLEANFILES+=netboot netboot.bin

.include "${S}/arch/mvme68k/stand/bugcrt/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libbug/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libsa/Makefile.inc"

SRCS=   boot.c conf.c version.c devopen.c dev_net.c
SRCS+=	if_ie.c if_le.c
OBJS=  ${SRCS:S/.c/.o/g}

realall:	netboot.bin

netboot: ${OBJS} ${LIBSA} ${BUGCRT} ${LIBBUG}
	${LD} -s -N -T ${RELOC} -o $@ \
		${SRTOBJ} ${BUGCRT} ${OBJS} ${LIBSA} ${LIBBUG}
	@size $@

netboot.bin: netboot
	dd ibs=32 skip=1 if=netboot of=$@

install:
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  netboot.bin ${DESTDIR}${MDEC_DIR}/netboot

afterdepend:	.depend
	@(TMP=/tmp/_depend$$$$; \
		sed -e 's/^\([^\.]*\).o[ ]*:/\1.o g\1.o s\1.o:/' \
		< .depend > $$TMP; \
	mv $$TMP .depend)

.include <bsd.prog.mk>
