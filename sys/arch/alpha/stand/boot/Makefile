# $NetBSD: Makefile,v 1.31 2001/09/22 03:34:17 tv Exp $

PROG = boot

SRCS = start.S boot.c blkdev.c prom.c prom_disp.S OSFpal.c
SRCS+= filesystem.c prom_swpal.S booted_dev.c

# XXX for debugging primary and secondary together
# SRCS+= putstr.c putulong.c

BOOT_RELOC = ${SECONDARY_LOAD_ADDRESS}

CPPFLAGS += ${SECONDARY_CPPFLAGS} \
		-DBOOT_TYPE_NAME='"Secondary"' \
		-DBOOTXX_RAID1_SUPPORT \
		-DLIBSA_SINGLE_DEVICE=blkdev \
		  -D"blkdevioctl(x,y,z)=EINVAL"

CLEANFILES+= ${PROG}.sym

${PROG}: ${PROG}.sym
	@echo creating ${PROG} from ${PROG}.sym...
	@${OBJCOPY} --output-target=binary ${PROG}.sym ${PROG}
	@chmod 644 ${PROG}
	@ls -l ${PROG}
	@${CHECKSIZE_CMD} ${PROG}.sym ${PROG} ${SECONDARY_MAX_LOAD} \
	    ${SECONDARY_MAX_TOTAL} || (rm -f ${PROG} ; false)

SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_USE_CREAD=yes
.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	${SIZE} ${PROG}.sym
