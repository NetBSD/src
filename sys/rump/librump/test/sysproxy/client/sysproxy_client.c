#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVPATH "/tmp/rump_sysproxy_test"
#define USE_UN

int
main()
{
	struct stat sb[3];
	int s, error;
        size_t len;
        int i;

#ifdef USE_UN
	struct sockaddr_un sun;

	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_LOCAL;
	strcpy(sun.sun_path, SERVPATH);
	if (connect(s, (struct sockaddr *)&sun, SUN_LEN(&sun)) == -1)
		err(1, "connect");
#else
	struct sockaddr_in sin;
	int x = 1;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(1, "socket");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(12345);
	sin.sin_addr.s_addr = inet_addr("10.181.181.1");
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
		err(1, "connect");

	if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &x, sizeof(x)) == -1)
		err(1, "setsockopt");
#endif

	rump_init();
	rump_sysproxy_socket_setup_client(s);

	if (rump_sys_stat("/", &sb[1]) == -1)
		err(1, "stat");
}
