#	$NetBSD: Makefile,v 1.7 2000/07/09 13:47:34 jdolecek Exp $

#
# sboot would like a newer GNU ld because it can generate S-Records.
# Until then, we convert.
#

S=${.CURDIR}/../../../..

COMPILE.s= $(AS) $(ASFLAGS) -o $*.o
LDFLAGS=-x -N -Ttext 0x4000 -e start
MKMAN=	no
SRCS=	start.s clock.c console.c etherfun.c le_poll.c libc_sa.c \
		oc_cksum.s sboot.c
CLEANFILES+= srec sboot sboot.srec

BOOTS=	sboot.srec
PROG=${BOOTS}

${PROG}: ${OBJS}
	${MAKE} sboot
	${MAKE} srec
	dd ibs=32 skip=1 if=sboot | ${.OBJDIR}/srec 4 0x4000 sboot > ${.TARGET}

sboot: ${OBJS}
	${LD} ${LDFLAGS} -o ${.TARGET} ${OBJS}

srec: ${.CURDIR}/srec.c
	${CC} ${CFLAGS} ${.CURDIR}/srec.c -o ${.TARGET}

install:
	${INSTALL} -c -m 444 -g bin -o bin sboot.srec \
	    ${DESTDIR}${MDEC_DIR}/sboot

afterdepend:	.depend
	@(TMP=/tmp/_depend$$$$; \
		sed -e 's/^\([^\.]*\).o[ ]*:/\1.o g\1.o s\1.o:/' \
		< .depend > $$TMP; \
	mv $$TMP .depend)

.include <bsd.prog.mk>
