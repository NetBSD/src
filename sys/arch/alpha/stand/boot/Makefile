# $NetBSD: Makefile,v 1.23 1999/04/02 03:11:57 cgd Exp $

PROG = boot

SRCS = start.S boot.c loadfile.c disk.c conf.c prom.c prom_disp.S OSFpal.c
SRCS+= devopen.c filesystem.c prom_swpal.S

BOOT_RELOC = ${SECONDARY_LOAD_ADDRESS}

CPPFLAGS += ${SECONDARY_CPPFLAGS}

CLEANFILES+= vers.c vers.o ${PROG}.sym

${PROG}: ${PROG}.sym
	@echo creating ${PROG} from ${PROG}.sym...
	@objcopy --output-target=binary ${PROG}.sym ${PROG}
	@chmod 644 ${PROG}
	@ls -l ${PROG}
	@${CHECKSIZE_CMD} ${PROG}.sym ${PROG} ${SECONDARY_MAX_LOAD} \
	    ${SECONDARY_MAX_TOTAL} || (rm -f ${PROG} ; false)

SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_USE_CREAD=yes
.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/newvers.sh ${.CURDIR}/version
	${COMPILE.c} vers.c
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	${SIZE} ${PROG}.sym
