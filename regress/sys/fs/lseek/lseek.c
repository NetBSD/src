#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/inttypes.h>

int main(int argc, char **argv)
{
	int fd;
	off_t cur;
	struct stat st;
	int error;

	if (argc != 2)
		errx(EXIT_FAILURE, "seektest filename");
	fd = open(argv[1], 0, O_RDONLY);
	if (fd <= 0)
		err(EXIT_FAILURE, "can't open `%s'", argv[1]);

	printf("Statting file\n");
	error = fstat(fd, &st);
	if (error)
		err(EXIT_FAILURE, "can't stat file");
	printf("fstat() returns %"PRIi64" as size\n", st.st_size);

	error = stat(argv[1], &st);
	if (error)
		err(EXIT_FAILURE, "can't stat file");
	printf("stat()  returns %"PRIi64" as size\n", st.st_size);

	error = lstat(argv[1], &st);
	if (error)
		err(EXIT_FAILURE, "can't lstat file");
	printf("lstat() returns %"PRIi64" as size\n", st.st_size);

	printf("\nTesting normal seeking\n");
	printf("get initial position\n");
	cur = lseek(fd, 0, SEEK_CUR);
	if (cur != 0)
		err(EXIT_FAILURE, "seek initial position wrong");
	printf("seek start %"PRIi64"\n", cur);

	printf("seeking end (filesize = %"PRIi64")\n", st.st_size);
	cur = lseek(fd, 0, SEEK_END);
	if (cur != st.st_size)
		err(EXIT_FAILURE, "seek to the end went wrong");
	printf("seek now %"PRIi64"\n", cur);

	printf("seeking backwards filesize - 150 steps\n");
	cur = lseek(fd, -(st.st_size - 150), SEEK_CUR);
	if (cur != 150)
		err(EXIT_FAILURE, "relative seek from end to 150 failed");
	printf("seek now %"PRIi64"\n", cur);

	printf("seek set 1000\n");
	cur = lseek(fd, 1000, SEEK_SET);
	if (cur != 1000)
		err(EXIT_FAILURE, "seek 1000 went wrong");
	printf("seek now %"PRIi64"\n", cur);

#if defined SEEK_DATA
	printf("\nOne piece non sparse file checking:\n");
	printf("seeking for sparse file data offset\n");
	cur = lseek(fd, 0, SEEK_DATA);
	if (cur != 0)
		err(EXIT_FAILURE, "Not getting start of data segment at 0");

	printf("if seek_data returns a 2nd part on a non-sparse file\n");
	cur = lseek(fd, st.st_size, SEEK_DATA);
	if (cur != -1)
		errx(EXIT_FAILURE, "Seek data gave 2nd part at end of file");
	if (errno != ENXIO)
		errx(EXIT_FAILURE, "Seek data on the end of file didn't"
			" raise ENXIO (%d)", errno);
	printf( "if seek_data in the file's last block of a non-sparse file "
		"returns the current position\n");
	cur = lseek(fd, st.st_size-50, SEEK_DATA);
	if (cur != st.st_size - 50) {
		errx(EXIT_FAILURE, "Seek data didn't give passed seek position "
		    "back %" PRIi64 " should be %" PRIi64, cur, st.st_size-50);
		return EXIT_FAILURE;
	}

	printf("if seek_data in the middle of the of a non-sparse file "
		"returns the current position\n");
	cur = lseek(fd, st.st_size - 100 * 1024, SEEK_DATA);
	if (cur != st.st_size - 100 * 1024)
		errx(EXIT_FAILURE, "Seek data didn't give passed seek "
		    "position back %" PRIi64 " should be %" PRIi64, cur,
		    st.st_size - 100 * 1024);

	printf("seeking for hole\n");
	cur = lseek(fd, 0, SEEK_HOLE);
	if (cur != st.st_size)
		errx(EXIT_FAILURE, "Seek hole didn't return end of file");

	printf("seeking if the end of the file is a hole\n");
	cur = lseek(fd, st.st_size, SEEK_HOLE);
	if (cur != st.st_size)
		errx(EXIT_FAILURE, "At the end of the file, no virtual hole "
		    "is returned");

	printf("seeking if a 2nd hole is returned outside the file range\n");
	cur = lseek(fd, st.st_size + 1, SEEK_HOLE);
	if (cur != -1)
		errx(EXIT_FAILURE, "Past the end of file, seek hole returned "
		    "another hole instead of raising an error");
	if (errno != ENXIO)
		errx(EXIT_FAILURE, "Seek hole past the end of file didn't "
		    "raise ENXIO (%d)", errno);
#endif

	return EXIT_SUCCESS;
}

