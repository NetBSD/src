#	$NetBSD: Makefile,v 1.58 1998/07/24 16:48:47 tv Exp $

.include <bsd.own.mk>			# for configuration variables.

HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share sys

.if exists(games)
SUBDIR+= games
.endif

SUBDIR+= gnu
# This is needed for libstdc++ and gen-params.
includes-gnu: includes-include includes-sys

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
.ifmake build
	@echo -n "Build started at: "
	@date
.endif
.ifndef DESTDIR
	(cd ${.CURDIR}/etc && ${MAKE} DESTDIR=/ distrib-dirs)
.else
	(cd ${.CURDIR}/etc && ${MAKE} distrib-dirs)
.endif

afterinstall:
.ifndef NOMAN
	(cd ${.CURDIR}/share/man && ${MAKE} makedb)
.endif

build: beforeinstall
	(cd ${.CURDIR}/share/mk && ${MAKE} install)
	(cd ${.CURDIR}/share/tmac && ${MAKE} && ${MAKE} install)
.if !defined(UPDATE)
	${MAKE} cleandir
.endif
	${MAKE} includes
	(cd ${.CURDIR}/lib/csu && ${MAKE} depend && ${MAKE} && ${MAKE} install)
	(cd ${.CURDIR}/lib && ${MAKE} depend && ${MAKE} && ${MAKE} install)
	(cd ${.CURDIR}/gnu/lib && ${MAKE} depend && ${MAKE} && ${MAKE} install)
	(cd ${.CURDIR}/gnu/usr.bin/gcc/libgcc && ${MAKE} depend && ${MAKE} && ${MAKE} install)
.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
# libtelnet depends on libdes and libkrb.  libkrb depends on
# libcom_err.
.if exists(domestic/lib/libdes)
	(cd ${.CURDIR}/domestic/lib/libdes && \
	    ${MAKE} depend && ${MAKE} && ${MAKE} install)
.endif
.if exists(domestic/lib/libcom_err)
	(cd ${.CURDIR}/domestic/lib/libcom_err && \
	    ${MAKE} depend && ${MAKE} && ${MAKE} install)
.endif
.if exists(domestic/lib/libkrb)
	(cd ${.CURDIR}/domestic/lib/libkrb && \
	    ${MAKE} depend && ${MAKE} && ${MAKE} install)
.endif
	(cd ${.CURDIR}/domestic/lib/ && ${MAKE} depend && ${MAKE} && \
	    ${MAKE} install)
.endif
	${MAKE} depend && ${MAKE} && ${MAKE} install
.if defined(USE_EGCS)
.if defined(DESTDIR)
.if (${HAVE_GCC28} == "")
	@echo '***** WARNING ***** Your system compiler is not GCC 2.8 or higher,'
	@echo 'and you have built a distribution with GCC 2.8 and DESTDIR set.'
	@echo 'You will need to rebuild libgcc from gnu/usr.bin/egcs/libgcc'
	@echo 'in order to have full C++ support in the binary set.'
.endif # HAVE_GCC28
.else
	(cd ${.CURDIR}/gnu/usr.bin/egcs/libgcc &&\
	    ${MAKE} depend && ${MAKE} && ${MAKE} install)
.endif # DESTDIR
.endif # USE_EGCS
	@echo -n "Build finished at: "
	@date

.include <bsd.subdir.mk>
