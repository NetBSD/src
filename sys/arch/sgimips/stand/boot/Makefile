#	$NetBSD: Makefile,v 1.5.2.1 2004/08/03 10:40:15 skrll Exp $

PROG=	aoutboot

# Don't strip the ECOFF'ed version on install -- strip gets confused by that,
# and it's already stripped since it's a copy of the stripped ELF one.
#
STRIPFLAG=

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c \
	getopt.c disk.c

${PROG}: ip2xboot ip3xboot
	${ELF2ECOFF} ip2xboot aoutboot

# XXX Temporary hack to install the ELF verision, too.
FILES+=		ip2xboot ip3xboot
CLEANFILES+=	ip2xboot ip3xboot

CLEANFILES+=	boot.map

.include "../Makefile.booters"

ip3xboot: ${OBJS} ${LIBS}
	${LD} -Map boot.map -x -Ttext ${LOAD_ADDRESS_IP32} ${LDBUG} \
	    -e start -o ip3xboot ${OBJS} ${LIBS}
	@${STRIP} -s ip3xboot
	@${SIZE} ip3xboot

ip2xboot: ${OBJS} ${LIBS}
	${LD} -Map boot.map -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -e start -o ip2xboot ${OBJS} ${LIBS}
	@${STRIP} -s ip2xboot
	@${SIZE} ip2xboot
