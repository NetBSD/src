#	$NetBSD: Makefile,v 1.219.2.2 2002/05/30 23:59:12 tv Exp $
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

# For NO_SENDMAIL, INSTPRIV, MKCRYPTO
.include <bsd.own.mk>

# For KERNSRCDIR, KERNOBJDIR, ...
.include <bsd.kernobj.mk>


.MAKEOVERRIDES+=	USETOOLS

TZDIR=		/usr/share/zoneinfo
LOCALTIME?=	UTC
PWD_MKDB?=	pwd_mkdb
MAKESUMS=	CKSUM=${CKSUM:Q} sh ../distrib/sets/makesums

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
INSTALLATION_DIRS=	binary binary/sets binary/kernel installation

.if exists(etc.${MACHINE}/Makefile.inc)
.include "etc.${MACHINE}/Makefile.inc"
.endif

# -rw-r--r--
BINOWN= root
BINGRP= wheel
BIN1+=	bootptab changelist csh.cshrc csh.login csh.logout daily \
	daily.conf dm.conf floppytab ftpchroot ftpusers ftpwelcome \
	gettytab group hosts hosts.lpd inetd.conf lkm.conf \
	mailer.conf man.conf monthly monthly.conf mrouted.conf \
	netconfig networks newsyslog.conf nsswitch.conf ntp.conf \
	passwd.conf phones printcap profile protocols rbootd.conf rc rc.conf \
	rc.lkm rc.local rc.subr rc.shutdown remote rpc \
	security security.conf services shells sysctl.conf syslog.conf \
	weekly weekly.conf wscons.conf \
	etc.${MACHINE}/ttys etc.${MACHINE}/disktab

.if	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "vax")
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

.if make(install-etc-files)						# {
.include <bsd.endian.mk>
.if ${TARGET_ENDIANNESS} == "1234"
TARGET_ENDIANNESS=	-L
.elif ${TARGET_ENDIANNESS} == "4321"
TARGET_ENDIANNESS=	-B
.else
TARGET_ENDIANNESS=
.endif
.endif	# install-etc-files						# }

obj:
	mkdir -p ${KERNOBJDIR}

distribution: check_DESTDIR .WAIT distrib-dirs
.if !defined(DISTRIBUTION_DONE)
.if !defined(INSTALL_DONE)
	(cd ..; ${MAKE} _DISTRIB= includes)
	(cd ..; ${MAKE} _DISTRIB= install)
.endif	# !INSTALL_DONE
	${MAKE} install-etc-files
.endif	# !DISTRIBUTION_DONE

install-etc-files: check_DESTDIR
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN1} ${DESTDIR}/etc
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 664 ${BIN2} ${DESTDIR}/etc
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 ${BIN3} ${DESTDIR}/etc
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 aliases \
	    ${DESTDIR}/etc/mail
	${INSTALL_FILE} -o root -g wheel -m 600 crontab \
	    ${DESTDIR}/var/cron/tabs/root
	${INSTALL_FILE} -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	${PWD_MKDB} -p ${TARGET_ENDIANNESS} -d ${DESTDIR}/ \
		${DESTDIR}/etc/master.passwd
.if defined(UNPRIVED)
	echo "${DESTDIR}/etc/passwd type=file mode=0644 uname=root gname=wheel" \
		| sed -e 's|^/|./|g' -e 's|//|/|g' >>${METALOG}
	echo "${DESTDIR}/etc/pwd.db type=file mode=0644 uname=root gname=wheel" \
		| sed -e 's|^/|./|g' -e 's|//|/|g' >>${METALOG}
	echo "${DESTDIR}/etc/spwd.db type=file mode=0600 uname=root gname=wheel" \
		| sed -e 's|^/|./|g' -e 's|//|/|g' >>${METALOG}
.endif	# UNPRIVED
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	${INSTALL_FILE} -o root -g wheel -m 600 minfree ${DESTDIR}/var/crash
	(cd root; \
		${INSTALL_FILE} -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		${INSTALL_FILE} -o root -g wheel -m 600 dot.klogin \
		    ${DESTDIR}/root/.klogin; \
		${INSTALL_FILE} -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		${INSTALL_FILE} -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		${INSTALL_FILE} -o root -g wheel -m 644 dot.shrc \
		    ${DESTDIR}/root/.shrc; \
		rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile; \
		${INSTALL_LINK} ${DESTDIR}/root/.cshrc ${DESTDIR}/.cshrc; \
		${INSTALL_LINK} ${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	(cd defaults; ${MAKE} install)
	(cd mtree; ${MAKE} install)
	(cd namedb; \
		${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
		    ${NAMEDB} ${DESTDIR}/etc/namedb)
	(cd rc.d; ${MAKE} install)
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		postinstall ${DESTDIR}/etc
	${INSTALL_SYMLINK} -o root -g wheel -m 755 \
		${TZDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
	${INSTALL_SYMLINK} -o root -g wheel -m 755 \
		/usr/sbin/rmt ${DESTDIR}/etc/rmt
	${INSTALL_FILE} -o ${BINOWN} -g operator -m 664 /dev/null \
		${DESTDIR}/etc/dumpdates
	${INSTALL_FILE} -o ${BINOWN} -g operator -m 600 /dev/null \
		${DESTDIR}/etc/skeykeys
	${INSTALL_FILE} -o root -g wheel -m 600 /dev/null \
		${DESTDIR}/var/at/at.deny
	${INSTALL_FILE} -o root -g wheel -m 600 /dev/null \
		${DESTDIR}/var/cron/log
	${INSTALL_FILE} -o nobody -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/db/locate.database
	${INSTALL_FILE} -o uucp -g dialer -m 640 /dev/null \
		${DESTDIR}/var/log/aculog
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/authlog
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/log/lastlog
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 640 /dev/null \
		${DESTDIR}/var/log/lpd-errs
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/maillog
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/messages
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/secure
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/sendmail.st
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/wtmp
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/xferlog
	${INSTALL_FILE} -o daemon -g staff -m 664 /dev/null \
		${DESTDIR}/var/msgs/bounds
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/run/utmp
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/atc_scores
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/battlestar.log
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/cfscores
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/criblog
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/robots_roll
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/rogue.scores
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/saillog
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/snakerawscores
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/snake.log
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/tetris.scores
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/llog12.0
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/lscore12.0
	${INSTALL_FILE} -o games -g games -m 664 /dev/null \
		${DESTDIR}/var/games/larn/playerids
	(cd skel; ${MAKE} distribution)
	(cd ../usr.bin/mail; ${MAKE} distribution)
	(cd ../gnu/usr.sbin/postfix/; ${MAKE} distribution)
.if (${MKCRYPTO} != "no")
	(cd ../usr.bin/ssh; ${MAKE} distribution)
.endif
.if !defined(NO_SENDMAIL)
	(cd ../gnu/usr.sbin/sendmail/cf/cf; ${MAKE} distribution)
.endif

distrib-dirs: check_DESTDIR
	${INSTALL_DIR} -o root -g wheel -m 755 ${DESTDIR}
	${MTREE} -def mtree/NetBSD.dist -p ${DESTDIR}/ -U ${UNPRIVED:D-W}
	rm -f ${DESTDIR}/sys
	${INSTALL_SYMLINK} -o root -g wheel -m 755 usr/src/sys ${DESTDIR}/sys

release snapshot: check_DESTDIR check_RELEASEDIR .WAIT \
    distribution .WAIT snap_pre snap_md_pre .WAIT snap_kern .WAIT snap_md_post
	(cd ../distrib/sets; ${MAKE} sets)
	${MAKESUMS} -t ${RELEASEDIR}/binary/kernel '*.gz'

# Standalone target to create a CDROM image after the release
# was composed. Should be run after "make build" in both src and xsrc
iso-image: check_DESTDIR check_RELEASEDIR .WAIT iso-image_md_post

iso-image_mi: check_DESTDIR check_RELEASEDIR .WAIT iso-image_md_pre
	@if ${MKISOFS} --version; then \
		mkdir -p ${RELEASEDIR}/installation/cdrom ; \
		${MKISOFS} ${MKISOFS_FLAGS} \
			-o ${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
			${RELEASEDIR} ; \
		${MAKESUMS} -t ${RELEASEDIR}/installation/cdrom '*.iso' ; \
	else \
		echo "install pkgsrc/sysutils/cdrecord and type 'make iso-image'." ; \
	fi

# Setup the $RELEASEDIR to produce a bootable CD image:
iso-image_md_pre: check_DESTDIR check_RELEASEDIR
# nothing here -- look in the machine-dependent Makefile.inc

# Fixup the  CD-image to be bootable
iso-image_md_post: check_DESTDIR check_RELEASEDIR .WAIT iso-image_mi
# nothing here -- look in the machine-dependent Makefile.inc

snap_pre: check_DESTDIR check_RELEASEDIR
# Could be a mount point, ignore the errors
	-/bin/rm -rf ${RELEASEDIR}
	${INSTALL} -d -m 755 ${RELEASEDIR}
.for dir in ${INSTALLATION_DIRS}
	${INSTALL} -d -m 755 ${RELEASEDIR}/${dir}
.endfor

# This target builds the kernels specified by each port.  A port may
# specify the following kernels:
#
#	KERNEL_SETS		The list of kernels that will be
#				packaged into sets, named
#				kern-${kernel}.tgz.  These kernels
#				are also placed in the binary/kernel
#				area of the release package as
#				netbsd-${kernel}.gz.
#
#	EXTRA_KERNELS		Additional kernels to place in the
#				binary/kernel area of the release
#				package as netbsd-${kernel}.gz, but
#				which are not placed into sets. This
#				allows a port to provide e.g. a netbootable
#				installation kernel containing a ramdisk.
#
#	BUILD_KERNELS		Additional kernels to build which are
#				not placed into sets nor into the
#				binary/kernel area of the release
#				package.  These are typically kernels
#				that are built for inclusion only in
#				installation disk/CD-ROM/tape images.
#
# A port may also specify KERNEL_SUFFIXES, which is an optional list
# of filename suffixes for kernels to include in the kernel sets and
# in the binary/kernel area of the release package (e.g. "netbsd" vs.
# "netbsd.ecoff" and "netbsd.srec").  It is not an error if kernels
# with these suffixes do not exist in the kernel build directory.
#
GETKERNELAWK=	awk '/^config/ {print $$2; found=1} \
		END{ if (found == 0) print "netbsd"; }'
#
.if !target(snap_kern)
snap_kern: check_DESTDIR check_RELEASEDIR
.if !defined(KERNELS_DONE)						# {
.for configfile in ${KERNEL_SETS} ${EXTRA_KERNELS} ${BUILD_KERNELS}
	cd ${KERNCONFDIR} && ${CONFIG} \
		-b ${KERNOBJDIR}/${configfile:C/.*\///} -s ${KERNSRCDIR} ${configfile}
.if !defined(UPDATE)
	cd ${KERNOBJDIR}/${configfile:C/.*\///} && ${MAKE} distclean
.endif
	cd ${KERNOBJDIR}/${configfile:C/.*\///} && ${MAKE} depend && ${MAKE}
.endfor # build kernels
.endif # KERNELS_DONE							# }
.for configfile in ${KERNEL_SETS}
	kernlist=`${GETKERNELAWK} ${KERNCONFDIR}/${configfile}`; \
	kerndir=${KERNOBJDIR}/${configfile:C/.*\///}; \
	kernsuffixes="${KERNEL_SUFFIXES}"; \
	cd $${kerndir} && ( \
		echo "/set uname=${BINOWN} gname=${BINGRP}"; \
		echo ". type=dir optional"; \
		for kernel in $${kernlist}; do \
			echo "./$${kernel} type=file"; \
			for s in $${kernsuffixes}; do \
				if [ -f $${kernel}.$${s} ]; then \
					echo "./$${kernel}.$${s} type=file"; \
				fi; \
			done; \
		done ) | GZIP=-9 ${PAX} -zw \
		    -M ${UNPRIVED:D-N${NETBSDSRCDIR}/etc} \
		    -f ${RELEASEDIR}/binary/sets/kern-${configfile}.tgz
.endfor # make kernel sets
.for configfile in ${KERNEL_SETS} ${EXTRA_KERNELS}
	kernlist=`${GETKERNELAWK} ${KERNCONFDIR}/${configfile:C/.*\///}`; \
	kerndir=${KERNOBJDIR}/${configfile:C/.*\///}; \
	kernsuffixes="${KERNEL_SUFFIXES}"; \
	cd $${kerndir} &&	\
		gziplist=`for kernel in $${kernlist}; do \
			echo "$${kernel}"; \
			for s in $${kernsuffixes}; do \
				if [ -f $${kernel}.$${s} ]; then \
					echo "$${kernel}.$${s}"; \
				fi; \
			done; \
		done`; \
		for kernel in $${gziplist} ; do \
		gzip -c -9 < $${kernel} > \
		    ${RELEASEDIR}/binary/kernel/$${kernel}-${configfile:C/.*\///}.gz ; done
.endfor # place KERNEL_SETS kernels + EXTRA_KERNELS in binary/kernel/...
.endif # no target(snap_kern)

snap_md_pre: check_DESTDIR check_RELEASEDIR
# nothing here -- look in the machine-dependent Makefile.inc

snap_md_post: check_DESTDIR check_RELEASEDIR
# nothing here -- look in the machine-dependent Makefile.inc

.include <bsd.prog.mk>
