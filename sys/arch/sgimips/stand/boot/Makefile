#	$NetBSD: Makefile,v 1.3 2001/11/22 00:58:08 thorpej Exp $

PROG=	boot

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c disk.c

${PROG}: ${PROG}.elf
	${OBJCOPY} -O ecoff-bigmips ${PROG}.elf ${PROG}
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
