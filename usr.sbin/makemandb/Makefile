# $NetBSD: Makefile,v 1.5 2013/01/14 18:04:58 christos Exp $

.include <bsd.own.mk>

MDOCDIR=${NETBSDSRCDIR}/external/bsd/mdocml
MANCONFDIR=${NETBSDSRCDIR}/usr.bin/man

PROGS=			makemandb apropos whatis
SRCS.makemandb=		makemandb.c apropos-utils.c manconf.c
SRCS.apropos=	apropos.c apropos-utils.c manconf.c
SRCS.whatis=	whatis.c apropos-utils.c manconf.c
MAN.makemandb=	makemandb.8
MAN.apropos=	apropos.1
MAN.whatis=	whatis.1

BINDIR.apropos=		/usr/bin
BINDIR.makemandb=	/usr/sbin
BINDIR.whatis=		/usr/bin

.PATH: ${MANCONFDIR}

CPPFLAGS+= -I${MDOCDIR} -I${MANCONFDIR} -I${.OBJDIR}

MDOCMLOBJDIR!=	cd ${MDOCDIR}/lib/libmandoc && ${PRINTOBJDIR}
MDOCMLLIB=	${MDOCMLOBJDIR}/libmandoc.a

DPADD.makemandb+= 	${MDOCMLLIB} ${LIBARCHIVE} ${LIBBZ2} ${LIBLZMA}
LDADD.makemandb+= 	-L${MDOCMLOBJDIR} -lmandoc -larchive -lbz2 -llzma
DPADD+=		${LIBSQLITE3} ${LIBM} ${LIBZ} ${LIBTERMLIB} ${LIBUTIL}
LDADD+=		-lsqlite3 -lm -lz -ltermlib -lutil

stopwords.c: stopwords.txt
	( set -e; ${TOOL_NBPERF} -n stopwords_hash -s -p ${.ALLSRC};	\
	echo 'static const char *stopwords[] = {';			\
	${TOOL_SED} -e 's|^\(.*\)$$|	"\1",|' ${.ALLSRC};		\
	echo '};'							\
	) > ${.TARGET}

DPSRCS+=	stopwords.c
CLEANFILES+=	stopwords.c

.include <bsd.prog.mk>
