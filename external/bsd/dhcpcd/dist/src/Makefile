# dhcpcd Makefile

PROG=		dhcpcd
SRCS=		common.c control.c dhcpcd.c duid.c eloop.c logerr.c
SRCS+=		if.c if-options.c sa.c route.c
SRCS+=		dhcp-common.c script.c

CFLAGS?=	-O2
SUBDIRS+=	${MKDIRS}

TOP?=		..
include ${TOP}/iconfig.mk

CSTD?=		c99
CFLAGS+=	-std=${CSTD}
CPPFLAGS+=	-I${TOP} -I${TOP}/src -I./crypt

SRCS+=		${DHCPCD_SRCS}
DHCPCD_DEFS?=	dhcpcd-definitions.conf

PCOMPAT_SRCS=	${COMPAT_SRCS:compat/%=${TOP}/compat/%}
PCRYPT_SRCS=	${CRYPT_SRCS:compat/%=${TOP}/compat/%}
OBJS+=		${SRCS:.c=.o} ${PCRYPT_SRCS:.c=.o} ${PCOMPAT_SRCS:.c=.o}

MAN5=		dhcpcd.conf.5
MAN8=		dhcpcd.8
CLEANFILES=	dhcpcd.conf.5 dhcpcd.8

FILES=		dhcpcd.conf
FILESDIR=	${SYSCONFDIR}

DEPEND!=	test -e .depend && echo ".depend" || echo ""

CLEANFILES+=	*.tar.xz

.PHONY:		import import-bsd dev test

.SUFFIXES:	.in

.in: Makefile ${TOP}/config.mk
	${SED} ${SED_RUNDIR} ${SED_DBDIR} ${SED_LIBDIR} ${SED_HOOKDIR} \
		${SED_SYS} ${SED_SCRIPT} ${SED_DATADIR} \
		${SED_SERVICEEXISTS} ${SED_SERVICECMD} ${SED_SERVICESTATUS} \
		${SED_STATUSARG} \
		$< > $@

all: ${TOP}/config.h ${PROG} ${SCRIPTS} ${MAN5} ${MAN8}
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

dev:
	cd dev && ${MAKE}

.c.o: Makefile ${TOP}/config.mk
	${CC} ${CFLAGS} ${CPPFLAGS} -c $< -o $@

CLEANFILES+=	dhcpcd-embedded.h dhcpcd-embedded.c

dhcpcd-embedded.h: genembedh ${DHCPCD_DEFS} dhcpcd-embedded.h.in
	${HOST_SH} ${.ALLSRC} $^ > $@

dhcpcd-embedded.c: genembedc ${DHCPCD_DEFS} dhcpcd-embedded.c.in
	${HOST_SH} ${.ALLSRC} $^ > $@

if-options.c: dhcpcd-embedded.h

.depend: ${SRCS} ${COMPAT_SRCS} ${CRYPT_SRCS}
	${CC} ${CPPFLAGS} -MM ${SRCS} ${COMPAT_SRCS} ${CRYPT_SRCS} > .depend

depend: .depend

${PROG}: ${DEPEND} ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

lint:
	${LINT} -Suz ${CPPFLAGS} ${SRCS} ${PCRYPT_SRCS} ${PCOMPAT_SRCS}

_embeddedinstall: ${DHCPCD_DEFS}
	${INSTALL} -d ${DESTDIR}${LIBEXECDIR}
	${INSTALL} -m ${CONFMODE} ${DHCPCD_DEFS} ${DESTDIR}${LIBEXECDIR}

_proginstall: ${PROG}
	${INSTALL} -d ${DESTDIR}${SBINDIR}
	${INSTALL} -m ${BINMODE} ${PROG} ${DESTDIR}${SBINDIR}
	${INSTALL} -d ${DESTDIR}${DBDIR}

proginstall: _proginstall ${EMBEDDEDINSTALL}
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

_maninstall: ${MAN5} ${MAN8}
	${INSTALL} -d ${DESTDIR}${MANDIR}/man5
	${INSTALL} -m ${MANMODE} ${MAN5} ${DESTDIR}${MANDIR}/man5
	${INSTALL} -d ${DESTDIR}${MANDIR}/man8
	${INSTALL} -m ${MANMODE} ${MAN8} ${DESTDIR}${MANDIR}/man8

_confinstall:
	${INSTALL} -d ${DESTDIR}${SYSCONFDIR}
	# Install a new default config if not present
	test -e ${DESTDIR}${SYSCONFDIR}/dhcpcd.conf || \
		${INSTALL} -m ${CONFMODE} dhcpcd.conf ${DESTDIR}${SYSCONFDIR}
	# Attempt to move files from sysconfig to dbdir
	if [ ! -e ${DESTDIR}${DBDIR}/duid -a \
	    -e ${DESTDIR}${SYSCONFDIR}/dhcpcd.duid ]; \
	then \
		mv ${DESTDIR}${SYSCONFDIR}/dhcpcd.duid \
			${DESTDIR}${DBDIR}/duid; \
	fi
	if [ ! -e ${DESTDIR}${DBDIR}/secret -a \
	    -e ${DESTDIR}${SYSCONFDIR}/dhcpcd.secret ]; \
	then \
		mv ${DESTDIR}${SYSCONFDIR}/dhcpcd.secret \
			${DESTDIR}${DBDIR}/secret; \
	fi
	# Move leases to new location
	for lease in ${DESTDIR}${DBDIR}/../dhcpcd-*.lease*; do \
		[ -f "$$lease" ] || continue; \
		newlease=$$(basename "$$lease" | ${SED} -e "s/dhcpcd-//"); \
		mv "$$lease" ${DESTDIR}${DBDIR}/"$$newlease"; \
	done
	# Move RDM monotonic to new location
	if [ ! -e ${DESTDIR}${DBDIR}/rdm_monotonic -a \
	    -e ${DESTDIR}${DBDIR}/../dhcpcd-rdm.monotonic ]; \
	then \
		mv ${DESTDIR}${DBDIR}/../dhcpcd-rdm.monotonic \
			${DESTDIR}${DBDIR}/rdm_monotonic; \
	fi
	
eginstall:

install: proginstall _maninstall _confinstall eginstall

clean:
	rm -f ${OBJS} ${PROG} ${PROG}.core ${CLEANFILES}
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@ || exit $$?; cd ..; done

distclean: clean
	rm -f .depend

_import-src: ${SRCS}
	${INSTALL} -d ${DESTDIR}/src
	touch if-bsd.h if-linux.h if-sun.h
	cp defs.h ${SRCS} ${SRCS:.c=.h} dev.h ${MAN5}.in ${MAN8}.in ${DESTDIR}/src
	rm if-bsd.h if-linux.h if-sun.h
	rm -f ${DESTDIR}/src/if-bsd.h ${DESTDIR}/src/if-linux.h ${DESTDIR}/src/if-sun.h
	if [ -n "${COMPAT_SRCS}" ]; then \
		${INSTALL} -d ${DESTDIR}/compat; \
		cd ..; \
		touch compat/rb.h compat/strtou.h; \
		cp ${COMPAT_SRCS} ${COMPAT_SRCS:.c=.h} ${DESTDIR}/compat; \
		rm compat/rb.h compat/strtou.h; \
		rm -f ${DESTDIR}/compat/rb.h ${DESTDIR}/compat/strtou.h; \
	fi
	if ! grep HAVE_SYS_BITOPS_H ../config.h; then \
		cp ../compat/bitops.h ${DESTDIR}/compat; \
	fi
	if grep compat/consttime_memequal.h ../config.h; then \
		cp ../compat/consttime_memequal.h ${DESTDIR}/compat; \
	fi
	if [ -e ${DESTDIR}/compat/rb.c ]; then \
		cp ../compat/rbtree.h ${DESTDIR}/compat; \
	fi
	if [ -e ${DESTDIR}/compat/strtoi.c ]; then \
		cp ../compat/_strtoi.h ${DESTDIR}/compat; \
	fi
	if [ -n "${CRYPT_SRCS}" ]; then \
		${INSTALL} -d ${DESTDIR}/compat/crypt; \
		cd ..; \
		cp ${CRYPT_SRCS} ${CRYPT_SRCS:.c=.h} ${DESTDIR}/compat/crypt; \
	fi
	# DragonFlyBSD builds base version with private crypto
	if [ `uname` = DragonFly ]; then rm ${DESTDIR}/compat/crypt/md5* ${DESTDIR}/compat/crypt/sha256*; fi

include ${TOP}/Makefile.inc
