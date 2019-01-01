#	$NetBSD: Makefile,v 1.21 2019/01/01 19:41:05 christos Exp $


# Don't strip the ECOFF'ed version on install -- strip gets confused by that,
# and it's already stripped since it's a copy of the stripped ELF one.
#
STRIPFLAG=

SRCS=	start.S boot.c bootinfo.c conf.c devopen.c putchar.c getchar.c \
	getopt.c disk.c arcbios_calls.S

PROG=	aoutboot
${PROG}: ip2xboot ip3xboot
.if ${MACHINE_ARCH} == "mipseb"
	${MIPS_ELF2ECOFF} ip2xboot ${.TARGET}
.else
	touch ${.TARGET}
.endif

# XXX Temporary hack to install the ELF version, too.
FILES+=		ip2xboot ip3xboot
CLEANFILES+=	ip2xboot ip2xboot.elf ip3xboot ip3xboot.elf

CLEANFILES+=	ip2xboot.map ip3xboot.map

.include "../Makefile.booters"

ip3xboot: ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
	${LD} -Map ${.TARGET}.map -N -x -Ttext ${LOAD_ADDRESS_IP32} ${LDBUG} \
	    -m elf32btsmip -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${.TARGET}.elf -o ${.TARGET}
	@${SIZE} ${.TARGET}

ip2xboot: ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
	${LD} -Map ${.TARGET}.map -N -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -m elf32btsmip -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
	@${STRIP} -s ${.TARGET}.elf -o ${.TARGET}
	@${SIZE} ${.TARGET}

cleandir distclean: .WAIT cleanlibdir
cleanlibdir:
	-rm -rf lib
