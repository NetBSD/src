#include <err.h>
#include <unistd.h>
#include <fcntl.h>

static const char fn[] = "/tmp/write-test.txt";
static const char str[] = "Test write() append\n";
static size_t len = sizeof(str) - 1;

int
main(void)
{
	int fd;
	off_t off;

	if ((fd = open(fn, O_CREAT | O_RDWR, 0600)) == -1)
		err(1, "Cannot open `%s'", fn);
	if (write(fd, str, len) != (ssize_t)len)
		err(1, "Write failed");
	if (close(fd) == -1)
		err(1, "Close failed");
	if ((fd = open(fn, O_WRONLY | O_APPEND)) == -1)
		err(1, "Cannot open `%s'", fn);
        if ((off = lseek(fd, (off_t) 0, SEEK_SET)) != (off_t)0)
		err(1, "Seek failed");
	if (write(fd, str, 0) != 0)
		err(1, "Write failed");
        if ((off = lseek(fd, (off_t) 0, SEEK_CUR)) != (off_t)0)
		errx(1, "bad seek offset %lld\n", (long long)off);
	if (close(fd) == -1)
		err(1, "Close failed");
	if (unlink(fn) == -1)
		err(1, "Unlink failed");
	return 0;
}
