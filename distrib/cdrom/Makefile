# $NetBSD: Makefile,v 1.5.2.1 2002/06/15 02:19:39 lukem Exp $
#
# Consult "*.conf" for the configuration variables; this Makefile is typically
# not edited for basic configuration changes.
##############################################################################

all:

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
RELEASENAME=		NetBSD-${CD_RELEASE}
MKISOFS_ARGS=		-f -A "NetBSD ${CD_RELEASE}" \
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

##### Default paths for programs #####

MKISOFS?=	/usr/pkg/bin/mkisofs
MKSUNBOOTCD?=	/usr/src/distrib/utils/mksunbootcd/mksunbootcd
RSYNC?=		/usr/pkg/bin/rsync
ALPHA_IBOOT?=	/sys/arch/alpha/stand/installboot/installboot
PMAX_IBOOT?=	/sys/arch/pmax/stand/installboot/installboot
VAX_IBOOT?=	/sys/arch/vax/stand/installboot/installboot
.if !empty(ALL_PORTS:Mmacppc)
MACPPC_IBOOTDIR!= cd ${.CURDIR}/macppc_installboot && ${PRINTOBJDIR}
.endif

RSYNC_SITE?=	rsync://rsync.netbsd.org/NetBSD/
RSYNC_ARGS?=	-va --delete
RSYNC_EXCLUDE+=	--exclude=/* --exclude=Split --exclude=cdrom

DISTDIR?=	${.OBJDIR}/download
STAGEDIR?=	${.OBJDIR}/staging
IMAGEDIR?=	${.OBJDIR}
EXTFILEDIR?=	${.OBJDIR}/extfiles
EASTER_EGG?=	/dev/null

RELEASE_SUBDIR?=${RELEASENAME}

##### MACHINE_ARCH for possible shared sets for each port #####

MACHINE_ARCH.amiga=	m68k
MACHINE_ARCH.amigappc=	powerpc
MACHINE_ARCH.arc=	mipsel
MACHINE_ARCH.atari=	m68k
MACHINE_ARCH.bebox=	powerpc
MACHINE_ARCH.cobalt=	mipsel
MACHINE_ARCH.hp300=	m68k
MACHINE_ARCH.hpcmips=	mipsel
MACHINE_ARCH.luna68k=	m68k
MACHINE_ARCH.mac68k=	m68k
MACHINE_ARCH.macppc=	powerpc
MACHINE_ARCH.mipsco=	mipseb
MACHINE_ARCH.mvme68k=	m68k
MACHINE_ARCH.news68k=	m68k
MACHINE_ARCH.newsmips=	mipseb
MACHINE_ARCH.next68k=	m68k
MACHINE_ARCH.ofppc=	powerpc
MACHINE_ARCH.pmax=	mipsel
MACHINE_ARCH.prep=	powerpc
MACHINE_ARCH.sgimips=	mipseb
MACHINE_ARCH.sun3=	m68k
MACHINE_ARCH.sun3x=	m68k
MACHINE_ARCH.x68k=	m68k

##### Fetched directories from FTP server #####

.if defined(ALL_PORTS) && !empty(ALL_PORTS)
RSYNC_INCLUDE+=		--include "/${RELEASENAME}/" \
			--include "/${RELEASENAME}/shared/"
RSYNC_EXCLUDE+=		--exclude "/${RELEASENAME}/[a-z]*" \
			--exclude "/${RELEASENAME}/shared/[a-z]*/"
.endif

.for port in ${ALL_PORTS}
RSYNC_INCLUDE+=		--include "/${RELEASENAME}/${port}/"
.if defined(MACHINE_ARCH.${port})
RSYNC_INCLUDE+=		--include "/${RELEASENAME}/${MACHINE_ARCH.${port}}/" \
			--include "/${RELEASENAME}/shared/${MACHINE_ARCH.${port}}/"
.endif
.endfor

##### Boot info and additional options for various platforms #####

.if defined(PUBLISHER)
MKISOFS_ARGS+=		-P "${PUBLISHER}"
.endif

.if defined(PREPARER)
MKISOFS_ARGS+=		-p "${PREPARER}"
.endif

.for image in ${CD_IMAGES}
ports:=${BASE_PORTS.${image}}

.if !defined(VOLID.${image}) && !target(.BEGIN)
.BEGIN:
	@echo "Error: VOLID.${image} not set in ${RELEASE}.conf"
	@false
.endif

MKISOFS_ARGS.${image}=	-V "${VOLID.${image}}"

.if defined(ABSTRACT.${image})
MKISOFS_ARGS.${image}+=	-abstract ${ABSTRACT.${image}}
.endif

# Options specified in release config file

EXTFILES?=
INTFILES.${image}?=
INTDIRS.${image}?=
ADDFILES.${image}?=

.for port in ${ports}
EXTFILES+=		${EXTFILES.${port}}
INTFILES.${image}+=	${INTFILES.${port}}
INTDIRS.${image}+=	${INTDIRS.${port}}
ADDFILES.${image}+=	${ADDFILES.${port}}
MKISOFS_ARGS.${image}+=	${MKISOFS_ARGS.${port}}
.endfor

# i386

.if !empty(ports:Mi386)
MKISOFS_ARGS.${image}+=	-b ${BOOTFILE.i386} -c boot.catalog
.endif

# Mac (mac68k, macppc)

.if !empty(ports:Mmacppc)
MKISOFS_ARGS.${image}+=	-hfs -part -hide-hfs-list ${.CURDIR}/hide-hfs.lst \
			--macbin -map ${.CURDIR}/hfsmap.lst \
			-boot-hfs-file ${MACPPC_IBOOTDIR}/macppcboot.raw
.elif defined(USE_APPLE_ISO) || !empty(ports:Mmac68k)
MKISOFS_ARGS.${image}+=	-apple --macbin -map ${.CURDIR}/hfsmap.lst
.endif

# Sun (sparc, sparc64, sun3)

SUN_BOOT_ARGS.${image}:=
.if !empty(ports:Msparc*) || !empty(ports:Msun3*)
SUN_BOOT.123:=	/dev/null
SUN_BOOT.4:=	/dev/null
SUN_BOOT.5:=	/dev/null

.if !empty(ports:Msparc)
SUN_BOOT.123:=	${BOOTFILE.sparc}
.endif

.if !empty(ports:Msun3)
.if !defined(BOOTFILE.sun3x)
# NetBSD >= 1.4; sun3 and sun3x are the same
SUN_BOOT.5:=	${BOOTFILE.sun3}
.endif
SUN_BOOT.4:=	${BOOTFILE.sun3}
.endif

.if !empty(ports:Msun3x) && defined(BOOTFILE.sun3x)
# NetBSD < 1.4; sun3 and sun3x are separate
SUN_BOOT.5:=	${BOOTFILE.sun3x}
.endif

.if !empty(ports:Msparc64)
.if (${SUN_BOOT.5} != "/dev/null")
# add warning
SUN_BOOT.5:=	${SUN_BOOT.5} ${BOOTFILE.sparc64}
.else
SUN_BOOT.5:=	${BOOTFILE.sparc64}
.endif
.endif

SUN_BOOT_ARGS.${image}:= ${SUN_BOOT.123} ${SUN_BOOT.123} ${SUN_BOOT.123} ${SUN_BOOT.4} ${SUN_BOOT.5}

.if empty(SUN_BOOT_ARGS.${image}:N/dev/null)
SUN_BOOT_ARGS.${image}:=
.endif
.endif

.endfor

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
			echo "ln -sf ${>:S/,link$//} $@"; \
			ln -sf ${>:S/,link$//} $@;; \
		*,link) \
			echo "ln -sf `pwd -P`/${>:S/,link$//} $@"; \
			ln -sf `pwd -P`/${>:S/,link$//} $@;; \
		*.tgz,*) \
			echo "Extracting: $> --> $@..."; \
			tar --fast-read -xzOf ${>:C/,.*$//} ${>:C/^.*,//} >$@;; \
		*.gz) echo "gunzip -c <$> >$@"; gunzip -c <$> >$@;; \
		*) echo "Unknown archive method for $@"; false;; \
	esac; else true; fi

##### Makefile rules #####

.for image in ${CD_IMAGES}
all: ${IMAGEDIR}/${image}.iso

stage-${image}:
	mkdir -p ${STAGEDIR}/${image} ${EXTFILEDIR}
.if defined(BASE_PORTS.${image}) && !empty(BASE_PORTS.${image})
.if !empty(RELEASE_SUBDIR)
	mkdir -p ${STAGEDIR}/${image}/${RELEASE_SUBDIR}
.endif
.for dir in ${INTDIRS.${image}}
	mkdir -p ${STAGEDIR}/${image}/${dir}
.endfor
.for arch in ${BASE_PORTS.${image}}
	ln -sf `cd ${DISTDIR} && pwd -P`/${RELEASENAME}/${arch} ${STAGEDIR}/${image}/${RELEASE_SUBDIR}/
.endfor
.endif

extfileprep:
.for file in ${EXTFILES}
extfileprep: ${EXTFILEDIR}/${file:C/:.*$//}
${EXTFILEDIR}/${file:C/:.*$//}: ${DISTDIR}/${RELEASENAME}/${file:C/^.*://} USE-FILE
${DISTDIR}/${RELEASENAME}/${file:C/^.*://}: .PHONY
.endfor

fileprep-${image}:
.for file in ${INTFILES.${image}}
fileprep-${image}: ${STAGEDIR}/${image}/${file:C/:.*$//}
${STAGEDIR}/${image}/${file:C/:.*$//}: ${DISTDIR}/${RELEASENAME}/${file:C/^.*://} USE-FILE
${DISTDIR}/${RELEASENAME}/${file:C/^.*://}: .PHONY
.endfor
.for file in ${ADDFILES.${image}}
fileprep-${image}: ${STAGEDIR}/${image}/${file:C/:.*$//}
${STAGEDIR}/${image}/${file:C/:.*$//}: ${file:C/^.*://} USE-FILE
${file:C/^.*://}: .PHONY
.endfor

# Size the CD image.  This is done via the following formula:
# 1. Size the image produced by mkisofs.
# 2. Add images added by mksunbootcd by rounding to a 320k boundary and
#    adding each Sun image rounded to a 320k boundary.
# 3. Add 64k if macppc is included (ofwboot munged to one 64k block).
# 4. Add bootfile sizes rounded up to 512 bytes for pmax and vax.
# 5. Round up to a 32k boundary, then add another 32k for TAO padding.

.if !empty(BASE_PORTS.${image}:Mmacppc)
size-${image}: all-macppc_installboot
.endif

size-${image}: stage-${image} extfileprep fileprep-${image}
	@size=$$((`cd ${STAGEDIR}/${image} && ${MKISOFS} ${MKISOFS_ARGS} ${MKISOFS_ARGS.${image}} -print-size . 2>&1 | tee /dev/stderr | sed '/=/!d;s/^[^=]*=//'` * 2048)) && \
	if [ "${SUN_BOOT_ARGS.${image}}" != "" ]; then \
		size=$$(($$(($$size + 327679)) / 327680 * 327680)) && \
		bootfiles=`for f in ${SUN_BOOT_ARGS.${image}:N/dev/null:O}; do echo $$f; done | uniq` && \
		for f in $$bootfiles; do \
			bfsize=`ls -l $$f | awk '{print $$5}'` && \
			size=$$(($$size + $$(($$(($$bfsize + 327679)) / 327680 * 327680)))); \
		done; \
	fi && \
	if [ "${BASE_PORTS.${image}:Mmacppc}" != "" ]; then \
		size=$$(($$size + 65536)); \
	fi && \
	if [ "${BASE_PORTS.${image}:Mpmax}" != "" ]; then \
		size=$$(($$size + $$(($$((`ls -l ${BOOTFILE.pmax} | awk '{print $$5}'` + 511)) / 512 * 512)))); \
	fi && \
	if [ "${BASE_PORTS.${image}:Mvax}" != "" ]; then \
		size=$$(($$size + $$(($$((`ls -l ${BOOTFILE.vax} | awk '{print $$5}'` + 511)) / 512 * 512)))); \
	fi && \
	size=$$(($$(($$size + 16383)) / 16384 * 16384 + 32768)) && \
	sizek=$$(($$size / 1024)) && \
	echo "Projected size of ${image}.iso: $$size bytes ($${sizek}K)." && \
	if [ $$sizek -gt 666000 ]; then echo "Image too large for 74-minute CD!"; false; fi

# Actually build the image with all the bootstrap goo....

${IMAGEDIR}/${image}.iso: fetch size-${image}
.if defined(LOG_MKISOFS)
	cd ${STAGEDIR}/${image} && ${MKISOFS} -o $@ ${MKISOFS_ARGS} ${MKISOFS_ARGS.${image}} -v -v . >${IMAGEDIR}/${image}.iso.log 2>&1
.else
	cd ${STAGEDIR}/${image} && ${MKISOFS} -o $@ ${MKISOFS_ARGS} ${MKISOFS_ARGS.${image}} .
.endif
.if !empty(SUN_BOOT_ARGS.${image})
	${MKSUNBOOTCD} -c 640 $@ ${SUN_BOOT_ARGS.${image}}
.endif
.if !empty(BASE_PORTS.${image}:Mmacppc)
	@blknum=$$((`ls -l $@ | awk '{print $$5}'` / 512)) && \
	${MACPPC_IBOOTDIR}/macppc_installboot $@ $$blknum 756
	dd if=${EXTFILEDIR}/macppc.ofwboot bs=64k count=1 conv=sync >>$@ 2>/dev/null
.endif
.if !empty(BASE_PORTS.${image}:Mpmax)
	${PMAX_IBOOT} -sa $@ ${BOOTFILE.pmax}
.endif
.if !empty(BASE_PORTS.${image}:Mvax)
	${VAX_IBOOT} -sa $@ ${BOOTFILE.vax}
.endif
.if !empty(BASE_PORTS.${image}:Malpha)
	${ALPHA_IBOOT} -s $@ ${BOOTFILE.alpha}
.endif
	@echo Rounding up to 32k boundary and padding 32k....
	@size=`ls -l $@ | awk '{print $$5}'` && \
	newsize=$$(($$(($$size + 32767)) / 32768 * 32768)) && \
	dd if=/dev/zero bs=512 count=$$(($$(($$newsize - $$size)) / 512)) >>$@ 2>/dev/null
	dd if=${EASTER_EGG} bs=32768 count=1 conv=sync >>$@ 2>/dev/null

.endfor

fetch:
.if !defined(NO_FETCH)
	@mkdir -p ${DISTDIR}
	@echo Fetching distributions....
	@${RSYNC} ${RSYNC_INCLUDE} ${RSYNC_ARGS} ${RSYNC_EXCLUDE} ${RSYNC_SITE} ${DISTDIR}
.endif

clean: cleanstagedir cleanimages
cleandir distclean: cleandistdir

cleanimages:
.for image in ${CD_IMAGES}
	-rm -f ${IMAGEDIR}/${image}.iso ${IMAGEDIR}/${image}.iso.log
.endfor

cleanstagedir:
	-rm -rf ${STAGEDIR} ${EXTFILEDIR}

cleandistdir:
	-rm -rf ${DISTDIR}
