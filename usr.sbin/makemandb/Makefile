# $NetBSD: Makefile,v 1.12 2020/11/12 17:53:43 nia Exp $

.include <bsd.own.mk>

MDOCDIR=${NETBSDSRCDIR}/external/bsd/mdocml
MANCONFDIR=${NETBSDSRCDIR}/usr.bin/man

PROGS=			makemandb apropos whatis
SRCS.makemandb=		makemandb.c apropos-utils.c manconf.c custom_apropos_tokenizer.c
SRCS.apropos=	apropos.c apropos-utils.c manconf.c custom_apropos_tokenizer.c
SRCS.whatis=	whatis.c apropos-utils.c manconf.c custom_apropos_tokenizer.c
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

DPADD.makemandb+= 	${MDOCMLLIB} ${LIBARCHIVE} ${LIBBZ2} ${LIBLZMA} ${LIBZ}
LDADD.makemandb+= 	-L${MDOCMLOBJDIR} -lmandoc -larchive -lbz2 -llzma -lz
LDADD.makemandb+=	-lcrypto
DPADD.makemandb+=	${LIBCRYPTO}

DPADD+=		${LIBSQLITE3} ${LIBM} ${LIBZ} ${LIBTERMLIB} ${LIBUTIL}
LDADD+=		-lsqlite3 -lm -lz -ltermlib -lutil

stopwords.c: stopwords.txt
	( set -e; ${TOOL_NBPERF} -n stopwords_hash -s -p ${.ALLSRC};	\
	echo 'static const char *stopwords[] = {';			\
	${TOOL_SED} -e 's|^\(.*\)$$|	"\1",|' ${.ALLSRC};		\
	echo '};'							\
	) > ${.TARGET}

nostem.c: nostem.txt
	( set -e; ${TOOL_NBPERF} -n nostem_hash -s -p ${.ALLSRC};	\
	echo 'static const char *nostem[] = {';			\
	${TOOL_SED} -e 's|^\(.*\)$$|	"\1",|' ${.ALLSRC};		\
	echo '};'							\
	) > ${.TARGET}

DPSRCS+=	stopwords.c nostem.c
CLEANFILES+=	stopwords.c nostem.c

.include <bsd.prog.mk>
