#	$NetBSD: Makefile,v 1.199 2001/12/12 00:07:46 tv Exp $
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
#	${KERNSRCDIR}/arch/${MACHINE}/compile, but can be overridden.
#   KERNCONFDIR is where the configuration files for kernels are found;
#	default is ${KERNSRCDIR}/arch/${MACHINE}/conf but can be overridden.
#   UPDATE is normally undefined; if defined, don't do a 'make clean'
#	before kernel compile
#   NO_SENDMAIL is normally undefined; if defined, it will not do a
#	`make distribution' in the sendmail config file source directory.
#
# Targets:
#    distribution: makes a full NetBSD distribution in DESTDIR. If
#	INSTALL_DONE is set, it will not do a `make install.'
#	if DISTRIBUTION_DONE is set, it will not do anything.
#    distrib-dirs: creates an empty NetBSD directory tree in DESTDIR.
#	Called by distribution.
#    snapshot: calls distribution, above, and then tars up the files
#	into a release(7) format in RELEASEDIR. Any port-dependent
#	stuff for this target is found in etc.${MACHINE}/Makefile.inc.
#    release: a synonym for `snapshot'

# setting NOOBJ prevents "make obj" from doing anything;
# an objdir would break the installation stuff below
NOOBJ=		# defined

# do this before bsd.own.mk so we get correct KERNSRCDIR
.include "../Makefile.inc"

# For NO_SENDMAIL, INSTPRIV, MKCRYPTO
.include <bsd.own.mk>

.MAKEOVERRIDES+=	USETOOLS

TZDIR=		/usr/share/zoneinfo
LOCALTIME?=	UTC
PWD_MKDB?=	pwd_mkdb

# Flags for creating ISO CDROM image
# mkisofs is expected to be in $PATH, install via pkgsrc/sysutils/cdrecord
MKISOFS?=	mkisofs
MKISOFS_FLAGS+= -J -l \
		-r -T -v \
		-P "The NetBSD Project" \
		-m "${RELEASEDIR}/installation/cdrom"

# MD Makefile.inc may append MD targets to BIN[123].  Make sure all
# are empty, to preserve the old semantics of setting them below with "=".
BIN1=	
BIN2=
BIN3=

# Directories to build in ${RELEASEDIR}.   MD Makefile.inc files can
# add to this.
# NOTE: Parent directories must be listed before subdirectories.
INSTALLATION_DIRS= \
	binary binary/sets binary/kernel installation

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
BIN1+=	bootptab changelist csh.cshrc csh.login csh.logout daily \
	daily.conf dm.conf floppytab ftpchroot ftpusers ftpwelcome \
	gettytab group hosts hosts.lpd inetd.conf lkm.conf \
	mailer.conf man.conf monthly monthly.conf mrouted.conf \
	netconfig networks newsyslog.conf nsswitch.conf ntp.conf \
	phones printcap profile protocols rbootd.conf rc rc.conf \
	rc.lkm rc.local rc.subr rc.shutdown remote rpc \
	security security.conf services shells sysctl.conf syslog.conf \
	weekly weekly.conf wscons.conf \
	etc.${MACHINE}/ttys etc.${MACHINE}/disktab

.if	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
BIN1+=	ld.so.conf
.elif exists(etc.${MACHINE_ARCH}/ld.so.conf)
BIN1+=	etc.${MACHINE_ARCH}/ld.so.conf
.endif

.if exists(etc.${MACHINE_ARCH}/ttyaction)
BIN1+=	etc.${MACHINE_ARCH}/ttyaction
.endif

# -rw-rw-r--
BIN2+=	motd

# -rw-------
BIN3+=	hosts.equiv

NAMEDB=	127 root.cache named.conf localhost loopback.v6
PCS=	pcs750.bin

.if make(release) || make(snapshot) || make(distribution)
# find out endianness of target and set proper flag for pwd_mkdb so that
# it creates database in same endianness
.if exists(${DESTDIR}/usr/include/sys/endian.h)
TARGET_ENDIANNESS!= \
	printf '\#include <sys/endian.h>\n_BYTE_ORDER\n' | \
	${CC} -I${DESTDIR}/usr/include -E - | tail -1 | awk '{print $$1}'
.else
TARGET_ENDIANNESS=
.endif

.if ${TARGET_ENDIANNESS} == "1234"
TARGET_ENDIANNESS= -L
.elif ${TARGET_ENDIANNESS} == "4321"
TARGET_ENDIANNESS= -B
.else
TARGET_ENDIANNESS=
.endif
.endif

.include <bsd.kernobj.mk>

obj:
	mkdir -p ${KERNOBJDIR}

.if !defined(DESTDIR) || ${DESTDIR} == ""
__warndestdir: .USE
	@echo setenv DESTDIR before doing that!
	@false
distribution distrib-dirs release snapshot snap_pre snap_md_pre \
iso-image: __warndestdir
.if !target(snap_md_post)
snap_md_post: __warndestdir
.endif
.else
distribution: distrib-dirs
.if !defined(DISTRIBUTION_DONE)
.if !defined(INSTALL_DONE)
	(cd ..; ${MAKE} _DISTRIB= includes)
	(cd ..; ${MAKE} _DISTRIB= install)
.endif
	${MAKE} install-etc-files
.endif

install-etc-files:
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN1} \
	    ${DESTDIR}/etc
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 664 ${BIN2} \
	    ${DESTDIR}/etc
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 600 ${BIN3} \
	    ${DESTDIR}/etc
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 aliases \
	    ${DESTDIR}/etc/mail
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 crontab \
	    ${DESTDIR}/var/cron/tabs/root
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 master.passwd \
	    ${DESTDIR}/etc
	${PWD_MKDB} -p ${TARGET_ENDIANNESS} -d ${DESTDIR}/ \
		${DESTDIR}/etc/master.passwd
.if defined(UNPRIVED)
	echo "${DESTDIR}/etc/passwd type=file mode=0644 uname=root gname=wheel" \
		| sed -e 's|^/|./|g' -e 's|//|/|g' >>${METALOG}
	echo "${DESTDIR}/etc/pwd.db type=file mode=0644 uname=root gname=wheel" \
		| sed -e 's|^/|./|g' -e 's|//|/|g' >>${METALOG}
	echo "${DESTDIR}/etc/spwd.db type=file mode=0600 uname=root gname=wheel" \
		| sed -e 's|^/|./|g' -e 's|//|/|g' >>${METALOG}
.endif
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
		${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 644 dot.shrc \
		    ${DESTDIR}/root/.shrc; \
		rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile; \
		${INSTALL} ${INSTPRIV} -l h \
			${DESTDIR}/root/.cshrc ${DESTDIR}/.cshrc; \
		${INSTALL} ${INSTPRIV} -l h \
			${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	(cd defaults; ${MAKE} install)
	(cd mtree; ${MAKE} install)
	(cd namedb; \
		${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 \
		    ${NAMEDB} ${DESTDIR}/etc/namedb)
	(cd rc.d; ${MAKE} install)
	${INSTALL} ${INSTPRIV} -l s -o root -g wheel -m 755 \
		${TZDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
	${INSTALL} ${INSTPRIV} -l s -o root -g wheel -m 755 \
		/usr/sbin/rmt ${DESTDIR}/etc/rmt
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g operator -m 664 /dev/null \
		${DESTDIR}/etc/dumpdates
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g operator -m 600 /dev/null \
		${DESTDIR}/etc/skeykeys
	${INSTALL} ${INSTPRIV} -c -o root -g wheel -m 600 /dev/null \
		${DESTDIR}/var/at/at.deny
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
		${DESTDIR}/var/log/sendmail.st
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
	(cd skel; ${MAKE} distribution)
	(cd ../usr.bin/mail; ${MAKE} distribution)
	(cd ../gnu/usr.sbin/postfix/; ${MAKE} distribution)
.if (${MKCRYPTO} != "no")
	(cd ../usr.bin/ssh; ${MAKE} distribution)
.endif
.ifndef NO_SENDMAIL
	(cd ../gnu/usr.sbin/sendmail/cf/cf; ${MAKE} distribution)
.endif

distrib-dirs:
	${INSTALL} ${INSTPRIV} -d -o root -g wheel -m 755 ${DESTDIR}
	${MTREE} -def mtree/NetBSD.dist -p ${DESTDIR}/ -U ${UNPRIVED:D-W}
	rm -f ${DESTDIR}/sys
	${INSTALL} ${INSTPRIV} -l s -o root -g wheel -m 755 \
		usr/src/sys ${DESTDIR}/sys

.if !defined(RELEASEDIR)
__warnreleasedir: .USE
	@echo setenv RELEASEDIR before doing that!
	@false
release snapshot snap_pre snap_md_pre iso-image: __warnreleasedir
.if !target(snap_md_post)
snap_md_post: __warnreleasedir
.endif
.else
release snapshot: distribution snap_pre snap_md_pre snap_kern snap_md_post
	(cd ../distrib/sets; ${MAKE} sets)
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/binary/kernel '*.gz'

# Standalone target to create a CDROM image after the release
# was composed. Should be run after "make build" in both src and xsrc
iso-image: iso-image_md_post

iso-image_mi: iso-image_md_pre
	@if ${MKISOFS} --version; then \
		mkdir -p ${RELEASEDIR}/installation/cdrom ; \
		${MKISOFS} ${MKISOFS_FLAGS} \
			-o ${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
			${RELEASEDIR} ; \
		sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/cdrom '*.iso' ; \
	else \
		echo "install pkgsrc/sysutils/cdrecord and type 'make iso-image'." ; \
	fi

# Setup the $RELEASEDIR to produce a bootable CD image:
iso-image_md_pre:
# nothing here -- look in the machine-dependent Makefile.inc

# Fixup the  CD-image to be bootable
iso-image_md_post: iso-image_mi
# nothing here -- look in the machine-dependent Makefile.inc

snap_pre:
# Could be a mount point, ignore the errors
	-/bin/rm -rf ${RELEASEDIR}
	${INSTALL} ${INSTPRIV} -d -o root -g wheel -m 755 ${RELEASEDIR}
.for dir in ${INSTALLATION_DIRS}
	${INSTALL} ${INSTPRIV} -d -o root -g wheel -m 755 ${RELEASEDIR}/${dir}
.endfor

# This target builds the kernels specified by each port.  A port may
# specify the following kernels:
#
#	KERNEL_SETS		The list of kernels that will be
#				packaged into sets, named
#				kern-${kernel}.tgz.  These kernels
#				are also placed in the binary/kernels
#				area of the release package as
#				netbsd-${kernel}.gz.
#
#	EXTRA_KERNELS		Additional kernels to place in the
#				binary/kernels area of the release
#				package as netbsd-${kernel}.gz, but
#				which are not placed into sets. This
#				allows a port to provide e.g. a netbootable
#				installation kernel containing a ramdisk.
#
#	BUILD_KERNELS		Additional kernels to build which are
#				not placed into sets nor into the
#				binary/kernels area of the release
#				package.  These are typically kernels
#				that are built for inclusion only in
#				installation disk/CD-ROM/tape images.
#
# A port may also specify KERNEL_SUFFIXES, which is an optional list
# of filename suffixes for kernels to include in the kernel sets and
# in the binary/kernels area of the release package (e.g. "netbsd" vs.
# "netbsd.ecoff" and "netbsd.srec").  It is not an error if kernels
# with these suffixes do not exist in the kernel build directory.
#
GETKERNELAWK=	awk '/^config/ {print $$2; found=1} \
		END{ if (found == 0) print "netbsd"; }'
#
.if !target(snap_kern)
snap_kern:
.ifndef KERNELS_DONE
.for configfile in ${KERNEL_SETS} ${EXTRA_KERNELS} ${BUILD_KERNELS}
	cd ${KERNCONFDIR} && ${CONFIG} \
		-b ${KERNOBJDIR}/${configfile:C/.*\///} -s ${KERNSRCDIR} ${configfile}
.ifndef UPDATE
	cd ${KERNOBJDIR}/${configfile:C/.*\///} && ${MAKE} distclean 
.endif
	cd ${KERNOBJDIR}/${configfile:C/.*\///} && ${MAKE} depend && ${MAKE} ${_J}
.endfor # build kernels
.for configfile in ${KERNEL_SETS}
	kernlist=`${GETKERNELAWK} ${KERNCONFDIR}/${configfile}`; \
	kerndir=${KERNOBJDIR}/${configfile:C/.*\///}; \
	cd $${kerndir} && \
		tarlist=`for kernel in $${kernlist}; do \
			echo "$${kernel}"; \
			for s in ${KERNEL_SUFFIXES}; do \
				if [ -f $${kernel}.$${s} ]; then \
					echo "$${kernel}.$${s}"; \
				fi; \
			done; \
		done`; \
		tar cf - $${tarlist} | \
		gzip -c -9 > ${RELEASEDIR}/binary/sets/kern-${configfile}.tgz
.endfor # make kernel sets
.for configfile in ${KERNEL_SETS} ${EXTRA_KERNELS}
	kernlist=`${GETKERNELAWK} ${KERNCONFDIR}/${configfile:C/.*\///}`; \
	kerndir=${KERNOBJDIR}/${configfile:C/.*\///}; \
	cd $${kerndir} &&	\
		gziplist=`for kernel in $${kernlist}; do \
			echo "$${kernel}"; \
			for s in ${KERNEL_SUFFIXES}; do \
				if [ -f $${kernel}.$${s} ]; then \
					echo "$${kernel}.$${s}"; \
				fi; \
			done; \
		done`; \
		for kernel in $${gziplist} ; do \
		gzip -c -9 < $${kernel} > \
			${RELEASEDIR}/binary/kernel/$${kernel}-${configfile:C/.*\///}.gz ; done
.endfor # place KERNEL_SETS kernels + EXTRA_KERNELS in binary/kernel/...
.endif # KERNELS_DONE
.endif # no target(snap_kern)

.endif # RELEASEDIR check

snap_md_pre:
# nothing here -- look in the machine-dependent Makefile.inc

snap_md_post:
# nothing here -- look in the machine-dependent Makefile.inc

.endif	# DESTDIR check

.include <bsd.prog.mk>
