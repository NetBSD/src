#	$NetBSD: Makefile,v 1.34 2003/08/01 17:04:05 lukem Exp $

.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/lib -I${NETBSDSRCDIR}/usr.bin/telnet
LDADD+=	-lcurses -lcrypt
DPADD+=	${LIBCURSES} ${LIBCRYPT}

PROG=	tn3270
SRCS=	api.c api_bsd.c api_exch.c apilib.c asc_ebc.c astosc.c commands.c \
	dctype.c disp_asc.c ebc_disp.c function.c genbsubs.c genget.c \
	globals.c inbound.c main.c map3270.c network.c oia.c options.c \
	outbound.c ring.c sys_bsd.c system.c telnet.c termin.c terminal.c \
	termout.c tn3270.c utilities.c
DPSRCS=	asc_disp.out astosc.out disp_asc.out kbd.out

MKASTOSCDIR !=cd $(.CURDIR)/../tools/mkastosc && ${PRINTOBJDIR}
MKASTOSC= ${MKASTOSCDIR}/mkastosc

${MKASTOSC}:
	cd ${.CURDIR}/../tools/mkastosc; ${MAKE}

MKASTODSDIR!=cd $(.CURDIR)/../tools/mkastods && ${PRINTOBJDIR}
MKASTODS= ${MKASTODSDIR}/mkastods

${MKASTODS}:
	cd ${.CURDIR}/../tools/mkastods; ${MAKE}

MKDSTOASDIR!=cd $(.CURDIR)/../tools/mkdstoas && ${PRINTOBJDIR}
MKDSTOAS= ${MKDSTOASDIR}/mkdstoas

${MKDSTOAS}:
	cd ${.CURDIR}/../tools/mkdstoas; ${MAKE}

MKHITSDIR!=cd $(.CURDIR)/../tools/mkhits && ${PRINTOBJDIR}
MKHITS= ${MKHITSDIR}/mkhits

${MKHITS}:
	cd ${.CURDIR}/../tools/mkhits; ${MAKE}

astosc.out: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} \
	    ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    < ${.CURDIR}/../ctlr/${KBD} > astosc.tmp
	mv -f astosc.tmp ${.TARGET}
CLEANFILES+=	astosc.tmp astosc.out

asc_disp.out: ${MKASTODS}
	${MKASTODS} > asc_disp.tmp
	mv -f asc_disp.tmp ${.TARGET}
CLEANFILES+=	asc_disp.tmp asc_disp.out

disp_asc.out: ${MKDSTOAS}
	${MKDSTOAS} > disp_asc.tmp
	mv -f disp_asc.tmp ${.TARGET}
CLEANFILES+=	disp_asc.tmp disp_asc.out

kbd.out: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/${KBD} ${MKHITS}
	${CC} ${CPPFLAGS} -E ${.CURDIR}/../ctlr/function.c > TMPfunc.out
	${MKHITS} \
	    ${.CURDIR}/../ctlr/hostctlr.h TMPfunc.out \
	    < ${.CURDIR}/../ctlr/${KBD} > kbd.tmp
	rm -f TMPFunc.out
	mv -f kbd.tmp ${.TARGET}
CLEANFILES+=	TMPfunc.out kbd.tmp kbd.out

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii ${.CURDIR}/../ctlr
.PATH: ${.CURDIR}/../general ${.CURDIR}/../sys_curses ${.CURDIR}/../../telnet
.PATH: ${NETBSDSRCDIR}/lib/libtelnet

astosc.o:	astosc.out
disp_asc.o:	asc_disp.out disp_asc.out
inbound.o:	kbd.out
