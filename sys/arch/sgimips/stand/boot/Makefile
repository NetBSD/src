#	$NetBSD: Makefile,v 1.5 2002/12/23 19:31:25 pooka Exp $

PROG=	boot

# Don't strip the ECOFF'ed version on install -- strip gets confused by that,
# and it's already stripped since it's a copy of the stripped ELF one.
#
STRIPFLAG=

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c disk.c

${PROG}: ${PROG}.elf ${PROG}.ip32
	${ELF2ECOFF} ${PROG}.elf ${PROG}
	@${SIZE} ${PROG}

# XXX Temporary hack to install the ELF verision, too.
FILES+=		${PROG}.elf ${PROG}.ip32
CLEANFILES+=	${PROG}.elf ${PROG}.ip32

CLEANFILES+=	${PROG}.map

.include "../Makefile.booters"

${PROG}.ip32: ${OBJS} ${LIBS}
	${LD} -Map ${PROG}.map -x -Ttext ${LOAD_ADDRESS_IP32} ${LDBUG} \
	    -e start -o ${PROG}.ip32 ${OBJS} ${LIBS}
	@${STRIP} -s ${PROG}.ip32
	@${SIZE} ${PROG}.ip32

${PROG}.elf: ${OBJS} ${LIBS}
	${LD} -Map ${PROG}.map -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -e start -o ${PROG}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${PROG}.elf
	@${SIZE} ${PROG}.elf
