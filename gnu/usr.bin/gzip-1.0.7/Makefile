# $Header: /cvsroot/src/gnu/usr.bin/gzip-1.0.7/Attic/Makefile,v 1.1 1993/04/10 15:55:11 cgd Exp $

PROG=	gzip
CFLAGS+=-DASMV -DHAVE_UNISTD_H=1 -DDIRENT=1
SRCS=	gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c \
	util.c crypt.c lzw.c unlzw.c unpack.c getopt.c _match.s
RANDOMPROGS= zdiff zmore znew zforce gzexe
CLEANFILES+= _match.s ${RANDOMPROGS}
MAN1=	gzexe.0 gzip.0 zdiff.0 zforce.0 zmore.0 znew.0

_match.s: ${.CURDIR}/match.S
	${CPP} ${.CURDIR}/match.S > $@

afterinstall: ${RANDOMPROGS}
	isntall ${COPY} -o root -g wheel -m 555 ${RANDOMPROGS} \
		${DESTDIR}${BINDIR}

${RANDOMPROGS}:
	sed "1d" ${.CURDIR}/$@.in > $@

.include <bsd.prog.mk>
