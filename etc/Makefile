#	$NetBSD: Makefile,v 1.126 1999/09/26 21:40:13 mjl Exp $
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
#   BUILD_KERNELS are a machine-dependent list of kernels that should just
#	be built in place but not made into sets or installed, e.g., an
#	INSTALL kernel used later in src/distrib/.
# 
# Targets:
#    distribution: makes a full NetBSD distribution in DESTDIR. If
#	INSTALL_DONE is set, it will not do a `make install.'
#    distrib-dirs: creates an empty NetBSD directory tree in DESTDIR.
#	Called by distribution.
#    snapshot: calls distribution, above, and then tars up the files
#	into a release(7) format in RELEASEDIR. Any port-dependent
#	stuff for this target is found in etc.${MACHINE}/Makefile.inc.
#    release: a synonym for `snapshot'

# XXX: For NO_SENDMAIL, USE_BIND4 and INSTPRIV
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

# Directories to build in ${RELEASEDIR}.   MD Makefile.inc files can
# add to this.
# NOTE: Parent directories must be listed before subdirectories.
INSTALLATION_DIRS= \
	binary binary/sets binary/kernel binary/security installation

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
BIN1+=	bootptab changelist csh.cshrc csh.login \
	csh.logout daily daily.conf dm.conf floppytab ftpchroot \
	ftpusers ftpwelcome gettytab group hosts hosts.lpd \
	inetd.conf lkm.conf mailer.conf man.conf \
	monthly monthly.conf mrouted.conf netstart networks \
	newsyslog.conf nsswitch.conf phones printcap profile protocols \
	rbootd.conf rc rc.conf rc.lkm rc.local rc.subr \
	rc.shutdown rc.wscons remote rpc security security.conf services \
	shells syslog.conf weekly weekly.conf wscons.conf \
	etc.${MACHINE}/ttys etc.${MACHINE}/disktab

.if	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
BIN1+=	ld.so.conf
.endif

# IPv6
BIN1+=	rtadvd.conf

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
distribution distrib-dirs release snapshot:
	@echo setenv DESTDIR before doing that!
	@false
.else
distribution: distrib-dirs
.if !defined(INSTALL_DONE)
	(cd ..; ${MAKE} _DISTRIB= includes)
	(cd ..; ${MAKE} _DISTRIB= install)
.endif
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN1} \
	    ${DESTDIR}/etc
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 664 ${BIN2} \
	    ${DESTDIR}/etc
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 600 ${BIN3} \
	    ${DESTDIR}/etc
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 aliases \
	    ${DESTDIR}/etc
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 crontab \
	    ${DESTDIR}/var/cron/tabs/root
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 master.passwd \
	    ${DESTDIR}/etc
	pwd_mkdb -p -d ${DESTDIR}/ ${DESTDIR}/etc/master.passwd
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 minfree \
	    ${DESTDIR}/var/crash
	(cd root; \
		${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 dot.klogin \
		    ${DESTDIR}/root/.klogin; \
		${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile; \
		ln ${DESTDIR}/root/.cshrc ${DESTDIR}/.cshrc; \
		ln ${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	(cd mtree; ${MAKE} install)
	(cd namedb; \
		${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 \
		    ${NAMEDB} ${DESTDIR}/etc/namedb)
	/bin/rm -f ${DESTDIR}/etc/localtime
	ln -s ${TZDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
	/bin/rm -f ${DESTDIR}/etc/rmt
	ln -s /usr/sbin/rmt ${DESTDIR}/etc/rmt
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g operator -m 664 /dev/null \
		${DESTDIR}/etc/dumpdates
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g operator -m 600 /dev/null \
		${DESTDIR}/etc/skeykeys
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 /dev/null \
		${DESTDIR}/var/cron/log
	${INSTALL} ${INSTPRIV} -c -o nobody -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/db/locate.database
	${INSTALL} ${INSTPRIV} -c -o uucp -g dialer -m 640 /dev/null \
		${DESTDIR}/var/log/aculog
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/authlog
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/log/lastlog
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 640 /dev/null \
		${DESTDIR}/var/log/lpd-errs
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/maillog
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/messages
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/secure
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/wtmp
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/xferlog
	${INSTALL} ${INSTPRIV} -c -o daemon -g staff -m 664 /dev/null \
		${DESTDIR}/var/msgs/bounds
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/run/utmp
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/atc_scores
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/battlestar.log
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/cfscores
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/criblog
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/robots_roll
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/rogue.scores
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/saillog
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/snakerawscores
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/snake.log
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/tetris.scores
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/llog12.0
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/lscore12.0
	${INSTALL} ${INSTPRIV} -c -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/playerids
	(cd etc.${MACHINE}; ${INSTALL} ${INSTPRIV} -c -o ${BINOWN} \
	    -g ${BINGRP} -m 444 fstab.* ${DESTDIR}/etc)
	(cd ${DESTDIR}/dev; ./MAKEDEV all)
	(cd ../usr.bin/mail; ${MAKE} distribution)
.ifndef NO_SENDMAIL
	(cd ../usr.sbin/sendmail/cf/cf; ${MAKE} distribution)
	${DESTDIR}/usr/libexec/sendmail/sendmail -C ${DESTDIR}/etc/sendmail.cf -O AliasFile=${DESTDIR}/etc/aliases -bi
.endif
.ifdef BUILD_POSTFIX
	(cd ../usr.sbin/postfix/etc; ${MAKE} distribution)
.endif


distrib-dirs:
	${INSTALL} ${INSTPRIV} -d -o root -g wheel -m 755 ${DESTDIR}
	-mtree -def mtree/NetBSD.dist -p ${DESTDIR}/ -u
	cd ${DESTDIR}; rm -f sys; ln -s usr/src/sys sys

.if !defined(RELEASEDIR)
release snapshot snap_pre snap_md_pre snap_md_post:
	@echo setenv RELEASEDIR before doing that!
	@false
.else
release snapshot: distribution snap_pre snap_md_pre snap_kern snap_md_post
	sh ../distrib/sets/maketars -s ../distrib/sets \
		-d ${DESTDIR} -t ${RELEASEDIR}/binary/sets
.if exists(../domestic) && !defined(EXPORTABLE_SYSTEM)
	sh ../distrib/sets/maketars -s ../distrib/sets \
		-d ${DESTDIR} -t ${RELEASEDIR}/binary/security secr
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/binary/security
.endif
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/binary/sets
.ifdef EXTRA_KERNELS
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/binary/kernel '*.gz'
.endif

snap_pre:
	/bin/rm -rf ${RELEASEDIR}
	${INSTALL} ${INSTPRIV} -d -o root -g wheel -m 755 ${RELEASEDIR}
.for dir in ${INSTALLATION_DIRS}
	${INSTALL} ${INSTPRIV} -d -o root -g wheel -m 755 ${RELEASEDIR}/${dir}
.endfor

# the regular expression does a basename(1) on .CURDIR so that we don't
# have to keep looking up .. at compile time.
KERNSRCDIR?=	${.CURDIR:C/[^\/]+$//}sys
KERNOBJDIR?=	${KERNSRCDIR}/arch/${MACHINE}/compile
KERNCONFDIR?=	${KERNSRCDIR}/arch/${MACHINE}/conf

# This target builds the GENERIC kernel (which must exist for all
# ports) and puts it in binary/sets/kern.tgz, and also builds any
# kernels specified in EXTRA_KERNELS. Since NetBSD's kernel build
# system can create more than one kernel from a single configuration
# we figure out how many there are, what they're named, and move them
# to binary/kernel/${KERNEL}.${CONFIGFILE}.gz - most often KERNEL will
# simply be "netbsd". If we don't find the "config" line, assume the
# kernel will be "netbsd" (some config files are simple additions on
# GENERIC, and just include it).
#
GETKERNELAWK=	awk '/^config/ {print $$2; found=1} \
		END{ if (found == 0) print "netbsd"; }'
#
.if !target(snap_kern)
snap_kern:
	cd ${KERNCONFDIR} && ${CONFIG} \
		-b ${KERNOBJDIR}/GENERIC -s ${KERNSRCDIR} GENERIC
.ifndef UPDATE
	cd ${KERNOBJDIR}/GENERIC && ${MAKE} clean 
.endif
	cd ${KERNOBJDIR}/GENERIC && ${MAKE} depend && ${MAKE} ${_J}
	cd ${KERNOBJDIR}/GENERIC &&	\
		tar cf - `${GETKERNELAWK} ${KERNCONFDIR}/GENERIC` |\
		gzip -c -9 > ${RELEASEDIR}/binary/sets/kern.tgz
# XXX use same loop for ${EXTRA_KERNELS} and ${BUILD_KERNELS} but
# XXX only install ${EXTRA_KERNELS} in binary/kernel???
.for configfile in ${EXTRA_KERNELS}
	cd ${KERNCONFDIR} && ${CONFIG} \
		-b ${KERNOBJDIR}/${configfile} -s ${KERNSRCDIR} ${configfile}
.ifndef UPDATE
	cd ${KERNOBJDIR}/${configfile} && ${MAKE} clean 
.endif
	cd ${KERNOBJDIR}/${configfile} && ${MAKE} depend && ${MAKE} ${_J}
	cd ${KERNOBJDIR}/${configfile} &&	\
		for kernel in `${GETKERNELAWK} \
			${KERNCONFDIR}/${configfile}` ; {   \
		gzip -c -9 < $${kernel} > \
			${RELEASEDIR}/binary/kernel/$${kernel}.${configfile}.gz ; }
.endfor # EXTRA_KERNELS
.for configfile in ${BUILD_KERNELS}
	cd ${KERNCONFDIR} && ${CONFIG} \
		-b ${KERNOBJDIR}/${configfile} -s ${KERNSRCDIR} ${configfile}
.ifndef UPDATE
	cd ${KERNOBJDIR}/${configfile} && ${MAKE} clean 
.endif
	cd ${KERNOBJDIR}/${configfile} && ${MAKE} depend && ${MAKE} ${_J}
.endfor # BUILD_KERNELS
.endif # no target(snap_kern)

.endif # RELEASEDIR check

snap_md_pre:
# nothing here -- look in the machine-dependent Makefile.inc

snap_md_post:
# nothing here -- look in the machine-dependent Makefile.inc

.endif	# DESTDIR check

.include <bsd.prog.mk>
