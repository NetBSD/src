#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.7 2000/07/09 13:47:33 jdolecek Exp $

RELOC=0x3F0000 

S=	${.CURDIR}/../../../..
DEFS= 
INCPATH=-I${.CURDIR} -I${.CURDIR}/../libsa -I${.CURDIR}/../libbug \
	-I${.CURDIR}/../../include -I${S} -I${S}/lib/libsa
CFLAGS=	-O2 ${INCPATH} ${DEFS} ${COPTS}
CLEANFILES+=stboot bootst bootst.bug

.include "${S}/arch/mvme68k/stand/libsa/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libbug/Makefile.inc"
.include "${S}/arch/mvme68k/stand/bugcrt/Makefile.inc"
.include "${S}/arch/mvme68k/stand/wrtvid/Makefile.inc"

PROG=dummy
MKMAN=no
SRCS=   boot.c conf.c dev_tape.c rawfs.c version.c
LIBS=	${LIBSA} ${LIBBUG}

${PROG}: ${OBJS} ${BUGCRT} ${LIBS}
	${LD} -s -N -T ${RELOC} ${BUGCRT} ${OBJS} ${LIBS} -o bootst.bug
	@size bootst.bug
	@${WRTVID} bootst.bug # creates bootst  and stboot
	touch ${PROG}

install:
	${INSTALL} -c -m 555 -g bin -o bin bootst ${DESTDIR}${MDEC_DIR}
	${INSTALL} -c -m 555 -g bin -o bin stboot ${DESTDIR}${MDEC_DIR}

afterdepend:	.depend
	@(TMP=/tmp/_depend$$$$; \
		sed -e 's/^\([^\.]*\).o[ ]*:/\1.o g\1.o s\1.o:/' \
	< .depend > $$TMP; \
	mv $$TMP .depend)

.include <bsd.prog.mk>
