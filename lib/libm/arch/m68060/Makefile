# $NetBSD: Makefile,v 1.2 2000/04/14 20:24:40 is Exp $

M060SP=		../../../../sys/arch/m68k/060sp
ASM2GAS=	${M060SP}/asm2gas
FPLSP_SA=	${M060SP}/dist/fplsp.sa

all: fplsp.hex fplsp_wrap.S

fplsp_wrap.S Makefile.list: makeas.sh
	sh makeas.sh

fplsp.hex: ${FPLSP_SA} ${ASM2GAS} makeoffs.awk
	sh ${ASM2GAS} ${FPLSP_SA} | awk -f makeoffs.awk > fplsp.hex

.if exists(Makefile.list)
.include "Makefile.list"
.endif

clean:
	-rm ${ARCH_SRCS} ${ARCH_ADDS} fplsp.hex Makefile.list
