# $NetBSD: Makefile,v 1.1 1999/04/11 03:38:51 cgd Exp $

PROG=	test
BINDIR=	/stand

SRCS = start.S test.c prom.c prom_disp.S

RELOC = ${STANDPROG_LOAD_ADDRESS}

CPPFLAGS += ${STANDPROG_CPPFLAGS}

CLEANFILES+= vers.c vers.o ${PROG}.sym

# XXX HACK!
${PROG}: ${PROG}.sym
	rm -f ${PROG}
	cp ${PROG}.sym ${PROG}

SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_USE_CREAD=no
.include "../Makefile.bootprogs"

${PROG}.sym: ${OBJS} ${LIBSA} ${LIBKERN}
	sh ${.CURDIR}/../common/newvers.sh ${.CURDIR}/version
	${COMPILE.c} vers.c
	${LD} -Ttext ${RELOC} -N -e start -o ${PROG}.sym \
	    ${OBJS} vers.o ${LIBSA} ${LIBKERN}
