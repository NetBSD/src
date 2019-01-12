#	$NetBSD: Makefile,v 1.1 2019/01/12 16:44:47 tsutsui Exp $

# Silicon Graphics "IRIS" series MIPS processors machine.
STRIPFLAG=
# If you try to use ARCBIOS(Indy or Indigo R4000) machine, Please comment it out!
# Be sure to edit /sgimips/stand/common/iris_machdep.h!
PRE_ARCS= yes

.if defined(PRE_ARCS)
SRCS=   iris_start.S 
.else
SRCS=   start.S arcbios_calls.S
.endif

SRCS+=  iris_boot.c
SRCS+=  bootinfo.c
SRCS+=  iris_prf.c iris_cons.c iris_zs.c
SRCS+=  iris_parse.c
SRCS+=  iris_autoconf.c iris_scsi.c
SRCS+=  iris_disk.c
SRCS+=  iris_scsictl.c
SRCS+=  iris_conf.c
SRCS+=  iris_devopen.c

PROG=	irisboot
${PROG}: ip12boot
.if ${MACHINE_ARCH} == "mipseb"
	cp ip12boot ip12boot.tmp
	${MIPS_ELF2ECOFF} ip12boot.tmp ${.TARGET}
.else
	touch ${.TARGET}
.endif

# XXX Temporary hack to install the ELF version, too.
FILES+=		ip12boot
CLEANFILES+=	ip12boot ip12boot.elf

CLEANFILES+=	ip12boot.map

CLEANFILES+=	ip12boot.tmp

.include "../Makefile.booters"

ip12boot: ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
.if defined(PRE_ARCS)
	${LD} -Map ${.TARGET}.map -N -x -Ttext ${LOAD_ADDRESS_IP12} ${LDBUG} \
	    -m elf32btsmip -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
.else
	${LD} -Map ${.TARGET}.map -N -x -Ttext ${LOAD_ADDRESS} ${LDBUG} \
	    -m elf32btsmip -e start -o ${.TARGET}.elf ${OBJS} ${LIBS}
.endif
	@${STRIP} -s ${.TARGET}.elf -o ${.TARGET}
	@${SIZE} ${.TARGET}

cleandir distclean: .WAIT cleanlibdir
cleanlibdir:
	-rm -rf lib
