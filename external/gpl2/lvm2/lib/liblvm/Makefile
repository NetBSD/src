#	$NetBSD: Makefile,v 1.4 2009/12/02 01:54:20 haad Exp $

LIBISPRIVATE=	yes

USE_FORT?=	no
NOLINT=		#defined

.include "../../lvm2tools.mk"

LIB=		lvm

CPPFLAGS+=	-I${LVM2_DISTDIR}/lib -I${LVM2_DISTDIR}/include \
		-I. 

# Some parts of liblvm can't be protected because if its
# alloca() usage.
#
.if defined(USE_SSP) && (${USE_SSP} != "no")
CPPFLAGS+=	-Wno-stack-protector
.endif

CPPFLAGS.imLcLkup.c=-Wno-error

SRCS+=		activate.c lvmcache.c toolcontext.c config.c btree.c \
		str_list.c dev-cache.c dev-io.c dev-md.c device.c dev-swap.c \
		display.c errseg.c filter-composite.c \
		filter-persistent.c \
		filter-regex.c filter-sysfs.c filter-md.c archive.c \
		archiver.c export.c flags.c format-text.c import.c \
		import_vsn1.c tags.c text_label.c freeseg.c label.c \
		file_locking.c locking.c no_locking.c log.c lv_manip.c \
		merge.c metadata.c mirror.c pv_manip.c pv_map.c segtype.c \
		snapshot_manip.c crc.c lvm-exec.c lvm-file.c lvm-string.c \
		lvm-wrappers.c timestamp.c util.c memlock.c report.c \
		striped.c unknown.c uuid.c zero.c disk-rep.c format1.c \
		import-export.c \
		import-extents.c layout.c lvm1-label.c vg_number.c \
		disk_rep.c format_pool.c import_export.c pool_label.c \
		filter_netbsd.c snapshot.c mirrored.c dev_manager.c fs.c dev.c \
		lvm-globals.c

LDADD+=		-lprop

.PATH:	${LVM2_DISTDIR}/lib/
.PATH:	${LVM2_DISTDIR}/lib/activate
.PATH:	${LVM2_DISTDIR}/lib/cache
.PATH:	${LVM2_DISTDIR}/lib/commands
.PATH:	${LVM2_DISTDIR}/lib/config
.PATH:	${LVM2_DISTDIR}/lib/datastruct
.PATH:	${LVM2_DISTDIR}/lib/device
.PATH:	${LVM2_DISTDIR}/lib/display
.PATH:	${LVM2_DISTDIR}/lib/error
.PATH:	${LVM2_DISTDIR}/lib/filters
.PATH:	${LVM2_DISTDIR}/lib/format1
.PATH:	${LVM2_DISTDIR}/lib/format_pool
.PATH:	${LVM2_DISTDIR}/lib/format_text
.PATH:	${LVM2_DISTDIR}/lib/freeseg
.PATH:	${LVM2_DISTDIR}/lib/label
.PATH:	${LVM2_DISTDIR}/lib/locking
.PATH:	${LVM2_DISTDIR}/lib/log
.PATH:	${LVM2_DISTDIR}/lib/metadata
.PATH:	${LVM2_DISTDIR}/lib/mirror
.PATH:	${LVM2_DISTDIR}/lib/misc
.PATH:	${LVM2_DISTDIR}/lib/mm
.PATH:	${LVM2_DISTDIR}/lib/report
.PATH:	${LVM2_DISTDIR}/lib/snapshot
.PATH:	${LVM2_DISTDIR}/lib/striped
.PATH:	${LVM2_DISTDIR}/lib/unknown
.PATH:	${LVM2_DISTDIR}/lib/uuid
.PATH:	${LVM2_DISTDIR}/lib/zero
.PATH:	${LVM2_DISTDIR}/lib/netbsd

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
