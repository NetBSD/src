# $NetBSD: Makefile,v 1.2 2000/07/21 21:25:19 jdolecek Exp $

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
	${LD} -Ttext ${RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} ${LIBSA} ${LIBKERN}
