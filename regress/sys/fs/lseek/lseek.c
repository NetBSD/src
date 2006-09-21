#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>

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

	error = fstat(fd, &st);
	if (error) {
		printf("can't stat file\n");
		return EXIT_FAILURE;
	}

	if (st.st_size < 1500) {
		printf("can't run this test on such a small file\n");
		return EXIT_FAILURE;
	}

	printf("get initial position\n");
	cur = lseek(fd, 0, SEEK_CUR);
	printf("seek start %d\n", cur);
	if (cur != 0) {
		printf("seek initial position wrong\n");
		return EXIT_FAILURE;
	}

	printf("seek set 1000\n");
	cur = lseek(fd, 1000, SEEK_SET);
	printf("seek now %d\n", cur);
	if (cur != 1000) {
		printf("seek 1000 went wrong\n");
		return EXIT_FAILURE;
	}

	printf("seeking end (filesize = %d)\n", (uint32_t) st.st_size);
	cur = lseek(fd, 0, SEEK_END);
	printf("seek now %d\n", cur);
	if (cur != st.st_size) {
		printf("seek to the end went wrong\n");
		return EXIT_FAILURE;
	}

	printf("seeking backwards filesize-150 steps\n");
	cur = lseek(fd, -(st.st_size - 150), SEEK_CUR);
	printf("seek now %d\n", cur);
	if (cur != 150) {
		printf("relative seek from end to 150 failed\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

