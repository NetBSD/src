#	$NetBSD: Makefile,v 1.13.22.1 2009/09/10 01:51:32 matt Exp $


# Don't strip the ECOFF'ed version on install -- strip gets confused by that,
# and it's already stripped since it's a copy of the stripped ELF one.
#
STRIPFLAG=

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c \
	getopt.c disk.c

PROG=	aoutboot
${PROG}: ip2xboot ip3xboot
.if ${MACHINE_ARCH} == "mipseb"
	${OBJCOPY} --impure -O ecoff-bigmips \
		-R .pdr -R .mdebug.nabi32 R .mdebug.abi32 \
		-R .comment -R .ident \
		ip2xboot ${.TARGET}
.else
	touch ${.TARGET}
.endif

# XXX Temporary hack to install the ELF version, too.
FILES+=		ip2xboot ip3xboot
CLEANFILES+=	ip2xboot ip2xboot.elf ip3xboot ip3xboot.elf

CLEANFILES+=	ip2xboot.map ip3xboot.map

.include "../Makefile.booters"

ip3xboot: ${OBJS} ${LIBS}
	${LD} -Map ${.TARGET}.map -N -x -Ttext ${LOAD_ADDRESS_IP32} ${LDBUG} \
	    -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${.TARGET}.elf -o ${.TARGET}
	@${SIZE} ${.TARGET}

ip2xboot: ${OBJS} ${LIBS}
	${LD} -Map ${.TARGET}.map -N -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${.TARGET}.elf -o ${.TARGET}
	@${SIZE} ${.TARGET}
