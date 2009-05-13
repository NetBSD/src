#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>

#include <rump/rump.h>

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SERVPATH "/tmp/rump_sysproxy_test"
#define USE_UN

static void
cleanup()
{

	unlink(SERVPATH);
}

static void
sigint(int sig)
{

	cleanup();
	_exit(0);
}

static void
sigabrt(int sig)
{

	cleanup();
	abort();
}

int
main()
{
	socklen_t slen;
	int s, s2;

#ifdef USE_UN
	struct sockaddr_un sun;

	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_LOCAL;
	strcpy(sun.sun_path, SERVPATH);
	if (bind(s, (struct sockaddr *)&sun, SUN_LEN(&sun)) == -1)
		err(1, "bind");
	atexit(cleanup);
	signal(SIGINT, sigint);
	signal(SIGSEGV, sigabrt);
	if (listen(s, 1) == -1)
		err(1, "listen");
	slen = sizeof(sun);
	s2 = accept(s, (struct sockaddr *)&sun, &slen);
	if (s2 == -1)
		err(1, "accept");
#else
	struct sockaddr_in sin;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(12345);
	sin.sin_addr.s_addr = INADDR_ANY;

	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
		err(1, "bind");
	if (listen(s, 1) == -1)
		err(1, "listen");
	slen = sizeof(sin);
	s2 = accept(s, (struct sockaddr *)&sin, &slen);
	if (s2 == -1)
		err(1, "accept");
#endif

	rump_init();
	rump_sysproxy_socket_setup_server(s2);

	pause();

	return 0;
}
