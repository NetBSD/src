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
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

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
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

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
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

distclean: clean
	rm -f .depend

include ${TOP}/Makefile.inc
