/*	$NetBSD: util.h,v 1.3 2025/10/26 12:51:50 andvar Exp $	*/

/* Create test image and filesystem, record superblock locations */
void create_lfs(size_t, size_t, int, int);

/* Write a well-known byte pattern into a file, appending if it exists */
int write_file(const char *, off_t, int);

/* Check the byte pattern and size of the file */
int check_file(const char *, int);

/* Check the file system for consistency */
int fsck(void);

/* Run dumplfs; for debugging */
void dumplfs(void);

#define MAXLINE 132
#define CHUNKSIZE 300
#define SEGSIZE 32768

#define IMGNAME "disk.img"
#define FAKEBLK "/dev/blk"
#define LOGFILE "newfs.log"

#define MP "/mp"

extern long long sbaddr[2];
