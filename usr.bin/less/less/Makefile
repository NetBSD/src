# Generated automatically from Makefile.in by configure.
# Makefile for less.

#### Start of system configuration section. ####

srcdir = .

CC = gcc
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CFLAGS = -g -O
CFLAGS_COMPILE_ONLY = -c
LDFLAGS = 
O=o

LIBS =   -lcurses -ltermcap

prefix = /usr
exec_prefix = ${prefix}

# Where the installed binary goes.
bindir = ${exec_prefix}/bin
binprefix = 

mandir = ${prefix}/man/man${manext}
manext = 1
manprefix = 

#### End of system configuration section. ####

SHELL = /bin/sh

# This rule allows us to supply the necessary -D options
# in addition to whatever the user asks for.
.c.o:
	${CC} -I. ${CFLAGS_COMPILE_ONLY} ${CPPFLAGS} ${CFLAGS} $<

OBJ =	main.${O} screen.${O} brac.${O} ch.${O} charset.${O} cmdbuf.${O} \
	command.${O} decode.${O} edit.${O} filename.${O} forwback.${O} \
	help.${O} ifile.${O} input.${O} jump.${O} line.${O} linenum.${O} \
	lsystem.${O} mark.${O} optfunc.${O} option.${O} opttbl.${O} os.${O} \
	output.${O} position.${O} prompt.${O} search.${O} signal.${O} \
	tags.${O} ttyin.${O} version.${O}  

all: less lesskey

less: ${OBJ}
	${CC} ${LDFLAGS} -o $@ ${OBJ} ${LIBS}

lesskey: lesskey.${O} version.${O}
	${CC} ${LDFLAGS} -o $@ lesskey.${O} version.${O}

${OBJ}: ${srcdir}/less.h ${srcdir}/funcs.h defines.h 

install: all ${srcdir}/less.hlp ${srcdir}/less.nro ${srcdir}/lesskey.nro installdirs
	${INSTALL_PROGRAM} less ${bindir}/${binprefix}less
	${INSTALL_PROGRAM} lesskey ${bindir}/${binprefix}lesskey
	${INSTALL_DATA} ${srcdir}/less.nro ${mandir}/${manprefix}less.${manext}
	${INSTALL_DATA} ${srcdir}/lesskey.nro ${mandir}/${manprefix}lesskey.${manext}

installdirs: mkinstalldirs
	${srcdir}/mkinstalldirs ${bindir} ${mandir}

uninstall:
	rm -f ${bindir}/${binprefix}less ${bindir}/${binprefix}lesskey
	rm -f ${mandir}/less.${manext} ${mandir}/lesskey.${manext}

info:
install-info:
dvi:
check:
installcheck:

TAGS:
	cd ${srcdir} && etags *.c *.h

# config.status might not change defines.h
# Don't rerun config.status if we just configured (so there's no stamp-h).
defines.h: stamp-h
stamp-h: defines.h.in config.status
	test ! -f stamp-h || CONFIG_FILES= CONFIG_HEADERS=defines.h ./config.status
	touch stamp-h
Makefile: ${srcdir}/Makefile.in config.status
	CONFIG_FILES=Makefile CONFIG_HEADERS= ./config.status
config.status: ${srcdir}/configure
	./config.status --recheck

${srcdir}/configure: ${srcdir}/configure.in
	cd ${srcdir}; autoheader; autoconf

clean:
	rm -f *.${O} core less lesskey

mostlyclean: clean

distclean: clean
	rm -f Makefile config.status config.log config.cache defines.h stamp-h

realclean: distclean
	rm -f TAGS

