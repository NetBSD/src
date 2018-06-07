SUBDIRS=	src hooks

VERSION!=	sed -n 's/\#define VERSION[[:space:]]*"\(.*\)".*/\1/p' src/defs.h

DIST!=		if test -f .fslckout; then echo "dist-fossil"; \
		elif test -d .git; then echo "dist-git"; \
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
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

depend: config.h
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

tests:
	cd $@; ${MAKE} $@

test: tests

hooks:
	cd $@; ${MAKE}

eginstall:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

install:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

proginstall:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

clean:
	rm -rf cov-int dhcpcd.xz
	for x in ${SUBDIRS} tests; do cd $$x; ${MAKE} $@; cd ..; done

distclean: clean
	rm -f config.h config.mk config.log \
		${DISTFILE} ${DISTFILEGZ} ${DISTINFO} ${DISTINFOSIGN}


dist-fossil:
	fossil tarball --name ${DISTPREFIX} ${FOSSILID} ${DISTFILEGZ}
	gunzip -c ${DISTFILEGZ} | xz >${DISTFILE}
	rm ${DISTFILEGZ}

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

import: dist
	rm -rf /tmp/${DISTPREFIX}
	${INSTALL} -d /tmp/${DISTPREFIX}
	tar xvJpf ${DISTFILE} -C /tmp

include Makefile.inc
