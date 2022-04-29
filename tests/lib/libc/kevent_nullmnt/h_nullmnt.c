#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/event.h>
#include <sys/time.h>

/*
 * External set-up code is expected to do the equivalent of
 *	cd $TOPDIR
 *	mkdir realdir
 *	mkdir nulldir
 *	mount -t null $TOPDIR/realdir $TOPDIR/nulldir
 *	rm -f $TOPDIR/realdir/afile
 *	touch $TOPDIR/realdir/afile
 * then execute this test program:
 *	./h_nullmnt $TOPDIR/realdir/afile $TOPDIR/nulldir/afile
 *
 * The expected result is that the write() to the nullfile will
 * queue up a preexisting kevent which will then be detected by
 * the (second) call to kevent(); the failure mode is that the
 * write()'s extension to the file is not seen, and the kevent
 * call times out after 5 seconds.
 *
 * Clean-up code should undo the null mount and delete everything
 * in the test directory.
 */

int main(int argc, char **argv)
{
	int realfile, nullfile;
	int kq, nev, rsize;
	struct timespec timeout;
	struct kevent eventlist;
	const char outbuf[] = "new\n";
	char inbuf[20];

	if (argc <= 2)
		errx(EXIT_FAILURE, "insufficient args %d", argc);

	realfile = open(argv[1], O_RDONLY);
	if (realfile == -1)
		err(EXIT_FAILURE, "failed to open realfile %s",
		    argv[1]);

	nullfile = open(argv[2], O_WRONLY, O_APPEND);
	if (nullfile == -1)
		err(EXIT_FAILURE, "failed to open nullfile %s",
		    argv[2]);

	if ((kq = kqueue()) == -1)
		err(EXIT_FAILURE, "Cannot create kqueue");

	timeout.tv_sec = 5;
	timeout.tv_nsec = 0;

	EV_SET(&eventlist, realfile,
	    EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
	    NOTE_WRITE | NOTE_EXTEND, 0, 0);
	if (kevent(kq, &eventlist, 1, NULL, 0, NULL) == -1)
		err(EXIT_FAILURE, "Failed to set eventlist for fd %d",
		    realfile);

	rsize = read(realfile, &inbuf, sizeof(inbuf));
	if (rsize)
		errx(EXIT_FAILURE, "Ooops we got %d bytes of data!\n", rsize);

	write(nullfile, &outbuf, sizeof(outbuf) - 1);

	nev = kevent(kq, NULL, 0, &eventlist, 1, &timeout);
	if (nev == -1)
		err(EXIT_FAILURE, "Failed to retrieve event");

	errx((nev == 0) ? EXIT_FAILURE : EXIT_SUCCESS,
	    "Retrieved %d events, first 0x%x", nev, eventlist.flags);
}
