#	$NetBSD: Makefile,v 1.245 2003/02/16 00:22:37 lukem Exp $
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
#	into a release(7) format in RELEASEDIR/${MACHINE}.
#	Any port-dependent stuff for this target is found in
#	etc.${MACHINE}/Makefile.inc.
#    release: a synonym for `snapshot'
#

# For NO_SENDMAIL, INSTPRIV, MKCRYPTO
#
.include <bsd.own.mk>

# For KERNSRCDIR, KERNOBJDIR, ...
#
.include <bsd.kernobj.mk>


.MAKEOVERRIDES+=	USETOOLS

TZDIR=		/usr/share/zoneinfo
LOCALTIME?=	UTC
PWD_MKDB?=	pwd_mkdb
MAKESUMS=	CKSUM=${CKSUM:Q} sh ${NETBSDSRCDIR}/distrib/sets/makesums

# Flags for creating ISO CDROM image
# mkisofs is expected to be in $PATH, install via pkgsrc/sysutils/cdrecord
# Note: At least mkisofs 2.0 should be used.
#
MKISOFS?=	mkisofs
DISTRIBREV!=	sh ${KERNSRCDIR}/conf/osrelease.sh -s
# ISO 9660 volume ID.  Note that this can only contain [A-Z0-9_].
ISO_VOLID!=	echo NETBSD_${MACHINE}_${DISTRIBREV} | tr a-z A-Z
MKISOFS_FLAGS+= -J -l \
		-r -T -v \
		-V ${ISO_VOLID} \
		-P "The NetBSD Project" \
		-m "${RELEASEDIR}/${MACHINE}/installation/cdrom"

# MD Makefile.inc may append MD targets to BIN[123].  Make sure all
# are empty, to preserve the old semantics of setting them below with "=".
#
BIN1=
BIN2=
BIN3=

# Directories to build in ${RELEASEDIR}/${MACHINE}.
# MD Makefile.inc files can add to this.
# NOTE: Parent directories must be listed before subdirectories.
#
INSTALLATION_DIRS=	binary binary/sets binary/kernel installation

.if exists(etc.${MACHINE}/Makefile.inc)
.include "etc.${MACHINE}/Makefile.inc"
.endif

# -rw-r--r--
BINOWN= root
BINGRP= wheel
UTMPGRP= utmp
BIN1+=	bootptab changelist csh.cshrc csh.login csh.logout daily \
	daily.conf dm.conf floppytab ftpchroot ftpusers \
	gettytab group hosts hosts.lpd inetd.conf lkm.conf \
	mailer.conf man.conf monthly monthly.conf mrouted.conf \
	netconfig networks newsyslog.conf nsswitch.conf ntp.conf \
	passwd.conf phones printcap profile protocols rbootd.conf rc rc.conf \
	rc.lkm rc.local rc.subr rc.shutdown remote rpc \
	security security.conf services shells sysctl.conf syslog.conf \
	weekly weekly.conf wscons.conf \
	etc.${MACHINE}/ttys

# Use machine-specific disktab if it exists, or the MI one otherwise
.if exists(etc.${MACHINE}/disktab)
BIN1+=	etc.${MACHINE}/disktab
.else
BIN1+=	disktab
.endif

.if exists(etc.${MACHINE}/ld.so.conf)
BIN1+=	etc.${MACHINE}/ld.so.conf
.endif

.if exists(etc.${MACHINE}/ttyaction)
BIN1+=	etc.${MACHINE}/ttyaction
.endif

# -rw-rw-r--
BIN2+=	motd

# -rw-------
BIN3+=	hosts.equiv

ETC_PKG=-T etc_pkg
BASE_PKG=-T base_pkg
ETC_INSTALL_FILE=cd ${.CURDIR} && ${INSTALL_FILE} ${ETC_PKG}
ETC_INSTALL_LINK=cd ${.CURDIR} && ${INSTALL_LINK} ${ETC_PKG}
ETC_INSTALL_SYMLINK=cd ${.CURDIR} && ${INSTALL_SYMLINK} ${ETC_PKG}

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


# distribution --
#	Build a distribution
#
distribution: check_DESTDIR .WAIT distrib-dirs
.if !defined(DISTRIBUTION_DONE)
.if !defined(INSTALL_DONE)
	(cd ${NETBSDSRCDIR}; ${MAKE} _DISTRIB= includes)
	(cd ${NETBSDSRCDIR}; ${MAKE} _DISTRIB= install)
.endif	# !INSTALL_DONE
	(cd ${.CURDIR}; ${MAKE} install-etc-files)
.endif	# !DISTRIBUTION_DONE


# install-etc-files --
#	Install etc (config) files; not performed by "make build"
#
install-etc-files: check_DESTDIR
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${BIN1} ${DESTDIR}/etc
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 664 \
	    ${BIN2} ${DESTDIR}/etc
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 \
	    ${BIN3} ${DESTDIR}/etc
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    aliases ${DESTDIR}/etc/mail
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    /dev/null ${DESTDIR}/etc/mail/local-host-names
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    crontab ${DESTDIR}/var/cron/tabs/root
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    master.passwd ${DESTDIR}/etc
	${PWD_MKDB} -p ${TARGET_ENDIANNESS} -d ${DESTDIR}/ \
	    ${DESTDIR}/etc/master.passwd
.if defined(UNPRIVED)
	( \
	    echo "./etc/passwd type=file mode=0644 uname=root gname=wheel tags=etc_pkg"; \
	    echo "./etc/pwd.db type=file mode=0644 uname=root gname=wheel tags=etc_pkg"; \
	    echo "./etc/spwd.db type=file mode=0600 uname=root gname=wheel tags=etc_pkg"; \
	) | ${METALOG.add}
.endif	# UNPRIVED
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
	    MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    minfree ${DESTDIR}/var/crash
.for file in cshrc login profile shrc
	${ETC_INSTALL_FILE} -o root -g wheel -m 644 \
	    root/dot.${file} ${DESTDIR}/root/.${file}
.endfor
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    root/dot.klogin ${DESTDIR}/root/.klogin
	rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile
	${ETC_INSTALL_LINK} ${DESTDIR}/root/.cshrc ${DESTDIR}/.cshrc
	${ETC_INSTALL_LINK} ${DESTDIR}/root/.profile ${DESTDIR}/.profile
	(cd ${.CURDIR}/defaults; ${MAKE} install)
	(cd ${.CURDIR}/mtree; ${MAKE} install)
.for file in ${NAMEDB}
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    namedb/${file} ${DESTDIR}/etc/namedb
.endfor
	(cd ${.CURDIR}/rc.d; ${MAKE} install)
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    postinstall ${DESTDIR}/etc
	${ETC_INSTALL_SYMLINK} -o root -g wheel -m 755 \
	    ${TZDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
	${ETC_INSTALL_SYMLINK} -o root -g wheel -m 755 \
	    /usr/sbin/rmt ${DESTDIR}/etc/rmt
	${ETC_INSTALL_FILE} -o ${BINOWN} -g operator -m 664 \
	    /dev/null ${DESTDIR}/etc/dumpdates
	${ETC_INSTALL_FILE} -o ${BINOWN} -g operator -m 600 \
	    /dev/null ${DESTDIR}/etc/skeykeys
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    /dev/null ${DESTDIR}/var/at/at.deny
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    /dev/null ${DESTDIR}/var/cron/log
	${ETC_INSTALL_FILE} -o nobody -g ${BINGRP} -m 664 \
	    /dev/null ${DESTDIR}/var/db/locate.database
	${ETC_INSTALL_FILE} -o uucp -g dialer -m 640 \
	    /dev/null ${DESTDIR}/var/log/aculog
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 \
	    /dev/null ${DESTDIR}/var/log/authlog
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 664 \
	    /dev/null ${DESTDIR}/var/log/lastlog
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 640 \
	    /dev/null ${DESTDIR}/var/log/lpd-errs
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 \
	    /dev/null ${DESTDIR}/var/log/maillog
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    /dev/null ${DESTDIR}/var/log/messages
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 \
	    /dev/null ${DESTDIR}/var/log/secure
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    /dev/null ${DESTDIR}/var/log/sendmail.st
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    /dev/null ${DESTDIR}/var/log/wtmp
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    /dev/null ${DESTDIR}/var/log/wtmpx
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 600 \
	    /dev/null ${DESTDIR}/var/log/xferlog
	${ETC_INSTALL_FILE} -o daemon -g staff -m 664 \
	    /dev/null ${DESTDIR}/var/msgs/bounds
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${UTMPGRP} -m 664 \
	    /dev/null ${DESTDIR}/var/run/utmp
	${ETC_INSTALL_FILE} -o ${BINOWN} -g ${UTMPGRP} -m 664 \
	    /dev/null ${DESTDIR}/var/run/utmpx
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/atc_scores
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/battlestar.log
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/cfscores
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/criblog
	${ETC_INSTALL_FILE} -o games -g games -m 660 \
	    /dev/null ${DESTDIR}/var/games/hackdir/perm
	${ETC_INSTALL_FILE} -o games -g games -m 660 \
	    /dev/null ${DESTDIR}/var/games/hackdir/record
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/larn/llog12.0
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/larn/lscore12.0
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/larn/playerids
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/robots_roll
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/rogue.scores
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/saillog
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/snakerawscores
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/snake.log
	${ETC_INSTALL_FILE} -o games -g games -m 664 \
	    /dev/null ${DESTDIR}/var/games/tetris.scores
	(cd ${.CURDIR}/skel; ${MAKE} distribution)
	(cd ${NETBSDSRCDIR}/usr.bin/mail; ${MAKE} distribution)
	(cd ${NETBSDSRCDIR}/gnu/usr.sbin/postfix/; ${MAKE} distribution)
.if (${MKCRYPTO} != "no")
	(cd ${NETBSDSRCDIR}/usr.bin/ssh; ${MAKE} distribution)
.endif
.if !defined(NO_SENDMAIL) || ${NO_SENDMAIL} == "no"
	(cd ${NETBSDSRCDIR}/gnu/usr.sbin/sendmail/cf/cf; ${MAKE} distribution)
.endif


# distrib-dirs --
#	Populate $DESTDIR with directories needed by NetBSD
#
distrib-dirs: check_DESTDIR
	${INSTALL_DIR} -o root -g wheel -m 755 ${BASE_PKG} ${DESTDIR}
	${MTREE} -def ${.CURDIR}/mtree/NetBSD.dist -N ${.CURDIR} \
	    -p ${DESTDIR}/ -U ${UNPRIVED:D-W}


# release, snapshot --
#	Build a full distribution including kernels & install media.
#
release snapshot: check_DESTDIR check_RELEASEDIR .WAIT \
		    distribution .WAIT \
		    snap_pre .WAIT snap_kern .WAIT \
		    snap_post .WAIT snap_md_post
	(cd ${NETBSDSRCDIR}/distrib/sets; ${MAKE} sets)
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/binary/kernel '*.gz'


# iso-image --
#	Standalone target to create a CDROM image after the release
#	was composed.  Should be run after "make release" in src and xsrc.
#
#	Note: At least mkisofs 2.0 should be used.
#
CDROM.image=${RELEASEDIR}/${MACHINE}/installation/cdrom/netbsd-${MACHINE}.iso
CDROM.dir=	cdrom.dir
CDROM.pathlist=	cdrom.pathlist

iso-image: check_DESTDIR check_RELEASEDIR .WAIT iso-image_md_post
#	empty target

iso-image_setup: check_RELEASEDIR
	rm -f ${CDROM.pathlist}
	echo "${MACHINE}=${RELEASEDIR}/${MACHINE}" >> ${CDROM.pathlist}
	mkdir -p ${CDROM.dir}
	mkdir -p ${RELEASEDIR}/${MACHINE}/installation/cdrom

# iso-image_mi --
#	Create the image after the MD operations have completed.
iso-image_mi: check_DESTDIR check_RELEASEDIR .WAIT \
		iso-image_setup .WAIT iso-image_md_pre
	@if ! ${MKISOFS} --version; then \
		echo "install pkgsrc/sysutils/cdrecord and run 'make iso-image'." ; \
		false; \
	fi
	${MKISOFS} ${MKISOFS_FLAGS} -graft-points -path-list ${CDROM.pathlist} \
	    -o ${CDROM.image} ${CDROM.dir}
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/cdrom '*.iso'

# iso-image_md_pre --
#	Setup ${CDROM.dir} to produce a bootable CD image.
#	Overridden by etc.$MACHINE/Makefile.inc
#
iso-image_md_pre: check_DESTDIR check_RELEASEDIR
#	(empty -- look in the machine-dependent Makefile.inc)

# iso-image_md_post --
#	Fixup the CD-image to be bootable.
#	Overridden by etc.$MACHINE/Makefile.inc
#
iso-image_md_post: check_DESTDIR check_RELEASEDIR .WAIT iso-image_mi
#	(empty -- look in the machine-dependent Makefile.inc)


# snap_pre --
#	Create ${RELEASEDIR} and necessary subdirectories.
#
snap_pre: check_DESTDIR check_RELEASEDIR
	${INSTALL} -d -m 755 ${RELEASEDIR}
.ifndef UPDATE
# Could be a mount point, ignore the errors
	-/bin/rm -rf ${RELEASEDIR}/${MACHINE}
.endif
	${INSTALL} -d -m 755 ${RELEASEDIR}/${MACHINE}
.for dir in ${INSTALLATION_DIRS}
	${INSTALL} -d -m 755 ${RELEASEDIR}/${MACHINE}/${dir}
.endfor

# snap_post --
#	Build the install media and notes from distrib
#
snap_post: check_DESTDIR check_RELEASEDIR
.ifndef UPDATE
	cd ${NETBSDSRCDIR}/distrib && ${MAKE} cleandir
.endif
	cd ${NETBSDSRCDIR}/distrib && ${MAKE} depend && ${MAKE} && \
	    ${MAKE} release

# snap_kern --
#	This target builds the kernels specified by each port.
#	A port may specify the following kernels:
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
#	A port may also specify KERNEL_SUFFIXES, which is an optional list
#	of filename suffixes for kernels to include in the kernel sets and
#	in the binary/kernel area of the release package (e.g. "netbsd" vs.
#	"netbsd.ecoff" and "netbsd.srec").  It is not an error if kernels
#	with these suffixes do not exist in the kernel build directory.
#
#
# A list of all the kernels to build, which can be overridden from
# external sources (such as make(1)'s environment or command line)
#
ALL_KERNELS?=	${KERNEL_SETS} ${EXTRA_KERNELS} ${BUILD_KERNELS}

GETKERNELAWK=	awk '/^config/ {print $$2; found=1} \
		END{ if (found == 0) print "netbsd"; }'

.if !target(snap_kern)						# {

# build_kernels --
#	Configure & compile kernels listed in ${ALL_KERNELS}
#
.if !defined(KERNELS_DONE)						# {
.for configfile in ${ALL_KERNELS}					# {
_KERNELS_TO_BUILD+=kern-${configfile}
kern-${configfile}:
	cd ${KERNCONFDIR} && ${CONFIG} -s ${KERNSRCDIR} \
	    -b ${KERNOBJDIR}/${configfile:C/.*\///} ${configfile}
.if !defined(UPDATE)
	cd ${KERNOBJDIR}/${configfile:C/.*\///} && ${MAKE} distclean
.endif
	cd ${KERNOBJDIR}/${configfile:C/.*\///} && ${MAKE} depend && ${MAKE}
.endfor	# ALL_KERNELS							# }
.endif	# KERNELS_DONE							# }

build_kernels: ${_KERNELS_TO_BUILD}

# build_kernelsets --
#	Create kernel sets from ${KERNEL_SETS} into
#	${RELEASEDIR}/${MACHINE}/binary/sets
#
.for configfile in ${KERNEL_SETS}					# {
_KERNELSETS_TO_BUILD+=kernset-${configfile}
kernset-${configfile}:
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
		done ) | GZIP=-9 ${PAX} -O -zw \
		    -M -N ${NETBSDSRCDIR}/etc \
		    -f ${RELEASEDIR}/${MACHINE}/binary/sets/kern-${configfile}.tgz
.endfor	# KERNEL_SETS							# }

build_kernelsets: ${_KERNELSETS_TO_BUILD}

# build_releasekernels --
#	Build kernel.gz from ${KERNEL_SETS} ${EXTRA_KERNELS} into
#	${RELEASEDIR}/${MACHINE}/binary/kernel
#
.for configfile in ${KERNEL_SETS} ${EXTRA_KERNELS}			# {
_RELEASEKERNELS_TO_BUILD+=releasekern-${configfile}
releasekern-${configfile}:
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
		    ${RELEASEDIR}/${MACHINE}/binary/kernel/$${kernel}-${configfile:C/.*\///}.gz ; done
.endfor	# KERNEL_SETS EXTRA_KERNELS					# }

build_releasekernels: ${_RELEASEKERNELS_TO_BUILD}

# snap_kern --
#	build the kernels, then the sets & release kernels
#
snap_kern: check_DESTDIR check_RELEASEDIR .WAIT \
	    build_kernels .WAIT \
	    build_kernelsets build_releasekernels

.endif # !target(snap_kern)					# }


# snap_md_post --
#	Machine dependent distribution media operations.
#	Overridden by etc.$MACHINE/Makefile.inc
#
snap_md_post: check_DESTDIR check_RELEASEDIR
#	(empty -- look in the machine-dependent Makefile.inc)


clean:
	-rm -rf ${CDROM.dir} ${CDROM.pathlist}

.include <bsd.prog.mk>
