#	$NetBSD: Makefile,v 1.101 1999/02/13 02:54:19 lukem Exp $
#	from: @(#)Makefile	8.7 (Berkeley) 5/25/95

# Environment variables without default values:
#   DESTDIR must be set before anything in this file will work.
#   RELEASEDIR is where the tarred up stuff for a snapshot or
#	release will be placed.
#
# Environment variables with default values:
#   LOCALTIME will set the default local time for the system you
#	build; it determines what /etc/localtime is symlink'd to.
#   KERNSRCDIR points to kernel source; it is set by default to ../sys,
#	but can be overridden.
#   KERNOBJDIR is the kernel build directory, it defaults to
#	${KERNSRCDIR}/arch/${MACHINE}/compile/KERNELNAME, but can be
#	overridden.
#   KERNCONFDIR is where the configuration files for kernels are found;
#	default is ${KERNSRCDIR}/arch/${MACHINE}/conf but can be overridden.
#   UPDATE is normally undefined; if defined, don't do a 'make clean'
#	before kernel compile
#   NO_SENDMAIL is normally undefined; if defined, it will not do a
#	`make distribution' in the sendmail config file source directory.
#   USE_BIND4 is normally unset; if set, it will install a named.boot
#	instead of a named.conf in etc/namedb.
#   EXTRA_KERNELS has a machine-dependent list of kernels to build added
#	to it, but you may also set this to have extra ones built.
# 
# Targets:
#    distribution: makes a full NetBSD distribution in DESTDIR. If
#	INSTALL_DONE is set, it will not do a `make install.'
#    distrib-dirs: creates an empty NetBSD directory tree in DESTDIR.
#	Called by distribution.
#    snapshot: calls distribution, above, and then tars up the files
#	into a release(7) format in RELEASEDIR. Any port-dependent
#	stuff for this target is found in etc.${MACHINE}/Makefile.inc.

# XXX: For NO_SENDMAIL and USE_BIND4
.include <bsd.own.mk>

TZDIR=		/usr/share/zoneinfo
LOCALTIME?=	US/Pacific

# setting NOOBJ prevents "make obj" from doing anything;
# an objdir would break the installation stuff below
MKOBJ=	no

# MD Makefile.inc may append MD targets to BIN[123].  Make sure all
# are empty, to preserve the old semantics of setting them below with "=".
BIN1=	
BIN2=
BIN3=

.if exists(etc.${MACHINE}/Makefile.inc)
.include "etc.${MACHINE}/Makefile.inc"
.endif

# Use multiple jobs for kernel builds, if NBUILDJOBS set.
# (Taken from src/Makefile.)
.if defined(NBUILDJOBS)
_J= -j${NBUILDJOBS}
.endif

# -rw-r--r--
BINOWN= root
BINGRP= wheel
BIN1+=	aliases bootptab changelist csh.cshrc csh.login \
	csh.logout daily daily.conf dm.conf floppytab ftpchroot \
	ftpusers ftpwelcome gettytab group hosts hosts.lpd \
	inetd.conf ld.so.conf lkm.conf mailer.conf man.conf \
	monthly monthly.conf mrouted.conf netstart networks \
	newsyslog.conf nsswitch.conf phones printcap profile protocols \
	rbootd.conf rc rc.conf rc.lkm rc.local rc.subr \
	rc.shutdown remote rpc security security.conf services \
	shells syslog.conf weekly weekly.conf etc.${MACHINE}/ttys \
	etc.${MACHINE}/disktab

# -rw-rw-r--
BIN2+=	motd

# -rw-------
BIN3+=	hosts.equiv

NAMEDB=	127 root.cache
.ifdef USE_BIND4
NAMEDB+=	named.boot
.else
NAMEDB+=	named.conf
.endif
PCS=	pcs750.bin

all clean cleandir depend distclean etc includes install lint:

.ifndef DESTDIR
distribution distrib-dirs snapshot:
	@echo setenv DESTDIR before doing that!
	@false
.else
distribution: distrib-dirs
.if !defined(INSTALL_DONE)
	(cd ..; ${MAKE} includes)
	(cd ..; ${MAKE} install)
.endif
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN1} ${DESTDIR}/etc
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 664 ${BIN2} ${DESTDIR}/etc
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 ${BIN3} ${DESTDIR}/etc
	${INSTALL} -c -o root -g wheel -m 600 crontab \
	    ${DESTDIR}/var/cron/tabs/root
	${INSTALL} -c -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	pwd_mkdb -p -d ${DESTDIR}/ ${DESTDIR}/etc/master.passwd
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	${INSTALL} -c -o root -g wheel -m 600 minfree \
	    ${DESTDIR}/var/crash
	(cd root; \
		${INSTALL} -c -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		${INSTALL} -c -o root -g wheel -m 600 dot.klogin \
		    ${DESTDIR}/root/.klogin; \
		${INSTALL} -c -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		${INSTALL} -c -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile; \
		ln ${DESTDIR}/root/.cshrc ${DESTDIR}/.cshrc; \
		ln ${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	(cd mtree; ${MAKE} install)
	(cd namedb; \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 ${NAMEDB} \
		    ${DESTDIR}/etc/namedb)
	/bin/rm -f ${DESTDIR}/etc/localtime
	ln -s ${TZDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
	/bin/rm -f ${DESTDIR}/etc/rmt
	ln -s /usr/sbin/rmt ${DESTDIR}/etc/rmt
	${INSTALL} -c -o ${BINOWN} -g operator -m 664 /dev/null \
		${DESTDIR}/etc/dumpdates
	${INSTALL} -c -o ${BINOWN} -g operator -m 600 /dev/null \
		${DESTDIR}/etc/skeykeys
	${INSTALL} -c -o root -g wheel -m 600 /dev/null \
		${DESTDIR}/var/cron/log
	${INSTALL} -c -o nobody -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/db/locate.database
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/authlog
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/log/lastlog
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 640 /dev/null \
		${DESTDIR}/var/log/lpd-errs
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/maillog
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/messages
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/secure
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/wtmp
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/xferlog
	${INSTALL} -c -o daemon -g staff -m 664 /dev/null \
		${DESTDIR}/var/msgs/bounds
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/run/utmp
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/atc_scores
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/battlestar.log
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/cfscores
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/criblog
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/robots_roll
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/rogue.scores
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/saillog
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/snakerawscores
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/snake.log
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/tetris.scores
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/llog12.0
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/lscore12.0
	${INSTALL} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/playerids
	(cd etc.${MACHINE}; ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    fstab.* ${DESTDIR}/etc)
	(cd ${DESTDIR}/dev; ./MAKEDEV all)
	(cd ../usr.bin/mail; ${MAKE} distribution)
.ifndef NO_SENDMAIL
	(cd ../usr.sbin/sendmail/cf/cf; ${MAKE} distribution)
.endif


distrib-dirs:
	${INSTALL} -d -o root -g wheel -m 755 ${DESTDIR}
	-mtree -def mtree/NetBSD.dist -p ${DESTDIR}/ -u
	cd ${DESTDIR}; rm -f sys; ln -s usr/src/sys sys

.if !defined(RELEASEDIR)
snapshot snap_pre:
	@echo setenv RELEASEDIR before doing that!
	@false
.else
release snapshot: distribution snap_pre snap_md snap_kern 
	sh ../distrib/sets/maketars -s ../distrib/sets \
		-d ${DESTDIR} -t ${RELEASEDIR}/binary/sets
	(cd ${RELEASEDIR}/binary/sets && \
		cksum -o 1 *.tgz >BSDSUM && \
		cksum *.tgz >CKSUM && \
		cksum -m *.tgz >MD5 && \
		cksum -o 2 *.tgz >SYSVSUM )
	(cd ${RELEASEDIR}/binary/kernel && if [ -r *.gz ]; then \
		cksum -o 1 *.gz >BSDSUM && \
		cksum *.gz >CKSUM && \
		cksum -m *.gz >MD5 && \
		cksum -o 2 *.gz >SYSVSUM \
		; fi )

snap_pre:
	/bin/rm -rf ${RELEASEDIR}
	${INSTALL} -d -o root -g wheel -m 755 ${RELEASEDIR}
	${INSTALL} -d -o root -g wheel -m 755 ${RELEASEDIR}/binary
	${INSTALL} -d -o root -g wheel -m 755 ${RELEASEDIR}/binary/sets
	${INSTALL} -d -o root -g wheel -m 755 ${RELEASEDIR}/binary/kernel
	${INSTALL} -d -o root -g wheel -m 755 ${RELEASEDIR}/binary/security
	${INSTALL} -d -o root -g wheel -m 755 ${RELEASEDIR}/installation

# the regular expression does a basename(1) on .CURDIR so that we don't
# have to keep looking up .. at compile time.
KERNSRCDIR?=	${.CURDIR:C/[^\/]+$//}sys
KERNOBJDIR?=	${KERNSRCDIR}/arch/${MACHINE}/compile
KERNCONFDIR?=	${KERNSRCDIR}/arch/${MACHINE}/conf

# This target builds the GENERIC kernel (which must exist for all
# ports) and puts it in binary/sets/kern.tgz, and also builds any
# kernels specified in EXTRA_KERNELS and puts them in
# binary/kernel/netbsd.${KERN}.gz
#
snap_kern:
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/GENERIC -s ${KERNSRCDIR} GENERIC
.ifndef UPDATE
	cd ${KERNOBJDIR}/GENERIC && ${MAKE} clean 
.endif
	cd ${KERNOBJDIR}/GENERIC &&	\
		${MAKE} depend && ${MAKE} ${_J} &&	\
		tar cf - netbsd |\
		gzip -c -9 > ${RELEASEDIR}/binary/sets/kern.tgz
.for kernel in ${EXTRA_KERNELS}
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${kernel} -s ${KERNSRCDIR} ${kernel}
.ifndef UPDATE
	cd ${KERNOBJDIR}/${kernel} && ${MAKE} clean 
.endif
	cd ${KERNOBJDIR}/${kernel} &&	\
		${MAKE} depend && ${MAKE} ${_J} &&	\
		tar cf - netbsd |\
		gzip -c -9 > ${RELEASEDIR}/binary/kernel/netbsd.${kernel}.gz
.endfor # EXTRA_KERNELS
	
.endif # RELEASEDIR check

snap_md:
# nothing here -- look in the machine-dependent Makefile.inc

.endif	# DESTDIR check

.include <bsd.prog.mk>
