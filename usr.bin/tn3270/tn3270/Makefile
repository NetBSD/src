#	$NetBSD: Makefile,v 1.24 1997/05/09 13:32:12 mycroft Exp $

.include "../../Makefile.inc"

CFLAGS += -I${.CURDIR} -I.
LDADD += -lcurses -ltermcap -lcrypt
DPADD += ${LIBCURSES} ${LIBTERMCAP} ${LIBCRYPT}

SRCS=	api.c api_bsd.c api_exch.c apilib.c asc_ebc.c astosc.c commands.c \
	dctype.c disp_asc.c ebc_disp.c function.c genbsubs.c genget.c \
	globals.c inbound.c main.c map3270.c network.c oia.c options.c \
	outbound.c ring.c sys_bsd.c system.c telnet.c termin.c terminal.c \
	termout.c tn3270.c utilities.c
DPSRCS=	asc_disp.OUT astosc.OUT disp_asc.OUT kbd.OUT

PROG = tn3270

MKASTOSC!=cd $(.CURDIR)/../tools/mkastosc; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkastosc\n" | ${MAKE} -s -f-

MKASTODS!=cd $(.CURDIR)/../tools/mkastods; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkastods\n" | ${MAKE} -s -f-

MKDSTOAS!=cd $(.CURDIR)/../tools/mkdstoas; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkdstoas\n" | ${MAKE} -s -f-

MKHITS!=cd $(.CURDIR)/../tools/mkhits; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/mkhits\n" | ${MAKE} -s -f-

CLEANFILES += astosc.OUT astosc.out
astosc.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD} ${MKASTOSC}
	${MKASTOSC} ${.CURDIR}/../ctlr/hostctlr.h \
		${.CURDIR}/../ctlr/function.h < ${.CURDIR}/../ctlr/${KBD} \
		> ${.TARGET}
	/bin/rm -f astosc.out
	ln -s astosc.OUT astosc.out

CLEANFILES += asc_disp.OUT asc_disp.out disp_asc.OUT disp_asc.out
asc_disp.OUT: ${MKASTODS}
	${MKASTODS} > ${.TARGET}
	/bin/rm -f asc_disp.out
	ln -s asc_disp.OUT asc_disp.out
disp_asc.OUT: ${MKDSTOAS}
	${MKDSTOAS} > ${.TARGET}
	/bin/rm -f disp_asc.out
	ln -s disp_asc.OUT disp_asc.out

CLEANFILES += TMPfunc.out kbd.OUT kbd.out
kbd.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/${KBD} \
	 ${MKHITS}
	${CC} ${CFLAGS} -E ${.CURDIR}/../ctlr/function.c > TMPfunc.out
	${MKHITS} ${.CURDIR}/../ctlr/hostctlr.h \
		TMPfunc.out < ${.CURDIR}/../ctlr/${KBD} > ${.TARGET}
	/bin/rm -f kbd.out
	ln -s kbd.OUT kbd.out

${MKASTOSC}:
	cd ${.CURDIR}/../tools/mkastosc; ${MAKE}
${MKASTODS}:
	cd ${.CURDIR}/../tools/mkastods; ${MAKE}
${MKDSTOAS}:
	cd ${.CURDIR}/../tools/mkdstoas; ${MAKE}
${MKHITS}:
	cd ${.CURDIR}/../tools/mkhits; ${MAKE}

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii ${.CURDIR}/../ctlr
.PATH: ${.CURDIR}/../general ${.CURDIR}/../sys_curses ${.CURDIR}/../../telnet
.PATH: ${.CURDIR}/../../../lib/libtelnet
