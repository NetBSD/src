# $NetBSD: Makefile,v 1.2 1999/04/05 06:42:15 cgd Exp $

PROG = ustarboot

SRCS = start.S boot.c loadfile.c blkdev.c prom.c prom_disp.S OSFpal.c
SRCS+= prom_swpal.S booted_dev.c

BOOT_RELOC = ${UNIFIED_LOAD_ADDRESS}

CPPFLAGS += ${UNIFIED_CPPFLAGS} \
		-DBOOT_TYPE_NAME='"ustar"' \
		-DLIBSA_SINGLE_DEVICE=blkdev \
		  -D"blkdevioctl(x,y,z)=EINVAL" \
		-DLIBSA_SINGLE_FILESYSTEM=ustarfs

CLEANFILES+= vers.c vers.o ${PROG}.sym

${PROG}: ${PROG}.sym
	@echo creating ${PROG} from ${PROG}.sym...
	@objcopy --output-target=binary ${PROG}.sym ${PROG}
	@chmod 644 ${PROG}
	@ls -l ${PROG}
	@${CHECKSIZE_CMD} ${PROG}.sym ${PROG} ${UNIFIED_MAX_LOAD} \
	    ${UNIFIED_MAX_TOTAL} || (rm -f ${PROG} ; false)

SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_USE_CREAD=yes
.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/newvers.sh ${.CURDIR}/version
	${COMPILE.c} vers.c
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	${SIZE} ${PROG}.sym
