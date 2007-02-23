/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
	BigEndian = 4321,
	LittleEndian = 1234
};

static struct stat	dbst;	/* stat info of database file */
static BTREEINFO	bt;	/* btree information */
static int		verbose; /* how chatty are we? */
static DB		*db;	/* data base operations struct */

/* perform the stat operation */
/* if this is the root, then just synthesise the data */
/* otherwise, retrieve the data, and be sure to fill in the size */
/* we use the mtime of the database file for each of the records */
static int 
dbfs_getattr(const char *path, struct stat *st)
{
	int     res = 0;
	DBT	k;
	DBT	v;

	(void) memset(st, 0x0, sizeof(*st));
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {
		k.size = strlen(k.data = __UNCONST(path + 1));
		if ((*db->seq)(db, &k, &v, R_CURSOR) != 0) {
			return -ENOENT;
		}
		st->st_mode = S_IFREG | 0444;
		st->st_nlink = 1;
		st->st_size = v.size;
		st->st_mtime = dbst.st_mtime;
		st->st_atime = dbst.st_atime;
		st->st_ctime = dbst.st_ctime;
		st->st_uid = dbst.st_uid;
		st->st_gid = dbst.st_gid;
	}
	return res;
}

/* readdir operation */
/* run through the database file, returning records by key */
static int 
dbfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info * fi)
{
	static int	ret;
	static DBT	k;
	int	flag;
	DBT	v;
	(void) fi;

	if (offset == 0) {
		(void) memset(&k, 0x0, sizeof(k));
		(void) filler(buf, ".", NULL, 0);
		(void) filler(buf, "..", NULL, 0);
		flag = R_FIRST;
		ret = 0;
	} else {
		flag = R_CURSOR;
	}
	if (ret != 0) {
		return 0;
	}
	for ( ; (ret = (*db->seq)(db, &k, &v, flag)) == 0 ; flag = R_NEXT) {
		if (filler(buf, k.data, NULL, 0) != 0) {
			return 0;
		}
	}
	return 0;
}

/* open the file in the file system */
/* use seq method to get the record - get doesn't seem to work */
static int 
dbfs_open(const char *path, struct fuse_file_info * fi)
{
	DBT	k;
	DBT	v;

	k.size = strlen(k.data = __UNCONST(path));
	if ((*db->get)(db, &k, &v, 0) != 0) {
		return -ENOENT;
	}

	if ((fi->flags & 3) != O_RDONLY) {
		return -EACCES;
	}
	return 0;
}

/* read the file's contents in the file system */
/* use seq method to get the record - get doesn't seem to work */
static int 
dbfs_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	size_t	len;
	DBT	k;
	DBT	v;
	(void) fi;

	k.size = strlen(k.data = __UNCONST(path + 1));
	if ((*db->seq)(db, &k, &v, R_CURSOR) != 0) {
		return -ENOENT;
	}
	len = v.size;
	if (offset < len) {
		if (offset + size > len) {
			size = len - offset;
		}
		(void) memcpy(buf, (char *) v.data + offset, size);
		buf[size - 1] = '\n';
	} else {
		size = 0;
	}
	return size;
}

/* fill in the statvfs struct */
static int
dbfs_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	st->f_blocks = dbst.st_size / dbst.st_blksize;
	st->f_bsize = st->f_frsize = st->f_iosize = dbst.st_blksize;
	st->f_owner = dbst.st_uid;
	st->f_files = 1;
	return 0;
}

/* operations struct */
static struct fuse_operations dbfs_oper = {
	.getattr = dbfs_getattr,
	.readdir = dbfs_readdir,
	.open = dbfs_open,
	.read = dbfs_read,
	.statfs = dbfs_statfs
};

int 
main(int argc, char **argv)
{
	int	i;

	while ((i = getopt(argc, argv, "v")) != -1) {
		switch(i) {
		case 'v':
			verbose = 1;
			break;
		}
	}
	if (stat(argv[optind], &dbst) != 0) {
		err(EXIT_FAILURE, "can't stat `%s'", argv[optind]);
	}
	bt.lorder = LittleEndian;
	if ((db = dbopen(argv[optind], O_RDONLY, 0666, DB_BTREE, &bt)) == NULL) {
		bt.lorder = BigEndian;
		db = dbopen(argv[optind], O_RDONLY, 0666, DB_BTREE, &bt);
		if (verbose) {
			printf("Database not little endian - trying big endian now\n");
		}
	}
	if (db == NULL) {
		err(EXIT_FAILURE, "can't open db `%s'", argv[optind]);
	}
	return fuse_main(argc, argv, &dbfs_oper);
}
