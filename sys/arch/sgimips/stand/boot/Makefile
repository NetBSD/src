#	$NetBSD: Makefile,v 1.13.18.1 2009/05/13 17:18:21 jym Exp $

PROG=	aoutboot

# Don't strip the ECOFF'ed version on install -- strip gets confused by that,
# and it's already stripped since it's a copy of the stripped ELF one.
#
STRIPFLAG=

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c \
	getopt.c disk.c

${PROG}: ip2xboot ip3xboot
	${OBJCOPY} --impure -O ecoff-bigmips \
		-R .pdr -R .mdebug.abi32 -R .comment -R .ident \
		ip2xboot aoutboot

# XXX Temporary hack to install the ELF version, too.
FILES+=		ip2xboot ip3xboot
CLEANFILES+=	ip2xboot ip2xboot.elf ip3xboot ip3xboot.elf

CLEANFILES+=	boot.map

.include "../Makefile.booters"

ip3xboot: ${OBJS} ${LIBS}
	${LD} -Map boot.map -N -x -Ttext ${LOAD_ADDRESS_IP32} ${LDBUG} \
	    -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${.TARGET}.elf -o ${.TARGET}
	@${SIZE} ${.TARGET}

ip2xboot: ${OBJS} ${LIBS}
	${LD} -Map boot.map -N -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${.TARGET}.elf -o ${.TARGET}
	@${SIZE} ${.TARGET}

cleandir distclean: cleanlibdir
cleanlibdir:
	-rm -rf lib
