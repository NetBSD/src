#	$NetBSD: Makefile,v 1.7 2003/11/13 08:01:17 sekiya Exp $

PROG=	boot

# Don't strip the ECOFF'ed version on install -- strip gets confused by that,
# and it's already stripped since it's a copy of the stripped ELF one.
#
STRIPFLAG=

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c \
	getopt.c disk.c

${PROG}: ip2x${PROG} ip3x${PROG}
	${ELF2ECOFF} ip2x${PROG} aout${PROG}

# XXX Temporary hack to install the ELF verision, too.
FILES+=		ip2x${PROG} ip3x${PROG}
CLEANFILES+=	ip2x${PROG} ip3x${PROG}

CLEANFILES+=	${PROG}.map

.include "../Makefile.booters"

ip3x${PROG}: ${OBJS} ${LIBS}
	${LD} -Map ${PROG}.map -x -Ttext ${LOAD_ADDRESS_IP32} ${LDBUG} \
	    -e start -o ip3x${PROG} ${OBJS} ${LIBS}
	@${STRIP} -s ip3x${PROG}
	@${SIZE} ip3x${PROG}

ip2x${PROG}: ${OBJS} ${LIBS}
	${LD} -Map ${PROG}.map -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -e start -o ip2x${PROG} ${OBJS} ${LIBS}
	@${STRIP} -s ip2x${PROG}
	@${SIZE} ip2x${PROG}
