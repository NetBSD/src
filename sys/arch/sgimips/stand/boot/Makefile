#	$NetBSD: Makefile,v 1.3.12.1 2002/07/15 16:03:51 thorpej Exp $

PROG=	boot

# Don't strip the ECOFF'ed version on install -- strip gets confused by that,
# and it's already stripped since it's a copy of the stripped ELF one.
#
STRIPFLAG=

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c disk.c

${PROG}: ${PROG}.elf
	${ELF2ECOFF} ${PROG}.elf ${PROG}
	@${SIZE} ${PROG}

# XXX Temporary hack to install the ELF verision, too.
FILES+=		${PROG}.elf
CLEANFILES+=	${PROG}.elf

CLEANFILES+=	${PROG}.map

.include "../Makefile.booters"

${PROG}.elf: ${OBJS} ${LIBS}
	${LD} -Map ${PROG}.map -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -e start -o ${PROG}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${PROG}.elf
	@${SIZE} ${PROG}.elf
