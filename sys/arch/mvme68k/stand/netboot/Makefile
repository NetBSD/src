#	$NetBSD: Makefile,v 1.7 2000/07/09 14:16:02 jdolecek Exp $

RELOC=0x3F0000

S=	${.CURDIR}/../../../..
DEFS= -DSUN_BOOTPARAMS
INCPATH=-I${.CURDIR} -I${.CURDIR}/../libsa -I${.CURDIR}/../libbug \
	-I${S} -I${S}/lib/libsa
CFLAGS=	-O2 ${INCPATH} ${DEFS} ${COPTS}
CLEANFILES+=netboot.tmp

.include "${S}/arch/mvme68k/stand/bugcrt/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libbug/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libsa/Makefile.inc"

SRCS=   boot.c conf.c version.c devopen.c dev_net.c
SRCS+=	if_ie.c if_le.c
PROG= netboot.bin
MKMAN=no

${PROG}: ${OBJS} ${LIBSA} ${BUGCRT} ${LIBBUG}
	${LD} -s -N -T ${RELOC} -o netboot.tmp \
		${SRTOBJ} ${BUGCRT} ${OBJS} ${LIBSA} ${LIBBUG}
	@size netboot.tmp
	dd ibs=32 skip=1 if=netboot.tmp of=$@

install:
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	  netboot.bin ${DESTDIR}${MDEC_DIR}/netboot

afterdepend:	.depend
	@(TMP=/tmp/_depend$$$$; \
		sed -e 's/^\([^\.]*\).o[ ]*:/\1.o g\1.o s\1.o:/' \
		< .depend > $$TMP; \
	mv $$TMP .depend)

.include <bsd.prog.mk>
