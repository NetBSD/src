#	$NetBSD: Makefile,v 1.25 1997/05/09 15:04:06 mycroft Exp $

.include "../../Makefile.inc"

CFLAGS+=-I${.CURDIR} -I.
LDADD+=	-lcurses -ltermcap -lcrypt
DPADD+=	${LIBCURSES} ${LIBTERMCAP} ${LIBCRYPT}

PROG=	tn3270
SRCS=	api.c api_bsd.c api_exch.c apilib.c asc_ebc.c astosc.c commands.c \
	dctype.c disp_asc.c ebc_disp.c function.c genbsubs.c genget.c \
	globals.c inbound.c main.c map3270.c network.c oia.c options.c \
	outbound.c ring.c sys_bsd.c system.c telnet.c termin.c terminal.c \
	termout.c tn3270.c utilities.c
DPSRCS=	asc_disp.out astosc.out disp_asc.out kbd.out

MKASTOSC!=cd $(.CURDIR)/../tools/mkastosc; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkastosc\n" | ${MAKE} -s -f-
${MKASTOSC}:
	cd ${.CURDIR}/../tools/mkastosc; ${MAKE}

MKASTODS!=cd $(.CURDIR)/../tools/mkastods; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkastods\n" | ${MAKE} -s -f-
${MKASTODS}:
	cd ${.CURDIR}/../tools/mkastods; ${MAKE}

MKDSTOAS!=cd $(.CURDIR)/../tools/mkdstoas; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkdstoas\n" | ${MAKE} -s -f-
${MKDSTOAS}:
	cd ${.CURDIR}/../tools/mkdstoas; ${MAKE}

MKHITS!=cd $(.CURDIR)/../tools/mkhits; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkhits\n" | ${MAKE} -s -f-
${MKHITS}:
	cd ${.CURDIR}/../tools/mkhits; ${MAKE}

astosc.out: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} \
	    ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    < ${.CURDIR}/../ctlr/${KBD} > astosc.tmp
	mv -f astosc.tmp ${.TARGET}
CLEANFILES+=	astosc.tmp

asc_disp.out: ${MKASTODS}
	${MKASTODS} > asc_disp.tmp
	mv -f asc_disp.tmp ${.TARGET}
CLEANFILES+=	asc_disp.tmp

disp_asc.out: ${MKDSTOAS}
	${MKDSTOAS} > disp_asc.tmp
	mv -f disp_asc.tmp ${.TARGET}
CLEANFILES+=	disp_asc.tmp

kbd.out: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/${KBD} ${MKHITS}
	${CC} ${CFLAGS} -E ${.CURDIR}/../ctlr/function.c > TMPfunc.out
	${MKHITS} \
	    ${.CURDIR}/../ctlr/hostctlr.h TMPfunc.out \
	    < ${.CURDIR}/../ctlr/${KBD} > kbd.tmp
	rm -f TMPFunc.out
	mv -f kbd.tmp ${.TARGET}
CLEANFILES+=	TMPfunc.out kbd.tmp

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii ${.CURDIR}/../ctlr
.PATH: ${.CURDIR}/../general ${.CURDIR}/../sys_curses ${.CURDIR}/../../telnet
.PATH: ${.CURDIR}/../../../lib/libtelnet
