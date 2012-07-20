# $NetBSD: Makefile,v 1.3 2012/07/20 15:23:02 matt Exp $

PROG=	test
BINDIR=	/stand

SRCS = start.S test.c prom.c prom_disp.S

RELOC = ${STANDPROG_LOAD_ADDRESS}

CPPFLAGS += ${STANDPROG_CPPFLAGS}

CLEANFILES+= ${PROG}.sym

# XXX HACK!
${PROG}: ${PROG}.sym
	rm -f ${PROG}
	cp ${PROG}.sym ${PROG}

SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_USE_CREAD=no
.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} -Ttext ${RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} ${LIBSA} ${LIBKERN}
