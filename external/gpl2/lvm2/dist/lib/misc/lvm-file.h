/*	$NetBSD: lvm-file.h,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LVM_FILE_H
#define _LVM_FILE_H

/*
 * Create a temporary filename, and opens a descriptor to the file.
 */
int create_temp_name(const char *dir, char *buffer, size_t len, int *fd,
		     unsigned *seed);

/*
 * NFS-safe rename of a temporary file to a common name, designed
 * to avoid race conditions and not overwrite the destination if
 * it exists.
 */
int lvm_rename(const char *old, const char *new);

/*
 * Return 1 if path exists else return 0
 */
int path_exists(const char *path);
int dir_exists(const char *path);

/*
 * Return 1 if dir is empty
 */
int is_empty_dir(const char *dir);

/* Sync directory changes */
void sync_dir(const char *file);

/* fcntl locking wrappers */
int fcntl_lock_file(const char *file, short lock_type, int warn_if_read_only);
void fcntl_unlock_file(int lockfd);

#define is_same_inode(buf1, buf2) \
  ((buf1).st_ino == (buf2).st_ino && \
   (buf1).st_dev == (buf2).st_dev)

/*
 * Close the specified stream, taking care to detect and diagnose any write
 * error.  If there is an error, use the supplied file name in a diagnostic
 * that is reported via log_error or log_sys_error, as appropriate.
 * Use this function to close a stream when you've written data to it via
 * unchecked fprintf, fputc, etc. calls.  Return 0 on success, EOF on failure.
 */
int lvm_fclose(FILE *fp, const char *filename);

#endif
