#	$NetBSD: Makefile,v 1.462 2022/05/28 12:36:56 nia Exp $
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
#   MKPOSTFIX; if not `no', install postfix configuration
#   MKUNPRIVED; if not `no', allow non-root installs.
#   MKUPDATE; if not `no', don't do a 'make clean' before kernel compile
#
# Targets:
#    distribution: makes a full NetBSD distribution in DESTDIR. If
#	INSTALL_DONE is set, it will not do a `make install.'
#	if DISTRIBUTION_DONE is set, it will not do anything.
#    distrib-dirs: creates an empty NetBSD directory tree in DESTDIR.
#	Called by distribution.
#    snapshot: calls distribution, above, and then tars up the files
#	into a release(7) format in RELEASEDIR/${RELEASEMACHINEDIR}.
#	Any port-dependent stuff for this target is found in
#	etc.${MACHINE}/Makefile.inc.
#    release: a synonym for `snapshot'
#

# For MK* vars
.include <bsd.own.mk>

.include <bsd.kernobj.mk>	# For KERNSRCDIR, KERNOBJDIR, ...
.include <bsd.endian.mk>	# For TARGET_ENDIANNESS

.MAKEOVERRIDES+=	USETOOLS

TZDIR=		/usr/share/zoneinfo
LOCALTIME?=	UTC
CKSUM?=		${TOOL_CKSUM}
MAKESUMS=	MAKE=${MAKE:Q} CKSUM=${CKSUM:Q} ${HOST_SH} ${NETBSDSRCDIR}/distrib/sets/makesums
DISTRIBVER!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh

GZIP_FLAGS= -9 ${GZIP_N_FLAG}

# Flags for creating ISO CDROM image
# mkisofs is expected to be in $PATH, install via pkgsrc/sysutils/cdrtools
# Note: At least mkisofs 2.0 should be used.
#
.if !defined(MKISOFS)
MKISOFS!=       (which mkisofs || echo true) 2>/dev/null
.endif
DISTRIBREV!=	${HOST_SH} ${KERNSRCDIR}/conf/osrelease.sh -s
# ISO 9660 volume ID.  Note that this can only contain [A-Z0-9_].
ISO_VOLID!=	echo NETBSD_${DISTRIBREV} | tr a-z A-Z
MKISOFS_FLAGS+= -J -l -hide-joliet-trans-tbl -r -T \
		-V ${ISO_VOLID} \
		-publisher "The NetBSD Project" \
		-m "${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/cdrom"
.if ${MKISOFS_FLAGS:N-v}
MKISOFS_FLAGS+=	-quiet
.endif
 
# Reproducible build parameters
.if ${MKREPRO:Uno} == "no"
BUILDER!=	echo $${USER-root}@$$(hostname)
.else
BUILDER=	builder@localhost.NetBSD.org
.endif
.if ${MKREPRO_TIMESTAMP:Uno} == "no"
PAX_TIMESTAMP=
BUILD_DATE!=	date -u
.else
PAX_TIMESTAMP=--timestamp "${MKREPRO_TIMESTAMP}"
BUILD_DATE!=	${TOOL_DATE} -u -r "${MKREPRO_TIMESTAMP}"
.endif

# MD Makefile.inc may append MD targets to BIN[123].  Make sure all
# are empty, to preserve the old semantics of setting them below with "+=".
#
BIN1=
BIN2=
BIN3=

# Directories to build in ${RELEASEDIR}/${RELEASEMACHINEDIR}.
# MD Makefile.inc files can add to this.
# NOTE: Parent directories must be listed before subdirectories.
#
INSTALLATION_DIRS=	binary binary/sets binary/kernel installation

.if exists(etc.${RELEASEMACHINE}/Makefile.inc)
.include "etc.${RELEASEMACHINE}/Makefile.inc"
.endif

# -rw-r--r--
BINOWN= root
BINGRP= wheel
UTMPGRP= utmp
BIN1+=	auto_master bootptab changelist csh.cshrc csh.login \
	csh.logout daily daily.conf dm.conf envsys.conf floppytab ftpchroot \
	ftpusers gettytab gpio.conf group hosts hosts.lpd inetd.conf \
	locate.conf login.conf mailer.conf man.conf monthly monthly.conf \
	named.conf netconfig networks newsyslog.conf \
	nsswitch.conf passwd.conf pkgpath.conf phones printcap \
	profile protocols rbootd.conf rc rc.conf rc.local rc.subr \
	rc.shutdown remote rpc security security.conf services shells \
	shrc sysctl.conf syslog.conf weekly weekly.conf wscons.conf

.if ${MKMROUTING} != "no"
BIN1+=	mrouted.conf
.endif

.if ${MKNTP} != "no"
BIN1+=	ntp.conf
.endif

# Use machine-specific disktab if it exists, or the MI one otherwise
.if exists(etc.${MACHINE}/disktab)
BIN1+=	etc.${MACHINE}/disktab
.else
BIN1+=	disktab
.endif

.if exists(etc.${MACHINE}/ld.so.conf) && \
    empty(MACHINE_ARCH:M*arm*hf*) && empty(MACHINE_ARCH:Maarch64*)
BIN1+=	etc.${MACHINE}/ld.so.conf
.endif

.if exists(etc.${MACHINE}/ttyaction)
BIN1+=	etc.${MACHINE}/ttyaction
.endif

# -rw-rw-r--
BIN2+=	motd
FILESBUILD_motd=	YES

# -rw-------
BIN3+=	hosts.equiv wpa_supplicant.conf

SYSPKG=	etc
ETC_PKG=-T etc_pkg
BASE_PKG=-T base_pkg
ETC_INSTALL_FILE=cd ${.CURDIR} && ${INSTALL_FILE} ${ETC_PKG}
ETC_INSTALL_OBJ_FILE=cd ${.OBJDIR} && ${INSTALL_FILE} ${ETC_PKG}

.if ${TARGET_ENDIANNESS} == "1234"
PWD_MKDB_ENDIAN=	-L
.elif ${TARGET_ENDIANNESS} == "4321"
PWD_MKDB_ENDIAN=	-B
.else
PWD_MKDB_ENDIAN=
.endif


# distribution --
#	Build a distribution
#
distribution: .PHONY .MAKE check_DESTDIR distrib-dirs
.if !defined(DISTRIBUTION_DONE)
.if !defined(INSTALL_DONE)
	${MAKEDIRTARGET} ${NETBSDSRCDIR} include _DISTRIB=
	${MAKEDIRTARGET} ${NETBSDSRCDIR} install _DISTRIB=
.endif	# !INSTALL_DONE
	${MAKEDIRTARGET} . install-debugkernels
	${MAKEDIRTARGET} . install-etc-files
. if ${MKX11} != "no"
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/external/mit/xorg distribution
. endif
. if ${MKEXTSRC} != "no"
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/extsrc distribution
. endif
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/distrib/sets makesetfiles
.endif	# !DISTRIBUTION_DONE


# motd is copied from a different ${MOTD_SOURCE} depending on DISTRIBVER
#
.if !empty(DISTRIBVER:M*.99.*)
MOTD_SOURCE=	motd.current
.elif !empty(DISTRIBVER:M*BETA*)
MOTD_SOURCE=	motd.beta
.elif !empty(DISTRIBVER:M*RC*)
MOTD_SOURCE=	motd.rc
.else
MOTD_SOURCE=	motd.default
.endif
CLEANFILES+= motd
motd: ${.CURDIR}/${MOTD_SOURCE} ${_NETBSD_VERSION_DEPENDS}
	${_MKTARGET_CREATE}
	${HOST_INSTALL_FILE} ${.CURDIR}/${MOTD_SOURCE} ${.TARGET}

CLEANFILES+=	MAKEDEV
MAKEDEV_MACHINE=${"${MACHINE_CPU}" == "aarch64":?${MACHINE_CPU}:${MACHINE}}
MAKEDEV: ${.CURDIR}/MAKEDEV.awk ${.CURDIR}/MAKEDEV.tmpl \
    ${.CURDIR}/etc.${MAKEDEV_MACHINE}/MAKEDEV.conf
	${_MKTARGET_CREATE}
	MACHINE=${MAKEDEV_MACHINE:Q} MACHINE_ARCH=${MACHINE_ARCH:Q} \
	    NETBSDSRCDIR=${NETBSDSRCDIR:Q} \
	    ${TOOL_AWK} -f ${.CURDIR}/MAKEDEV.awk ${.CURDIR}/MAKEDEV.tmpl \
	    > ${.TARGET}

.include "${NETBSDSRCDIR}/etc/Makefile.params"

CLEANFILES+=	etc-release
etc-release: .EXEC .MAKE
	${_MKTARGET_CREATE}
	@(	echo "NetBSD ${DISTRIBVER}/${MACHINE}"; \
		echo ; \
		cat ${NETBSDSRCDIR}/sys/conf/copyright; \
		echo ; \
		echo "Build information:"; \
		printf "%20s   %s\n" "Build date" "${BUILD_DATE}"; \
		printf "%20s   %s\n"  "Built by" "${BUILDER}"; \
		if [ -n "${BUILDID}" ]; then \
		    printf "%20s   %s\n"  "Build ID" "${BUILDID}" ; \
		fi ; \
		if [ -n "${BUILDINFO}" ]; then \
		    echo ; \
		    info="$$(printf "%b" ${BUILDINFO:Q})" ; \
		    printf "%s\n" "$${info}" \
		    | ${TOOL_SED} -e 's/^/        /' ; \
		fi ; \
		echo ; \
		echo "Build settings:"; \
		echo ; \
		${PRINT_PARAMS} ; \
	) >${.OBJDIR}/${.TARGET}

install-etc-release: .PHONY etc-release
	${_MKMSG_INSTALL} etc/release
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    etc-release ${DESTDIR}/etc/release


FILESDIR=		/etc
CONFIGFILES=
CONFIGSYMLINKS=

.for file in ${BIN1}
CONFIGFILES+=		${file}
FILESMODE_${file:T}=	644
.endfor

.for file in ${BIN2}
CONFIGFILES+=		${file}
FILESMODE_${file:T}=	664
.endfor

.for file in ${BIN3}
CONFIGFILES+=		${file}
FILESMODE_${file:T}=	600
.endfor

.if (${MKPOSTFIX} != "no")
CONFIGFILES+=		aliases
FILESDIR_aliases=	/etc/mail
FILESMODE_aliases=	644
.endif

CONFIGFILES+=		MAKEDEV.local
FILESDIR_MAKEDEV.local=	/dev
FILESMODE_MAKEDEV.local=${BINMODE}

CONFIGFILES+=		crontab
FILESDIR_crontab=	/var/cron/tabs
FILESNAME_crontab=	root
FILESMODE_crontab=	600

CONFIGFILES+=		minfree
FILESDIR_minfree=	/var/crash
FILESMODE_minfree=	600

CONFIGSYMLINKS+=	${TZDIR}/${LOCALTIME}	/etc/localtime \
			/usr/sbin/rmt		/etc/rmt


# install-etc-files --
#	Install etc (config) files; not performed by "make build"
#
install-etc-files: .PHONY .MAKE check_DESTDIR MAKEDEV
	${_MKMSG_INSTALL} ${DESTDIR}/etc/master.passwd
	${ETC_INSTALL_FILE} -o root -g wheel -m 600 \
	    master.passwd ${DESTDIR}/etc
	${TOOL_PWD_MKDB} -p ${PWD_MKDB_ENDIAN} -d ${DESTDIR}/ \
	    ${DESTDIR}/etc/master.passwd
.if ${MKUNPRIVED} != "no"
	( \
		mode=0600; \
		for metaent in spwd.db passwd pwd.db; do \
	    		echo "./etc/$${metaent} type=file mode=$${mode} uname=root gname=wheel tags=etc_pkg"; \
			mode=0644; \
		done; \
	) | ${METALOG.add}
.endif	# MKUNPRIVED != no
	${_MKMSG_INSTALL} ${DESTDIR}/etc/ttys
.if exists(${.CURDIR}/etc.${MACHINE}/ttys)
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${.CURDIR}/etc.${MACHINE}/ttys ${DESTDIR}/etc
.else
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${.CURDIR}/etc.${MACHINE_CPU}/ttys ${DESTDIR}/etc
.endif
.if exists(etc.${MACHINE}/boot.cfg)
	${_MKMSG_INSTALL} ${DESTDIR}/boot.cfg
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
	    ${.CURDIR}/etc.${MACHINE}/boot.cfg ${DESTDIR}/
.endif
	${_MKMSG_INSTALL} ${DESTDIR}/dev/MAKEDEV
	${ETC_INSTALL_OBJ_FILE} -o ${BINOWN} -g ${BINGRP} -m 555 \
	    MAKEDEV ${DESTDIR}/dev
.for owner group mode file in \
		${BINOWN} operator	664	/etc/dumpdates  \
		${BINOWN} operator	600	/etc/skeykeys \
		root wheel		600	/var/at/at.deny \
		root wheel		644	/var/db/locate.database \
		${BINOWN} ${BINGRP}	600	/var/log/authlog \
		root wheel		600	/var/log/cron \
		${BINOWN} ${UTMPGRP}	664	/var/log/lastlog \
		${BINOWN} ${UTMPGRP}	664	/var/log/lastlogx \
		${BINOWN} ${BINGRP}	640	/var/log/lpd-errs \
		${BINOWN} ${BINGRP}	600	/var/log/maillog \
		${BINOWN} ${BINGRP}	644	/var/log/messages \
		${BINOWN} ${BINGRP}	600	/var/log/secure \
		${BINOWN} ${UTMPGRP}	664	/var/log/wtmp \
		${BINOWN} ${UTMPGRP}	664	/var/log/wtmpx \
		${BINOWN} ${BINGRP}	600	/var/log/xferlog \
		daemon staff		664	/var/msgs/bounds \
		${BINOWN} ${UTMPGRP}	664	/var/run/utmp \
		${BINOWN} ${UTMPGRP}	664	/var/run/utmpx \
		games games		664	/var/games/atc_score \
		games games		664	/var/games/battlestar.log \
		games games		664	/var/games/cfscores \
		games games		664	/var/games/criblog \
		games games		660	/var/games/hackdir/perm \
		games games		660	/var/games/hackdir/record \
		games games		664	/var/games/larn/llog12.0 \
		games games		664	/var/games/larn/lscore12.0 \
		games games		664	/var/games/larn/playerids \
		games games		664	/var/games/robots_roll \
		games games		664	/var/games/rogue.scores \
		games games		664	/var/games/saillog \
		games games		664	/var/games/snakerawscores \
		games games		664	/var/games/snake.log \
		games games		664	/var/games/tetris.scores
	${_MKMSG_INSTALL} ${DESTDIR}${file}
	if [ ! -e ${DESTDIR}${file} -o -s ${DESTDIR}${file} ]; then \
		${ETC_INSTALL_FILE} -o ${owner} -g ${group} -m ${mode} \
			/dev/null ${DESTDIR}${file}; \
	else true; fi
.endfor
.for subdir in . defaults autofs bluetooth iscsi mtree namedb pam.d powerd rc.d root skel ssh
	${MAKEDIRTARGET} ${subdir} configinstall
.endfor
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/external/bsd/dhcpcd/sbin/dhcpcd configinstall
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/usr.bin/mail configinstall
.if (${MKPF} != "no")
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/usr.sbin/pf configinstall
.endif
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/crypto/external/bsd/openssh/bin configinstall
.if (${MKPOSTFIX} != "no")
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/external/ibm-public/postfix configinstall
.endif
.if (${MKATF} != "no")
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/external/bsd/atf/etc/atf configinstall
.endif
.if (${MKKYUA} != "no")
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/external/bsd/kyua-cli/etc/kyua configinstall
.endif


# install-obsolete-lists --
#	Install var/db/obsolete set lists; this is performed by "make build"
#
OBSOLETE.dir=		${.OBJDIR}/obsolete.dir
OBSOLETE.files=		base comp etc games gpufw man misc rescue text
.if ${MKDEBUG} != "no"
OBSOLETE.files+=	debug
.endif
.if ${MKDTB} != "no"
OBSOLETE.files+=	dtb
.endif
.if ${MKKMOD} != "no"
OBSOLETE.files+=	modules
.endif
.if ${MKATF} != "no"
OBSOLETE.files+=	tests
.endif
.if ${MKX11} != "no"
OBSOLETE.files+=	xbase xcomp xetc xfont xserver
.if ${MKDEBUG} != "no"
OBSOLETE.files+=	xdebug
.endif
.endif

# XXX make "makeobsolete" set wise; then generate files respectively
install-obsolete-lists: .PHONY .MAKE
	mkdir -p ${OBSOLETE.dir}
.if ${MKX11} != "no"
	(cd ${NETBSDSRCDIR}/distrib/sets && \
	    AWK=${TOOL_AWK:Q} MAKE=${MAKE:Q} ${HOST_SH} ./makeobsolete -b -t ${OBSOLETE.dir})
.else
	(cd ${NETBSDSRCDIR}/distrib/sets && \
	    AWK=${TOOL_AWK:Q} MAKE=${MAKE:Q} ${HOST_SH} ./makeobsolete -t ${OBSOLETE.dir})
.endif
.for file in ${OBSOLETE.files}
	${_MKMSG_INSTALL} ${DESTDIR}/var/db/obsolete/${file}
	if [ ! -e ${DESTDIR}/var/db/obsolete/${file} ] || \
	    ! cmp -s ${OBSOLETE.dir}/${file} ${DESTDIR}/var/db/obsolete/${file}; then \
		${ETC_INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 644 \
		    ${OBSOLETE.dir}/${file} ${DESTDIR}/var/db/obsolete; \
	else true; fi
.endfor


# distrib-dirs --
#	Populate $DESTDIR with directories needed by NetBSD
#
distrib-dirs: .PHONY check_DESTDIR
	cd ${NETBSDSRCDIR}/etc/mtree && ${MAKE} distrib-dirs

COMPRESS_PROGRAM=${"${USE_XZ_SETS:Uno}"!="no":?${TOOL_XZ}:${TOOL_GZIP}}
XZ_OPT=-9
TAR_SUFF=${"${USE_XZ_SETS:Uno}"!="no":?tar.xz:tgz}

# release, snapshot --
#	Build a full distribution including kernels & install media.
#
release snapshot: .PHONY .MAKE check_DESTDIR check_RELEASEDIR snap_md_post
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/distrib/sets sets
	${MAKESUMS} -A -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/sets \
		${KERNEL_SETS:@.SETS.@kern-${.SETS.}.${TAR_SUFF}@}
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel '*.gz'


# iso-image --
#	Standalone target to create a CDROM image after the release
#	was composed.  Should be run after "make release" in src and xsrc.
#	The do-iso-image is to be called from etc.$MACHINE/Makefile.inc
#
#	Note: At least mkisofs 2.0 should be used.
#
CDROM_NAME_ADD?=
CDROM_IMAGE?=${RELEASEDIR}/images/NetBSD-${DISTRIBVER}-${MACHINE}.iso
CDROM.dir=	${.OBJDIR}/cdrom.dir
CDROM.pathlist=	${.OBJDIR}/cdrom.pathlist

iso-image:

.if ${MKISOFS} != true
do-iso-image: .PHONY check_DESTDIR check_RELEASEDIR iso-image-md-post
	${MAKESUMS} -t ${RELEASEDIR}/images/ '*.iso'
	@echo "iso-image created as: ${CDROM_IMAGE}"
.else
do-iso-image:
	@echo iso-image: mkisofs not found
.endif

iso-image-setup: .PHONY check_RELEASEDIR
	rm -f ${CDROM.pathlist}
.for extra in README SOURCE_DATE source
.if exists(${RELEASEDIR}/${extra})
	echo "${extra}=${RELEASEDIR}/${extra}" >> ${CDROM.pathlist}
.endif
.endfor
	echo "${MACHINE}/=${RELEASEDIR}/${RELEASEMACHINEDIR}/" >> ${CDROM.pathlist}
	mkdir -p ${CDROM.dir}

check_imagedir:
	mkdir -p ${RELEASEDIR}/images

# iso-image-mi --
#	Create the image after the MD operations have completed.
#
iso-image-mi: .PHONY check_DESTDIR check_RELEASEDIR iso-image-md-pre check_imagedir
	@if ! ${MKISOFS} --version; then \
		echo "install pkgsrc/sysutils/cdrtools and run 'make iso-image'." ; \
		false; \
	fi
	${MKISOFS} ${MKISOFS_FLAGS} -graft-points -path-list ${CDROM.pathlist} \
	    -o ${CDROM_IMAGE} ${CDROM.dir}

# iso-image-md-pre --
#	Setup ${CDROM.dir} to produce a bootable CD image.
#	Overridden by etc.$MACHINE/Makefile.inc
#
iso-image-md-pre: .PHONY check_DESTDIR check_RELEASEDIR iso-image-setup
#	(empty -- look in the machine-dependent Makefile.inc)

# iso-image-md-post --
#	Fixup the CD-image to be bootable.
#	Overridden by etc.$MACHINE/Makefile.inc
#
iso-image-md-post: .PHONY check_DESTDIR check_RELEASEDIR iso-image-mi
#	(empty -- look in the machine-dependent Makefile.inc)


# live-image --
#	Standalone target to create live images after the release was composed.
#	Should be run after "make release" in src and xsrc.
#	LIVEIMG_RELEASEDIR specifies where to install live images and
#	it can be set in MD etc.${MACHINE}/Makefile.inc.
#
LIVEIMG_RELEASEDIR?= ${RELEASEDIR}/images

live-image: .PHONY check_DESTDIR check_RELEASEDIR
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/distrib live_image \
	    LIVEIMG_RELEASEDIR=${LIVEIMG_RELEASEDIR}
	${MAKESUMS} -t ${LIVEIMG_RELEASEDIR} '*.img.gz'

# install-image --
#	Standalone target to create installation images
#	after the release was composed.
#	Should be run after "make release" in src and xsrc.
#	INSTIMG_RELEASEDIR specifies where to install live images and
#	it can be set in MD etc.${MACHINE}/Makefile.inc.
#
INSTIMG_RELEASEDIR?= ${RELEASEDIR}/images

install-image: .PHONY check_DESTDIR check_RELEASEDIR
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/distrib install_image \
	    INSTIMG_RELEASEDIR=${INSTIMG_RELEASEDIR}
	${MAKESUMS} -t ${INSTIMG_RELEASEDIR} '*.img.gz'

# snap_pre --
#	Create ${RELEASEDIR} and necessary subdirectories.
#
snap_pre: .PHONY check_DESTDIR check_RELEASEDIR distribution
	${INSTALL} -d -m 755 ${RELEASEDIR}
.if ${MKUPDATE} == "no"
# Could be a mount point, ignore the errors
	-/bin/rm -rf ${RELEASEDIR}/${RELEASEMACHINEDIR}
.endif
	${INSTALL} -d -m 755 ${RELEASEDIR}/${RELEASEMACHINEDIR}
.for dir in ${INSTALLATION_DIRS}
	${INSTALL} -d -m 755 ${RELEASEDIR}/${RELEASEMACHINEDIR}/${dir}
.endfor

# snap_post --
#	Build the install media and notes from distrib
#
snap_post: .PHONY .MAKE build_kernelsets build_releasekernels
.if ${MKUPDATE} == "no"
	cd ${NETBSDSRCDIR}/distrib && ${MAKE} cleandir
.endif
	cd ${NETBSDSRCDIR}/distrib && ${MAKE} depend && ${MAKE} && \
	    ${MAKE} release

# build kernels --
#	This target builds the kernels specified by each port.
#	A port may specify the following kernels:
#
#	KERNEL_SETS		The list of kernels that will be
#				packaged into sets, named
#				kern-${kernel}.tgz (or .tar.xz).
#				These kernels are also placed in the
#				binary/kernel area of the release package
#				as netbsd-${kernel}.gz.
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
.export ALL_KERNELS

GETKERNELAWK=	${TOOL_AWK} '/^config/ {print $$2; found=1} \
		END{ if (found == 0) print "netbsd"; }'

build_kernels: .PHONY
#	Configure & compile kernels listed in ${ALL_KERNELS}
#
.if !defined(KERNELS_DONE)						# {
.for configfile in ${ALL_KERNELS:O:u}					# {
build_kernels: kern-${configfile}
kern-${configfile}: .PHONY .MAKE
	cd ${KERNCONFDIR} && ${TOOL_CONFIG} ${CONFIGOPTS} -s ${KERNSRCDIR} \
	    ${MKDEBUGKERNEL == "yes" :? -DDEBUG=-g :} \
	    -b ${KERNOBJDIR}/${configfile:C/.*\///} ${configfile}
.if ${MKUPDATE} == "no"
	${MAKE} -C ${KERNOBJDIR}/${configfile:C/.*\///} distclean
.endif
	${MAKE} -C ${KERNOBJDIR}/${configfile:C/.*\///} depend && \
	${MAKE} -C ${KERNOBJDIR}/${configfile:C/.*\///} && \
	${MAKE} -C ${KERNOBJDIR}/${configfile:C/.*\///} debuginstall
.endfor	# ALL_KERNELS							# }
.endif	# KERNELS_DONE							# }

build_kernelsets: .PHONY
#	Create kernel sets from ${KERNEL_SETS} into
#	${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/sets
#
.for configfile in ${KERNEL_SETS:O:u}					# {
.for configsel in ${ALL_KERNELS:O:u}
.if ${configfile} == ${configsel}
build_kernelsets: kernset-${configfile}
kernset-${configfile}: .PHONY build_kernels snap_pre
	@kernlist=$$(${GETKERNELAWK} ${KERNCONFDIR}/${configfile}); \
	kerndir=${KERNOBJDIR}/${configfile:C/.*\///}; \
	kernsuffixes="${KERNEL_SUFFIXES:S/^/./}"; \
	kern_tgz=${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/sets/kern-${configfile}.${TAR_SUFF}; \
	pax_cmd="GZIP=${GZIP_FLAGS:Q} XZ_OPT=${XZ_OPT:Q} ${TOOL_PAX} ${PAX_TIMESTAMP} --use-compress-program ${COMPRESS_PROGRAM:Q} -O -w -M -N ${NETBSDSRCDIR}/etc -f $${kern_tgz}"; \
	cd $${kerndir} && { \
		kernels=; newest=; \
		for kernel in $${kernlist}; do \
			for s in "" $${kernsuffixes}; do \
				ks="$${kernel}$${s}"; \
				[ -f $${ks} ] || continue; \
				kernels="$${kernels} $${ks}"; \
				[ -z "$${newest}" -o $${ks} \
				    -nt "$${newest}" ] && newest=$${ks}; \
				[ ${MKDEBUGKERNEL} = "no" -o \
				     ! -f $${ks}.gdb ] && continue; \
				kernels="$${kernels} $${ks}.gdb"; \
			done; \
		done; \
		[ $${kern_tgz} -nt "$${newest}" ] || { \
			echo "echo $${kernels} | $${pax_cmd}"; \
			if [ ${KERNEL_DIR} = "yes" ]; then \
				tmpdir=kernel$$; \
				trap "rm -fr $${tmpdir}" 0 1 2 3 15; \
				mkdir -p $${tmpdir}/netbsd; \
				d=./netbsd; \
				cd $${tmpdir}; \
			else \
				d=.; \
			fi; \
			( echo "/set uname=${BINOWN} gname=${BINGRP}"; \
			echo ". type=dir optional"; \
			if [ ${KERNEL_DIR} = "yes" ]; then \
				echo "./netbsd type=dir optional"; \
			fi; \
			for kernel in $${kernels}; do \
				if [ ${KERNEL_DIR} = "yes" ]; then \
					newname=$$(echo $${kernel} | \
					    ${TOOL_SED} -e s/netbsd/kernel/); \
					ln ../$${kernel} ./netbsd/$${newname}; \
				else \
					newname=$${kernel}; \
				fi; \
				echo "$${d}/$${newname} type=file"; \
			done ) | eval $${pax_cmd}; \
			if [ ${KERNEL_DIR} = "yes" ]; then \
				mv $${kern_tgz} ..; \
			fi; \
		} \
	}
.endif
.endfor
.endfor	# KERNEL_SETS							# }

build_releasekernels: .PHONY
#	Build kernel.gz from ${KERNEL_SETS} ${EXTRA_KERNELS} into
#	${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel
#
.for configfile in ${KERNEL_SETS:O:u} ${EXTRA_KERNELS:O:u}		# {
.for configsel in ${ALL_KERNELS:O:u}
.if ${configfile} == ${configsel}
build_releasekernels: releasekern-${configfile}
releasekern-${configfile}: .PHONY build_kernels snap_pre
	@kernlist=$$(${GETKERNELAWK} ${KERNCONFDIR}/${configfile:C/.*\///}); \
	kerndir=${KERNOBJDIR}/${configfile:C/.*\///}; \
	kernsuffixes="${KERNEL_SUFFIXES:S/^/./}"; \
	dest="${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel"; \
	cd $${kerndir} && {	\
		for kernel in $${kernlist}; do \
			for s in "" $${kernsuffixes}; do \
				ks="$${kernel}$${s}"; \
				[ ! -f $${ks} ] && continue; \
				knl_gz="$${dest}/$${kernel}-${configfile:C/.*\///}$${s}.gz"; \
				[ $${knl_gz} -nt $${ks} ] && continue; \
				rm -f $${knl_gz}; \
				echo "${TOOL_GZIP} ${GZIP_FLAGS} -c < $${kerndir}/$${ks} > $${knl_gz}"; \
				${TOOL_GZIP} ${GZIP_FLAGS} -c < $${ks} > $${knl_gz}; \
			done; \
		done; \
	}
.endif
.endfor
.endfor	# KERNEL_SETS EXTRA_KERNELS					# }

# install-debugkernels:
#	Create fake kernel debug files to satisfy the sets requirements
#	They will be later replaced with the real ones if we are building
#	kernels. This mess is needed because checkflist runs before the
#	kernels are built. It should really run before the sets are build.
install-debugkernels:
.if ${MKDEBUG:Uno} == "yes"
	@for i in $$(${NETBSDSRCDIR}/distrib/sets/makeflist debug | \
	    ${TOOL_GREP} -E '^./usr/libdata/debug/(nfs)?netbsd-'); do \
		echo touch ${DESTDIR}/$$i; \
		touch ${DESTDIR}/$$i; \
	done
.	for k in ${ALL_KERNELS}
install-debugkernels: ${DESTDIR}${DEBUGDIR}/netbsd-${k}.debug
		
${DESTDIR}${DEBUGDIR}/netbsd-${k}.debug:
	${_MKTARGET_INSTALL}
	touch ${.TARGET}
.	endfor
.endif

# snap_md_post --
#	Machine dependent distribution media operations.
#	Overridden by etc.$MACHINE/Makefile.inc
#
snap_md_post: .PHONY check_DESTDIR check_RELEASEDIR snap_post
#	(empty -- look in the machine-dependent Makefile.inc)


clean:
	-rm -rf ${CDROM.dir} ${CDROM.pathlist} ${OBSOLETE.dir}

SUBDIR=	defaults rc.d mtree

.include <bsd.prog.mk>
.include <bsd.subdir.mk>

test:
	@echo ${OBSOLETE.files}
