.include <../../Makefile.inc>

CFLAGS += -I${.CURDIR} -I.
LDADD += -lcrypt -lcurses -ltermcap -ltelnet
DPADD += ${LIBCRYPT} ${LIBCURSES} /usr/lib/libtermcap.a /usr/lib/libtelnet.a

MAN1 = tn3270.0

SRCS += apilib.c api_bsd.c api_exch.c asc_ebc.c astosc.c dctype.c
SRCS += disp_asc.c ebc_disp.c
SRCS += map3270.c termin.c
SRCS += api.c function.c inbound.c oia.c options.c outbound.c
SRCS += genbsubs.c globals.c system.c termout.c
SRCS += commands.c main.c network.c ring.c sys_bsd.c telnet.c terminal.c
SRCS += tn3270.c utilities.c

PROG = tn3270

astosc.o: astosc.OUT
CLEANFILES += astosc.OUT astosc.out
astosc.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/function.h \
	    ${.CURDIR}/../ctlr/${KBD}
	${.CURDIR}/../tools/mkastosc/obj/mkastosc ${.CURDIR}/../ctlr/hostctlr.h \
		${.CURDIR}/../ctlr/function.h < ${.CURDIR}/../ctlr/${KBD} \
		> ${.TARGET}
	ln -s astosc.OUT astosc.out

disp_asc.o: asc_disp.OUT disp_asc.OUT
CLEANFILES += asc_disp.OUT asc_disp.out disp_asc.OUT disp_asc.out
asc_disp.OUT:
	${.CURDIR}/../tools/mkastods/obj/mkastods > ${.TARGET}
	ln -s asc_disp.OUT asc_disp.out
disp_asc.OUT:
	${.CURDIR}/../tools/mkdstoas/obj/mkdstoas > ${.TARGET}
	ln -s disp_asc.OUT disp_asc.out

inbound.o: kbd.OUT
CLEANFILES += kbd.OUT kbd.out
kbd.OUT: ${.CURDIR}/../ctlr/hostctlr.h ${.CURDIR}/../ctlr/${KBD}
	${CC} ${CFLAGS} -E ${.CURDIR}/../ctlr/function.c > TMPfunc.out
	${.CURDIR}/../tools/mkhits/obj/mkhits ${.CURDIR}/../ctlr/hostctlr.h \
		TMPfunc.out < ${.CURDIR}/../ctlr/${KBD} > ${.TARGET}
	ln -s kbd.OUT kbd.out

# astosc.out
# asc_disp.out disp_asc.out
# default.map
# kbd.out

.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../api ${.CURDIR}/../ascii ${.CURDIR}/../ctlr
.PATH: ${.CURDIR}/../general ${.CURDIR}/../sys_curses ${.CURDIR}/../../telnet
