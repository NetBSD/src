/*	$NetBSD: util.c,v 1.1 2025/10/13 00:44:35 perseant Exp $	*/

#include <sys/mount.h>

#include <ctype.h>
#include <fcntl.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"
#include "util.h"

long long sbaddr[2] = { -1, -1 };

/* Create filesystem, note superblock locations */
void create_lfs(size_t imgsize, size_t fssize, int width, int do_setup)
{
	FILE *pipe;
	char cmd[MAXLINE];
	char buf[MAXLINE];
	
	/* Create image file larger than filesystem */
	sprintf(cmd, "dd if=/dev/zero of=%s bs=512 count=%zd",
		IMGNAME, imgsize);
	if (system(cmd) == -1)
		atf_tc_fail_errno("create image failed");

	/* Create filesystem */
	fprintf(stderr, "* Create file system\n");
	sprintf(cmd, "newfs_lfs -D -F -s %zd -w%d ./%s > %s",
		fssize, width, IMGNAME, LOGFILE);
	if (system(cmd) == -1)
		atf_tc_fail_errno("newfs failed");
	pipe = fopen(LOGFILE, "r");
	if (pipe == NULL)
		atf_tc_fail_errno("newfs failed to execute");
	while (fgets(buf, MAXLINE, pipe) != NULL) {
		if (sscanf(buf, "%lld,%lld", sbaddr, sbaddr + 1) == 2)
			break;
	}
	while (fgets(buf, MAXLINE, pipe) != NULL)
		;
	fclose(pipe);
	if (sbaddr[0] < 0 || sbaddr[1] < 0)
		atf_tc_fail("superblock not found");
	fprintf(stderr, "* Superblocks at %lld and %lld\n",
		sbaddr[0], sbaddr[1]);

	if (do_setup) {
		/* Set up rump */
		rump_init();
		if (rump_sys_mkdir(MP, 0777) == -1)
			atf_tc_fail_errno("cannot create mountpoint");
		rump_pub_etfs_register(FAKEBLK, IMGNAME, RUMP_ETFS_BLK);
	}
}

/* Write some data into a file */
int write_file(const char *filename, off_t len, int close)
{
	int fd, size, i;
	struct stat statbuf;
	unsigned char b;
	int flags = O_CREAT|O_WRONLY;

	if (rump_sys_stat(filename, &statbuf) < 0)
		size = 0;
	else {
		size = statbuf.st_size;
		flags |= O_APPEND;
	}

	fd = rump_sys_open(filename, flags);

	for (i = 0; i < len; i++) {
		b = ((unsigned)(size + i)) & 0xff;
		rump_sys_write(fd, &b, 1);
	}
	if (close) {
		rump_sys_close(fd);
		fd = -1;
	}

	return fd;
}

/* Check file's existence, size and contents */
int check_file(const char *filename, int size)
{
	int fd, i;
	struct stat statbuf;
	unsigned char b;

	if (rump_sys_stat(filename, &statbuf) < 0) {
		fprintf(stderr, "%s: stat failed\n", filename);
		return 1;
	}
	if (size != statbuf.st_size) {
		fprintf(stderr, "%s: expected %d bytes, found %d\n",
			filename, size, (int)statbuf.st_size);
		return 2;
	}

	fd = rump_sys_open(filename, O_RDONLY);
	for (i = 0; i < size; i++) {
		rump_sys_read(fd, &b, 1);
		if (b != (((unsigned)i) & 0xff)) {
			fprintf(stderr, "%s: byte %d: expected %x found %x\n",
				filename, i, ((unsigned)(i)) & 0xff, b);
			rump_sys_close(fd);
			return 3;
		}
	}
	rump_sys_close(fd);
	fprintf(stderr, "%s: no problem\n", filename);
	return 0;
}

/* Run a file system consistency check */
int fsck(void)
{
	char s[MAXLINE];
	int i, errors = 0;
	FILE *pipe;
	char cmd[MAXLINE];

	for (i = 0; i < 2; i++) {
		sprintf(cmd, "fsck_lfs -n -b %jd -f " IMGNAME,
			(intmax_t)sbaddr[i]);
		pipe = popen(cmd, "r");
		while (fgets(s, MAXLINE, pipe) != NULL) {
			if (isdigit((int)s[0])) /* "5 files ... " */
				continue;
			if (isspace((int)s[0]) || s[0] == '*')
				continue;
			if (strncmp(s, "Alternate", 9) == 0)
				continue;
			if (strncmp(s, "ROLL ", 5) == 0)
				continue;
			fprintf(stderr, "FSCK[sb@%lld]: %s", sbaddr[i], s);
			++errors;
		}
		pclose(pipe);
		if (errors) {
			break;
		}
	}

	return errors;
}

/* Run dumplfs */
void dumplfs()
{
	char s[MAXLINE];
	FILE *pipe;

	pipe = popen("dumplfs -S -s 2 -s 1 -s 0 " IMGNAME, "r");
	while (fgets(s, MAXLINE, pipe) != NULL)
		fprintf(stderr, "DUMPLFS: %s", s);
	pclose(pipe);
}
