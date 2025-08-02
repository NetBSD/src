#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pppd-private.h"

/* globals used in test.c... */
int debug = 1;
int error_count;
int unsuccess;

/* check if path exists and returns its type */
static int
file_type(char *path)
{
    struct stat statbuf;

    if (stat(path, &statbuf) < 0)
	return -1;

    return statbuf.st_mode & S_IFMT;
}

int
test_simple() {
    if (mkdir_recursive("dir"))
	return -1;

    if (file_type("dir") != S_IFDIR)
	return -1;

    rmdir("dir");
    return 0;
}

int
test_recurse() {
    if (mkdir_recursive("dir/subdir/subsubdir"))
	return -1;

    if (file_type("dir/subdir/subsubdir") != S_IFDIR)
	return -1;

    rmdir("dir/subdir/subsubdir");

    /* try again with partial existence */
    if (mkdir_recursive("dir/subdir/subsubdir"))
	return -1;

    if (file_type("dir/subdir/subsubdir") != S_IFDIR)
	return -1;

    rmdir("dir/subdir/subsubdir");
    rmdir("dir/subdir");
    rmdir("dir");
    return 0;
}

int
test_recurse_multislash() {
    if (mkdir_recursive("dir/subdir///subsubdir"))
	return -1;

    if (file_type("dir/subdir/subsubdir") != S_IFDIR)
	return -1;

    rmdir("dir/subdir/subsubdir");
    rmdir("dir/subdir");

    /* try again with partial existence */
    if (mkdir_recursive("dir/subdir/subsubdir///"))
	return -1;

    if (file_type("dir/subdir/subsubdir") != S_IFDIR)
	return -1;

    rmdir("dir/subdir/subsubdir");
    rmdir("dir/subdir");
    rmdir("dir");
    return 0;
}

int
test_parent_notdir() {
    int fd = open("file", O_CREAT, 0600);
    if (fd < 0)
	return -1;
    close(fd);

    if (mkdir_recursive("file") == 0)
	return -1;
    if (mkdir_recursive("file/dir") == 0)
	return -1;

    unlink("file");
    return 0;
}

int
main()
{
    char *base_dir = strdup("/tmp/ppp_utils_utest.XXXXXX");
    int failure = 0;

    if (mkdtemp(base_dir) == NULL) {
	printf("Could not create test directory, aborting\n");
	return 1;
    }

    if (chdir(base_dir) < 0) {
	printf("Could not enter newly created test dir, aborting\n");
	return 1;
    }

    if (test_simple()) {
	printf("Could not create simple directory\n");
	failure++;
    }

    if (test_recurse()) {
	printf("Could not create recursive directory\n");
	failure++;
    }

    if (test_recurse_multislash()) {
	printf("Could not create recursive directory with multiple slashes\n");
	failure++;
    }

    if (test_parent_notdir()) {
	printf("Creating over a file appeared to work?\n");
	failure++;
    }

    rmdir(base_dir);
    free(base_dir);
    return failure;
}
