/*	$NetBSD: uipc_syscalls_43.c,v 1.2 2002/02/21 07:38:20 itojun Exp $	*/

/*
 * This is regression test for COMPAT_43 code. Tested 4.3 syscalls are:
 * - getsockname(2), getpeername(2)
 * - recv(2), recvfrom(2), recvmsg(2)
 * - send(2), sendmsg(2)
 * 
 * This program uses inetd echo service. You need to configure
 * inetd to provide echo for both tcp and udp in order to run
 * this program successfully, and adjust 'echoserver' to IP address
 * of the machine running the service.
 *
 * Public domain. Do whatever you please with this. Jaromir Dolecek
 */

#include <sys/syscall.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/uio.h>

const char *unixd = "unixdomain";
const char *echoserver = "127.0.0.1";
const char *localhost = "127.0.0.1";
const int echoport = 7;

int
main()
{
	int s, descr;
	socklen_t sz;
	struct sockaddr_in sa;
	struct sockaddr_un sun;
	struct osockaddr *osa = (struct osockaddr *) &sa;
	struct omsghdr msg;
	struct iovec iov;
	char buf[10];

	/*
	 * TCP connection, test connect(2), bind(2), recv(2), send(2),
	 * getsockname(2), getpeername(2).
	 */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket");
		
	sa.sin_addr.s_addr = inet_addr(echoserver);
	sa.sin_port = htons(echoport);
	osa->sa_family = AF_INET;
	if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		err(1, "connect");

	/* ogetpeername */
	sz = sizeof(sa);
	memset(&sa, '\0', sizeof(sa));
	if (syscall(SYS_compat_43_ogetpeername, s, (struct sockaddr *) &sa, &sz))
		err(1, "getpeername");

	printf("ogetpeername: sz %d:%d name %s port %d family %d\n",
		sizeof(sa), sz,
		inet_ntoa(sa.sin_addr),
		ntohs(sa.sin_port),
		osa->sa_family);
	
	/* ogetsockname */
	sz = sizeof(sa);
	memset(&sa, '\0', sizeof(sa));
	if (syscall(SYS_compat_43_ogetsockname, s, (struct sockaddr *) &sa, &sz))
		err(1, "getsockname");

	printf("osockname: sz %d:%d name %s port %d family %d\n",
		sizeof(sa), sz,
		inet_ntoa(sa.sin_addr),
		ntohs(sa.sin_port),
		osa->sa_family);
	
	/* osend */
	if (syscall(SYS_compat_43_osend, s, "fobj", 4, 0) < 0)
		err(1, "osend");

	/* orecv */
	memset(buf, '\0', sizeof(buf));
	if (syscall(SYS_compat_43_orecv, s, buf, sizeof(buf), 0) < 0)
		err(1, "orecv");

	printf("orecv: %s\n", buf);

	shutdown(s, SHUT_RDWR);
	close(s);

	/* UDP connection, test sendto()/recvfrom() */

	if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err(1, "socket");
		
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(65533);
	osa->sa_family = AF_INET;
	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)))
		err(1, "bind1");

	/* ogetsockname */
	sz = sizeof(sa);
	memset(&sa, '\0', sizeof(sa));
	if (syscall(SYS_compat_43_ogetsockname, s, (struct sockaddr *) &sa, &sz))
		err(1, "getsockname");

	printf("osockname2: sz %d:%d name %s port %d family %d\n",
		sizeof(sa), sz,
		inet_ntoa(sa.sin_addr),
		ntohs(sa.sin_port),
		osa->sa_family);
	
	sa.sin_addr.s_addr = inet_addr(echoserver);
	sa.sin_port = htons(echoport);
	osa->sa_family = AF_INET;
	/* common sendto(2) - not versioned */
	if (sendto(s, "fob2", 4, 0, (struct sockaddr *) &sa, sizeof(sa)) < 0)
		err(1, "sendto");

	/* orecvfrom */
	memset(buf, '\0', sizeof(buf));
	memset(&sa, '\0', sizeof(sa));
	sz = sizeof(sa);
	if (syscall(SYS_compat_43_orecvfrom, s, buf, sizeof(buf), 0, (struct osockaddr *) &sa, &sz) < 0)
		err(1, "orecvfrom");
	printf("orecvfrom: '%s' sz %d:%d name %s port %d family %d\n",
		buf,
		sizeof(sa), sz,
		inet_ntoa(sa.sin_addr),
		ntohs(sa.sin_port),
		osa->sa_family);

	shutdown(s, SHUT_RDWR);
	close(s);

	/* UDP connection, test sendmsg()/recvmsg() */

	if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err(1, "socket");
		
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(65533);
	osa->sa_family = AF_INET;
	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)))
		err(1, "bind2");

	sa.sin_addr.s_addr = inet_addr(echoserver);
	sa.sin_port = htons(echoport);
	osa->sa_family = AF_INET;
	memset(&msg, '\0', sizeof(msg));
	msg.msg_name = (void *) &sa;
	msg.msg_namelen = sizeof(sa);
	iov.iov_base = "fob3";
	iov.iov_len = 4;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	/* osendmsg */
	if (syscall(SYS_compat_43_osendmsg, s, &msg, 0) < 0)
		err(1, "osendmsg");

	/* orecvmsg */
	memset(&sa, '\0', sizeof(sa));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);
	if (syscall(SYS_compat_43_orecvmsg, s, &msg, 0) < 0)
		err(1, "orecvmsg");

	printf("orecvmsg: '%s' sz %d:%d name %s port %d family %d\n",
		buf,
		sizeof(sa), msg.msg_namelen,
		inet_ntoa(sa.sin_addr),
		ntohs(sa.sin_port),
		osa->sa_family);

	shutdown(s, SHUT_RDWR);
	close(s);

	/*
	 * Local (unix domain) socket, test sendmsg()/recvmsg() with
	 * accrights
	 */

	if ((s = socket(PF_LOCAL, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");
		
	osa = (struct osockaddr *) &sun;
	strcpy(sun.sun_path, unixd);
	osa->sa_family = AF_LOCAL;
	if (bind(s, (struct sockaddr *) &sun, SUN_LEN(&sun)))
		err(1, "bind3");

	/* osendmsg, old style descriptor passing */
	memset(&msg, '\0', sizeof(msg));
	msg.msg_name = (void *) &sun;
	msg.msg_namelen = sizeof(sun);
	iov.iov_base = "fob4";
	iov.iov_len = 4;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	descr = s;
	msg.msg_accrights = (caddr_t) &descr;
	msg.msg_accrightslen = sizeof(int);
	if (syscall(SYS_compat_43_osendmsg, s, &msg, 0) < 0) {
		unlink(unixd);
		err(1, "osendmsg");
	}

	memset(&sun, '\0', sizeof(sa));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);
	descr = -1;

	/* orecvmsg */
	if (syscall(SYS_compat_43_orecvmsg, s, &msg, 0) < 0) {
		unlink(unixd);
		err(1, "orecvmsg");
	}

	printf("orecvmsg: '%s' sz %d:%d name '%s' family %d descr %d\n",
		buf,
		sizeof(sun), msg.msg_namelen, sun.sun_path,
		osa->sa_family, descr);

	unlink(unixd);
	close(s);
	close(descr);

}
