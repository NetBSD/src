/* socket.c

   BSD socket interface code... */

/*
 * Copyright (c) 1995, 1996 The Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: socket.c,v 1.1.1.1 1996/10/03 06:33:25 mrg Exp $ Copyright (c) 1995, 1996 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

#ifdef USE_SOCKET_FALLBACK
#  define USE_SOCKET_SEND
#  define if_register_send if_register_fallback
#  define send_packet send_fallback
#endif

#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_RECEIVE)
/* Generic interface registration routine... */
int if_register_socket (info, interface)
	struct interface_info *info;
	struct ifreq *interface;
{
	struct sockaddr_in name;
	int sock;
	int flag;
	static int once = 0;

	/* Make sure only one interface is registered. */
	if (once)
		error ("The standard socket API can only support %s%s%s%s%s",
		       "hosts with a single network interface.   If you must ",
		       "run dhcpd on a host with multiple interfaces, ",
		       "you must compile in BPF or NIT support.   If neither ",
		       "option is supported on your system, please let us ",
		       "know.");

	/* Technically, we need to know what interface every packet
	   comes in on, which means that we can only operate on a
	   machine with a single interface configured.   However,
	   we generally don't expect to get broadcast packets on
	   point-to-point interfaces, so we can bend the rules a bit
	   and not count them.   This won't allow DHCP-over-PPP,
	   but it's probably right in a lot of cases, and the issue
	   will have to be revisited when DHCP-over-PPP support is
	   done.   Currently we determine whether an interface is
	   point-to-point by seeing if it has a link-level address.
	   This only works on 4.4BSD and derivative networking. */
#ifdef AF_LINK
	if (info -> hw_address.hlen) /* XXX */
#endif
		once = 1;

	/* Set up the address we're going to bind to. */
	name.sin_family = AF_INET;
	name.sin_port = server_port;
	name.sin_addr.s_addr = INADDR_ANY;
	memset (name.sin_zero, 0, sizeof (name.sin_zero));

	/* Make a socket... */
	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		error ("Can't create dhcp socket: %m");

	/* Set the REUSEADDR option so that we don't fail to start if
	   we're being restarted. */
	flag = 1;
	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&flag, sizeof flag) < 0)
		error ("Can't set SO_REUSEADDR option on dhcp socket: %m");

	/* Set the BROADCAST option so that we can broadcast DHCP responses. */
	if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST,
			(char *)&flag, sizeof flag) < 0)
		error ("Can't set SO_BROADCAST option on dhcp socket: %m");

	/* Bind the socket to this interface's IP address. */
	if (bind (sock, (struct sockaddr *)&name, sizeof name) < 0)
		error ("Can't bind to dhcp address: %m");

	return sock;
}
#endif /* USE_SOCKET_SEND || USE_SOCKET_RECEIVE */

#ifdef USE_SOCKET_SEND
void if_register_send (info, interface)
	struct interface_info *info;
	struct ifreq *interface;

{
#ifndef USE_SOCKET_RECEIVE
	info -> wfdesc = if_register_socket (info, interface);
#else
	info -> wfdesc = info -> rfdesc;
#endif
	note ("Sending on   Socket/%s/%s",
	      info -> name,
	      (info -> shared_network ?
	       info -> shared_network -> name : "unattached"));

}
#endif /* USE_SOCKET_SEND */

#ifdef USE_SOCKET_RECEIVE
void if_register_receive (info, interface)
	struct interface_info *info;
	struct ifreq *interface;
{
	/* If we're using the socket API for sending and receiving,
	   we don't need to register this interface twice. */
	info -> rfdesc = if_register_socket (info, interface);
	note ("Listening on Socket/%s/%s",
	      info -> name,
	      (info -> shared_network ?
	       info -> shared_network -> name : "unattached"));
}
#endif /* USE_SOCKET_RECEIVE */

#ifdef USE_SOCKET_SEND
size_t send_packet (interface, packet, raw, len, from, to, hto)
	struct interface_info *interface;
	struct packet *packet;
	struct dhcp_packet *raw;
	size_t len;
	struct in_addr from;
	struct sockaddr_in *to;
	struct hardware *hto;
{
	return sendto (interface -> wfdesc, (char *)raw, len, 0,
		       (struct sockaddr *)to, sizeof *to);
}
#endif /* USE_SOCKET_SEND */

#ifdef USE_SOCKET_RECEIVE
size_t receive_packet (interface, buf, len, from, hfrom)
	struct interface_info *interface;
	unsigned char *buf;
	size_t len;
	struct sockaddr_in *from;
	struct hardware *hfrom;
{
	int flen = sizeof *from;

	return recvfrom (interface -> rfdesc, buf, len, 0,
			 (struct sockaddr *)from, &flen);
}
#endif /* USE_SOCKET_RECEIVE */

#ifdef USE_SOCKET_FALLBACK
/* This just reads in a packet and silently discards it. */

size_t fallback_discard (interface)
	struct interface_info *interface;
{
	char buf [1540];
	struct sockaddr_in from;
	int flen = sizeof from;

	return recvfrom (interface -> wfdesc, buf, sizeof buf, 0,
			 (struct sockaddr *)&from, &flen);
}
#endif /* USE_SOCKET_RECEIVE */
