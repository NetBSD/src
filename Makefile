#	$NetBSD: Makefile,v 1.36 1996/08/10 23:29:00 explorer Exp $

.include <bsd.own.mk>			# for configuration variables.

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share

.if exists(games)
SUBDIR+= games
.endif

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
	@(cd ${.CURDIR}/regress && ${MAKE} regress)
.endif

beforeinstall:
.ifndef DESTDIR
	(cd ${.CURDIR}/etc && ${MAKE} DESTDIR=/ distrib-dirs)
.else
	(cd ${.CURDIR}/etc && ${MAKE} distrib-dirs)
.endif

afterinstall:
.ifndef NOMAN
	(cd ${.CURDIR}/share/man && ${MAKE} makedb)
.endif

build:
	(cd ${.CURDIR}/share/mk && ${MAKE} install)
	${MAKE} includes
	${MAKE} cleandir
	(cd ${.CURDIR}/lib && ${MAKE} depend && ${MAKE} && ${MAKE} install)
	(cd ${.CURDIR}/gnu/lib && ${MAKE} depend && ${MAKE} && ${MAKE} install)
.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
	# XXX should do the whole of domestic/lib
	(cd ${.CURDIR}/domestic/lib/libcrypt && ${MAKE} depend && ${MAKE} && \
	    ${MAKE} install)
.endif
	${MAKE} depend && ${MAKE} && ${MAKE} install

.include <bsd.subdir.mk>
