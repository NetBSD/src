/* dhcp.c

   DHCP Client (really lame DHCP client). */

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
"$Id: dhclient.c,v 1.1.1.1 1996/10/03 06:33:25 mrg Exp $ Copyright (c) 1995, 1996 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

TIME cur_time;
TIME default_lease_time = 43200; /* 12 hours... */
TIME max_lease_time = 86400; /* 24 hours... */
struct tree_cache *global_options [256];

struct iaddr server_identifier;
int server_identifier_matched;
int log_perror = 1;

#ifdef USE_FALLBACK
struct interface_info fallback_interface;
#endif

u_int16_t server_port;
int log_priority;

int lexline, lexchar;
char *tlname, *token_line;

static void usage PROTO ((void));

int main (argc, argv, envp)
	int argc;
	char **argv, **envp;
{
	int i;
	struct servent *ent;
	struct interface_info *interface;

#ifdef SYSLOG_4_2
	openlog ("dhclient", LOG_NDELAY);
	log_priority = LOG_DAEMON;
#else
	openlog ("dhclient", LOG_NDELAY, LOG_DAEMON);
#endif

#ifndef	NO_PUTENV
	/* ensure mktime() calls are processed in UTC */
	putenv("TZ=GMT0");
#endif /* !NO_PUTENV */

#if !(defined (DEBUG) || defined (SYSLOG_4_2))
	setlogmask (LOG_UPTO (LOG_INFO));
#endif	

	for (i = 1; i < argc; i++) {
		if (!strcmp (argv [i], "-p")) {
			if (++i == argc)
				usage ();
			server_port = htons (atoi (argv [i]));
			debug ("binding to user-specified port %d",
			       ntohs (server_port));
		} else
			usage ();
	}

	/* Default to the DHCP/BOOTP port. */
	if (!server_port)
	{
		ent = getservbyname ("dhcpc", "udp");
		if (!ent)
			server_port = htons (68);
		else
			server_port = ent -> s_port;
		endservent ();
	}
  
	/* Get the current time... */
	GET_TIME (&cur_time);

	/* Discover all the network interfaces and initialize them. */
	discover_interfaces (0);

	for (interface = interfaces; interface; interface = interface -> next)
		send_discover (interface);

	/* Receive packets and dispatch them... */
	dispatch ();

	/* Not reached */
	return 0;
}

static void usage ()
{
	error ("Usage: dhclient [-p <port>]");
}

void cleanup ()
{
}

int commit_leases ()
{
	return 0;
}

int write_lease (lease)
	struct lease *lease;
{
	return 0;
}

void db_startup ()
{
}

void bootp (packet)
	struct packet *packet;
{
	note ("BOOTREPLY from %s",
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr));
}

void dhcp (packet)
	struct packet *packet;
{
	switch (packet -> packet_type) {
	      case DHCPOFFER:
		dhcpoffer (packet);
		break;

	      case DHCPNAK:
		dhcpnak (packet);
		break;

	      case DHCPACK:
		dhcpack (packet);
		break;

	      default:
		break;
	}
}

void dhcpoffer (packet)
	struct packet *packet;
{
	note ("DHCPOFFER from %s",
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr));

	dump_packet (packet);
	send_request (packet);
}

void dhcpack (packet)
	struct packet *packet;
{
	note ("DHCPACK from %s",
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr));
	dump_packet (packet);
}

void dhcpnak (packet)
	struct packet *packet;
{
	note ("DHCPNAK from %s",
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr));
}

static u_int8_t requested_options [] = {
	DHO_DHCP_REQUESTED_ADDRESS,
	DHO_SUBNET_MASK,
	DHO_ROUTERS,
	DHO_DOMAIN_NAME_SERVERS,
	DHO_HOST_NAME,
	DHO_DOMAIN_NAME,
	DHO_BROADCAST_ADDRESS };

void send_discover (interface)
	struct interface_info *interface;
{
	struct sockaddr_in to;
	int result;
	struct dhcp_packet raw;
	unsigned char discover = DHCPDISCOVER;
	struct packet outgoing;

	struct tree_cache *options [256];
	struct tree_cache dhcpdiscover_tree;
	struct tree_cache dhcprqo_tree;

	memset (options, 0, sizeof options);
	memset (&outgoing, 0, sizeof outgoing);
	memset (&raw, 0, sizeof raw);
	outgoing.raw = &raw;

	/* Set DHCP_MESSAGE_TYPE to DHCPNAK */
	options [DHO_DHCP_MESSAGE_TYPE] = &dhcpdiscover_tree;
	options [DHO_DHCP_MESSAGE_TYPE] -> value = &discover;
	options [DHO_DHCP_MESSAGE_TYPE] -> len = sizeof discover;
	options [DHO_DHCP_MESSAGE_TYPE] -> buf_size = sizeof discover;
	options [DHO_DHCP_MESSAGE_TYPE] -> timeout = 0xFFFFFFFF;
	options [DHO_DHCP_MESSAGE_TYPE] -> tree = (struct tree *)0;

	/* Set DHCP_MESSAGE to whatever the message is */
	options [DHO_DHCP_MESSAGE] = &dhcprqo_tree;
	options [DHO_DHCP_MESSAGE] -> value = requested_options;
	options [DHO_DHCP_MESSAGE] -> len = sizeof requested_options;
	options [DHO_DHCP_MESSAGE] -> buf_size = sizeof requested_options;
	options [DHO_DHCP_MESSAGE] -> timeout = 0xFFFFFFFF;
	options [DHO_DHCP_MESSAGE] -> tree = (struct tree *)0;

	/* Set up the option buffer... */
	cons_options ((struct packet *)0, &outgoing, options, 0, 0);

	memset (&raw.ciaddr, 0, sizeof raw.ciaddr);
	memset (&raw.siaddr, 0, sizeof raw.siaddr);
	memset (&raw.giaddr, 0, sizeof raw.giaddr);
	memcpy (raw.chaddr,
		interface -> hw_address.haddr, interface -> hw_address.hlen);
	raw.hlen = interface -> hw_address.hlen;
	raw.htype = interface -> hw_address.htype;

	raw.xid = random ();
	raw.secs = 1;
	raw.flags = htons (BOOTP_BROADCAST);
	raw.hops = 0;
	raw.op = BOOTREQUEST;

	/* Report what we're sending... */
	note ("DHCPDISCOVER to %s", interface -> name);

#ifdef DEBUG_PACKET
	dump_packet (&outgoing);
	dump_raw ((unsigned char *)&raw, outgoing.packet_length);
#endif

	/* Set up the common stuff... */
	to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
	to.sin_len = sizeof to;
#endif
	memset (to.sin_zero, 0, sizeof to.sin_zero);

	to.sin_addr.s_addr = htonl (INADDR_BROADCAST);
	to.sin_port = htons (ntohs (server_port) - 1); /* XXX */

	errno = 0;
	result = send_packet (interface, (struct packet *)0,
			      &raw, outgoing.packet_length,
			      raw.siaddr, &to, (struct hardware *)0);
	if (result < 0)
		warn ("send_packet: %m");
}

void send_request (packet)
	struct packet *packet;
{
	struct sockaddr_in to;
	int result;
	struct dhcp_packet raw;
	unsigned char request = DHCPREQUEST;
	struct packet outgoing;

	struct tree_cache *options [256];
	struct tree_cache dhcprequest_tree;
	struct tree_cache dhcprqo_tree;
	struct tree_cache dhcprqa_tree;
	struct tree_cache dhcpsid_tree;

	memset (options, 0, sizeof options);
	memset (&outgoing, 0, sizeof outgoing);
	memset (&raw, 0, sizeof raw);
	outgoing.raw = &raw;

	/* Set DHCP_MESSAGE_TYPE to DHCPNAK */
	options [DHO_DHCP_MESSAGE_TYPE] = &dhcprequest_tree;
	options [DHO_DHCP_MESSAGE_TYPE] -> value = &request;
	options [DHO_DHCP_MESSAGE_TYPE] -> len = sizeof request;
	options [DHO_DHCP_MESSAGE_TYPE] -> buf_size = sizeof request;
	options [DHO_DHCP_MESSAGE_TYPE] -> timeout = 0xFFFFFFFF;
	options [DHO_DHCP_MESSAGE_TYPE] -> tree = (struct tree *)0;

	/* Set DHCP_MESSAGE to whatever the message is */
	options [DHO_DHCP_MESSAGE] = &dhcprqo_tree;
	options [DHO_DHCP_MESSAGE] -> value = requested_options;
	options [DHO_DHCP_MESSAGE] -> len = sizeof requested_options;
	options [DHO_DHCP_MESSAGE] -> buf_size = sizeof requested_options;
	options [DHO_DHCP_MESSAGE] -> timeout = 0xFFFFFFFF;
	options [DHO_DHCP_MESSAGE] -> tree = (struct tree *)0;

	/* Request the address we were offered... */
	options [DHO_DHCP_REQUESTED_ADDRESS] = &dhcprqa_tree;
	options [DHO_DHCP_REQUESTED_ADDRESS] -> value = 
		(unsigned char *)&packet -> raw -> yiaddr;
	options [DHO_DHCP_REQUESTED_ADDRESS] -> len = 4;
	options [DHO_DHCP_REQUESTED_ADDRESS] -> buf_size = 4;
	options [DHO_DHCP_REQUESTED_ADDRESS] -> timeout = 0xFFFFFFFF;
	options [DHO_DHCP_REQUESTED_ADDRESS] -> tree = (struct tree *)0;

	/* Send back the server identifier... */
        options [DHO_DHCP_SERVER_IDENTIFIER] = &dhcpsid_tree;
        options [DHO_DHCP_SERVER_IDENTIFIER] -> value =
                packet -> options [DHO_DHCP_SERVER_IDENTIFIER].data;
        options [DHO_DHCP_SERVER_IDENTIFIER] -> len =
                packet -> options [DHO_DHCP_SERVER_IDENTIFIER].len;
        options [DHO_DHCP_SERVER_IDENTIFIER] -> buf_size =
                packet -> options [DHO_DHCP_SERVER_IDENTIFIER].len;
        options [DHO_DHCP_SERVER_IDENTIFIER] -> timeout = 0xFFFFFFFF;
        options [DHO_DHCP_SERVER_IDENTIFIER] -> tree = (struct tree *)0;

	/* Set up the option buffer... */
	cons_options ((struct packet *)0, &outgoing, options, 0, 0);

	memset (&raw.ciaddr, 0, sizeof raw.ciaddr);
	raw.siaddr = packet -> raw -> siaddr;
	raw.giaddr = packet -> raw -> giaddr;
	memcpy (raw.chaddr,
		packet -> interface -> hw_address.haddr,
		packet -> interface -> hw_address.hlen);
	raw.hlen = packet -> interface -> hw_address.hlen;
	raw.htype = packet -> interface -> hw_address.htype;

	raw.xid = packet -> raw -> xid;
	raw.secs = packet -> raw -> secs;
	raw.flags = htons (BOOTP_BROADCAST);
	raw.hops = packet -> raw -> hops;
	raw.op = BOOTREQUEST;

	/* Report what we're sending... */
	note ("DHCPREQUEST to %s", packet -> interface -> name);

#ifdef DEBUG_PACKET
	dump_packet (&outgoing);
	dump_raw ((unsigned char *)&raw, outgoing.packet_length);
#endif

	/* Set up the common stuff... */
	to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
	to.sin_len = sizeof to;
#endif
	memset (to.sin_zero, 0, sizeof to.sin_zero);

	to.sin_addr.s_addr = htonl (INADDR_BROADCAST);
	to.sin_port = htons (ntohs (server_port) - 1); /* XXX */

	errno = 0;
	result = send_packet (packet -> interface, (struct packet *)0,
			      &raw, outgoing.packet_length,
			      raw.siaddr, &to, (struct hardware *)0);
	if (result < 0)
		warn ("send_packet: %m");
}
