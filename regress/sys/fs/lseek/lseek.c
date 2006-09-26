#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/inttypes.h>

int main(int argc, char **argv)
{
	int fd;
	off_t cur;
	struct stat st;
	int error;

	if (argc != 2) {
		printf("seektest filename\n");
		return EXIT_FAILURE;
	}
	fd = open(argv[1], 0, O_RDONLY);
	if (fd <= 0) {
		printf("can't open `%s` : %s\n", argv[1], strerror(errno));
		return EXIT_FAILURE;
	}

	printf("Statting file\n");
	error = fstat(fd, &st);
	if (error) {
		printf("can't stat file\n");
		return EXIT_FAILURE;
	}
	printf("fstat() returns %"PRIi64" as size\n", st.st_size);

	error = stat(argv[1], &st);
	if (error) {
		printf("can't stat file\n");
		return EXIT_FAILURE;
	}
	printf("stat()  returns %"PRIi64" as size\n", st.st_size);

	error = lstat(argv[1], &st);
	if (error) {
		printf("can't lstat file\n");
		return EXIT_FAILURE;
	}
	printf("lstat() returns %"PRIi64" as size\n", st.st_size);

	printf("\nTesting normal seeking\n");
	printf("get initial position\n");
	cur = lseek(fd, 0, SEEK_CUR);
	printf("seek start %"PRIi64"\n", cur);
	if (cur != 0) {
		printf("seek initial position wrong\n");
		return EXIT_FAILURE;
	}

	printf("seeking end (filesize = %"PRIi64")\n", st.st_size);
	cur = lseek(fd, 0, SEEK_END);
	printf("seek now %"PRIi64"\n", cur);
	if (cur != st.st_size) {
		printf("seek to the end went wrong\n");
		return EXIT_FAILURE;
	}

	printf("seeking backwards filesize-150 steps\n");
	cur = lseek(fd, -(st.st_size - 150), SEEK_CUR);
	printf("seek now %"PRIi64"\n", cur);
	if (cur != 150) {
		printf("relative seek from end to 150 failed\n");
		return EXIT_FAILURE;
	}

	printf("seek set 1000\n");
	cur = lseek(fd, 1000, SEEK_SET);
	printf("seek now %"PRIi64"\n", cur);
	if (cur != 1000) {
		printf("seek 1000 went wrong\n");
		return EXIT_FAILURE;
	}

#if defined SEEK_DATA
	printf("\nOne piece non sparse file checking:\n");
	printf("seeking for sparse file data offset\n");
	cur = lseek(fd, 0, SEEK_DATA);
	if (cur != 0) {
		printf("Not getting start of data segment at 0\n");
		return EXIT_FAILURE;
	}

	printf("if seek_data returns a 2nd part on a non-sparse file\n");
	cur = lseek(fd, st.st_size, SEEK_DATA);
	if (cur != -1) {
		printf("Seek data gave 2nd part at end of file\n");
		return EXIT_FAILURE;
	}
	if (errno != ENXIO) {
		printf( "Seek data on the end of file didn't"
			" raise ENXIO\n");
		return EXIT_FAILURE;
	}

	printf( "if seek_data in the file's last block of a non-sparse file "
		"returns the current position\n");
	cur = lseek(fd, st.st_size-50, SEEK_DATA);
	if (cur != st.st_size - 50) {
		printf("Seek data didn't give passed seek position back\n");
		printf("%"PRIi64" should be %"PRIi64"\n", cur, st.st_size-50);
		return EXIT_FAILURE;
	}

	printf( "if seek_data in the middle of the of a non-sparse file "
		"returns the current position\n");
	cur = lseek(fd, st.st_size-100*1024, SEEK_DATA);
	if (cur != st.st_size - 100*1024) {
		printf("Seek data didn't give passed seek position back\n");
		printf("%"PRIi64" should be %"PRIi64"\n", cur,
			st.st_size-100*1024);
		return EXIT_FAILURE;
	}

	printf("seeking for hole\n");
	cur = lseek(fd, 0, SEEK_HOLE);
	if (cur != st.st_size) {
		printf("Seek hole didn't return end of file\n");
		return EXIT_FAILURE;
	}

	printf("seeking if the end of the file is a hole\n");
	cur = lseek(fd, st.st_size, SEEK_HOLE);
	if (cur != st.st_size) {
		printf("At the end of the file, no virtual hole is returned\n");
		return EXIT_FAILURE;
	}

	printf("seeking if a 2nd hole is returned outside the file range\n");
	cur = lseek(fd, st.st_size + 1, SEEK_HOLE);
	if (cur != -1) {
		printf( "Past the end of file, seek hole returned another hole "
			"instead of raising an error\n");
		return EXIT_FAILURE;
	}
	if (errno != ENXIO) {
		printf("Seek hole past the end of file didn't raise ENXIO\n");
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}

