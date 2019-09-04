SUBDIRS=	src hooks

VERSION!=	sed -n 's/\#define VERSION[[:space:]]*"\(.*\)".*/\1/p' src/defs.h

DIST!=		if test -d .git; then echo "dist-git"; \
		else echo "dist-inst"; fi
FOSSILID?=	current
GITREF?=	HEAD

DISTSUFFIX=
DISTPREFIX?=	dhcpcd-${VERSION}${DISTSUFFIX}
DISTFILEGZ?=	${DISTPREFIX}.tar.gz
DISTFILE?=	${DISTPREFIX}.tar.xz
DISTINFO=	${DISTFILE}.distinfo
DISTINFOSIGN=	${DISTINFO}.asc

CLEANFILES+=	*.tar.xz

.PHONY:		hooks import import-bsd tests

.SUFFIXES:	.in

all: config.h
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

depend: config.h
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

tests:
	cd $@; ${MAKE} $@

test: tests

hooks:
	cd $@; ${MAKE}

eginstall:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

install:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

proginstall:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

clean:
	rm -rf cov-int dhcpcd.xz
	for x in ${SUBDIRS} tests; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

distclean: clean
	rm -f config.h config.mk config.log \
		${DISTFILE} ${DISTFILEGZ} ${DISTINFO} ${DISTINFOSIGN}

dist-git:
	git archive --prefix=${DISTPREFIX}/ ${GITREF} | xz >${DISTFILE}

dist-inst:
	mkdir /tmp/${DISTPREFIX}
	cp -RPp * /tmp/${DISTPREFIX}
	(cd /tmp/${DISTPREFIX}; make clean)
	tar -cvjpf ${DISTFILE} -C /tmp ${DISTPREFIX}
	rm -rf /tmp/${DISTPREFIX}

dist: ${DIST}

distinfo: dist
	rm -f ${DISTINFO} ${DISTINFOSIGN}
	${CKSUM} ${DISTFILE} >${DISTINFO}
	#printf "SIZE (${DISTFILE}) = %s\n" $$(wc -c <${DISTFILE}) >>${DISTINFO}
	${PGP} --clearsign --output=${DISTINFOSIGN} ${DISTINFO}
	chmod 644 ${DISTINFOSIGN}
	ls -l ${DISTFILE} ${DISTINFO} ${DISTINFOSIGN}

snapshot:
	rm -rf /tmp/${DISTPREFIX}
	${INSTALL} -d /tmp/${DISTPREFIX}
	cp -RPp * /tmp/${DISTPREFIX}
	${MAKE} -C /tmp/${DISTPREFIX} distclean
	tar cf - -C /tmp ${DISTPREFIX} | xz >${DISTFILE}
	ls -l ${DISTFILE}

_import: dist
	rm -rf ${DESTDIR}/*
	${INSTALL} -d ${DESTDIR}
	tar xvpf ${DISTFILE} -C ${DESTDIR} --strip 1
	@${ECHO}
	@${ECHO} "============================================================="
	@${ECHO} "dhcpcd-${VERSION} imported to ${DESTDIR}"

import:
	${MAKE} _import DESTDIR=`if [ -n "${DESTDIR}" ]; then echo "${DESTDIR}"; else  echo /tmp/${DISTPREFIX}; fi`


_import-src:
	rm -rf ${DESTDIR}/*
	${INSTALL} -d ${DESTDIR}
	cp LICENSE README.md ${DESTDIR};
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} DESTDIR=${DESTDIR} $@ || exit $$?; cd ..; done
	@${ECHO}
	@${ECHO} "============================================================="
	@${ECHO} "dhcpcd-${VERSION} imported to ${DESTDIR}"

import-src:
	${MAKE} _import-src DESTDIR=`if [ -n "${DESTDIR}" ]; then echo "${DESTDIR}"; else  echo /tmp/${DISTPREFIX}; fi`

include Makefile.inc
