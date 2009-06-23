/*	$NetBSD: stream_test.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

#include "sys_defs.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "iostuff.h"
#include "msg.h"
#include "msg_vstream.h"
#include "listen.h"
#include "connect.h"

#ifdef SUNOS5
#include <stropts.h>

#define FIFO	"/tmp/test-fifo"

static const char *progname;

static void print_fstat(int fd)
{
    struct stat st;

    if (fstat(fd, &st) < 0)
	msg_fatal("fstat: %m");
    vstream_printf("fd	%d\n", fd);
    vstream_printf("dev	%ld\n", (long) st.st_dev);
    vstream_printf("ino	%ld\n", (long) st.st_ino);
    vstream_fflush(VSTREAM_OUT);
}

static NORETURN usage(void)
{
    msg_fatal("usage: %s [-p] [-n count] [-v]", progname);
}

int     main(int argc, char **argv)
{
    int     server_fd;
    int     client_fd;
    int     fd;
    int     print_fstats = 0;
    int     count = 1;
    int     ch;
    int     i;

    progname = argv[0];
    msg_vstream_init(argv[0], VSTREAM_ERR);

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "pn:v")) > 0) {
	switch (ch) {
	default:
	    usage();
	case 'p':
	    print_fstats = 1;
	    break;
	case 'n':
	    if ((count = atoi(optarg)) < 1)
		usage();
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }
    server_fd = stream_listen(FIFO, 0, 0);
    if (readable(server_fd))
	msg_fatal("server fd is readable after create");

    /*
     * Connect in client.
     */
    for (i = 0; i < count; i++) {
	msg_info("connect attempt %d", i);
	if ((client_fd = stream_connect(FIFO, 0, 0)) < 0)
	    msg_fatal("open %s as client: %m", FIFO);
	if (readable(server_fd))
	    msg_info("server fd is readable after client open");
	if (close(client_fd) < 0)
	    msg_fatal("close client fd: %m");
    }

    /*
     * Accept in server.
     */
    for (i = 0; i < count; i++) {
	msg_info("receive attempt %d", i);
	if (!readable(server_fd)) {
	    msg_info("wait for server fd to become readable");
	    read_wait(server_fd, -1);
	}
	if ((fd = stream_accept(server_fd)) < 0)
	    msg_fatal("receive fd: %m");
	if (print_fstats)
	    print_fstat(fd);
	if (close(fd) < 0)
	    msg_fatal("close received fd: %m");
    }
    if (close(server_fd) < 0)
	msg_fatal("close server fd");
    return (0);
}
#else
int     main(int argc, char **argv)
{
    return (0);
}
#endif
