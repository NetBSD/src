# $NetBSD: Makefile,v 1.15 1998/03/28 00:21:35 thorpej Exp $

PROG = bootxx

SRCS = start.S bootxx.c prom.c prom_disp.S puts.c

BOOT_RELOC = ${PRIMARY_LOAD_ADDRESS}
HEADERSIZE_PROG = headersize

CPPFLAGS += ${PRIMARY_CPPFLAGS}

CLEANFILES+= ${PROG}.sym ${PROG}.nosym ${HEADERSIZE_PROG}

${PROG}.nosym: ${PROG}.sym
	cp ${PROG}.sym ${PROG}.nosym
	${STRIP} ${PROG}.nosym

${PROG}: ${PROG}.nosym ${HEADERSIZE_PROG}
	dd if=${PROG}.nosym of=${PROG} \
	    ibs=`./${HEADERSIZE_PROG} ${BOOT_RELOC} ${PROG}.nosym` skip=1 \
	    obs=`expr 15 \* 512` conv=osync

.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBKERN}
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${PROG}.sym ${OBJS} \
	    ${LIBKERN}
	${SIZE} ${PROG}.sym
