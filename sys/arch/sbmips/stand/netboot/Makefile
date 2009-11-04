# $NetBSD: Makefile,v 1.4 2009/11/04 17:03:52 skrll Exp $

PROG = netboot

SRCS = start.S boot.c cfe_api.c cfe.c
SRCS+= dev_net.c conf.c devopen.c
SRCS+= if_cfe.c getsecs.c booted_dev.c

BOOT_RELOC = ${UNIFIED_LOAD_ADDRESS}

CLEANFILES+= ${PROG}.sym

CPPFLAGS += ${UNIFIED_CPPFLAGS} -DSUPPORT_DHCP \
		-DBOOT_TYPE_NAME='"Network"'

${PROG}: ${PROG}.sym
	@echo creating ${PROG} from ${PROG}.sym...
	@${OBJCOPY} --output-target=binary -R .reginfo -R .rel.dyn \
	    ${PROG}.sym ${PROG}
	@chmod 644 ${PROG}
	@ls -l ${PROG}
	@${CHECKSIZE_CMD} ${PROG}.sym ${PROG} ${UNIFIED_MAX_LOAD} \
	    ${UNIFIED_MAX_TOTAL} || (rm -f ${PROG} ; false)

SAMISCMAKEFLAGS= SA_INCLUDE_NET=yes SA_USE_CREAD=yes
.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN} 
	${SIZE} ${PROG}.sym
