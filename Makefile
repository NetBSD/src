#	$Id: Makefile,v 1.21 1995/02/19 12:20:06 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnu

SUBDIR+= sys

.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
SUBDIR+= domestic
.endif

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
.if exists(domestic)
	(cd domestic/libcrypt && ${MAKE} depend && ${MAKE} && ${MAKE} install)
.endif
.if exists(kerberosIV)
	(cd kerberosIV && ${MAKE} depend && ${MAKE} && ${MAKE} install)
.endif
	${MAKE} depend && ${MAKE} && ${MAKE} install

.include <bsd.subdir.mk>
