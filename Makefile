#	$Id: Makefile,v 1.20 1994/12/22 12:43:45 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnu

SUBDIR+= sys

.if exists(regress)
.ifmake !(install)
SUBDIR+= regress
.endif

regression-tests:
	@echo Running regression tests...
	@(cd regress && ${MAKE} regress)
.endif

.include <bsd.own.mk>	# for NOMAN, if it's there.

afterinstall:
.ifndef NOMAN
	(cd share/man && ${MAKE} makedb)
.endif

build:
	(cd include && ${MAKE} install)
	${MAKE} cleandir
	(cd lib && ${MAKE} depend && ${MAKE} && ${MAKE} install)
	(cd gnu/lib && ${MAKE} depend && ${MAKE} && ${MAKE} install)
.if exists(kerberosIV)
	(cd kerberosIV && ${MAKE} depend && ${MAKE} && ${MAKE} install)
.endif
	${MAKE} depend && ${MAKE} && ${MAKE} install

.include <bsd.subdir.mk>
