# $NetBSD: Makefile,v 1.49 2009/12/05 16:29:10 pooka Exp $
#
# Consult "*.conf" for the configuration variables; this Makefile is typically
# not edited for basic configuration changes.
##############################################################################

all:	cdimages

.include <bsd.own.mk>

.if exists(site.conf)
.include "site.conf"
.endif

.if !defined(RELEASE)
.BEGIN:
	@echo "Please set RELEASE in the environment or in site.conf to the basename"
	@echo "of the release configuration file to use.  For example:"
	@echo "> ${MAKE} RELEASE=foo"
	@echo "will use the configuration file foo.conf."
	@false
.endif

.if exists(${RELEASE}.conf)
.include "${RELEASE}.conf"
.elif !target(.BEGIN)
.BEGIN:
	@echo "Error: File does not exist: ${RELEASE}.conf"
	@false
.endif

CD_RELEASE?=
CD_SIZE?=		650000
RELEASENAME?=		NetBSD-${CD_RELEASE}
MKISOFS_ARGS=		-A "NetBSD ${CD_RELEASE}" \
			-hide-rr-moved -m Split -m cdrom
#			-hide-list ${.CURDIR}/hide-iso.lst

ALL_PORTS=
.for image in ${CD_IMAGES}
BASE_PORTS.${image}?=
ALL_PORTS:=		${ALL_PORTS} ${BASE_PORTS.${image}}
.endfor

.if exists(${RELEASENAME}.mk)
.include "${RELEASENAME}.mk"
.elif !target(.BEGIN)
.if !empty(CD_RELEASE)
.BEGIN:
	@echo "Error: File does not exist: ${RELEASENAME}.mk"
	@echo "(specified by variable CD_RELEASE in ${RELEASE}.conf)"
	@false
.else
.BEGIN:
	@echo "Error: CD_RELEASE not set in ${RELEASE}.conf"
	@false
.endif
.endif

SUBDIR=

.if !empty(ALL_PORTS:Mmacppc)
SUBDIR+=	macppc_installboot
.endif

.if !empty(SUBDIR)
.include <bsd.subdir.mk>
.endif

.include <bsd.obj.mk>
.include <bsd.own.mk>

##### Default paths for programs #####

MKISOFS?=	/usr/local/bin/mkisofs
RSYNC?=		/usr/pkg/bin/rsync
ALPHA_IBOOT?=	${TOOL_INSTALLBOOT} -m alpha -o sunsum,append
PMAX_IBOOT?=	${TOOL_INSTALLBOOT} -m pmax -o sunsum,append
VAX_IBOOT?=	${TOOL_INSTALLBOOT} -m vax -o sunsum,append
.if !empty(ALL_PORTS:Mmacppc)
MACPPC_IBOOTDIR!= cd ${.CURDIR}/macppc_installboot && ${PRINTOBJDIR}
.endif
.if !empty(ALL_PORTS:Msgimips)
SGIMIPS_SGIVOL?=	${TOOLDIR}/bin/nbsgivol
.endif

check_mkisofs:
.if !exists(${MKISOFS})
	@echo "Error: File does not exist: ${MKISOFS}"
	@echo "Please build and install mkisofs(8) utility and set MKISOFS in the environment"
	@echo "or in site.conf to create image."
	@false
.endif

check_rsync:
.if !exists(${RSYNC})
	@echo "Error: File does not exist: ${RSYNC}"
	@echo "Please build and install rsync(1) utility and set RSYNC in the environment"
	@echo "or in site.conf to fetch release binaries."
	@false
.endif

##### rsync settings #####

# rsync host; check mirrors on http://www.NetBSD.org/mirrors/#rsync
# (note not all rsync mirrors provide NetBSD-daily)
RSYNC_HOST?=	rsync.NetBSD.org

# extra path for rsync mirrors ("/pub", "/netbsd" etc.)
RSYNC_PATH_PREFIX?=

.if !target(.BEGIN)
.if ${RELEASE} == "current"
# daily date dir name
BRANCH?=	HEAD
#DAILY_DIR?=	yyyymmddhhmmZ
RSYNC_PATH?=	${RSYNC_PATH_PREFIX}/NetBSD-daily/${BRANCH}/${DAILY_DIR}
.else
# extra suffix on pre-release ("_BETA2", "_RC5" etc.)
RELEASE_SUFFIX?=
RSYNC_PATH?=	${RSYNC_PATH_PREFIX}/NetBSD/${RELEASENAME}${RELEASE_SUFFIX}

# alternative for NetBSD-daily sets of stable branches
#DAILY_DIR?=	yyyymmddhhmmZ
#BRANCH?=	netbsd-4
#RSYNC_PATH?=	${RSYNC_PATH_PREFIX}/NetBSD-daily/${BRANCH}/${DAILY_DIR}
.endif
.endif

RSYNC_SITE?=	rsync://${RSYNC_HOST}${RSYNC_PATH}
RSYNC_ARGS?=	-va --delete
RSYNC_EXCLUDE+=	--exclude=/* --exclude=Split --exclude=cdrom

DOWNLOADDIR?=	${.OBJDIR}/download
DISTRIBDIR?=	${DOWNLOADDIR}
STAGEDIR?=	${.OBJDIR}/staging
IMAGEDIR?=	${.OBJDIR}
EXTFILEDIR?=	${.OBJDIR}/extfiles
EASTER_EGG?=	/dev/null

##### MACHINE_ARCH for possible shared sets for each (shared) port #####

MACHINE_ARCH.acorn26=	arm
MACHINE_ARCH.acorn32=	arm
MACHINE_ARCH.algor=	mipsel
MACHINE_ARCH.amiga=	m68k
MACHINE_ARCH.amigappc=	powerpc
MACHINE_ARCH.arc=	mipsel
MACHINE_ARCH.atari=	m68k
MACHINE_ARCH.bebox=	powerpc
MACHINE_ARCH.cats=	arm
MACHINE_ARCH.cesfic=	m68k
MACHINE_ARCH.cobalt=	mipsel
MACHINE_ARCH.dreamcast=	sh3el
MACHINE_ARCH.evbarm=	arm
MACHINE_ARCH.evbmips=	mipsel
MACHINE_ARCH.evbppc=	powerpc
MACHINE_ARCH.evbsh3=	sh3eb
MACHINE_ARCH.hp300=	m68k
MACHINE_ARCH.hpcarm=	arm
MACHINE_ARCH.hpcmips=	mipsel
MACHINE_ARCH.hpcsh=	sh3el
MACHINE_ARCH.iyonix=	arm
MACHINE_ARCH.luna68k=	m68k
MACHINE_ARCH.mac68k=	m68k
MACHINE_ARCH.macppc=	powerpc
MACHINE_ARCH.mipsco=	mipseb
MACHINE_ARCH.mmeye=	sh3eb
MACHINE_ARCH.mvme68k=	m68k
MACHINE_ARCH.netwinder=	arm
MACHINE_ARCH.news68k=	m68k
MACHINE_ARCH.newsmips=	mipseb
MACHINE_ARCH.next68k=	m68k
MACHINE_ARCH.ofppc=	powerpc
MACHINE_ARCH.pmax=	mipsel
MACHINE_ARCH.prep=	powerpc
MACHINE_ARCH.sandpoint=	powerpc
MACHINE_ARCH.sbmips=	mipseb
MACHINE_ARCH.sgimips=	mipseb
MACHINE_ARCH.shark=	arm
MACHINE_ARCH.sun3=	m68k
MACHINE_ARCH.x68k=	m68k

##### Fetched directories from FTP server #####

.if defined(ALL_PORTS) && !empty(ALL_PORTS)
RSYNC_INCLUDE+=		--include "/shared"
RSYNC_EXCLUDE+=		--exclude "/[a-z]*" \
			--exclude "/shared/[a-z]*"
.endif

RSYNC_ARCHS=
.for port in ${ALL_PORTS:O:u}			# {
RSYNC_INCLUDE+=		--include "/${port}"
.if defined(MACHINE_ARCH.${port})
RSYNC_ARCHS+=		${MACHINE_ARCH.${port}}
.endif
.endfor						# }
.for arch in ${RSYNC_ARCHS:O:u}
RSYNC_INCLUDE+=		--include "/shared/${arch}"
.endfor

##### Boot info and additional options for various platforms #####

.if defined(PUBLISHER)
MKISOFS_ARGS+=		-publisher "${PUBLISHER}"
.endif

.if defined(PREPARER)
MKISOFS_ARGS+=		-p "${PREPARER}"
.endif


##### Per image settings #####

.for image in ${CD_IMAGES}			# {
ports:=${BASE_PORTS.${image}}

.if !defined(VOLID.${image}) && !target(.BEGIN)
.BEGIN:
	@echo "Error: VOLID.${image} not set in ${RELEASE}.conf"
	@false
.endif

MKISOFS_ARGS.${image}=	-V "${VOLID.${image}}"

pathlist.${image}:=${STAGEDIR}/${image}.pathlist
contents.${image}:=${IMAGEDIR}/${image}.contents

MKISOFS_ARGS.${image}+=	-graft-points -path-list ${pathlist.${image}}

.if defined(ABSTRACT.${image})
MKISOFS_ARGS.${image}+=	-abstract ${ABSTRACT.${image}}
.endif

# Options specified in release config file

EXTFILES?=
INTFILES.${image}?=
INTDIRS.${image}?=
ADDFILES.${image}?=

bports=			# empty first
# For a source only cd just ignore the shared/ALL directory
.if ${ports} != "source" && exists(${DISTRIBDIR}/shared/ALL)
bports+=		shared/ALL
.endif
.for port in ${ports}					# {
EXTFILES+=		${EXTFILES.${port}}
INTFILES.${image}+=	${INTFILES.${port}}
INTDIRS.${image}+=	${INTDIRS.${port}}
ADDFILES.${image}+=	${ADDFILES.${port}}
MKISOFS_ARGS.${image}+=	${MKISOFS_ARGS.${port}}
# add shared/ALL and shared/${MACHINE_ARCH} to BASE_PORTS.${image}
bports+=		${port}
.if defined(MACHINE_ARCH.${port}) &&	\
    exists(${DISTRIBDIR}/shared/${MACHINE_ARCH.${port}})
bports+=		shared/${MACHINE_ARCH.${port}}
.endif
.endfor							# }
BASE_PORTS.${image}:=	${bports:O:u}


# i386

.if !empty(ports:Mi386)
.if empty(INTFILES.i386:Mbootxx_cd9660)
# non-emulation boot
MKISOFS_ARGS.${image}+=	-b ${BOOTFILE.i386} -c boot.catalog -no-emul-boot
.else
#2.88MB emulation boot
MKISOFS_ARGS.${image}+=	-b ${BOOTFILE.i386} -c boot.catalog
.endif
.endif

# amd64

.if !empty(ports:Mamd64)
.if empty(INTFILES.amd64:Mbootxx_cd9660)
# non-emulation boot
MKISOFS_ARGS.${image}+=	-b ${BOOTFILE.amd64} -c boot.catalog -no-emul-boot
.else
#2.88MB emulation boot
MKISOFS_ARGS.${image}+= -b ${BOOTFILE.amd64} -c boot.catalog
.endif
.endif
        
# Mac (mac68k, macppc)

.if !empty(ports:Mmacppc)
BOOTHFS=	boothfs
MKISOFS_ARGS.${image}+=	-hfs -part -hide-hfs-list ${.CURDIR}/hide-hfs.lst \
	--macbin -map ${.CURDIR}/hfsmap.lst \
	-boot-hfs-file ${.OBJDIR}/${BOOTHFS}
UUDECODE_FILES=	${BOOTHFS}
.include <bsd.files.mk>
.elif defined(USE_APPLE_ISO) || !empty(ports:Mmac68k)
MKISOFS_ARGS.${image}+=	-apple --macbin -map ${.CURDIR}/hfsmap.lst
.endif

# Sun (sparc, sparc64, sun3)

SUN_BOOT_ARGS.${image}:=
.if !empty(ports:Msparc*) || !empty(ports:Msun3*)	# {
SUN_BOOT.123:=	-
SUN_BOOT.4:=	-
SUN_BOOT.5:=	-

.if !empty(ports:Msparc)
SUN_BOOT.123:=	${BOOTFILE.sparc}
.endif

.if !empty(ports:Msun3)					# {
.if !defined(BOOTFILE.sun3x)
# NetBSD >= 1.4; sun3 and sun3x are the same
SUN_BOOT.5:=	${BOOTFILE.sun3}
.endif
SUN_BOOT.4:=	${BOOTFILE.sun3}
.endif							# }

.if !empty(ports:Msun3x) && defined(BOOTFILE.sun3x)
# NetBSD < 1.4; sun3 and sun3x are separate
SUN_BOOT.5:=	${BOOTFILE.sun3x}
.endif

.if !empty(ports:Msparc64)				# {
.if (${SUN_BOOT.5} != "-")
# add warning
SUN_BOOT.5:=	${SUN_BOOT.5} ${BOOTFILE.sparc64}
.else
SUN_BOOT.5:=	${BOOTFILE.sparc64}
.endif
.endif							# }

SUN_BOOT_ARGS.${image}:= ${SUN_BOOT.123} ${SUN_BOOT.123} ${SUN_BOOT.123} ${SUN_BOOT.4} ${SUN_BOOT.5}

.if empty(SUN_BOOT_ARGS.${image}:N/dev/null)
SUN_BOOT_ARGS.${image}:=
.endif
.endif							# }

.endfor		# image in ${CD_IMAGES}		# }

##### Additional options for host OS's #####

# (USE_APPLE_ISO is tested above)

.if defined(USE_ROCK_RIDGE)
MKISOFS_ARGS+=		-r
.endif

.if defined(USE_LONG_NAMES)
MKISOFS_ARGS+=		-l
.endif

.if defined(USE_TRANS_TBL)
MKISOFS_ARGS+=		-T
.endif

.if defined(USE_JOLIET)
MKISOFS_ARGS+=		-J -hide-joliet-list ${.CURDIR}/hide-jol.lst
.if defined(USE_TRANS_TBL)
MKISOFS_ARGS+=		-hide-joliet-trans-tbl
.endif
.endif

##### File extract rule #####

USE-FILE: .USE
	@if [ ! -r $@ ]; then case $> in \
		/*,link) \
			echo "cp ${>:S/,link$//} $@"; \
			cp ${>:S/,link$//} $@;; \
		*,link) \
			echo "cp `pwd -P`/${>:S/,link$//} $@"; \
			cp `pwd -P`/${>:S/,link$//} $@;; \
		*.tgz,*) \
			echo "Extracting: $> --> $@..."; \
			${TOOL_PAX} -rnz -f ${>:C/,.*$//} ${>:C/^.*,//} ; \
			cp ${>:C/^.*,//} $@; rm -rf ${>:C/^.*,//} ;; \
		*.gz) echo "gunzip -c <$> >$@"; gunzip -c <$> >$@;; \
		*) echo "Unknown archive method for $@"; false;; \
	esac; else true; fi

##### Makefile rules #####

.for image in ${CD_IMAGES}			# {
cdimages: ${IMAGEDIR}/${image}.iso

stage-${image}:
	mkdir -p ${STAGEDIR}/${image} ${EXTFILEDIR}
	rm -f ${pathlist.${image}} ${contents.${image}}
.if defined(BASE_PORTS.${image}) && !empty(BASE_PORTS.${image})
.for dir in ${INTDIRS.${image}}
	mkdir -p ${STAGEDIR}/${image}/${dir}
.endfor
.for arch in ${BASE_PORTS.${image}}
	@echo "${arch}=${DISTRIBDIR}/${arch}" >> ${pathlist.${image}}
	@echo "${arch}" >> ${contents.${image}}
.endfor
.endif

extfileprep:
.for file in ${EXTFILES}			# {
extfileprep: ${EXTFILEDIR}/${file:C/:.*$//}
${EXTFILEDIR}/${file:C/:.*$//}: ${DISTRIBDIR}/${file:C/^.*://} USE-FILE
${DISTRIBDIR}/${file:C/^.*://}: .PHONY
.endfor						# }

fileprep-${image}:
.for file in ${INTFILES.${image}}		# {
fileprep-${image}: ${STAGEDIR}/${image}/${file:C/:.*$//}
${STAGEDIR}/${image}/${file:C/:.*$//}: ${DISTRIBDIR}/${file:C/^.*://} USE-FILE
${DISTRIBDIR}/${file:C/^.*://}: .PHONY
.endfor						# }
.for file in ${ADDFILES.${image}}		# {
fileprep-${image}: ${STAGEDIR}/${image}/${file:C/:.*$//}
${STAGEDIR}/${image}/${file:C/:.*$//}: ${file:C/^.*://} USE-FILE
${file:C/^.*://}: .PHONY
.endfor						# }

.if !empty(BASE_PORTS.${image}:Mmacppc)
fileprep-${image}: ${BOOTHFS}
.endif

# Size the CD image.  This is done via the following formula:
# 1. Size the image produced by mkisofs.
# 2. Add images added by distrib/common/sunbootcd.sh by rounding to a 320k
#    boundary and adding each Sun image rounded to a 320k boundary.
# 3. Add bootfile sizes rounded up to 512 bytes for pmax and vax.
# 4. Round up to a 32k boundary, then add another 32k for TAO padding.

SGIVOLHDR.size= 4096

size-${image}: stage-${image} extfileprep fileprep-${image} check_mkisofs
	@size=$$((`cd ${STAGEDIR}/${image} && \
		   ${MKISOFS} ${MKISOFS_ARGS} ${MKISOFS_ARGS.${image}} \
			-print-size . 2>&1 \
		   | tee /dev/stderr \
		   | ${TOOL_SED} '/=/!d;s/^[^=]*=//'` * 2048)) && \
	if [ "${SUN_BOOT_ARGS.${image}}" != "" ]; then \
		size=$$(($$(($$size + 327679)) / 327680 * 327680)) && \
		bootfiles=`for f in ${SUN_BOOT_ARGS.${image}:N-:O}; do echo $$f; done | uniq` && \
		for f in $$bootfiles; do \
			bfsize=$$(${TOOL_STAT} -f '%z' $$f) && \
			size=$$(($$size + $$(($$(($$bfsize + 327679)) / 327680 * 327680)))); \
		done; \
	fi && \
	if [ "${BASE_PORTS.${image}:Mpmax}" != "" ]; then \
		size=$$(($$size + $$(($$(($$(${TOOL_STAT} -f '%z' ${BOOTFILE.pmax}) + 511)) / 512 * 512)))); \
	fi && \
	if [ "${BASE_PORTS.${image}:Mvax}" != "" ]; then \
		size=$$(($$size + $$(($$(($$(${TOOL_STAT} -f '%z' ${BOOTFILE.vax})  + 511)) / 512 * 512)))); \
	fi && \
	if [ "${BASE_PORTS.${image}:Msgimips}" != "" -a \
	    "${EXTFILES.sgimips}" != "" ]; then \
		size=$$(($$size + ${SGIVOLHDR.size} * 512)); \
	fi && \
	size=$$(($$(($$size + 16383)) / 16384 * 16384 + 32768)) && \
	sizek=$$(($$size / 1024)) && \
	echo "Projected size of ${image}.iso: $$size bytes ($${sizek}K)." && \
	if [ $$sizek -gt ${CD_SIZE} ]; then echo "Image too large for ${CD_SIZE} KB CD!"; false; fi

# Actually build the image with all the bootstrap goo....

.if !empty(BASE_PORTS.${image}:Mmacppc)
${IMAGEDIR}/${image}.iso: all-macppc_installboot
.endif

${IMAGEDIR}/${image}.iso: size-${image} check_mkisofs
	@sort -o ${contents.${image}} ${contents.${image}}
.if defined(LOG_MKISOFS)
	cd ${STAGEDIR}/${image} && ${MKISOFS} -o $@ ${MKISOFS_ARGS} ${MKISOFS_ARGS.${image}} -v -v . >${IMAGEDIR}/${image}.iso.log 2>&1
.else
	cd ${STAGEDIR}/${image} && ${MKISOFS} -o $@ ${MKISOFS_ARGS} ${MKISOFS_ARGS.${image}} .
.endif
.if !empty(SUN_BOOT_ARGS.${image})
	SUNLABEL=${TOOL_SUNLABEL:Q} \
	    ${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
	    ${.TARGET} ${SUN_BOOT_ARGS.${image}}
.endif
.if !empty(BASE_PORTS.${image}:Mmacppc)
	${MACPPC_IBOOTDIR}/macppc_installboot \
	    $@ ${EXTFILEDIR}/macppc.bootxx /ofwboot
.endif
.if !empty(BASE_PORTS.${image}:Mpmax)
	${PMAX_IBOOT} $@ ${BOOTFILE.pmax}
.endif
.if !empty(BASE_PORTS.${image}:Mvax)
	${VAX_IBOOT} $@ ${BOOTFILE.vax}
.endif
.if !empty(BASE_PORTS.${image}:Malpha)
	${ALPHA_IBOOT} $@ ${BOOTFILE.alpha}
.endif
.if !empty(BASE_PORTS.${image}:Msgimips) && defined(EXTFILES.sgimips)
	@echo "Prepending SGI volume header"
	mv $@ $@.raw
	dd if=/dev/zero of=$@.tmp bs=512 count=${SGIVOLHDR.size}
	dd if=$@.raw of=$@.tmp bs=512 seek=${SGIVOLHDR.size}
	${SGIMIPS_SGIVOL} -f -i -h ${SGIVOLHDR.size} $@.tmp
	${SGIMIPS_SGIVOL} -f -w aoutboot ${STAGEDIR}/${image}/aoutboot $@.tmp
	${SGIMIPS_SGIVOL} -f -w ip2xboot ${STAGEDIR}/${image}/ip2xboot $@.tmp
	${SGIMIPS_SGIVOL} -f -w ip3xboot ${STAGEDIR}/${image}/ip3xboot $@.tmp
	mv $@.tmp $@
	rm -f $@.raw $@.tmp
.endif
	@echo Rounding up to 32k boundary and padding 32k....
	@size=$$(${TOOL_STAT} -f '%z' $@); \
	    pad=$$(( 32768 - ( $$size & 32767 ) )); \
	    [ $$pad = 32768 ] || \
	    dd if=/dev/zero bs=1 count=$$pad >>$@ 2>/dev/null

.endfor		# image in ${CD_IMAGES}		# }

fetch: check_rsync
	@mkdir -p ${DISTRIBDIR}
	@echo Fetching distributions....
	${RSYNC} ${RSYNC_INCLUDE} ${RSYNC_ARGS} ${RSYNC_EXCLUDE} ${RSYNC_SITE}/ ${DISTRIBDIR}/

clean: cleanstagedir cleanimages
cleandir distclean: cleandownloaddir

cleanimages:
.for image in ${CD_IMAGES}
	-rm -f ${IMAGEDIR}/${image}.iso ${IMAGEDIR}/${image}.iso.log \
		${contents.${image}}
.endfor

cleanstagedir:
	-rm -rf ${STAGEDIR} ${EXTFILEDIR}

cleandownloaddir:
	-rm -rf ${DOWNLOADDIR}
