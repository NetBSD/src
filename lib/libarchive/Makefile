#	$NetBSD: Makefile,v 1.2 2007/08/06 12:31:16 joerg Exp $

USE_FORT?=	yes # complex string handling

.include <bsd.own.mk>

LIBARCHIVEDIR=	${NETBSDSRCDIR}/dist/libarchive
.PATH:		${LIBARCHIVEDIR}/libarchive

LIB=		archive

LIBDPLIBS+=	bz2	${.CURDIR}/../libbz2 \
		z	${.CURDIR}/../libz

SRCS=		archive_check_magic.c \
		archive_entry.c \
		archive_entry_copy_stat.c \
		archive_entry_stat.c \
		archive_read.c \
		archive_read_data_into_fd.c \
		archive_read_extract.c \
		archive_read_open_fd.c \
		archive_read_open_file.c \
		archive_read_open_filename.c \
		archive_read_open_memory.c \
		archive_read_support_compression_all.c \
		archive_read_support_compression_bzip2.c \
		archive_read_support_compression_compress.c \
		archive_read_support_compression_gzip.c \
		archive_read_support_compression_none.c \
		archive_read_support_compression_program.c \
		archive_read_support_format_all.c \
		archive_read_support_format_ar.c \
		archive_read_support_format_cpio.c \
		archive_read_support_format_empty.c \
		archive_read_support_format_iso9660.c \
		archive_read_support_format_tar.c \
		archive_read_support_format_zip.c \
		archive_string.c \
		archive_string_sprintf.c \
		archive_util.c \
		archive_virtual.c \
		archive_write.c \
		archive_write_disk.c \
		archive_write_disk_set_standard_lookup.c \
		archive_write_open_fd.c \
		archive_write_open_file.c \
		archive_write_open_filename.c \
		archive_write_open_memory.c \
		archive_write_set_compression_bzip2.c \
		archive_write_set_compression_gzip.c \
		archive_write_set_compression_none.c \
		archive_write_set_compression_program.c \
		archive_write_set_format.c \
		archive_write_set_format_ar.c \
		archive_write_set_format_by_name.c \
		archive_write_set_format_cpio.c \
		archive_write_set_format_cpio_newc.c \
		archive_write_set_format_pax.c \
		archive_write_set_format_shar.c \
		archive_write_set_format_ustar.c \
		filter_fork.c

INCS=		archive.h archive_entry.h
INCSDIR=	/usr/include

DPSRCS+=	archive.h
CLEANFILES+=	archive.h

CPPFLAGS+=	-I${.CURDIR} -DPLATFORM_CONFIG_H=\"config_netbsd.h\"

MAN=		archive_entry.3 \
		archive_read.3 \
		archive_util.3 \
		archive_write.3 \
		archive_write_disk.3 \
		libarchive.3 \
		libarchive-formats.5 tar.5 \
		libarchive_internals.3

MLINKS+=	archive_entry.3 archive_entry_acl_add_entry.3
MLINKS+=	archive_entry.3 archive_entry_acl_add_entry_w.3
MLINKS+=	archive_entry.3 archive_entry_acl_clear.3
MLINKS+=	archive_entry.3 archive_entry_acl_count.3
MLINKS+=	archive_entry.3 archive_entry_acl_next.3
MLINKS+=	archive_entry.3 archive_entry_acl_next_w.3
MLINKS+=	archive_entry.3 archive_entry_acl_reset.3
MLINKS+=	archive_entry.3 archive_entry_acl_text_w.3
MLINKS+=	archive_entry.3 archive_entry_clear.3
MLINKS+=	archive_entry.3 archive_entry_clone.3
MLINKS+=	archive_entry.3 archive_entry_copy_fflags_text_w.3
MLINKS+=	archive_entry.3 archive_entry_copy_gname.3
MLINKS+=	archive_entry.3 archive_entry_copy_gname_w.3
MLINKS+=	archive_entry.3 archive_entry_copy_hardlink_w.3
MLINKS+=	archive_entry.3 archive_entry_copy_pathname_w.3
MLINKS+=	archive_entry.3 archive_entry_copy_stat.3
MLINKS+=	archive_entry.3 archive_entry_copy_symlink_w.3
MLINKS+=	archive_entry.3 archive_entry_copy_uname.3
MLINKS+=	archive_entry.3 archive_entry_copy_uname_w.3
MLINKS+=	archive_entry.3 archive_entry_dev.3
MLINKS+=	archive_entry.3 archive_entry_devmajor.3
MLINKS+=	archive_entry.3 archive_entry_devminor.3
MLINKS+=	archive_entry.3 archive_entry_filetype.3
MLINKS+=	archive_entry.3 archive_entry_fflags.3
MLINKS+=	archive_entry.3 archive_entry_fflags_text.3
MLINKS+=	archive_entry.3 archive_entry_free.3
MLINKS+=	archive_entry.3 archive_entry_gid.3
MLINKS+=	archive_entry.3 archive_entry_gname.3
MLINKS+=	archive_entry.3 archive_entry_gname_w.3
MLINKS+=	archive_entry.3 archive_entry_hardlink.3
MLINKS+=	archive_entry.3 archive_entry_ino.3
MLINKS+=	archive_entry.3 archive_entry_mode.3
MLINKS+=	archive_entry.3 archive_entry_mtime.3
MLINKS+=	archive_entry.3 archive_entry_mtime_nsec.3
MLINKS+=	archive_entry.3 archive_entry_nlink.3
MLINKS+=	archive_entry.3 archive_entry_new.3
MLINKS+=	archive_entry.3 archive_entry_pathname.3
MLINKS+=	archive_entry.3 archive_entry_pathname_w.3
MLINKS+=	archive_entry.3 archive_entry_rdev.3
MLINKS+=	archive_entry.3 archive_entry_rdevmajor.3
MLINKS+=	archive_entry.3 archive_entry_rdevminor.3
MLINKS+=	archive_entry.3 archive_entry_set_atime.3
MLINKS+=	archive_entry.3 archive_entry_set_ctime.3
MLINKS+=	archive_entry.3 archive_entry_set_dev.3
MLINKS+=	archive_entry.3 archive_entry_set_devmajor.3
MLINKS+=	archive_entry.3 archive_entry_set_devminor.3
MLINKS+=	archive_entry.3 archive_entry_set_fflags.3
MLINKS+=	archive_entry.3 archive_entry_set_gid.3
MLINKS+=	archive_entry.3 archive_entry_set_gname.3
MLINKS+=	archive_entry.3 archive_entry_set_hardlink.3
MLINKS+=	archive_entry.3 archive_entry_set_link.3
MLINKS+=	archive_entry.3 archive_entry_set_mode.3
MLINKS+=	archive_entry.3 archive_entry_set_mtime.3
MLINKS+=	archive_entry.3 archive_entry_set_nlink.3
MLINKS+=	archive_entry.3 archive_entry_set_pathname.3
MLINKS+=	archive_entry.3 archive_entry_set_rdev.3
MLINKS+=	archive_entry.3 archive_entry_set_rdevmajor.3
MLINKS+=	archive_entry.3 archive_entry_set_rdevminor.3
MLINKS+=	archive_entry.3 archive_entry_set_size.3
MLINKS+=	archive_entry.3 archive_entry_set_symlink.3
MLINKS+=	archive_entry.3 archive_entry_set_uid.3
MLINKS+=	archive_entry.3 archive_entry_set_uname.3
MLINKS+=	archive_entry.3 archive_entry_size.3
MLINKS+=	archive_entry.3 archive_entry_stat.3
MLINKS+=	archive_entry.3 archive_entry_symlink.3
MLINKS+=	archive_entry.3 archive_entry_uid.3
MLINKS+=	archive_entry.3 archive_entry_uname.3
MLINKS+=	archive_entry.3 archive_entry_uname_w.3
MLINKS+=	archive_read.3 archive_read_data.3
MLINKS+=	archive_read.3 archive_read_data_block.3
MLINKS+=	archive_read.3 archive_read_data_into_buffer.3
MLINKS+=	archive_read.3 archive_read_data_into_fd.3
MLINKS+=	archive_read.3 archive_read_data_skip.3
MLINKS+=	archive_read.3 archive_read_extract.3
MLINKS+=	archive_read.3 archive_read_extract_set_progress_callback.3
MLINKS+=	archive_read.3 archive_read_extract_set_skip_file.3
MLINKS+=	archive_read.3 archive_read_finish.3
MLINKS+=	archive_read.3 archive_read_new.3
MLINKS+=	archive_read.3 archive_read_next_header.3
MLINKS+=	archive_read.3 archive_read_open.3
MLINKS+=	archive_read.3 archive_read_open2.3
MLINKS+=	archive_read.3 archive_read_open_FILE.3
MLINKS+=	archive_read.3 archive_read_open_fd.3
MLINKS+=	archive_read.3 archive_read_open_file.3
MLINKS+=	archive_read.3 archive_read_open_filename.3
MLINKS+=	archive_read.3 archive_read_open_memory.3
MLINKS+=	archive_read.3 archive_read_support_compression_all.3
MLINKS+=	archive_read.3 archive_read_support_compression_bzip2.3
MLINKS+=	archive_read.3 archive_read_support_compression_compress.3
MLINKS+=	archive_read.3 archive_read_support_compression_gzip.3
MLINKS+=	archive_read.3 archive_read_support_compression_none.3
MLINKS+=	archive_read.3 archive_read_support_compression_program.3
MLINKS+=	archive_read.3 archive_read_support_format_all.3
MLINKS+=	archive_read.3 archive_read_support_format_cpio.3
MLINKS+=	archive_read.3 archive_read_support_format_iso9660.3
MLINKS+=	archive_read.3 archive_read_support_format_tar.3
MLINKS+=	archive_read.3 archive_read_support_format_zip.3
MLINKS+=	archive_util.3 archive_clear_error.3
MLINKS+=	archive_util.3 archive_compression.3
MLINKS+=	archive_util.3 archive_compression_name.3
MLINKS+=	archive_util.3 archive_errno.3
MLINKS+=	archive_util.3 archive_error_string.3
MLINKS+=	archive_util.3 archive_format.3
MLINKS+=	archive_util.3 archive_format_name.3
MLINKS+=	archive_util.3 archive_set_error.3
MLINKS+=	archive_write.3 archive_write_data.3
MLINKS+=	archive_write.3 archive_write_finish.3
MLINKS+=	archive_write.3 archive_write_finish_entry.3
MLINKS+=	archive_write.3 archive_write_get_bytes_in_last_block.3
MLINKS+=	archive_write.3 archive_write_get_bytes_per_block.3
MLINKS+=	archive_write.3 archive_write_header.3
MLINKS+=	archive_write.3 archive_write_new.3
MLINKS+=	archive_write.3 archive_write_open.3
MLINKS+=	archive_write.3 archive_write_open_FILE.3
MLINKS+=	archive_write.3 archive_write_open_fd.3
MLINKS+=	archive_write.3 archive_write_open_file.3
MLINKS+=	archive_write.3 archive_write_open_filename.3
MLINKS+=	archive_write.3 archive_write_open_memory.3
MLINKS+=	archive_write.3 archive_write_set_bytes_in_last_block.3
MLINKS+=	archive_write.3 archive_write_set_bytes_per_block.3
MLINKS+=	archive_write.3 archive_write_set_callbacks.3
MLINKS+=	archive_write.3 archive_write_set_compression_bzip2.3
MLINKS+=	archive_write.3 archive_write_set_compression_gzip.3
MLINKS+=	archive_write.3 archive_write_set_compression_none.3
MLINKS+=	archive_write.3 archive_write_set_compression_program.3
MLINKS+=	archive_write.3 archive_write_set_format_pax.3
MLINKS+=	archive_write.3 archive_write_set_format_shar.3
MLINKS+=	archive_write.3 archive_write_set_format_ustar.3
MLINKS+=	archive_write_disk.3 archive_write_disk_new.3
MLINKS+=	archive_write_disk.3 archive_write_disk_set_group_lookup.3
MLINKS+=	archive_write_disk.3 archive_write_disk_set_options.3
MLINKS+=	archive_write_disk.3 archive_write_disk_set_skip_file.3
MLINKS+=	archive_write_disk.3 archive_write_disk_set_standard_lookup.3
MLINKS+=	archive_write_disk.3 archive_write_disk_set_user_lookup.3
MLINKS+=	libarchive.3 archive.3

ARCHIVE_VERSION= 		2.2.6
ARCHIVE_API_MAJOR=	${ARCHIVE_VERSION:C/([0-9]*).*/\1/}
ARCHIVE_API_MINOR=	${ARCHIVE_VERSION:C/[0-9]*\.([0-9]*).*/\1/}
ARCHIVE_API_REV=	${ARCHIVE_VERSION:C/[0-9]*\.[0-9]*\.([0-9]*).*/\1/}

.if !empty(ARCHIVE_API_MINOR:M?)
ARCHIVE_API_MINOR_=	00${ARCHIVE_API_MINOR}
.elif !empty(ARCHIVE_API_MINOR:M??)
ARCHIVE_API_MINOR_=	0${ARCHIVE_API_MINOR}
.else
ARCHIVE_API_MINOR_=	${ARCHIVE_API_MINOR}
.endif
.if !empty(ARCHIVE_API_REV:M?)
ARCHIVE_API_REV_=	00${ARCHIVE_API_REV}
.elif !empty(ARCHIVE_API_REV:M??)
ARCHIVE_API_REV_=	0${ARCHIVE_API_REV}
.else
ARCHIVE_API_REV_=	${ARCHIVE_API_REV}
.endif
ARCHIVE_VERSION_STAMP=	\
	${ARCHIVE_API_MAJOR}${ARCHIVE_API_MINOR_}${ARCHIVE_API_REV_}

# Build archive.h from archive.h.in by substituting version information.
# Note: NetBSD has inttypes.h, so enable that include in archive.h.in
archive.h:	archive.h.in
	sed -e 's/@ARCHIVE_VERSION@/${ARCHIVE_VERSION}/g' \
	    -e 's/@SHLIB_MAJOR@/${ARCHIVE_API_MAJOR}/g' \
	    -e 's/@ARCHIVE_API_MAJOR@/${ARCHIVE_API_MAJOR}/g' \
	    -e 's/@ARCHIVE_API_MINOR@/${ARCHIVE_API_MINOR}/g' \
	    -e 's/@ARCHIVE_VERSION_STAMP@/${ARCHIVE_VERSION_STAMP}/g' \
	    -e 's|@ARCHIVE_H_INCLUDE_INTTYPES_H@|#include <inttypes.h>  /* For int64_t */|g' \
		< ${LIBARCHIVEDIR}/libarchive/archive.h.in > ${.TARGET}

.include <bsd.lib.mk>
