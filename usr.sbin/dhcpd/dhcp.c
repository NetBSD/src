/* dhcp.c

   DHCP Protocol engine. */

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
"$Id: dhcp.c,v 1.1.1.1 1996/10/03 06:33:25 mrg Exp $ Copyright (c) 1995, 1996 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

static unsigned char dhcp_message [256];

void dhcp (packet)
	struct packet *packet;
{
	if (!locate_network (packet))
		return;

	switch (packet -> packet_type) {
	      case DHCPDISCOVER:
		dhcpdiscover (packet);
		break;

	      case DHCPREQUEST:
		dhcprequest (packet);
		break;

	      case DHCPRELEASE:
		dhcprelease (packet);
		break;

	      case DHCPDECLINE:
		dhcpdecline (packet);
		break;

	      case DHCPINFORM:
		dhcpinform (packet);
		break;

	      default:
		break;
	}
}

void dhcpdiscover (packet)
	struct packet *packet;
{
	struct lease *lease = find_lease (packet, packet -> shared_network);
	struct host_decl *hp;

	note ("DHCPDISCOVER from %s via %s",
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr),
	      packet -> raw -> giaddr.s_addr
	      ? inet_ntoa (packet -> raw -> giaddr)
	      : packet -> interface -> name);

	/* Sourceless packets don't make sense here. */
	if (!packet -> shared_network) {
		note ("Packet from unknown subnet: %s",
		      inet_ntoa (packet -> raw -> giaddr));
		return;
	}

	/* If we didn't find a lease, try to allocate one... */
	if (!lease) {
		lease = packet -> shared_network -> last_lease;

		/* If there are no leases in that subnet that have
		   expired, we have nothing to offer this client. */
		if (!lease || lease -> ends > cur_time) {
			note ("no free leases on subnet %s",
			      packet -> shared_network -> name);
			return;
		}

		/* Try to find a host_decl that matches the client
		   identifier or hardware address on the packet, and
		   has no fixed IP address.   If there is one, hang
		   it off the lease so that its option definitions
		   can be used. */
		if (((packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].len
		      != 0) &&
		     ((hp = find_hosts_by_uid
		       (packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].data,
			packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].len))
		      != (struct host_decl *)0)) ||
		    ((hp = find_hosts_by_haddr (packet -> raw -> htype,
						packet -> raw -> chaddr,
						packet -> raw -> hlen))
		     != (struct host_decl *)0)) {
			for (; hp; hp = hp -> n_ipaddr) {
				if (!hp -> fixed_addr) {
					lease -> host = hp;
					break;
				}
			}
		} else {
			lease -> host = (struct host_decl *)0;
		}
	}

	/* If this subnet won't boot unknown clients, ignore the
	   request. */
	if (!lease -> host &&
	    !lease -> subnet -> group -> boot_unknown_clients) {
		note ("Ignoring unknown client %s",
		      print_hw_addr (packet -> raw -> htype,
				     packet -> raw -> hlen,
				     packet -> raw -> chaddr));
	} else
		ack_lease (packet, lease, DHCPOFFER, cur_time + 120);
}

void dhcprequest (packet)
	struct packet *packet;
{
	struct lease *lease;
	struct iaddr cip;
	struct subnet *subnet;

	if (packet -> options [DHO_DHCP_REQUESTED_ADDRESS].len) {
		cip.len = 4;
		memcpy (cip.iabuf,
			packet -> options [DHO_DHCP_REQUESTED_ADDRESS].data,
			4);
	} else {
		cip.len = 4;
		memcpy (cip.iabuf, &packet -> raw -> ciaddr.s_addr, 4);
	}

	/* Find the lease that matches the address requested by the
	   client. */
	subnet = find_subnet (cip);
	lease = find_lease (packet, (subnet
				     ? subnet -> shared_network
				     : (struct shared_network *)0));

	note ("DHCPREQUEST for %s from %s via %s",
	      piaddr (cip),
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr),
	      packet -> raw -> giaddr.s_addr
	      ? inet_ntoa (packet -> raw -> giaddr)
	      : packet -> interface -> name);


	/* If a client on a given network wants to request a lease on
	   an address on a different network, NAK it.   If the Requested
	   Address option was used, the protocol says that it must have
	   been broadcast, so we can trust the source network information.

	   If ciaddr was specified and Requested Address was not, then
	   we really only know for sure what network a packet came from
	   if it came through a BOOTP gateway - if it came through an
	   IP router, we'll just have to assume that it's cool.

	   This violates the protocol spec in the case that the client
	   is in the REBINDING state and broadcasts a DHCPREQUEST on
	   the local wire.  We're supposed to check ciaddr for
	   validity in that case, but if the packet was unicast
	   through a router from a client in the RENEWING state, it
	   would look exactly the same to us and it would be very
	   bad to send a DHCPNAK.   I think we just have to live with
	   this. */
	if ((packet -> raw -> ciaddr.s_addr &&
	     packet -> raw -> giaddr.s_addr) ||
	    packet -> options [DHO_DHCP_REQUESTED_ADDRESS].len) {
		
		/* If we don't know where it came from but we do know
		   where it claims to have come from, it didn't come
		   from there.   Fry it. */
		if (!packet -> shared_network) {
			if (subnet) {
				nak_lease (packet, &cip);
				return;
			}
			/* Otherwise, ignore it. */
			return;
		}

		/* If we do know where it came from and we don't know
		   where it claims to have come from, same deal - fry it. */
		subnet = find_grouped_subnet (packet -> shared_network, cip);
		if (!subnet) {
			nak_lease (packet, &cip);
			return;
		}
	}

	/* If we own the lease that the client is asking for,
	   and it's already been assigned to the client, ack it. */
	if (lease &&
	    ((lease -> uid_len && lease -> uid_len == 
	      packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].len &&
	      !memcmp (packet -> options
		       [DHO_DHCP_CLIENT_IDENTIFIER].data,
		       lease -> uid, lease -> uid_len)) ||
	     (lease -> hardware_addr.hlen == packet -> raw -> hlen &&
	      lease -> hardware_addr.htype == packet -> raw -> htype &&
	      !memcmp (lease -> hardware_addr.haddr,
		       packet -> raw -> chaddr,
		       packet -> raw -> hlen)))) {
		ack_lease (packet, lease, DHCPACK, 0);
		return;
	}
		
	/* Otherwise, if we have a lease for this client,
	   release it, and in any case don't reply to the
	   DHCPREQUEST. */
	if (packet -> options [DHO_DHCP_SERVER_IDENTIFIER].len
	    && memcmp (packet ->
		       options [DHO_DHCP_SERVER_IDENTIFIER].data,
		       server_identifier.iabuf,
		       server_identifier.len)) {
		if (lease)
			release_lease (lease);
		return;
	}
}

void dhcprelease (packet)
	struct packet *packet;
{
	struct lease *lease = find_lease (packet, packet -> shared_network);

	note ("DHCPRELEASE of %s from %s via %s",
	      inet_ntoa (packet -> raw -> ciaddr),
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr),
	      packet -> raw -> giaddr.s_addr
	      ? inet_ntoa (packet -> raw -> giaddr)
	      : packet -> interface -> name);


	/* If we found a lease, release it. */
	if (lease) {
		release_lease (lease);
	}
}

void dhcpdecline (packet)
	struct packet *packet;
{
	struct lease *lease = find_lease (packet, packet -> shared_network);
	struct iaddr cip;

	if (packet -> options [DHO_DHCP_REQUESTED_ADDRESS].len) {
		cip.len = 4;
		memcpy (cip.iabuf,
			packet -> options [DHO_DHCP_REQUESTED_ADDRESS].data,
			4);
	} else {
		cip.len = 0;
	}

	note ("DHCPDECLINE on %s from %s via %s",
	      piaddr (cip),
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr),
	      packet -> raw -> giaddr.s_addr
	      ? inet_ntoa (packet -> raw -> giaddr)
	      : packet -> interface -> name);


	/* If we found a lease, mark it as unusable and complain. */
	if (lease) {
		abandon_lease (lease);
	}
}

void dhcpinform (packet)
	struct packet *packet;
{
	note ("DHCPINFORM from %s",
	      inet_ntoa (packet -> raw -> ciaddr));
}

void nak_lease (packet, cip)
	struct packet *packet;
	struct iaddr *cip;
{
	struct sockaddr_in to;
	struct in_addr from;
	int result;
	struct dhcp_packet raw;
	unsigned char nak = DHCPNAK;
	struct packet outgoing;
	struct hardware hto;

	struct tree_cache *options [256];
	struct tree_cache dhcpnak_tree;
	struct tree_cache dhcpmsg_tree;

	memset (options, 0, sizeof options);
	memset (&outgoing, 0, sizeof outgoing);
	memset (&raw, 0, sizeof raw);
	outgoing.raw = &raw;

	/* Set DHCP_MESSAGE_TYPE to DHCPNAK */
	options [DHO_DHCP_MESSAGE_TYPE] = &dhcpnak_tree;
	options [DHO_DHCP_MESSAGE_TYPE] -> value = &nak;
	options [DHO_DHCP_MESSAGE_TYPE] -> len = sizeof nak;
	options [DHO_DHCP_MESSAGE_TYPE] -> buf_size = sizeof nak;
	options [DHO_DHCP_MESSAGE_TYPE] -> timeout = 0xFFFFFFFF;
	options [DHO_DHCP_MESSAGE_TYPE] -> tree = (struct tree *)0;

	/* Set DHCP_MESSAGE to whatever the message is */
	options [DHO_DHCP_MESSAGE] = &dhcpmsg_tree;
	options [DHO_DHCP_MESSAGE] -> value = dhcp_message;
	options [DHO_DHCP_MESSAGE] -> len = strlen (dhcp_message);
	options [DHO_DHCP_MESSAGE] -> buf_size = strlen (dhcp_message);
	options [DHO_DHCP_MESSAGE] -> timeout = 0xFFFFFFFF;
	options [DHO_DHCP_MESSAGE] -> tree = (struct tree *)0;

	/* Do not use the client's requested parameter list. */
	packet -> options [DHO_DHCP_PARAMETER_REQUEST_LIST].len = 0;
	packet -> options [DHO_DHCP_PARAMETER_REQUEST_LIST].data =
		(unsigned char *)0;

	/* Set up the option buffer... */
	cons_options (packet, &outgoing, options, 0, 0);

/*	memset (&raw.ciaddr, 0, sizeof raw.ciaddr);*/
	memcpy (&raw.siaddr, server_identifier.iabuf, 4);
	raw.giaddr = packet -> raw -> giaddr;
	memcpy (raw.chaddr, packet -> raw -> chaddr, sizeof raw.chaddr);
	raw.hlen = packet -> raw -> hlen;
	raw.htype = packet -> raw -> htype;

	raw.xid = packet -> raw -> xid;
	raw.secs = packet -> raw -> secs;
	raw.flags = packet -> raw -> flags | htons (BOOTP_BROADCAST);
	raw.hops = packet -> raw -> hops;
	raw.op = BOOTREPLY;

	/* Report what we're sending... */
	note ("DHCPNAK on %s to %s via %s",
	      piaddr (*cip),
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr),
	      packet -> raw -> giaddr.s_addr
	      ? inet_ntoa (packet -> raw -> giaddr)
	      : packet -> interface -> name);



#ifdef DEBUG_PACKET
	dump_packet (packet);
	dump_raw ((unsigned char *)packet -> raw, packet -> packet_length);
	dump_packet (&outgoing);
	dump_raw ((unsigned char *)&raw, outgoing.packet_length);
#endif

	hto.htype = packet -> raw -> htype;
	hto.hlen = packet -> raw -> hlen;
	memcpy (hto.haddr, packet -> raw -> chaddr, hto.hlen);

	/* Set up the common stuff... */
	to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
	to.sin_len = sizeof to;
#endif
	memset (to.sin_zero, 0, sizeof to.sin_zero);

	if (server_identifier.len)
		memcpy (&from, server_identifier.iabuf, 4);
	else
		memset (&from, 0, 4);

	/* If this was gatewayed, send it back to the gateway.
	   Otherwise, broadcast it on the local network. */
	if (raw.giaddr.s_addr) {
		to.sin_addr = raw.giaddr;
		to.sin_port = server_port;

		if (outgoing.packet_length < BOOTP_MIN_LEN)
			outgoing.packet_length = BOOTP_MIN_LEN;

#ifdef USE_FALLBACK
		result = send_fallback (&fallback_interface,
					packet, &raw, outgoing.packet_length,
					from, &to, &hto);
		if (result < 0)
			warn ("send_fallback: %m");
		return;
#endif
	} else {
		to.sin_addr.s_addr = htonl (INADDR_BROADCAST);
		to.sin_port = packet->client_port;
	}

	errno = 0;
	result = send_packet (packet -> interface,
			      packet, &raw, outgoing.packet_length,
			      from, &to, (struct hardware *)0);
	if (result < 0)
		warn ("send_packet: %m");
}

void ack_lease (packet, lease, offer, when)
	struct packet *packet;
	struct lease *lease;
	unsigned char offer;
	TIME when;
{
	struct lease lt;
	TIME lease_time;
	TIME offered_lease_time;

	int bufs = 0;
	struct packet outgoing;
	struct dhcp_packet raw;
	struct tree_cache *options [256];
	struct sockaddr_in to;
	struct in_addr from;
	struct hardware hto;
	int result;

	unsigned char lease_time_buf [4];
	unsigned char lease_t1_buf [4];
	unsigned char lease_t2_buf [4];
	struct tree_cache lease_time_tree;
	struct tree_cache lease_t1_tree;
	struct tree_cache lease_t2_tree;
	struct tree_cache dhcpoffer_tree;
	struct tree_cache server_id_tree;
	struct tree_cache vendor_class_tree;
	struct tree_cache user_class_tree;
	struct tree_cache hostname_tree;
	struct tree_cache netmask_tree;

	struct class *vendor_class, *user_class;
	char *filename;
	char *server_name;
	int i;

	if (packet -> options [DHO_DHCP_CLASS_IDENTIFIER].len) {
		vendor_class =
			find_class (0,
				    packet ->
				    options [DHO_DHCP_CLASS_IDENTIFIER].data,
				    packet ->
				    options [DHO_DHCP_CLASS_IDENTIFIER].len);
	} else {
		vendor_class = (struct class *)0;
	}

	if (packet -> options [DHO_DHCP_USER_CLASS_ID].len) {
		user_class =
			find_class (0,
				    packet ->
				    options [DHO_DHCP_USER_CLASS_ID].data,
				    packet ->
				    options [DHO_DHCP_USER_CLASS_ID].len);
	} else {
		user_class = (struct class *)0;
	}

	/* Choose a filename; first from the host_decl, if any, then from
	   the user class, then from the vendor class. */
	if (lease -> host && lease -> host -> group -> filename)
		filename = lease -> host -> group -> filename;
	else if (user_class && user_class -> group -> filename)
		filename = user_class -> group -> filename;
	else if (vendor_class  && vendor_class -> group -> filename)
		filename = vendor_class -> group -> filename;
	else filename = (char *)0;

	/* Choose a server name as above. */
	if (lease -> host && lease -> host -> group -> server_name)
		server_name = lease -> host -> group -> server_name;
	else if (user_class && user_class -> group -> server_name)
		server_name = user_class -> group -> server_name;
	else if (vendor_class  && vendor_class -> group -> server_name)
		server_name = vendor_class -> group -> server_name;
	else server_name = (char *)0;

	/* At this point, we have a lease that we can offer the client.
	   Now we construct a lease structure that contains what we want,
	   and call supersede_lease to do the right thing with it. */

	memset (&lt, 0, sizeof lt);

	/* Use the ip address of the lease that we finally found in
	   the database. */
	lt.ip_addr = lease -> ip_addr;

	/* Start now. */
	lt.starts = cur_time;

	/* Figure out how long a lease to assign.    If this is a
	   dynamic BOOTP lease, its duration must be infinite. */
	if (offer) {
		if (packet -> options [DHO_DHCP_LEASE_TIME].len == 4) {
			lease_time = getULong
				(packet -> options [DHO_DHCP_LEASE_TIME].data);
			
			/* Don't let the client ask for a longer lease than
			   is supported for this subnet or host. */
			if (lease -> host &&
			    lease -> host -> group -> max_lease_time) {
				if (lease_time >
				    lease -> host -> group -> max_lease_time)
					lease_time = (lease -> host ->
						      group -> max_lease_time);
			} else {
				if (lease_time >
				    lease -> subnet -> group -> max_lease_time)
					lease_time = (lease -> subnet ->
						      group -> max_lease_time);
			}
		} else {
			if (lease -> host
			    && lease -> host -> group -> default_lease_time)
				lease_time = (lease -> host ->
					      group -> default_lease_time);
			else
				lease_time = (lease -> subnet ->
					      group -> default_lease_time);
		}
		
		lt.offered_expiry = cur_time + lease_time;
		if (when)
			lt.ends = when;
		else
			lt.ends = lt.offered_expiry;
	} else {
		if (lease -> host &&
		    lease -> host -> group -> bootp_lease_length)
			lt.ends = (cur_time +
				   lease -> host ->
				   group -> bootp_lease_length);
		else if (lease -> subnet -> group -> bootp_lease_length)
			lt.ends = (cur_time +
				   lease -> subnet ->
				   group -> bootp_lease_length);
		else if (lease -> host &&
			 lease -> host -> group -> bootp_lease_cutoff)
			lt.ends = lease -> host -> group -> bootp_lease_cutoff;
		else
			lt.ends = (lease -> subnet ->
				   group -> bootp_lease_cutoff);
		lt.offered_expiry = lt.ends;
		lt.flags = BOOTP_LEASE;
	}

	lt.timestamp = cur_time;

	/* Record the uid, if given... */
	if (packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].len) {
		lt.uid_len =
			packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].len;
		lt.uid = packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].data;
		packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].data =
			(unsigned char *)0;
		packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].len = 0;
	}

	/* Record the hardware address, if given... */
	lt.hardware_addr.hlen = packet -> raw -> hlen;
	lt.hardware_addr.htype = packet -> raw -> htype;
	memcpy (lt.hardware_addr.haddr, packet -> raw -> chaddr,
		packet -> raw -> hlen);

	lt.host = lease -> host;
	lt.subnet = lease -> subnet;
	lt.shared_network = lease -> shared_network;

	/* Record the transaction id... */
	lt.xid = packet -> raw -> xid;

	/* Don't call supersede_lease on a mocked-up lease. */
	if (lease -> flags & STATIC_LEASE)
		;
	else
	/* Install the new information about this lease in the database.
	   If this is a DHCPACK or a dynamic BOOTREPLY and we can't write
	   the lease, don't ACK it (or BOOTREPLY it) either. */
		if (!(supersede_lease (lease, &lt, !offer || offer == DHCPACK)
		      || (offer && offer != DHCPACK)))
			return;

	/* Send a response to the client... */

	memset (&outgoing, 0, sizeof outgoing);
	memset (&raw, 0, sizeof raw);
	outgoing.raw = &raw;

	/* Copy in the filename if given; otherwise, flag the filename
	   buffer as available for options. */
	if (filename)
		strncpy (raw.file, filename, sizeof raw.file);
	else
		bufs |= 1;

	/* Copy in the server name if given; otherwise, flag the
	   server_name buffer as available for options. */
	if (server_name)
		strncpy (raw.sname, server_name, sizeof raw.sname);
	else
		bufs |= 2; /* XXX */

	memcpy (raw.chaddr, packet -> raw -> chaddr, packet -> raw -> hlen);
	raw.hlen = packet -> raw -> hlen;
	raw.htype = packet -> raw -> htype;

	/* Start out with the subnet options... */
	memcpy (options, lease -> subnet -> group -> options, sizeof options);

	/* Vendor and user classes are only supported for DHCP clients. */
	if (offer) {
		/* If we have a vendor class, install those options,
		   superseding any subnet options. */
		if (vendor_class) {
			for (i = 0; i < 256; i++)
				if (vendor_class -> group -> options [i])
					options [i] = (vendor_class -> group ->
						       options [i]);
		}

		/* If we have a user class, install those options,
		   superseding any subnet and vendor class options. */
		if (user_class) {
			for (i = 0; i < 256; i++)
				if (user_class -> group -> options [i])
					options [i] = (user_class -> group ->
						       options [i]);
		}

	}

	/* If we have a host_decl structure, install the associated
	   options, superseding anything that's in the way. */
	if (lease -> host) {
		for (i = 0; i < 256; i++)
			if (lease -> host -> group -> options [i])
				options [i] = (lease -> host ->
					       group -> options [i]);
	}

	/* If we didn't get a hostname from an option somewhere, see if
	   we can get one from the lease. */
	if (!options [DHO_HOST_NAME] && lease -> hostname) {
		options [DHO_HOST_NAME] = &hostname_tree;
		options [DHO_HOST_NAME] -> value =
			(unsigned char *)lease -> hostname;
		options [DHO_HOST_NAME] -> buf_size = 
			options [DHO_HOST_NAME] -> len =
				strlen (lease -> hostname);
		options [DHO_HOST_NAME] -> timeout = 0xFFFFFFFF;
		options [DHO_HOST_NAME] -> tree = (struct tree *)0;
	}

	/* Now, if appropriate, put in DHCP-specific options that
           override those. */
	if (offer) {
		options [DHO_DHCP_MESSAGE_TYPE] = &dhcpoffer_tree;
		options [DHO_DHCP_MESSAGE_TYPE] -> value = &offer;
		options [DHO_DHCP_MESSAGE_TYPE] -> len = sizeof offer;
		options [DHO_DHCP_MESSAGE_TYPE] -> buf_size = sizeof offer;
		options [DHO_DHCP_MESSAGE_TYPE] -> timeout = 0xFFFFFFFF;
		options [DHO_DHCP_MESSAGE_TYPE] -> tree = (struct tree *)0;

		options [DHO_DHCP_SERVER_IDENTIFIER] = &server_id_tree;
		options [DHO_DHCP_SERVER_IDENTIFIER] ->
			value = server_identifier.iabuf;
		options [DHO_DHCP_SERVER_IDENTIFIER] ->
			len = server_identifier.len;
		options [DHO_DHCP_SERVER_IDENTIFIER] ->
			buf_size = server_identifier.len;
		options [DHO_DHCP_SERVER_IDENTIFIER] ->
			timeout = 0xFFFFFFFF;
		options [DHO_DHCP_SERVER_IDENTIFIER] ->
			tree = (struct tree *)0;

		/* Sanity check the lease time. */
		if ((lease->offered_expiry - cur_time) < 15)
			offered_lease_time = (lease -> subnet ->
					      group -> default_lease_time);
		else if (lease -> offered_expiry - cur_time >
			 lease -> subnet -> group -> max_lease_time) 
			offered_lease_time = (lease -> subnet ->
					      group -> max_lease_time);
		else 
			offered_lease_time =
				lease -> offered_expiry - cur_time;

		putULong (lease_time_buf, offered_lease_time);
		options [DHO_DHCP_LEASE_TIME] = &lease_time_tree;
		options [DHO_DHCP_LEASE_TIME] -> value = lease_time_buf;
		options [DHO_DHCP_LEASE_TIME] -> len = sizeof lease_time_buf;
		options [DHO_DHCP_LEASE_TIME] ->
			buf_size = sizeof lease_time_buf;
		options [DHO_DHCP_LEASE_TIME] -> timeout = 0xFFFFFFFF;
		options [DHO_DHCP_LEASE_TIME] -> tree = (struct tree *)0;

		/* Renewal time is lease time * 0.5. */
		offered_lease_time /= 2;
		putULong (lease_t1_buf, offered_lease_time);
		options [DHO_DHCP_RENEWAL_TIME] = &lease_t1_tree;
		options [DHO_DHCP_RENEWAL_TIME] -> value = lease_t1_buf;
		options [DHO_DHCP_RENEWAL_TIME] -> len = sizeof lease_t1_buf;
		options [DHO_DHCP_RENEWAL_TIME] ->
			buf_size = sizeof lease_t1_buf;
		options [DHO_DHCP_RENEWAL_TIME] -> timeout = 0xFFFFFFFF;
		options [DHO_DHCP_RENEWAL_TIME] -> tree = (struct tree *)0;

		/* Rebinding time is lease time * 0.875. */
		offered_lease_time += (offered_lease_time / 2
				       + offered_lease_time / 4);
		putULong (lease_t2_buf, offered_lease_time);
		options [DHO_DHCP_REBINDING_TIME] = &lease_t2_tree;
		options [DHO_DHCP_REBINDING_TIME] -> value = lease_t2_buf;
		options [DHO_DHCP_REBINDING_TIME] -> len = sizeof lease_t2_buf;
		options [DHO_DHCP_REBINDING_TIME] ->
			buf_size = sizeof lease_t2_buf;
		options [DHO_DHCP_REBINDING_TIME] -> timeout = 0xFFFFFFFF;
		options [DHO_DHCP_REBINDING_TIME] -> tree = (struct tree *)0;

		/* If we used the vendor class the client specified, we
		   have to return it. */
		if (vendor_class) {
			options [DHO_DHCP_CLASS_IDENTIFIER] =
				&vendor_class_tree;
			options [DHO_DHCP_CLASS_IDENTIFIER] ->
				value = (unsigned char *)vendor_class -> name;
			options [DHO_DHCP_CLASS_IDENTIFIER] ->
				len = strlen (vendor_class -> name);
			options [DHO_DHCP_CLASS_IDENTIFIER] ->
				buf_size = strlen (vendor_class -> name);
			options [DHO_DHCP_CLASS_IDENTIFIER] ->
				timeout = 0xFFFFFFFF;
			options [DHO_DHCP_CLASS_IDENTIFIER] ->
				tree = (struct tree *)0;
		}

		/* If we used the user class the client specified, we
		   have to return it. */
		if (user_class) {
			options [DHO_DHCP_USER_CLASS_ID] = &user_class_tree;
			options [DHO_DHCP_USER_CLASS_ID] ->
				value = (unsigned char *)user_class -> name;
			options [DHO_DHCP_USER_CLASS_ID] ->
				len = strlen (user_class -> name);
			options [DHO_DHCP_USER_CLASS_ID] ->
				buf_size = strlen (user_class -> name);
			options [DHO_DHCP_USER_CLASS_ID] ->
				timeout = 0xFFFFFFFF;
			options [DHO_DHCP_USER_CLASS_ID] ->
				tree = (struct tree *)0;
		}
	}

	/* Use the subnet mask from the subnet declaration if no other
	   mask has been provided. */
	if (!options [DHO_SUBNET_MASK]) {
		options [DHO_SUBNET_MASK] = &netmask_tree;
		netmask_tree.value = lease -> subnet -> netmask.iabuf;
		netmask_tree.len = lease -> subnet -> netmask.len;
		netmask_tree.buf_size = lease -> subnet -> netmask.len;
		netmask_tree.timeout = 0xFFFFFFFF;
		netmask_tree.tree = (struct tree *)0;
	}

	/* See if this is a Microsoft client that NUL-terminates its
	   strings and expects us to do likewise... */
	if (packet -> options [DHO_HOST_NAME].data &&
	    packet -> options [DHO_HOST_NAME].data
	    [packet -> options [DHO_HOST_NAME].len - 1] == '\0')
		cons_options (packet, &outgoing, options, bufs, 1);
	else
		cons_options (packet, &outgoing, options, bufs, 0);
	if (!offer && outgoing.packet_length < BOOTP_MIN_LEN)
		outgoing.packet_length = BOOTP_MIN_LEN;

	raw.ciaddr = packet -> raw -> ciaddr;
	memcpy (&raw.yiaddr, lease -> ip_addr.iabuf, 4);

	/* Figure out the address of the next server. */
	if (lease -> host && lease -> host -> group -> next_server.len)
		memcpy (&raw.siaddr,
			lease -> host -> group -> next_server.iabuf, 4);
	else if (lease -> subnet -> group -> next_server.len)
		memcpy (&raw.siaddr,
			lease -> subnet -> group -> next_server.iabuf, 4);
	else if (lease -> subnet -> interface_address.len)
		memcpy (&raw.siaddr,
			lease -> subnet -> interface_address.iabuf, 4);
	else
		memcpy (&raw.siaddr, server_identifier.iabuf, 4);

	raw.giaddr = packet -> raw -> giaddr;

	raw.xid = packet -> raw -> xid;
	raw.secs = packet -> raw -> secs;
	raw.flags = packet -> raw -> flags;
	raw.hops = packet -> raw -> hops;
	raw.op = BOOTREPLY;

	/* Say what we're doing... */
	note ("%s on %s to %s via %s",
	      (offer
	       ? (offer == DHCPACK ? "DHCPACK" : "DHCPOFFER")
	       : "BOOTREPLY"),
	      piaddr (lease -> ip_addr),
	      print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr),
	      packet -> raw -> giaddr.s_addr
	      ? inet_ntoa (packet -> raw -> giaddr)
	      : packet -> interface -> name);

	/* Set up the hardware address... */
	hto.htype = packet -> raw -> htype;
	hto.hlen = packet -> raw -> hlen;
	memcpy (hto.haddr, packet -> raw -> chaddr, hto.hlen);

	to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
	to.sin_len = sizeof to;
#endif
	memset (to.sin_zero, 0, sizeof to.sin_zero);

	if (server_identifier.len)
		memcpy (&from, server_identifier.iabuf, 4);
	else
		memset (&from, 0, 4);

#ifdef DEBUG_PACKET
	dump_packet (packet);
	dump_raw ((unsigned char *)packet -> raw, packet -> packet_length);
	dump_packet (&outgoing);
	dump_raw ((unsigned char *)&raw, outgoing.packet_length);
#endif

	/* If this was gatewayed, send it back to the gateway... */
	if (raw.giaddr.s_addr) {
		to.sin_addr = raw.giaddr;
		to.sin_port = server_port;

		if (outgoing.packet_length < BOOTP_MIN_LEN)
			outgoing.packet_length = BOOTP_MIN_LEN;

#ifdef USE_FALLBACK
		result = send_fallback (&fallback_interface,
					packet, &raw, outgoing.packet_length,
					raw.siaddr, &to, &hto);
		if (result < 0)
			warn ("send_fallback: %m");
		return;
#endif

	/* If it comes from a client who already knows its address and
	   is not requesting a broadcast response, sent it directly to
	   that client. */
	} else if (raw.ciaddr.s_addr && offer == DHCPACK &&
		   !(raw.flags & BOOTP_BROADCAST)) {
		to.sin_addr = packet -> raw -> ciaddr;
		to.sin_port = htons (ntohs (server_port) + 1); /* XXX */

#ifdef USE_FALLBACK
		result = send_fallback (&fallback_interface,
					packet, &raw, outgoing.packet_length,
					raw.siaddr, &to, &hto);
		if (result < 0)
			warn ("send_fallback: %m");
		return;
#endif

	/* Otherwise, broadcast it on the local network. */
	} else {
		to.sin_addr.s_addr = htonl (INADDR_BROADCAST);
		to.sin_port = htons (ntohs (server_port) + 1); /* XXX */
	}


	result = send_packet (packet -> interface,
			      packet, &raw, outgoing.packet_length,
			      raw.siaddr, &to, &hto);
	if (result < 0)
		warn ("sendpkt: %m");
}

struct lease *find_lease (packet, share)
	struct packet *packet;
	struct shared_network *share;
{
	struct lease *uid_lease, *ip_lease, *hw_lease;
	struct lease *lease = (struct lease *)0;
	struct iaddr cip;
	struct host_decl *hp, *host = (struct host_decl *)0;
	struct lease *fixed_lease;

	/* Try to find a host or lease that's been assigned to the
	   specified unique client identifier. */
	if (packet -> options [DHO_DHCP_CLIENT_IDENTIFIER].len) {
		/* First, try to find a fixed host entry for the specified
		   client identifier... */
		hp = find_hosts_by_uid (packet -> options
					[DHO_DHCP_CLIENT_IDENTIFIER].data,
					packet -> options
					[DHO_DHCP_CLIENT_IDENTIFIER].len);
		if (hp) {
			host = hp;
			fixed_lease = mockup_lease (packet, share, hp);
			uid_lease = (struct lease *)0;
		} else {
			uid_lease = find_lease_by_uid
				(packet -> options
				 [DHO_DHCP_CLIENT_IDENTIFIER].data,
				 packet -> options
				 [DHO_DHCP_CLIENT_IDENTIFIER].len);
			/* Find the lease matching this uid that's on the
			   network the packet came from (if any). */
			for (; uid_lease; uid_lease = uid_lease -> n_uid)
				if (uid_lease -> shared_network == share)
					break;
			fixed_lease = (struct lease *)0;
		}
	} else {
		uid_lease = (struct lease *)0;
		fixed_lease = (struct lease *)0;
	}

	/* If we didn't find a fixed lease using the uid, try doing
	   it with the hardware address... */
	if (!fixed_lease) {
		hp = find_hosts_by_haddr (packet -> raw -> htype,
					  packet -> raw -> chaddr,
					  packet -> raw -> hlen);
		if (hp) {
			host = hp; /* Save it for later. */
			fixed_lease = mockup_lease (packet, share, hp);
		}
	}

	/* Try to find a lease that's been attached to the client's
	   hardware address... */
	hw_lease = find_lease_by_hw_addr (packet -> raw -> chaddr,
					  packet -> raw -> hlen);
	/* Find the lease that's on the network the packet came from
	   (if any). */
	for (; hw_lease; hw_lease = hw_lease -> n_hw)
		if (hw_lease -> shared_network == share)
			break;

	/* Try to find a lease that's been allocated to the client's
	   IP address. */
	if (packet -> options [DHO_DHCP_REQUESTED_ADDRESS].len &&
	    packet -> options [DHO_DHCP_REQUESTED_ADDRESS].len == 4) {
		cip.len = 4;
		memcpy (cip.iabuf,
			packet -> options [DHO_DHCP_REQUESTED_ADDRESS].data,
			cip.len);
		ip_lease = find_lease_by_ip_addr (cip);
	} else if (packet -> raw -> ciaddr.s_addr) {
		cip.len = 4;
		memcpy (cip.iabuf, &packet -> raw -> ciaddr, 4);
		ip_lease = find_lease_by_ip_addr (cip);
	} else
		ip_lease = (struct lease *)0;

	/* If the requested IP address isn't on the network the packet
	   came from, don't use it (this is probably taken care of at
	   a higher level, but it's cheap to make sure here too). */
	if (ip_lease && ip_lease -> shared_network != share)
		ip_lease = (struct lease *)0;

	/* Toss ip_lease if it hasn't yet expired and the uid doesn't
	   match */
	if (ip_lease &&
	    ip_lease -> ends >= cur_time &&
	    ip_lease -> uid && ip_lease != uid_lease)
		ip_lease = (struct lease *)0;

	/* Toss hw_lease if it hasn't yet expired and the uid doesn't
	   match, except that if the hardware address matches and the
	   client is now doing dynamic BOOTP (and thus hasn't provided
	   a uid) we let the client get away with it. */
	if (hw_lease &&
	    hw_lease -> ends >= cur_time &&
	    hw_lease -> uid && hw_lease != uid_lease &&
	    (packet -> packet_type != 0 ||
	     !(hw_lease -> flags & DYNAMIC_BOOTP_OK)))
		hw_lease = (struct lease *)0;

	/* Toss extra pointers to the same lease... */
	if (ip_lease == hw_lease)
		ip_lease = (struct lease *)0;
	if (hw_lease == uid_lease)
		hw_lease = (struct lease *)0;
	if (ip_lease == uid_lease)
		ip_lease = (struct lease *)0;

	/* If we got an ip address lease, make sure it isn't assigned to
	   some *other* client!   If it was assigned to this client, we'd
	   have zeroed it out above, so the only way we can take it at this
	   point is if some other client had it but it's timed out, or if no
	   other client has ever had it. */
	if (ip_lease &&
	    ip_lease -> ends >= cur_time)
		ip_lease = (struct lease *)0;

	/* If we've already eliminated the lease, it wasn't there to
	   begin with.   If we have come up with a matching lease,
	   set the message to bad network in case we have to throw it out. */
	if (!ip_lease && !hw_lease && !uid_lease) {
		strcpy (dhcp_message, "requested address not available");
	} else {
		strcpy (dhcp_message, "requested address on bad subnet");
	}

	/* Now eliminate leases that are on the wrong network... */
	if (ip_lease &&
	    (share != ip_lease -> shared_network)) {
		release_lease (ip_lease);
		ip_lease = (struct lease *)0;
	}
	if (uid_lease &&
	    (share != uid_lease -> shared_network)) {
		release_lease (uid_lease);
		uid_lease = (struct lease *)0;
	}
	if (hw_lease &&
	    (share != hw_lease -> shared_network)) {
		release_lease (hw_lease);
		hw_lease = (struct lease *)0;
	}

	/* At this point, if fixed_lease is nonzero, we can assign it to
	   this client. */
	if (fixed_lease) {
		lease = fixed_lease;
	}

	/* If we got a lease that matched the ip address and don't have
	   a better offer, use that; otherwise, release it. */
	if (ip_lease) {
		if (lease) {
			release_lease (ip_lease);
		} else {
			lease = ip_lease;
			lease -> host = (struct host_decl *)0;
		}
	}

	/* If we got a lease that matched the client identifier, we may want
	   to use it, but if we already have a lease we like, we must free
	   the lease that matched the client identifier. */
	if (uid_lease) {
		if (lease) {
			release_lease (uid_lease);
		} else {
			lease = uid_lease;
			lease -> host = (struct host_decl *)0;
		}
	}

	/* The lease that matched the hardware address is treated likewise. */
	if (hw_lease) {
		if (lease) {
			release_lease (hw_lease);
		} else {
			lease = hw_lease;
			lease -> host = (struct host_decl *)0;
		}
	}

	/* If we found a host_decl but no matching address, try to
	   find a host_decl that has no address, and if there is one,
	   hang it off the lease so that we can use the supplied
	   options. */
	if (lease && host && !lease -> host) {
		for (; host; host = host -> n_ipaddr) {
			if (!host -> fixed_addr) {
				lease -> host = host;
				break;
			}
		}
	}

	return lease;
}

/* Search the provided host_decl structure list for an address that's on
   the specified shared network.  If one is found, mock up and return a
   lease structure for it; otherwise return the null pointer. */

struct lease *mockup_lease (packet, share, hp)
	struct packet *packet;
	struct shared_network *share;
	struct host_decl *hp;
{
	static struct lease mock;
	
	mock.subnet = find_host_for_network (&hp, &mock.ip_addr, share);
	if (!mock.subnet)
		return (struct lease *)0;
	mock.next = mock.prev = (struct lease *)0;
	mock.shared_network = mock.subnet -> shared_network;
	mock.host = hp;

	if (hp -> group -> options [DHO_DHCP_CLIENT_IDENTIFIER]) {
		mock.uid = hp -> group ->
			options [DHO_DHCP_CLIENT_IDENTIFIER] -> value;
		mock.uid_len = hp -> group ->
			options [DHO_DHCP_CLIENT_IDENTIFIER] -> len;
	} else {
		mock.uid = (unsigned char *)0;
		mock.uid_len = 0;
	}

	mock.hardware_addr = hp -> interface;
	mock.starts = mock.timestamp = mock.ends = MIN_TIME;
	mock.flags = STATIC_LEASE;
	return &mock;
}
