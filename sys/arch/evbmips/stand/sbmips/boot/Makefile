# $NetBSD: Makefile,v 1.1.1.1.6.2 2017/12/03 11:36:10 jdolecek Exp $

PROG = boot

SRCS = start.S boot.c blkdev.c cfe.c cfe_api.c 
SRCS+= filesystem.c booted_dev.c

BOOT_RELOC = ${SECONDARY_LOAD_ADDRESS}

CPPFLAGS += ${SECONDARY_CPPFLAGS} \
		-DBOOT_TYPE_NAME='"Secondary"' \
		-DLIBSA_SINGLE_DEVICE=blkdev \
		  -D"blkdevioctl(x,y,z)=EINVAL"

CLEANFILES+= ${PROG}.sym

${PROG}: ${PROG}.sym
	@echo creating ${PROG} from ${PROG}.sym...
	@${OBJCOPY} --output-target=binary -R .reginfo ${PROG}.sym ${PROG}
	@chmod 644 ${PROG}
	@ls -l ${PROG}
	@${CHECKSIZE_CMD} ${PROG}.sym ${PROG} ${SECONDARY_MAX_LOAD} \
	    ${SECONDARY_MAX_TOTAL} || (rm -f ${PROG} ; false)

SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_USE_CREAD=yes
.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN} ${LDSCRIPT}
	${_MKTARGET_LINK}
	${LD} -T ${LDSCRIPT} -N -e start -o ${PROG}.sym \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	${SIZE} ${PROG}.sym
