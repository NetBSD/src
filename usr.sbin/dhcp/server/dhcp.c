/* dhcp.c

   DHCP Protocol engine. */

/*
 * Copyright (c) 1995-2000 Internet Software Consortium.
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
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: dhcp.c,v 1.12 2000/04/26 23:11:20 thorpej Exp $ Copyright (c) 1995-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

int outstanding_pings;

static char dhcp_message [256];

void dhcp (packet)
	struct packet *packet;
{
	int ms_nulltp = 0;
	struct option_cache *oc;

	if (!locate_network (packet) && packet -> packet_type != DHCPREQUEST)
		return;

	/* Classify the client. */
	if ((oc = lookup_option (&dhcp_universe, packet -> options,
				 DHO_HOST_NAME))) {
		if (!oc -> expression)
			if (oc -> data.len &&
			    oc -> data.data [oc -> data.len - 1] == 0) {
				ms_nulltp = 1;
				oc -> data.len--;
			}
	}

	classify_client (packet);

	switch (packet -> packet_type) {
	      case DHCPDISCOVER:
		dhcpdiscover (packet, ms_nulltp);
		break;

	      case DHCPREQUEST:
		dhcprequest (packet, ms_nulltp);
		break;

	      case DHCPRELEASE:
		dhcprelease (packet, ms_nulltp);
		break;

	      case DHCPDECLINE:
		dhcpdecline (packet, ms_nulltp);
		break;

	      case DHCPINFORM:
		dhcpinform (packet, ms_nulltp);
		break;

	      default:
		break;
	}
}

void dhcpdiscover (packet, ms_nulltp)
	struct packet *packet;
	int ms_nulltp;
{
	struct lease *lease;
	char msgbuf [1024]; /* XXX */
	TIME when;
	char *s;

	lease = find_lease (packet, packet -> shared_network, 0);

	if (lease && lease -> client_hostname &&
	    db_printable (lease -> client_hostname))
		s = lease -> client_hostname;
	else
		s = (char *)0;

	/* Say what we're doing... */
	sprintf (msgbuf, "DHCPDISCOVER from %s %s%s%svia %s",
		 (packet -> raw -> htype
		  ? print_hw_addr (packet -> raw -> htype,
				   packet -> raw -> hlen,
				   packet -> raw -> chaddr)
		  : (lease
		     ? print_hex_1 (lease -> uid_len, lease -> uid, 
				    lease -> uid_len)
		     : "<no identifier>")),
		  s ? "(" : "", s ? s : "", s ? ") " : "",
		  packet -> raw -> giaddr.s_addr
		  ? inet_ntoa (packet -> raw -> giaddr)
		  : packet -> interface -> name);

	/* Sourceless packets don't make sense here. */
	if (!packet -> shared_network) {
		log_info ("Packet from unknown subnet: %s",
		      inet_ntoa (packet -> raw -> giaddr));
		return;
	}

	/* If we didn't find a lease, try to allocate one... */
	if (!lease) {
		lease = allocate_lease (packet,
					packet -> shared_network -> pools, 0);
		if (!lease) {
			log_info ("no free leases on network %s match %s",
			      packet -> shared_network -> name,
			      print_hw_addr (packet -> raw -> htype,
					     packet -> raw -> hlen,
					     packet -> raw -> chaddr));
			return;
		}
	}

	/* If it's an expired lease, get rid of any bindings. */
	if (lease -> ends < cur_time && lease -> scope.bindings)
		free_bindings (&lease -> scope, MDL);

	/* Set the lease to really expire in 2 minutes, unless it has
	   not yet expired, in which case leave its expiry time alone. */
	when = cur_time + 120;
	if (when < lease -> ends)
		when = lease -> ends;

	ack_lease (packet, lease, DHCPOFFER, when, msgbuf, ms_nulltp);
}

void dhcprequest (packet, ms_nulltp)
	struct packet *packet;
	int ms_nulltp;
{
	struct lease *lease;
	struct iaddr cip;
	struct subnet *subnet;
	int ours = 0;
	struct option_cache *oc;
	struct data_string data;
	int status;
	char msgbuf [1024]; /* XXX */
	char *s;

	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_DHCP_REQUESTED_ADDRESS);
	memset (&data, 0, sizeof data);
	if (oc &&
	    evaluate_option_cache (&data, packet, (struct lease *)0,
				   packet -> options, (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		cip.len = 4;
		memcpy (cip.iabuf, data.data, 4);
		data_string_forget (&data, MDL);
	} else {
		oc = (struct option_cache *)0;
		cip.len = 4;
		memcpy (cip.iabuf, &packet -> raw -> ciaddr.s_addr, 4);
	}
	subnet = find_subnet (cip);

	/* Find the lease that matches the address requested by the
	   client. */

	if (subnet)
		lease = find_lease (packet, subnet -> shared_network, &ours);
	else
		lease = (struct lease *)0;

	if (lease && lease -> client_hostname &&
	    db_printable (lease -> client_hostname))
		s = lease -> client_hostname;
	else
		s = (char *)0;

	/* Say what we're doing... */
	sprintf (msgbuf, "DHCPREQUEST for %s from %s %s%s%svia %s",
		 piaddr (cip),
		 (packet -> raw -> htype
		  ? print_hw_addr (packet -> raw -> htype,
				   packet -> raw -> hlen,
				   packet -> raw -> chaddr)
		  : (lease
		     ? print_hex_1 (lease -> uid_len, lease -> uid, 
				    lease -> uid_len)
		     : "<no identifier>")),
		 s ? "(" : "", s ? s : "", s ? ") " : "",
		  packet -> raw -> giaddr.s_addr
		  ? inet_ntoa (packet -> raw -> giaddr)
		  : packet -> interface -> name);

	/* If a client on a given network REQUESTs a lease on an
	   address on a different network, NAK it.  If the Requested
	   Address option was used, the protocol says that it must
	   have been broadcast, so we can trust the source network
	   information.

	   If ciaddr was specified and Requested Address was not, then
	   we really only know for sure what network a packet came from
	   if it came through a BOOTP gateway - if it came through an
	   IP router, we can't respond.   However, a client in REBINDING
	   state with the wrong IP address will also look like this, and
	   this is more likely, so we NAK these packets - if the packet
	   came through a router, the NAK won't reach the client anyway.

	   If we don't think we know where the packet came from, it
	   came through a gateway from an unknown network, so it's not
	   from a RENEWING client.  If we recognize the network it
	   *thinks* it's on, we can NAK it even though we don't
	   recognize the network it's *actually* on; otherwise we just
	   have to ignore it.

	   We don't currently try to take advantage of access to the
	   raw packet, because it's not available on all platforms.
	   So a packet that was unicast to us through a router from a
	   RENEWING client is going to look exactly like a packet that
	   was broadcast to us from an INIT-REBOOT client.

	   Since we can't tell the difference between these two kinds
	   of packets, if the packet appears to have come in off the
	   local wire, we have to treat it as if it's a RENEWING
	   client.  This means that we can't NAK a RENEWING client on
	   the local wire that has a bogus address.  The good news is
	   that we won't ACK it either, so it should revert to INIT
	   state and send us a DHCPDISCOVER, which we *can* work with.

	   Because we can't detect that a RENEWING client is on the
	   wrong wire, it's going to sit there trying to renew until
	   it gets to the REBIND state, when we *can* NAK it because
	   the packet will get to us through a BOOTP gateway.  We
	   shouldn't actually see DHCPREQUEST packets from RENEWING
	   clients on the wrong wire anyway, since their idea of their
	   local router will be wrong.  In any case, the protocol
	   doesn't really allow us to NAK a DHCPREQUEST from a
	   RENEWING client, so we can punt on this issue. */

	if (!packet -> shared_network ||
	    (packet -> raw -> ciaddr.s_addr &&
	     packet -> raw -> giaddr.s_addr) ||
	    (!oc && packet -> raw -> ciaddr.s_addr) ||
	    (oc && !packet -> raw -> ciaddr.s_addr)) {
		
		/* If we don't know where it came from but we do know
		   where it claims to have come from, it didn't come
		   from there.   Fry it. */
		if (!packet -> shared_network) {
			if (subnet && subnet -> group -> authoritative) {
				log_info ("%s: wrong network.", msgbuf);
				nak_lease (packet, &cip);
				return;
			}
			/* Otherwise, ignore it. */
			log_info ("%s: ignored (not authoritative).", msgbuf);
			return;
		}

		/* If we do know where it came from and it asked for an
		   address that is not on that shared network, nak it. */
		subnet = find_grouped_subnet (packet -> shared_network, cip);
		if (!subnet) {
			if (packet -> shared_network -> group -> authoritative)
			{
				log_info ("%s: wrong network.", msgbuf);
				nak_lease (packet, &cip);
				return;
			}
			log_info ("%s: ignored (not authoritative).", msgbuf);
			return;
		}
	}

	/* If the address the client asked for is ours, but it wasn't
           available for the client, NAK it. */
	if (!lease && ours) {
		log_info ("%s: lease %s unavailable.", msgbuf, piaddr (cip));
		nak_lease (packet, &cip);
		return;
	}

	/* Otherwise, send the lease to the client if we found one. */
	if (lease) {
		ack_lease (packet, lease, DHCPACK, 0, msgbuf, ms_nulltp);
	} else
		log_info ("%s: unknown lease %s.", msgbuf, piaddr (cip));
}

void dhcprelease (packet, ms_nulltp)
	struct packet *packet;
	int ms_nulltp;
{
	struct lease *lease;
	struct iaddr cip;
	struct option_cache *oc;
	struct data_string data;
	char *s;

	/* DHCPRELEASE must not specify address in requested-address
           option, but old protocol specs weren't explicit about this,
           so let it go. */
	if ((oc = lookup_option (&dhcp_universe, packet -> options,
				 DHO_DHCP_REQUESTED_ADDRESS))) {
		log_info ("DHCPRELEASE from %s specified requested-address.",
		      print_hw_addr (packet -> raw -> htype,
				     packet -> raw -> hlen,
				     packet -> raw -> chaddr));
	}

	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_DHCP_CLIENT_IDENTIFIER);
	memset (&data, 0, sizeof data);
	if (oc &&
	    evaluate_option_cache (&data, packet, (struct lease *)0,
				   packet -> options, (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		lease = find_lease_by_uid (data.data, data.len);
		data_string_forget (&data, MDL);

		/* See if we can find a lease that matches the IP address
		   the client is claiming. */
		for (; lease; lease = lease -> n_uid) {
			if (!memcmp (&packet -> raw -> ciaddr,
				     lease -> ip_addr.iabuf, 4)) {
				break;
			}
		}
	} else
		lease = (struct lease *)0;

	/* The client is supposed to pass a valid client-identifier,
	   but the spec on this has changed historically, so try the
	   IP address in ciaddr if the client-identifier fails. */
	if (!lease) {
		cip.len = 4;
		memcpy (cip.iabuf, &packet -> raw -> ciaddr, 4);
		lease = find_lease_by_ip_addr (cip);
	}


	if (lease && lease -> client_hostname &&
	    db_printable (lease -> client_hostname))
		s = lease -> client_hostname;
	else
		s = (char *)0;

	/* Say what we're doing... */
	log_info ("DHCPRELEASE of %s from %s %s%s%svia %s (%sfound)",
		  inet_ntoa (packet -> raw -> ciaddr),
		  (packet -> raw -> htype
		   ? print_hw_addr (packet -> raw -> htype,
			     packet -> raw -> hlen,
			     packet -> raw -> chaddr)
		   : (lease
		      ? print_hex_1 (lease -> uid_len, lease -> uid, 
				     lease -> uid_len)
		      : "<no identifier>")),
		  s ? "(" : "", s ? s : "", s ? ") " : "",
		  packet -> raw -> giaddr.s_addr
		  ? inet_ntoa (packet -> raw -> giaddr)
		  : packet -> interface -> name,
		  lease ? "" : "not ");

	/* If we found a lease, release it. */
	if (lease && lease -> ends > cur_time)
		release_lease (lease, packet);
}

void dhcpdecline (packet, ms_nulltp)
	struct packet *packet;
	int ms_nulltp;
{
	struct lease *lease;
	struct iaddr cip;
	struct option_cache *oc;
	struct data_string data;
	struct option_state *options = (struct option_state *)0;
	int ignorep;
	int i;
	const char *status;
	char *s;

	/* DHCPDECLINE must specify address. */
	if (!(oc = lookup_option (&dhcp_universe, packet -> options,
				  DHO_DHCP_REQUESTED_ADDRESS)))
		return;
	memset (&data, 0, sizeof data);
	if (!evaluate_option_cache (&data, packet, (struct lease *)0,
				    packet -> options,
				    (struct option_state *)0,
				    &global_scope, oc, MDL))
		return;

	cip.len = 4;
	memcpy (cip.iabuf, data.data, 4);
	data_string_forget (&data, MDL);
	lease = find_lease_by_ip_addr (cip);

	option_state_allocate (&options, MDL);

	/* Execute statements in scope starting with the subnet scope. */
	if (lease)
		execute_statements_in_scope (packet, (struct lease *)0,
					     packet -> options, options,
					     &global_scope,
					     lease -> subnet -> group,
					     (struct group *)0);

	/* Execute statements in the class scopes. */
	for (i = packet -> class_count; i > 0; i--) {
		execute_statements_in_scope
			(packet, (struct lease *)0, packet -> options, options,
			 &global_scope, packet -> classes [i - 1] -> group,
			 lease ? lease -> subnet -> group : (struct group *)0);
	}

	/* Drop the request if dhcpdeclines are being ignored. */
	oc = lookup_option (&server_universe, options, SV_DECLINES);
	if (!oc ||
	    evaluate_boolean_option_cache (&ignorep, packet, lease,
					   packet -> options, options,
					   &lease -> scope, oc, MDL)) {
		/* If we found a lease, mark it as unusable and complain. */
		if (lease) {
			abandon_lease (lease, "declined.");
			status = "";
		}
		status = " (not found)";
	} else
		status = " (ignored)";

	if (!ignorep) {
		char *s;
		if (lease && lease -> client_hostname &&
		    db_printable (lease -> client_hostname))
			s = lease -> client_hostname;
		else
			s = (char *)0;

		log_info ("DHCPDECLINE of %s from %s %s%s%svia %s %s",
			  piaddr (cip),
			  (packet -> raw -> htype
			   ? print_hw_addr (packet -> raw -> htype,
					    packet -> raw -> hlen,
					    packet -> raw -> chaddr)
			   : (lease
			      ? print_hex_1 (lease -> uid_len, lease -> uid, 
					     lease -> uid_len)
			      : "<no identifier>")),
			  s ? "(" : "", s ? s : "", s ? ") " : "",
			  packet -> raw -> giaddr.s_addr
			  ? inet_ntoa (packet -> raw -> giaddr)
			  : packet -> interface -> name,
			  status);
	}
		
	option_state_dereference (&options, MDL);
}

void dhcpinform (packet, ms_nulltp)
	struct packet *packet;
	int ms_nulltp;
{
	char msgbuf [1024];
	struct data_string d1, prl;
	struct option_cache *oc;
	struct expression *expr;
	struct option_state *options = (struct option_state *)0;
	struct dhcp_packet raw;
	struct packet outgoing;
	unsigned char dhcpack = DHCPACK;
	struct subnet *subnet;
	struct iaddr cip;
	unsigned i, j;
	int nulltp;
	struct sockaddr_in to;
	struct in_addr from;

	/* The client should set ciaddr to its IP address, but apparently
	   it's common for clients not to do this, so we'll use their IP
	   source address if they didn't set ciaddr. */
	if (!packet -> raw -> ciaddr.s_addr) {
		cip.len = 4;
		memcpy (cip.iabuf, &packet -> client_addr, 4);
	} else {
		cip.len = 4;
		memcpy (cip.iabuf, &packet -> raw -> ciaddr, 4);
	}

	sprintf (msgbuf, "DHCPINFORM from %s via %s",
		 piaddr (cip), packet -> interface -> name);

	/* If the IP source address is zero, don't respond. */
	if (!memcmp (cip.iabuf, "\0\0\0", 4)) {
		log_info ("%s: ignored (null source address).", msgbuf);
		return;
	}

	/* Find the subnet that the client is on. */
	oc = (struct option_cache *)0;
	subnet = find_subnet (cip);

	/* Sourceless packets don't make sense here. */
	if (!subnet) {
		log_info ("%s: unknown subnet %s",
			  msgbuf, inet_ntoa (packet -> raw -> giaddr));
	}

	/* We don't respond to DHCPINFORM packets if we're not authoritative.
	   It would be nice if a per-host value could override this, but
	   there's overhead involved in checking this, so let's see how people
	   react first. */
	if (subnet && !subnet -> group -> authoritative) {
		log_info ("%s: not authoritative for subnet %s",
			  msgbuf, piaddr (subnet -> net));
		return;
	}

	memset (&d1, 0, sizeof d1);
	option_state_allocate (&options, MDL);
	memset (&outgoing, 0, sizeof outgoing);
	memset (&raw, 0, sizeof raw);
	outgoing.raw = &raw;

	/* Execute statements in scope starting with the subnet scope. */
	if (subnet)
		execute_statements_in_scope (packet, (struct lease *)0,
					     packet -> options, options,
					     &global_scope, subnet -> group,
					     (struct group *)0);

	/* Execute statements in the class scopes. */
	for (i = packet -> class_count; i > 0; i--) {
		execute_statements_in_scope
			(packet, (struct lease *)0, packet -> options, options,
			 &global_scope, packet -> classes [i - 1] -> group,
			 subnet ? subnet -> group : (struct group *)0);
	}

	/* Figure out the filename. */
	memset (&d1, 0, sizeof d1);
	oc = lookup_option (&server_universe, options, SV_FILENAME);
	if (oc &&
	    evaluate_option_cache (&d1, packet, (struct lease *)0,
				   packet -> options, (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		i = d1.len;
		if (i > sizeof raw.file)
			i = sizeof raw.file;
		else
			raw.file [i] = 0;
		memcpy (raw.file, d1.data, i);
		data_string_forget (&d1, MDL);
	}

	/* Choose a server name as above. */
	oc = lookup_option (&server_universe, options, SV_SERVER_NAME);
	if (oc &&
	    evaluate_option_cache (&d1, packet, (struct lease *)0, 
				   packet -> options, (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		i = d1.len;
		if (i > sizeof raw.sname)
			i = sizeof raw.sname;
		else
			raw.sname [i] = 0;
		memcpy (raw.sname, d1.data, i);
		data_string_forget (&d1, MDL);
	}

	/* Set a flag if this client is a lame Microsoft client that NUL
	   terminates string options and expects us to do likewise. */
	nulltp = 0;
	if ((oc = lookup_option (&dhcp_universe, packet -> options,
				 DHO_HOST_NAME))) {
		if (evaluate_option_cache (&d1, packet, (struct lease *)0, 
					   packet -> options, options,
					   &global_scope, oc, MDL)) {
			if (d1.data [d1.len - 1] == '\0')
				nulltp = 1;
			data_string_forget (&d1, MDL);
		}
	}

	/* Put in DHCP-specific options. */
	i = DHO_DHCP_MESSAGE_TYPE;
	oc = (struct option_cache *)0;
	if (option_cache_allocate (&oc, MDL)) {
		if (make_const_data (&oc -> expression, &dhcpack, 1, 0, 0)) {
			oc -> option = dhcp_universe.options [i];
			save_option (&dhcp_universe, options, oc);
		}
		option_cache_dereference (&oc, MDL);
	}

	i = DHO_DHCP_SERVER_IDENTIFIER;
	if (!(oc = lookup_option (&dhcp_universe, options, i))) {
	      use_primary:
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data
			    (&oc -> expression,
			     ((unsigned char *)
			      &packet -> interface -> primary_address),
			     sizeof packet -> interface -> primary_address,
			     0, 0)) {
				oc -> option =
					dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
		from = packet -> interface -> primary_address;
	} else {
		if (evaluate_option_cache (&d1, packet, (struct lease *)0,
					   packet -> options, options,
					   &global_scope, oc, MDL)) {
			if (!d1.len || d1.len != sizeof from) {
				data_string_forget (&d1, MDL);
				goto use_primary;
			}
			memcpy (&from, d1.data, sizeof from);
			data_string_forget (&d1, MDL);
		} else
			goto use_primary;
	}

	/* Use the subnet mask from the subnet declaration if no other
	   mask has been provided. */
	i = DHO_SUBNET_MASK;
	if (subnet && !lookup_option (&dhcp_universe, options, i)) {
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data (&oc -> expression,
					     subnet -> netmask.iabuf,
					     subnet -> netmask.len, 0, 0)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe, options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
	}

	/* Make an encapsulation for the NWIP suboptions if the client
	   asked for them. */
	i = DHO_NWIP_SUBOPTIONS;
	if (!lookup_option (&dhcp_universe, options, i)) {
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			memset (&d1, 0, sizeof d1);
			d1.data = (const unsigned char *)"nwip";
			d1.len = 4;
			if (make_encapsulation (&oc -> expression, &d1)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe, options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
	}

	/* If we've been given a vendor option space, and there's something
	   in it, and we weren't given a vendor-encapsulated-options option,
	   then cons one up. */
	i = DHO_VENDOR_ENCAPSULATED_OPTIONS;
	j = SV_VENDOR_OPTION_SPACE;
	if (!lookup_option (&dhcp_universe, options, i) &&
	    (oc = lookup_option (&server_universe, options, j)) &&
	    evaluate_option_cache (&d1, packet, (struct lease *)0,
				   packet -> options, options,
				   &global_scope, oc, MDL)) {
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_encapsulation (&oc -> expression, &d1)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe, options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
		data_string_forget (&d1, MDL);
	}

	/* If a site option space has been specified, use that for
	   site option codes. */
	i = SV_SITE_OPTION_SPACE;
	if ((oc = lookup_option (&server_universe, options, i)) &&
	    evaluate_option_cache (&d1, packet, (struct lease *)0,
				   packet -> options, options,
				   &global_scope, oc, MDL)) {
		const struct universe *u;
		
		u = ((const struct universe *)
		     hash_lookup (universe_hash, d1.data, d1.len));
		if (!u) {
			log_error ("unknown option space %s.", d1.data);
			option_state_dereference (&options, MDL);
			return;
		}

		options -> site_universe = u -> index;
		options -> site_code_min = 128; /* XXX */
		data_string_forget (&d1, MDL);
	} else {
		options -> site_universe = dhcp_universe.index;
		options -> site_code_min = 0; /* Trust me, it works. */
	}

	memset (&prl, 0, sizeof prl);

	/* Use the parameter list from the scope if there is one. */
	oc = lookup_option (&dhcp_universe, options,
			    DHO_DHCP_PARAMETER_REQUEST_LIST);

	/* Otherwise, if the client has provided a list of options
	   that it wishes returned, use it to prioritize.  Otherwise,
	   prioritize based on the default priority list. */

	if (!oc)
		oc = lookup_option (&dhcp_universe, packet -> options,
				    DHO_DHCP_PARAMETER_REQUEST_LIST);

	if (oc)
		evaluate_option_cache (&prl, packet, (struct lease *)0,
				       packet -> options, options,
				       &global_scope, oc, MDL);

#ifdef DEBUG_PACKET
	dump_packet (packet);
	dump_raw ((unsigned char *)packet -> raw, packet -> packet_length);
#endif

	log_info ("%s", msgbuf);

	/* Figure out the address of the boot file server. */
	raw.siaddr = from;
	if ((oc =
	     lookup_option (&server_universe, options, SV_NEXT_SERVER))) {
		if (evaluate_option_cache (&d1, packet, (struct lease *)0,
					   packet -> options, options,
					   &global_scope, oc, MDL)) {
			/* If there was more than one answer,
			   take the first. */
			if (d1.len >= 4 && d1.data)
				memcpy (&raw.siaddr, d1.data, 4);
			data_string_forget (&d1, MDL);
		}
	}

	/* Set up the option buffer... */
	outgoing.packet_length =
		cons_options (packet, outgoing.raw, (struct lease *)0,
			      0, packet -> options, options, &global_scope,
			      0, nulltp, 0,
			      prl.len ? &prl : (struct data_string *)0);
	option_state_dereference (&options, MDL);
	data_string_forget (&prl, MDL);

	/* Make sure that the packet is at least as big as a BOOTP packet. */
	if (outgoing.packet_length < BOOTP_MIN_LEN)
		outgoing.packet_length = BOOTP_MIN_LEN;

	raw.giaddr = packet -> raw -> giaddr;
	raw.ciaddr = packet -> raw -> ciaddr;
	memcpy (raw.chaddr, packet -> raw -> chaddr, sizeof raw.chaddr);
	raw.hlen = packet -> raw -> hlen;
	raw.htype = packet -> raw -> htype;

	raw.xid = packet -> raw -> xid;
	raw.secs = packet -> raw -> secs;
	raw.flags = packet -> raw -> flags;
	raw.hops = packet -> raw -> hops;
	raw.op = BOOTREPLY;

	/* Report what we're sending... */
	log_info ("DHCPACK to %s", inet_ntoa (raw.ciaddr));

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

	/* Use the IP address we derived for the client. */
	memcpy (&to.sin_addr, cip.iabuf, 4);
	to.sin_port = remote_port;

	errno = 0;
	send_packet ((fallback_interface
		      ? fallback_interface : packet -> interface),
		     &outgoing, &raw, outgoing.packet_length,
		     from, &to, (struct hardware *)0);
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
	int i;
	struct data_string data;
	struct option_state *options = (struct option_state *)0;
	struct expression *expr;
	struct option_cache *oc = (struct option_cache *)0;

	option_state_allocate (&options, MDL);
	memset (&outgoing, 0, sizeof outgoing);
	memset (&raw, 0, sizeof raw);
	outgoing.raw = &raw;

	/* Set DHCP_MESSAGE_TYPE to DHCPNAK */
	if (!option_cache_allocate (&oc, MDL)) {
		log_error ("No memory for DHCPNAK message type.");
		option_state_dereference (&options, MDL);
		return;
	}
	if (!make_const_data (&oc -> expression, &nak, sizeof nak, 0, 0)) {
		log_error ("No memory for expr_const expression.");
		option_cache_dereference (&oc, MDL);
		option_state_dereference (&options, MDL);
		return;
	}
	oc -> option = dhcp_universe.options [DHO_DHCP_MESSAGE_TYPE];
	save_option (&dhcp_universe, options, oc);
	option_cache_dereference (&oc, MDL);
		     
	/* Set DHCP_MESSAGE to whatever the message is */
	if (!option_cache_allocate (&oc, MDL)) {
		log_error ("No memory for DHCPNAK message type.");
		option_state_dereference (&options, MDL);
		return;
	}
	if (!make_const_data (&oc -> expression,
			      (unsigned char *)dhcp_message,
			      strlen (dhcp_message), 1, 0)) {
		log_error ("No memory for expr_const expression.");
		option_cache_dereference (&oc, MDL);
		option_state_dereference (&options, MDL);
		return;
	}
	oc -> option = dhcp_universe.options [DHO_DHCP_MESSAGE];
	save_option (&dhcp_universe, options, oc);
	option_cache_dereference (&oc, MDL);
		     
	/* Do not use the client's requested parameter list. */
	delete_option (&dhcp_universe, packet -> options,
		       DHO_DHCP_PARAMETER_REQUEST_LIST);

	/* Set up the option buffer... */
	outgoing.packet_length =
		cons_options (packet, outgoing.raw, (struct lease *)0,
			      0, packet -> options, options, &global_scope,
			      0, 0, 0, (struct data_string *)0);
	option_state_dereference (&options, MDL);

/*	memset (&raw.ciaddr, 0, sizeof raw.ciaddr);*/
	raw.siaddr = packet -> interface -> primary_address;
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
	log_info ("DHCPNAK on %s to %s via %s",
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

	hto.hbuf [0] = packet -> raw -> htype;
	hto.hlen = packet -> raw -> hlen;
	memcpy (&hto.hbuf [1], packet -> raw -> chaddr, hto.hlen);
	hto.hlen++;

	/* Set up the common stuff... */
	to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
	to.sin_len = sizeof to;
#endif
	memset (to.sin_zero, 0, sizeof to.sin_zero);

	from = packet -> interface -> primary_address;

	/* Make sure that the packet is at least as big as a BOOTP packet. */
	if (outgoing.packet_length < BOOTP_MIN_LEN)
		outgoing.packet_length = BOOTP_MIN_LEN;

	/* If this was gatewayed, send it back to the gateway.
	   Otherwise, broadcast it on the local network. */
	if (raw.giaddr.s_addr) {
		to.sin_addr = raw.giaddr;
		if (raw.giaddr.s_addr != INADDR_LOOPBACK)
			to.sin_port = local_port;
		else
			to.sin_port = remote_port; /* for testing. */

		if (fallback_interface) {
			result = send_packet (fallback_interface,
					      packet, &raw,
					      outgoing.packet_length,
					      from, &to, &hto);
			return;
		}
	} else {
		to.sin_addr = limited_broadcast;
		to.sin_port = remote_port;
	}

	errno = 0;
	result = send_packet (packet -> interface,
			      packet, &raw, outgoing.packet_length,
			      from, &to, (struct hardware *)0);
}

void ack_lease (packet, lease, offer, when, msg, ms_nulltp)
	struct packet *packet;
	struct lease *lease;
	unsigned int offer;
	TIME when;
	char *msg;
	int ms_nulltp;
{
	struct lease lt;
	struct lease_state *state;
	TIME lease_time;
	TIME offered_lease_time;
	struct data_string d1;
	TIME min_lease_time;
	TIME max_lease_time;
	TIME default_lease_time;
	struct option_cache *oc;
	struct expression *expr;
	int status;

	unsigned i, j;
	int s1, s2;
	int val;
	int ignorep;

	/* If we're already acking this lease, don't do it again. */
	if (lease -> state)
		return;

	/* Allocate a lease state structure... */
	state = new_lease_state (MDL);
	if (!state)
		log_fatal ("unable to allocate lease state!");
	state -> got_requested_address = packet -> got_requested_address;
	state -> shared_network = packet -> interface -> shared_network;

	/* See if we got a server identifier option. */
	if (lookup_option (&dhcp_universe,
			   packet -> options, DHO_DHCP_SERVER_IDENTIFIER))
		state -> got_server_identifier = 1;

	/* Replace the old lease hostname with the new one, if it's changed. */
	oc = lookup_option (&dhcp_universe, packet -> options, DHO_HOST_NAME);
	memset (&d1, 0, sizeof d1);
	if (oc)
		s1 = evaluate_option_cache (&d1, packet, (struct lease *)0,
					    packet -> options,
					    (struct option_state *)0,
					    &global_scope, oc, MDL);
	if (oc && s1 &&
	    lease -> client_hostname &&
	    strlen (lease -> client_hostname) == d1.len &&
	    !memcmp (lease -> client_hostname, d1.data, d1.len)) {
		/* Hasn't changed. */
		data_string_forget (&d1, MDL);
	} else if (oc && s1) {
		if (lease -> client_hostname)
			dfree (lease -> client_hostname, MDL);
		lease -> client_hostname =
			dmalloc (d1.len + 1, MDL);
		if (!lease -> client_hostname)
			log_error ("no memory for client hostname.");
		else {
			memcpy (lease -> client_hostname, d1.data, d1.len);
			lease -> client_hostname [d1.len] = 0;
		}
		data_string_forget (&d1, MDL);
	} else if (lease -> client_hostname) {
		dfree (lease -> client_hostname, MDL);
		lease -> client_hostname = 0;
	}

	/* Steal the agent options from the packet. */
	if (packet -> options -> universes [agent_universe.index]) {
		state -> options -> universes [agent_universe.index] =
			packet -> options -> universes [agent_universe.index];
		packet -> options -> universes [agent_universe.index] =
			(struct agent_options *)0;
	}

	/* If we are offering a lease that is still currently valid, preserve
	   the events.  We need to do this because if the client does not
	   REQUEST our offer, it will expire in 2 minutes, overriding the
	   expire time in the currently in force lease.  We want the expire
	   events to be executed at that point. */
	if (lease -> ends <= cur_time && offer != DHCPOFFER) {
		/* Get rid of any old expiry or release statements - by
		   executing the statements below, we will be inserting new
		   ones if there are any to insert. */
		if (lease -> on_expiry)
			executable_statement_dereference (&lease -> on_expiry,
							  MDL);
		if (lease -> on_commit)
			executable_statement_dereference (&lease -> on_commit,
							  MDL);
		if (lease -> on_release)
			executable_statement_dereference (&lease -> on_release,
							  MDL);
	}

	/* Execute statements in scope starting with the subnet scope. */
	execute_statements_in_scope (packet, lease,
				     packet -> options,
				     state -> options, &lease -> scope,
				     lease -> subnet -> group,
				     (struct group *)0);

	/* If the lease is from a pool, run the pool scope. */
	if (lease -> pool)
		execute_statements_in_scope (packet, lease,
					     packet -> options,
					     state -> options, &lease -> scope,
					     lease -> pool -> group,
					     lease -> subnet -> group);

	/* Execute statements from class scopes. */
	for (i = packet -> class_count; i > 0; i--) {
		execute_statements_in_scope
			(packet, lease, packet -> options, state -> options,
			 &lease -> scope, packet -> classes [i - 1] -> group,
			 (lease -> pool
			  ? lease -> pool -> group
			  : lease -> subnet -> group));
	}

	/* If we have a host_decl structure, run the options associated
	   with its group. */
	if (lease -> host)
		execute_statements_in_scope (packet, lease, packet -> options,
					     state -> options, &lease -> scope,
					     lease -> host -> group,
					     (lease -> pool
					      ? lease -> pool -> group
					      : lease -> subnet -> group));
	
	/* See if the client is only supposed to have one lease at a time,
	   and if so, find its other leases and release them.    We can only
	   do this on DHCPREQUEST.    It's a little weird to do this before
	   looking at permissions, because the client might not actually
	   _get_ a lease after we've done the permission check, but the
	   assumption for this option is that the client has exactly one
	   network interface, and will only ever remember one lease.   So
	   if it sends a DHCPREQUEST, and doesn't get the lease, it's already
	   forgotten about its old lease, so we can too. */
	if (packet -> packet_type == DHCPREQUEST &&
	    (oc = lookup_option (&server_universe, state -> options,
				 SV_ONE_LEASE_PER_CLIENT)) &&
	    evaluate_boolean_option_cache (&ignorep,
					   packet, lease, packet -> options,
					   state -> options, &lease -> scope,
					   oc, MDL)) {
		struct lease *seek;
		if (lease -> uid_len) {
			do {
				seek = find_lease_by_uid (lease -> uid,
							  lease -> uid_len);
				/* Don't release expired leases, and don't
				   release the lease we're going to assign. */
				while (seek) {
					if (seek != lease &&
					    seek -> ends > cur_time)
						break;
					seek = seek -> n_uid;
				}
				if (seek) {
					release_lease (seek, packet);
				}
			} while (seek);
		}
		if (!lease -> uid_len ||
		    (lease -> host &&
		     !lease -> host -> client_identifier.len &&
		     (oc = lookup_option (&server_universe, state -> options,
					  SV_DUPLICATES)) &&
		     !evaluate_boolean_option_cache (&ignorep, packet, lease,
						     packet -> options,
						     state -> options,
						     &lease -> scope,
						     oc, MDL))) {
			do {
				seek = (find_lease_by_hw_addr
					(lease -> hardware_addr.hbuf,
					 lease -> hardware_addr.hlen));
				while (seek) {
					if (seek != lease &&
					    seek -> ends > cur_time)
						break;
					seek = seek -> n_hw;
				}
				if (seek) {
					release_lease (seek, packet);
				}
			} while (seek);
		}
	}
	

	/* Make sure this packet satisfies the configured minimum
	   number of seconds. */
	if (offer == DHCPOFFER &&
	    (oc = lookup_option (&server_universe, state -> options,
				 SV_MIN_SECS))) {
		if (evaluate_option_cache (&d1, packet, lease,
					   packet -> options, state -> options,
					   &lease -> scope, oc, MDL)) {
			if (d1.len && packet -> raw -> secs < d1.data [0]) {
				log_info ("%s: %d secs < %d", msg,
					  packet -> raw -> secs, d1.data [0]);
				data_string_forget (&d1, MDL);
				free_lease_state (state, MDL);
				static_lease_dereference (lease, MDL);
				return;
			}
			data_string_forget (&d1, MDL);
		}
	}

	/* Try to find a matching host declaration for this lease. */
	if (!lease -> host) {
		struct host_decl *hp;

		/* Try to find a host_decl that matches the client
		   identifier or hardware address on the packet, and
		   has no fixed IP address.   If there is one, hang
		   it off the lease so that its option definitions
		   can be used. */
		oc = lookup_option (&dhcp_universe, packet -> options,
				    DHO_DHCP_CLIENT_IDENTIFIER);
		if (oc &&
		    evaluate_option_cache (&d1, packet, lease,
					   packet -> options, state -> options,
					   &lease -> scope, oc, MDL)) {
			hp = find_hosts_by_uid (d1.data, d1.len);
			data_string_forget (&d1, MDL);
			if (!hp)
				hp = find_hosts_by_haddr
					(packet -> raw -> htype,
					 packet -> raw -> chaddr,
					 packet -> raw -> hlen);
			for (; hp; hp = hp -> n_ipaddr) {
				if (!hp -> fixed_addr)
					break;
			}
			lease -> host = hp;
		} else
			lease -> host = (struct host_decl *)0;
	}

	/* Drop the request if it's not allowed for this client.   By
	   default, unknown clients are allowed. */
	if (!lease -> host &&
	    (oc = lookup_option (&server_universe, state -> options,
				 SV_BOOT_UNKNOWN_CLIENTS)) &&
	    !evaluate_boolean_option_cache (&ignorep,
					    packet, lease, packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL)) {
		if (!ignorep)
			log_info ("%s: unknown client", msg);
		free_lease_state (state, MDL);
		static_lease_dereference (lease, MDL);
		return;
	} 

	/* Drop the request if it's not allowed for this client. */
	if (!offer &&
	    (oc = lookup_option (&server_universe, state -> options,
				   SV_ALLOW_BOOTP)) &&
	    !evaluate_boolean_option_cache (&ignorep,
					    packet, lease, packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL)) {
		if (!ignorep)
			log_info ("%s: bootp disallowed", msg);
		free_lease_state (state, MDL);
		static_lease_dereference (lease, MDL);
		return;
	} 

	/* Drop the request if booting is specifically denied. */
	oc = lookup_option (&server_universe, state -> options,
			    SV_ALLOW_BOOTING);
	if (oc &&
	    !evaluate_boolean_option_cache (&ignorep,
					    packet, lease, packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL)) {
		if (!ignorep)
			log_info ("%s: booting disallowed", msg);
		free_lease_state (state, MDL);
		static_lease_dereference (lease, MDL);
		return;
	}

	/* If we are configured to do per-class billing, do it. */
	if (have_billing_classes) {

		/* See if the lease is currently being billed to a
		   class, and if so, whether or not it can continue to
		   be billed to that class. */
		if (lease -> billing_class) {
			for (i = 0; i < packet -> class_count; i++)
				if (packet -> classes [i] ==
				    lease -> billing_class)
					break;
			if (i == packet -> class_count)
				unbill_class (lease, lease -> billing_class);
		}
		
		/* If we don't have an active billing, see if we need
		   one, and if we do, try to do so. */
		if (!lease -> billing_class) {
			for (i = 0; i < packet -> class_count; i++) {
				if (packet -> classes [i] -> lease_limit)
					break;
			}
			if (i != packet -> class_count) {
				for (i = 0; i < packet -> class_count; i++)
					if ((packet -> 
					     classes [i] -> lease_limit) &&
					    bill_class (lease,
							packet -> classes [i]))
						break;
				if (i == packet -> class_count) {
					log_info ("%s: no available billing",
						  msg);
					free_lease_state (state, MDL);
					/* XXX probably not necessary: */
					static_lease_dereference (lease, MDL);
					return;
				}
			}
		}
	}

	/* Figure out the filename. */
	oc = lookup_option (&server_universe, state -> options, SV_FILENAME);
	if (oc)
		evaluate_option_cache (&state -> filename, packet, lease,
				       packet -> options, state -> options,
				       &lease -> scope, oc, MDL);

	/* Choose a server name as above. */
	oc = lookup_option (&server_universe, state -> options,
			    SV_SERVER_NAME);
	if (oc)
		evaluate_option_cache (&state -> server_name, packet, lease,
				       packet -> options, state -> options,
				       &lease -> scope, oc, MDL);

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
		default_lease_time = DEFAULT_DEFAULT_LEASE_TIME;
		if ((oc = lookup_option (&server_universe, state -> options,
					 SV_DEFAULT_LEASE_TIME))) {
			if (evaluate_option_cache (&d1, packet, lease,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
				if (d1.len == sizeof (u_int32_t))
					default_lease_time =
						getULong (d1.data);
				data_string_forget (&d1, MDL);
			}
		}

		if ((oc = lookup_option (&dhcp_universe, packet -> options,
					 DHO_DHCP_LEASE_TIME)))
			s1 = evaluate_option_cache (&d1, packet, lease,
						    packet -> options,
						    state -> options,
						    &lease -> scope, oc, MDL);
		else
			s1 = 0;
		if (s1 && d1.len == sizeof (u_int32_t)) {
			lease_time = getULong (d1.data);
			data_string_forget (&d1, MDL);
		} else {
			if (s1)
				data_string_forget (&d1, MDL);
			lease_time = default_lease_time;
		}
		
		/* See if there's a maximum lease time. */
		max_lease_time = DEFAULT_MAX_LEASE_TIME;
		if ((oc = lookup_option (&server_universe, state -> options,
					 SV_MAX_LEASE_TIME))) {
			if (evaluate_option_cache (&d1, packet, lease,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
				if (d1.len == sizeof (u_int32_t))
					max_lease_time =
						getULong (d1.data);
				data_string_forget (&d1, MDL);
			}
		}

		/* Enforce the maximum lease length. */
		if (lease_time < 0 /* XXX */
		    || lease_time > max_lease_time)
			lease_time = max_lease_time;
			
		min_lease_time = DEFAULT_MIN_LEASE_TIME;
		if (min_lease_time > max_lease_time)
			min_lease_time = max_lease_time;

		if ((oc = lookup_option (&server_universe, state -> options,
					 SV_MIN_LEASE_TIME))) {
			if (evaluate_option_cache (&d1, packet, lease,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
				if (d1.len == sizeof (u_int32_t))
					min_lease_time = getULong (d1.data);
				data_string_forget (&d1, MDL);
			}
		}

		if (lease_time < min_lease_time) {
			if (min_lease_time)
				lease_time = min_lease_time;
			else
				lease_time = default_lease_time;
		}

#if defined (FAILOVER_PROTOCOL)
		/* Okay, we know the lease duration.   Now check the
		   failover state, if any. */
		if (lease -> pool -> shared_network -> failover_peer) {
			dhcp_failover_state_t *peer =
			    lease -> pool -> failover_peer;

			/* If the lease time we arrived at exceeds what
			   the peer has, first update the peer. */
			/* XXX This may result in updates that only push
			   XXX the peer's expiry time for this lease up
			   XXX by a few seconds - think about this again
			   XXX later. */
			if (cur_time + lease_time > lease -> tsfp) {
				/* Here we're assuming that if we don't have
				   to update tstp, there's already an update
				   queued.   May want to revisit this.  */
				if (cur_time + lease_time < lease -> tstp) {
					lease -> tstp = cur_time + lease_time;
					if (offer == DHCPACK)
						update_partner (lease);
				}
				/* Now choose a lease time that is either
				   MCLT, for a lease that's never before been
				   assigned, or TSFP + MCLT for a lease that
				   has.
				   XXX Note that TSFP may be < cur_time.
				   XXX What do we do in this case?
				   XXX should the expiry timer on the lease
				   XXX set tsfp and tstp to zero? */
				if (lease -> tsfp == 0)
					lease_time = peer -> mclt;
				else
					lease_time = (lease -> tsfp  - cur_time
						      + peer -> mclt);
			}
		}
#endif /* FAILOVER_PROTOCOL */

		/* If the lease duration causes the time value to wrap,
		   use the maximum expiry time. */
		if (cur_time + lease_time < cur_time)
			state -> offered_expiry = MAX_TIME;
		else
			state -> offered_expiry = cur_time + lease_time;
		if (when)
			lt.ends = when;
		else
			lt.ends = state -> offered_expiry;
	} else {
		lease_time = MAX_TIME - cur_time;

		if ((oc = lookup_option (&server_universe, state -> options,
					 SV_BOOTP_LEASE_LENGTH))) {
			if (evaluate_option_cache (&d1, packet, lease,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
				if (d1.len == sizeof (u_int32_t))
					lease_time = getULong (d1.data);
				data_string_forget (&d1, MDL);
			}
		}

		if ((oc = lookup_option (&server_universe, state -> options,
					 SV_BOOTP_LEASE_CUTOFF))) {
			if (evaluate_option_cache (&d1, packet, lease,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
				if (d1.len == sizeof (u_int32_t))
					lease_time = (getULong (d1.data) -
						      cur_time);
				data_string_forget (&d1, MDL);
			}
		}

		lt.ends = state -> offered_expiry = cur_time + lease_time;
		lt.flags = BOOTP_LEASE;
	}

	lt.timestamp = cur_time;

	/* Record the uid, if given... */
	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_DHCP_CLIENT_IDENTIFIER);
	if (oc &&
	    evaluate_option_cache (&d1, packet, lease,
				   packet -> options, state -> options,
				   &lease -> scope, oc, MDL)) {
		if (d1.len <= sizeof lt.uid_buf) {
			memcpy (lt.uid_buf, d1.data, d1.len);
			lt.uid = lt.uid_buf;
			lt.uid_max = sizeof lt.uid_buf;
			lt.uid_len = d1.len;
		} else {
			unsigned char *tuid;
			lt.uid_max = d1.len;
			lt.uid_len = d1.len;
			tuid = (unsigned char *)dmalloc (lt.uid_max, MDL);
			/* XXX inelegant */
			if (!tuid)
				log_fatal ("no memory for large uid.");
			memcpy (tuid, d1.data, lt.uid_len);
			lt.uid = tuid;
		}
		data_string_forget (&d1, MDL);
	}

	lt.host = lease -> host;
	lt.subnet = lease -> subnet;
	lt.billing_class = lease -> billing_class;

	/* Set a flag if this client is a broken client that NUL
	   terminates string options and expects us to do likewise. */
	if (ms_nulltp)
		lease -> flags |= MS_NULL_TERMINATION;
	else
		lease -> flags &= ~MS_NULL_TERMINATION;

	/* If there are statements to execute when the lease is
	   committed, execute them. */
	if (lease -> on_commit && (!offer || offer == DHCPACK)) {
		execute_statements (packet, lease, packet -> options,
				    state -> options, &lease -> scope,
				    lease -> on_commit);
		if (lease -> on_commit)
			executable_statement_dereference (&lease -> on_commit,
							  MDL);
	}

	/* Save any bindings. */
	lt.scope.bindings = lease -> scope.bindings;
	lease -> scope.bindings = (struct binding *)0;

	/* Don't call supersede_lease on a mocked-up lease. */
	if (lease -> flags & STATIC_LEASE) {
		/* Copy the hardware address into the static lease
		   structure. */
		lease -> hardware_addr.hlen = packet -> raw -> hlen + 1;
		lease -> hardware_addr.hbuf [0] = packet -> raw -> htype;
		memcpy (&lease -> hardware_addr.hbuf [1],
			packet -> raw -> chaddr,
			sizeof packet -> raw -> chaddr); /* XXX */
	} else {
		/* Record the hardware address, if given... */
		lt.hardware_addr.hlen = packet -> raw -> hlen + 1;
		lt.hardware_addr.hbuf [0] = packet -> raw -> htype;
		memcpy (&lt.hardware_addr.hbuf [1], packet -> raw -> chaddr,
			sizeof packet -> raw -> chaddr);

		/* Install the new information about this lease in the
		   database.  If this is a DHCPACK or a dynamic BOOTREPLY
		   and we can't write the lease, don't ACK it (or BOOTREPLY
		   it) either. */

		if (!(supersede_lease (lease, &lt, !offer || offer == DHCPACK)
		      || (offer && offer != DHCPACK))) {
			log_info ("%s: database update failed", msg);
			free_lease_state (state, MDL);
			static_lease_dereference (lease, MDL);
			return;
		} else {
			/* If this is a DHCPOFFER transaction, supersede_lease
			   will not add the timer for the expire event to the
			   queue.  This is because DHCPOFFERS are not commited,
			   and supersede_lease only adds commited leases to the
			   timer queue.  So if supersede_lease set this lease
			   as the next one to expire for the pool we need to
			   put it on the timer queue ourself. */
			/* XXX need to think about this. */
			if (offer == DHCPOFFER && lease -> pool &&
			    lease -> pool -> next_expiry == lease)
				add_timeout (lease -> ends, pool_timer,
					     lease -> pool);
		}
	}

	/* Remember the interface on which the packet arrived. */
	state -> ip = packet -> interface;

	/* Remember the giaddr, xid, secs, flags and hops. */
	state -> giaddr = packet -> raw -> giaddr;
	state -> ciaddr = packet -> raw -> ciaddr;
	state -> xid = packet -> raw -> xid;
	state -> secs = packet -> raw -> secs;
	state -> bootp_flags = packet -> raw -> flags;
	state -> hops = packet -> raw -> hops;
	state -> offer = offer;

	/* If we're always supposed to broadcast to this client, set
	   the broadcast bit in the bootp flags field. */
	if ((oc = lookup_option (&server_universe, state -> options,
				SV_ALWAYS_BROADCAST)) &&
	    evaluate_boolean_option_cache (&ignorep, packet, lease,
					   packet -> options, state -> options,
					   &lease -> scope, oc, MDL))
		state -> bootp_flags |= htons (BOOTP_BROADCAST);

	/* Get the Maximum Message Size option from the packet, if one
	   was sent. */
	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_DHCP_MAX_MESSAGE_SIZE);
	if (oc &&
	    evaluate_option_cache (&d1, packet, lease,
				   packet -> options, state -> options,
				   &lease -> scope, oc, MDL)) {
		if (d1.len == sizeof (u_int16_t))
			state -> max_message_size = getUShort (d1.data);
		data_string_forget (&d1, MDL);
	}

	/* Now, if appropriate, put in DHCP-specific options that
           override those. */
	if (state -> offer) {
		i = DHO_DHCP_MESSAGE_TYPE;
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data (&oc -> expression,
					     &state -> offer, 1, 0, 0)) {
				oc -> option =
					dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
		i = DHO_DHCP_SERVER_IDENTIFIER;
		if (!(oc = lookup_option (&dhcp_universe,
					  state -> options, i))) {
		 use_primary:
			oc = (struct option_cache *)0;
			if (option_cache_allocate (&oc, MDL)) {
				if (make_const_data
				    (&oc -> expression,
				     ((unsigned char *)
				      &state -> ip -> primary_address),
				     sizeof state -> ip -> primary_address,
				     0, 0)) {
					oc -> option =
						dhcp_universe.options [i];
					save_option (&dhcp_universe,
						     state -> options, oc);
				}
				option_cache_dereference (&oc, MDL);
			}
			state -> from.len =
				sizeof state -> ip -> primary_address;
			memcpy (state -> from.iabuf,
				&state -> ip -> primary_address,
				state -> from.len);
		} else {
			if (evaluate_option_cache (&d1, packet, lease,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
				if (!d1.len ||
				    d1.len > sizeof state -> from.iabuf) {
					data_string_forget (&d1, MDL);
					goto use_primary;
				}
				memcpy (state -> from.iabuf, d1.data, d1.len);
				state -> from.len = d1.len;
				data_string_forget (&d1, MDL);
			} else
				goto use_primary;
		}

		offered_lease_time =
			state -> offered_expiry - cur_time;

		putULong ((unsigned char *)&state -> expiry,
			  (unsigned long)offered_lease_time);
		i = DHO_DHCP_LEASE_TIME;
		if (lookup_option (&dhcp_universe, state -> options, i))
			log_error ("dhcp-lease-time option for %s overridden.",
			      inet_ntoa (state -> ciaddr));
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data (&oc -> expression,
					     (unsigned char *)&state -> expiry,
					     sizeof state -> expiry, 0, 0)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}

		/* Renewal time is lease time * 0.5. */
		offered_lease_time /= 2;
		putULong ((unsigned char *)&state -> renewal,
			  (unsigned long)offered_lease_time);
		i = DHO_DHCP_RENEWAL_TIME;
		if (lookup_option (&dhcp_universe, state -> options, i))
			log_error ("overriding dhcp-renewal-time for %s.",
				   inet_ntoa (state -> ciaddr));
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data (&oc -> expression,
					     (unsigned char *)
					     &state -> renewal,
					     sizeof state -> renewal, 0, 0)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}

		/* Rebinding time is lease time * 0.875. */
		offered_lease_time += (offered_lease_time / 2
				       + offered_lease_time / 4);
		putULong ((unsigned char *)&state -> rebind,
			  (unsigned)offered_lease_time);
		i = DHO_DHCP_REBINDING_TIME;
		if (lookup_option (&dhcp_universe, state -> options, i))
			log_error ("overriding dhcp-rebinding-time for %s.",
			      inet_ntoa (state -> ciaddr));
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data (&oc -> expression,
					     (unsigned char *)&state -> rebind,
					     sizeof state -> rebind, 0, 0)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
	} else {
		state -> from.len =
			sizeof state -> ip -> primary_address;
		memcpy (state -> from.iabuf,
			&state -> ip -> primary_address,
			state -> from.len);
	}

	/* Figure out the address of the boot file server. */
	memcpy (&state -> siaddr, state -> from.iabuf, sizeof state -> siaddr);
	if ((oc =
	     lookup_option (&server_universe,
			    state -> options, SV_NEXT_SERVER))) {
		if (evaluate_option_cache (&d1, packet, lease,
					   packet -> options, state -> options,
					   &lease -> scope, oc, MDL)) {
			/* If there was more than one answer,
			   take the first. */
			if (d1.len >= 4 && d1.data)
				memcpy (&state -> siaddr, d1.data, 4);
			data_string_forget (&d1, MDL);
		}
	}

	/* Use the subnet mask from the subnet declaration if no other
	   mask has been provided. */
	i = DHO_SUBNET_MASK;
	if (!lookup_option (&dhcp_universe, state -> options, i)) {
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data (&oc -> expression,
					     lease -> subnet -> netmask.iabuf,
					     lease -> subnet -> netmask.len,
					     0, 0)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
	}

	/* Use the hostname from the host declaration if there is one
	   and no hostname has otherwise been provided, and if the 
	   use-host-decl-name flag is set. */
	i = DHO_HOST_NAME;
	j = SV_USE_HOST_DECL_NAMES;
	if (!lookup_option (&dhcp_universe, state -> options, i) &&
	    lease -> host && lease -> host -> name &&
	    (evaluate_boolean_option_cache
	     (&ignorep, packet, lease, packet -> options, state -> options,
	      &lease -> scope,
	      lookup_option (&server_universe, state -> options, j), MDL))) {
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_const_data (&oc -> expression,
					     ((unsigned char *)
					      lease -> host -> name),
					     strlen (lease -> host -> name),
					     1, 0)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
	}

	/* If we don't have a hostname yet, and we've been asked to do
	   a reverse lookup to find the hostname, do it. */
	j = SV_GET_LEASE_HOSTNAMES;
	if (!lookup_option (&server_universe, state -> options, i) &&
	    (evaluate_boolean_option_cache
	     (&ignorep, packet, lease, packet -> options, state -> options,
	      &lease -> scope, lookup_option (&server_universe,
						state -> options, j), MDL))) {
		struct in_addr ia;
		struct hostent *h;
		
		memcpy (&ia, lease -> ip_addr.iabuf, 4);
		
		h = gethostbyaddr ((char *)&ia, sizeof ia, AF_INET);
		if (!h)
			log_error ("No hostname for %s", inet_ntoa (ia));
		else {
			oc = (struct option_cache *)0;
			if (option_cache_allocate (&oc, MDL)) {
				if (make_const_data (&oc -> expression,
						     ((unsigned char *)
						      h -> h_name),
						     strlen (h -> h_name) + 1,
						     1, 1)) {
					oc -> option =
						dhcp_universe.options [i];
					save_option (&dhcp_universe,
						     state -> options, oc);
				}
				option_cache_dereference (&oc, MDL);
			}
		}
	}

	/* If so directed, use the leased IP address as the router address.
	   This supposedly makes Win95 machines ARP for all IP addresses,
	   so if the local router does proxy arp, you win. */

	if (evaluate_boolean_option_cache
	    (&ignorep, packet, lease, packet -> options, state -> options,
	     &lease -> scope,
	     lookup_option (&server_universe, state -> options,
			    SV_USE_LEASE_ADDR_FOR_DEFAULT_ROUTE), MDL)) {
		i = DHO_ROUTERS;
		oc = lookup_option (&dhcp_universe, state -> options, i);
		if (!oc) {
			oc = (struct option_cache *)0;
			if (option_cache_allocate (&oc, MDL)) {
				if (make_const_data (&oc -> expression,
						     lease -> ip_addr.iabuf,
						     lease -> ip_addr.len,
						     0, 0)) {
					oc -> option =
						dhcp_universe.options [i];
					save_option (&dhcp_universe,
						     state -> options, oc);
				}
				option_cache_dereference (&oc, MDL);	
			}
		}
	}

	/* Make an encapsulation for the NWIP suboptions if the client
	   asked for them. */
	i = DHO_NWIP_SUBOPTIONS;
	if (!(oc = lookup_option (&dhcp_universe, state -> options, i))) {
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			memset (&d1, 0, sizeof d1);
			d1.data = (const unsigned char *)"nwip";
			d1.len = 4;
			if (make_encapsulation (&oc -> expression, &d1)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
	}

	/* If we've been given a vendor option space, and there's something
	   in it, and we weren't given a vendor-encapsulated-options option,
	   then cons one up. */
	i = DHO_VENDOR_ENCAPSULATED_OPTIONS;
	j = SV_VENDOR_OPTION_SPACE;
	if (!lookup_option (&dhcp_universe, state -> options, i) &&
	    (oc = lookup_option (&server_universe, state -> options, j)) &&
	    evaluate_option_cache (&d1, packet, lease,
				   packet -> options, state -> options,
				   &lease -> scope, oc, MDL)) {
		oc = (struct option_cache *)0;
		if (option_cache_allocate (&oc, MDL)) {
			if (make_encapsulation (&oc -> expression, &d1)) {
				oc -> option = dhcp_universe.options [i];
				save_option (&dhcp_universe,
					     state -> options, oc);
			}
			option_cache_dereference (&oc, MDL);
		}
		data_string_forget (&d1, MDL);
	}

	/* If a site option space has been specified, use that for
	   site option codes. */
	i = SV_SITE_OPTION_SPACE;
	if ((oc = lookup_option (&server_universe, state -> options, i)) &&
	    evaluate_option_cache (&d1, packet, lease,
				   packet -> options, state -> options,
				   &lease -> scope, oc, MDL)) {
		const struct universe *u;
		
		u = ((const struct universe *)
		     hash_lookup (universe_hash, d1.data, d1.len));
		if (!u) {
			log_error ("unknown option space %s.", d1.data);
			return;
		}

		state -> options -> site_universe = u -> index;
		state -> options -> site_code_min = 128; /* XXX */
		data_string_forget (&d1, MDL);
	} else {
		state -> options -> site_code_min = 0;
		state -> options -> site_universe = dhcp_universe.index;
	}

	/* If the client has provided a list of options that it wishes
	   returned, use it to prioritize.  If there's a parameter
	   request list in scope, use that in preference.  Otherwise
	   use the default priority list. */

	oc = lookup_option (&dhcp_universe, state -> options,
			    DHO_DHCP_PARAMETER_REQUEST_LIST);

	if (!oc)
		oc = lookup_option (&dhcp_universe, packet -> options,
				    DHO_DHCP_PARAMETER_REQUEST_LIST);
	if (oc)
		evaluate_option_cache (&state -> parameter_request_list,
				       packet, lease,
				       packet -> options, state -> options,
				       &lease -> scope, oc, MDL);

#ifdef DEBUG_PACKET
	dump_packet (packet);
	dump_raw ((unsigned char *)packet -> raw, packet -> packet_length);
#endif

	lease -> state = state;

	log_info ("%s", msg);

	/* Hang the packet off the lease state. */
	packet_reference (&lease -> state -> packet, packet, MDL);

	/* If this is a DHCPOFFER, ping the lease address before actually
	   sending the offer. */
	if (offer == DHCPOFFER && !(lease -> flags & STATIC_LEASE) &&
	    cur_time - lease -> timestamp > 60) {
		lease -> timestamp = cur_time;
		icmp_echorequest (&lease -> ip_addr);
		add_timeout (cur_time + 1, lease_ping_timeout, lease);
		++outstanding_pings;
	} else {
		lease -> timestamp = cur_time;
		static_lease_dereference (lease, MDL);
		dhcp_reply (lease);
	}
}

void dhcp_reply (lease)
	struct lease *lease;
{
	int bufs = 0;
	unsigned packet_length;
	struct dhcp_packet raw;
	struct sockaddr_in to;
	struct in_addr from;
	struct hardware hto;
	int result;
	int i;
	struct lease_state *state = lease -> state;
	int nulltp, bootpp;
	struct agent_options *a, *na;
	struct option_tag *ot, *not;
	struct data_string d1;
	struct option_cache *oc;
	char *s;

	if (!state)
		log_fatal ("dhcp_reply was supplied lease with no state!");

	/* Compose a response for the client... */
	memset (&raw, 0, sizeof raw);
	memset (&d1, 0, sizeof d1);

	/* Copy in the filename if given; otherwise, flag the filename
	   buffer as available for options. */
	if (state -> filename.len && state -> filename.data) {
		memcpy (raw.file,
			state -> filename.data,
			state -> filename.len > sizeof raw.file
			? sizeof raw.file : state -> filename.len);
		if (sizeof raw.file > state -> filename.len)
			memset (&raw.file [state -> filename.len], 0,
				(sizeof raw.file) - state -> filename.len);
	} else
		bufs |= 1;

	/* Copy in the server name if given; otherwise, flag the
	   server_name buffer as available for options. */
	if (state -> server_name.len && state -> server_name.data) {
		memcpy (raw.sname,
			state -> server_name.data,
			state -> server_name.len > sizeof raw.sname
			? sizeof raw.sname : state -> server_name.len);
		if (sizeof raw.sname > state -> server_name.len)
			memset (&raw.sname [state -> server_name.len], 0,
				(sizeof raw.sname) - state -> server_name.len);
	} else
		bufs |= 2; /* XXX */

	memcpy (raw.chaddr,
		&lease -> hardware_addr.hbuf [1], sizeof raw.chaddr);
	raw.hlen = lease -> hardware_addr.hlen - 1;
	raw.htype = lease -> hardware_addr.hbuf [0];

	/* See if this is a Microsoft client that NUL-terminates its
	   strings and expects us to do likewise... */
	if (lease -> flags & MS_NULL_TERMINATION)
		nulltp = 1;
	else
		nulltp = 0;

	/* See if this is a bootp client... */
	if (state -> offer)
		bootpp = 0;
	else
		bootpp = 1;

	/* Insert such options as will fit into the buffer. */
	packet_length = cons_options (state -> packet, &raw, lease,
				      state -> max_message_size,
				      state -> packet -> options,
				      state -> options, &global_scope,
				      bufs, nulltp, bootpp,
				      &state -> parameter_request_list);

	memcpy (&raw.ciaddr, &state -> ciaddr, sizeof raw.ciaddr);
	memcpy (&raw.yiaddr, lease -> ip_addr.iabuf, 4);
	raw.siaddr = state -> siaddr;
	raw.giaddr = state -> giaddr;

	raw.xid = state -> xid;
	raw.secs = state -> secs;
	raw.flags = state -> bootp_flags;
	raw.hops = state -> hops;
	raw.op = BOOTREPLY;

	if (lease -> client_hostname &&
	    db_printable (lease -> client_hostname))
		s = lease -> client_hostname;
	else
		s = (char *)0;

	/* Say what we're doing... */
	log_info ("%s on %s to %s %s%s%svia %s",
		  (state -> offer
		   ? (state -> offer == DHCPACK ? "DHCPACK" : "DHCPOFFER")
		   : "BOOTREPLY"),
		  piaddr (lease -> ip_addr),
		  (lease -> hardware_addr.hlen
		   ? print_hw_addr (lease -> hardware_addr.hbuf [0],
				    lease -> hardware_addr.hlen - 1,
				    &lease -> hardware_addr.hbuf [1])
		   : print_hex_1 (lease -> uid_len, lease -> uid, 
				  lease -> uid_len)),
		  s ? "(" : "", s ? s : "", s ? ") " : "",
		  (state -> giaddr.s_addr
		   ? inet_ntoa (state -> giaddr)
		   : state -> ip -> name));

	/* Set up the hardware address... */
	hto.hlen = lease -> hardware_addr.hlen;
	memcpy (hto.hbuf, lease -> hardware_addr.hbuf, hto.hlen);

	to.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
	to.sin_len = sizeof to;
#endif
	memset (to.sin_zero, 0, sizeof to.sin_zero);

#ifdef DEBUG_PACKET
	dump_raw ((unsigned char *)&raw, packet_length);
#endif

	/* Make sure outgoing packets are at least as big
	   as a BOOTP packet. */
	if (packet_length < BOOTP_MIN_LEN)
		packet_length = BOOTP_MIN_LEN;

	/* If this was gatewayed, send it back to the gateway... */
	if (raw.giaddr.s_addr) {
		to.sin_addr = raw.giaddr;
		if (raw.giaddr.s_addr != INADDR_LOOPBACK)
			to.sin_port = local_port;
		else
			to.sin_port = remote_port; /* For debugging. */

		if (fallback_interface) {
			result = send_packet (fallback_interface,
					      (struct packet *)0,
					      &raw, packet_length,
					      raw.siaddr, &to,
					      (struct hardware *)0);

			free_lease_state (state, MDL);
			lease -> state = (struct lease_state *)0;
			return;
		}

	/* If the client is RENEWING, unicast to the client using the
	   regular IP stack.  Some clients, particularly those that
	   follow RFC1541, are buggy, and send both ciaddr and server
	   identifier.  We deal with this situation by assuming that
	   if we got both dhcp-server-identifier and ciaddr, and
	   giaddr was not set, then the client is on the local
	   network, and we can therefore unicast or broadcast to it
	   successfully.  A client in REQUESTING state on another
	   network that's making this mistake will have set giaddr,
	   and will therefore get a relayed response from the above
	   code. */
	} else if (raw.ciaddr.s_addr &&
		   !((state -> got_server_identifier ||
		      (raw.flags & htons (BOOTP_BROADCAST))) &&
		     /* XXX This won't work if giaddr isn't zero, but it is: */
		     (state -> shared_network ==
		      lease -> subnet -> shared_network)) &&
		   state -> offer == DHCPACK) {
		to.sin_addr = raw.ciaddr;
		to.sin_port = remote_port;

		if (fallback_interface) {
			result = send_packet (fallback_interface,
					      (struct packet *)0,
					      &raw, packet_length,
					      raw.siaddr, &to,
					      (struct hardware *)0);
			free_lease_state (state, MDL);
			lease -> state = (struct lease_state *)0;
			return;
		}

	/* If it comes from a client that already knows its address
	   and is not requesting a broadcast response, and we can
	   unicast to a client without using the ARP protocol, sent it
	   directly to that client. */
	} else if (!(raw.flags & htons (BOOTP_BROADCAST)) &&
		   can_unicast_without_arp (state -> ip)) {
		to.sin_addr = raw.yiaddr;
		to.sin_port = remote_port;

	/* Otherwise, broadcast it on the local network. */
	} else {
		to.sin_addr = limited_broadcast;
		to.sin_port = remote_port;
	}

	memcpy (&from, state -> from.iabuf, sizeof from);

	result = send_packet (state -> ip,
			      (struct packet *)0, &raw, packet_length,
			      from, &to, &hto);

	/* Free all of the entries in the option_state structure
	   now that we're done with them. */

	free_lease_state (state, MDL);
	lease -> state = (struct lease_state *)0;
}

struct lease *find_lease (packet, share, ours)
	struct packet *packet;
	struct shared_network *share;
	int *ours;
{
	struct lease *uid_lease, *ip_lease, *hw_lease;
	struct lease *lease = (struct lease *)0;
	struct iaddr cip;
	struct host_decl *hp, *host = (struct host_decl *)0;
	struct lease *fixed_lease, *next;
	struct option_cache *oc;
	struct data_string d1;
	int have_client_identifier = 0;
	struct data_string client_identifier;
	int status;
	struct hardware h;

	/* Look up the requested address. */
	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_DHCP_REQUESTED_ADDRESS);
	memset (&d1, 0, sizeof d1);
	if (oc &&
	    evaluate_option_cache (&d1, packet, (struct lease *)0,
				   packet -> options, (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		packet -> got_requested_address = 1;
		cip.len = 4;
		memcpy (cip.iabuf, d1.data, cip.len);
		data_string_forget (&d1, MDL);
	} else if (packet -> raw -> ciaddr.s_addr) {
		cip.len = 4;
		memcpy (cip.iabuf, &packet -> raw -> ciaddr, 4);
	} else
		cip.len = 0;

	/* Try to find a host or lease that's been assigned to the
	   specified unique client identifier. */
	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_DHCP_CLIENT_IDENTIFIER);
	memset (&client_identifier, 0, sizeof client_identifier);
	if (oc &&
	    evaluate_option_cache (&client_identifier,
				   packet, (struct lease *)0,
				   packet -> options, (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		/* Remember this for later. */
		have_client_identifier = 1;

		/* First, try to find a fixed host entry for the specified
		   client identifier... */
		hp = find_hosts_by_uid (client_identifier.data,
					client_identifier.len);
		if (hp) {
			/* Remember if we know of this client. */
			packet -> known = 1;
			fixed_lease = mockup_lease (packet, share, hp);
		} else
			fixed_lease = (struct lease *)0;

#if defined (DEBUG_FIND_LEASE)
		if (fixed_lease) {
			log_info ("Found host for client identifier: %s.",
			      piaddr (fixed_lease -> ip_addr));
		}
#endif
		if (!fixed_lease)
			host = hp;	/* Save the host if we found one. */

		uid_lease = find_lease_by_uid (client_identifier.data,
					       client_identifier.len);
	} else {
		uid_lease = (struct lease *)0;
		fixed_lease = (struct lease *)0;
	}

	/* If we didn't find a fixed lease using the uid, try doing
	   it with the hardware address... */
	if (!fixed_lease && !host) {
		hp = find_hosts_by_haddr (packet -> raw -> htype,
					  packet -> raw -> chaddr,
					  packet -> raw -> hlen);
		if (hp) {
			/* Remember if we know of this client. */
			packet -> known = 1;
			host = hp; /* Save it for later. */
			fixed_lease = mockup_lease (packet, share, hp);
#if defined (DEBUG_FIND_LEASE)
			if (fixed_lease) {
				log_info ("Found host for link address: %s.",
				      piaddr (fixed_lease -> ip_addr));
			}
#endif
		}
	}

	/* If fixed_lease is present but does not match the requested
	   IP address, and this is a DHCPREQUEST, then we can't return
	   any other lease, so we might as well return now. */
	if (packet -> packet_type == DHCPREQUEST && fixed_lease &&
	    (fixed_lease -> ip_addr.len != cip.len ||
	     memcmp (fixed_lease -> ip_addr.iabuf,
		     cip.iabuf, cip.len))) {
		if (ours)
			*ours = 1;
		strcpy (dhcp_message, "requested address is incorrect");
#if defined (DEBUG_FIND_LEASE)
		log_info ("Client's fixed-address %s doesn't match %s%s",
			  piaddr (fixed_lease -> ip_addr), "request ",
			  print_dotted_quads (cip.len, cip.iabuf));
#endif
		goto out;
	}

	/* If we found leases matching the client identifier, loop through
	   the n_uid pointer looking for one that's actually valid.   We
	   can't do this until we get here because we depend on
	   packet -> known, which may be set by either the uid host
	   lookup or the haddr host lookup. */
	for (; uid_lease; uid_lease = next) {
#if defined (DEBUG_FIND_LEASE)
		log_info ("trying next lease matching client id: %s",
			  piaddr (uid_lease -> ip_addr));
#endif
		if (uid_lease -> subnet -> shared_network != share) {
#if defined (DEBUG_FIND_LEASE)
			log_info ("wrong network segment: %s",
				  piaddr (uid_lease -> ip_addr));
#endif
			next = uid_lease -> n_uid;
			continue;
		}
		if ((uid_lease -> pool -> prohibit_list &&
		     permitted (packet, uid_lease -> pool -> prohibit_list)) ||
		    (uid_lease -> pool -> permit_list &&
		     !permitted (packet, uid_lease -> pool -> permit_list))) {
#if defined (DEBUG_FIND_LEASE)
			log_info ("not permitted: %s",
				  piaddr (uid_lease -> ip_addr));
#endif
			next = uid_lease -> n_uid;
			if (!packet -> raw -> ciaddr.s_addr)
				release_lease (uid_lease, packet);
			continue;
		}
		break;
	}
#if defined (DEBUG_FIND_LEASE)
	if (uid_lease)
		log_info ("Found lease for client id: %s.",
		      piaddr (uid_lease -> ip_addr));
#endif

	/* Find a lease whose hardware address matches, whose client
	   identifier matches, that's permitted, and that's on the
	   correct subnet. */
	h.hlen = packet -> raw -> hlen + 1;
	h.hbuf [0] = packet -> raw -> htype;
	memcpy (&h.hbuf [1], packet -> raw -> chaddr, packet -> raw -> hlen);
	hw_lease = find_lease_by_hw_addr (h.hbuf, h.hlen);
	for (; hw_lease; hw_lease = next) {
#if defined (DEBUG_FIND_LEASE)
		log_info ("trying next lease matching hw addr: %s",
			  piaddr (hw_lease -> ip_addr));
#endif
		if (hw_lease -> ends >= cur_time &&
		    hw_lease -> uid &&
		    (!have_client_identifier ||
		     hw_lease -> uid_len != client_identifier.len ||
		     memcmp (hw_lease -> uid, client_identifier.data,
			     hw_lease -> uid_len))) {
#if defined (DEBUG_FIND_LEASE)
			log_info ("wrong client identifier: %s",
				  piaddr (hw_lease -> ip_addr));
#endif
			next = hw_lease -> n_hw;
			continue;
		}
		if (hw_lease -> subnet -> shared_network != share) {
#if defined (DEBUG_FIND_LEASE)
			log_info ("wrong network segment: %s",
				  piaddr (hw_lease -> ip_addr));
#endif
			next = hw_lease -> n_hw;
			continue;
		}
		if ((hw_lease -> pool -> prohibit_list &&
		      permitted (packet, hw_lease -> pool -> prohibit_list)) ||
		    (hw_lease -> pool -> permit_list &&
		     !permitted (packet, hw_lease -> pool -> permit_list))) {
#if defined (DEBUG_FIND_LEASE)
			log_info ("not permitted: %s",
				  piaddr (hw_lease -> ip_addr));
#endif
			next = hw_lease -> n_hw;
			if (!packet -> raw -> ciaddr.s_addr)
				release_lease (hw_lease, packet);
			continue;
		}
		break;
	}
#if defined (DEBUG_FIND_LEASE)
	if (hw_lease)
		log_info ("Found lease for hardware address: %s.",
		      piaddr (hw_lease -> ip_addr));
#endif

	/* Try to find a lease that's been allocated to the client's
	   IP address. */
	if (cip.len)
		ip_lease = find_lease_by_ip_addr (cip);
	else
		ip_lease = (struct lease *)0;

#if defined (DEBUG_FIND_LEASE)
	if (ip_lease)
		log_info ("Found lease for requested address: %s.",
		      piaddr (ip_lease -> ip_addr));
#endif

	/* If ip_lease is valid at this point, set ours to one, so that
	   even if we choose a different lease, we know that the address
	   the client was requesting was ours, and thus we can NAK it. */
	if (ip_lease && ours)
		*ours = 1;

	/* If the requested IP address isn't on the network the packet
	   came from, don't use it.  Allow abandoned leases to be matched
	   here - if the client is requesting it, there's a decent chance
	   that it's because the lease database got trashed and a client
	   that thought it had this lease answered an ARP or PING, causing the
	   lease to be abandoned.   If so, this request probably came from
	   that client. */
	if (ip_lease && (ip_lease -> subnet -> shared_network != share)) {
		if (ours)
			*ours = 1;
#if defined (DEBUG_FIND_LEASE)
		log_info ("...but it was on the wrong shared network.");
#endif
		strcpy (dhcp_message, "requested address on bad subnet");
		ip_lease = (struct lease *)0;
	}

	/* Toss ip_lease if it hasn't yet expired and doesn't belong to the
	   client. */
	if (ip_lease &&
	    ip_lease -> ends >= cur_time &&
	    ((ip_lease -> uid &&
	      (!have_client_identifier ||
	       ip_lease -> uid_len != client_identifier.len ||
	       memcmp (ip_lease -> uid, client_identifier.data,
		       ip_lease -> uid_len))) ||
	     (!ip_lease -> uid &&
	      (ip_lease -> hardware_addr.hbuf [0] != packet -> raw -> htype ||
	       ip_lease -> hardware_addr.hlen != packet -> raw -> hlen + 1 ||
	       memcmp (&ip_lease -> hardware_addr.hbuf [1],
		       packet -> raw -> chaddr,
		       (unsigned)(ip_lease -> hardware_addr.hlen - 1)))))) {
#if defined (DEBUG_FIND_LEASE)
		if (ip_lease)
			log_info ("rejecting lease for requested address.");
#endif
		ip_lease = (struct lease *)0;
	}

	/* If for some reason the client has more than one lease
	   on the subnet that matches its uid, pick the one that
	   it asked for and (if we can) free the other. */
	if (ip_lease &&
	    ip_lease -> ends >= cur_time &&
	    ip_lease -> uid && ip_lease != uid_lease) {
		if (have_client_identifier &&
		    (ip_lease -> uid_len == client_identifier.len) &&
		    !memcmp (client_identifier.data,
			     ip_lease -> uid, ip_lease -> uid_len)) {
			if (uid_lease) {
			    if (uid_lease -> ends > cur_time) {
				log_error ("client %s has duplicate%s on %s",
					   (print_hw_addr
					    (packet -> raw -> htype,
					     packet -> raw -> hlen,
					     packet -> raw -> chaddr)),
					   " leases",
					   (ip_lease -> subnet ->
					    shared_network -> name));

				/* If the client is REQUESTing the lease,
				   it shouldn't still be using the old
				   one, so we can free it for allocation. */
				if (uid_lease &&
				    !packet -> raw -> ciaddr.s_addr &&
				    (share ==
				     uid_lease -> subnet -> shared_network) &&
				    packet -> packet_type == DHCPREQUEST)
					dissociate_lease (uid_lease);
			    }
			    uid_lease = ip_lease;
			}
		}

		/* If we get to here and fixed_lease is not null, that means
		   that there are both a dynamic lease and a fixed-address
		   declaration for the same IP address. */
		if (packet -> packet_type == DHCPREQUEST && fixed_lease) {
			fixed_lease = (struct lease *)0;
		      db_conflict:
			log_error ("Dynamic and static leases present for %s.",
				   piaddr (cip));
			log_error ("Remove host declaration %s or remove %s",
				   (fixed_lease && fixed_lease -> host
				    ? (fixed_lease -> host -> name
				       ? fixed_lease -> host -> name
				       : piaddr (cip))
				    : piaddr (cip)),
				    piaddr (cip));
			log_error ("from the dynamic address pool for %s",
				   ip_lease -> subnet -> shared_network -> name
				  );
			if (fixed_lease)
				ip_lease = (struct lease *)0;
			strcpy (dhcp_message,
				"database conflict - call for help!");
		}
	}

	/* If we get to here with both fixed_lease and ip_lease not
	   null, then we have a configuration file bug. */
	if (packet -> packet_type == DHCPREQUEST && fixed_lease && ip_lease)
		goto db_conflict;

	/* Make sure the client is permitted to use the requested lease. */
	if (ip_lease &&
	    ((ip_lease -> pool -> prohibit_list &&
	      permitted (packet, ip_lease -> pool -> prohibit_list)) ||
	     (ip_lease -> pool -> permit_list &&
	      !permitted (packet, ip_lease -> pool -> permit_list)))) {
		if (!packet -> raw -> ciaddr.s_addr)
			release_lease (ip_lease, packet);
		ip_lease = (struct lease *)0;
	}

	/* Toss extra pointers to the same lease... */
	if (hw_lease == uid_lease) {
#if defined (DEBUG_FIND_LEASE)
		log_info ("hardware lease and uid lease are identical.");
#endif
		hw_lease = (struct lease *)0;
	}
	if (ip_lease == hw_lease) {
		hw_lease = (struct lease *)0;
#if defined (DEBUG_FIND_LEASE)
		log_info ("hardware lease and ip lease are identical.");
#endif
	}
	if (ip_lease == uid_lease) {
		uid_lease = (struct lease *)0;
#if defined (DEBUG_FIND_LEASE)
		log_info ("uid lease and ip lease are identical.");
#endif
	}

	/* If we've already eliminated the lease, it wasn't there to
	   begin with.   If we have come up with a matching lease,
	   set the message to bad network in case we have to throw it out. */
	if (!ip_lease) {
		strcpy (dhcp_message, "requested address not available");
	}

	/* If this is a DHCPREQUEST, make sure the lease we're going to return
	   matches the requested IP address.   If it doesn't, don't return a
	   lease at all. */
	if (packet -> packet_type == DHCPREQUEST &&
	    !ip_lease && !fixed_lease) {
#if defined (DEBUG_FIND_LEASE)
		log_info ("no applicable lease found for DHCPREQUEST.");
#endif
		goto out;
	}

	/* At this point, if fixed_lease is nonzero, we can assign it to
	   this client. */
	if (fixed_lease) {
		lease = fixed_lease;
#if defined (DEBUG_FIND_LEASE)
		log_info ("choosing fixed address.");
#endif
	}

	/* If we got a lease that matched the ip address and don't have
	   a better offer, use that; otherwise, release it. */
	if (ip_lease) {
		if (lease) {
			if (!packet -> raw -> ciaddr.s_addr)
				release_lease (ip_lease, packet);
#if defined (DEBUG_FIND_LEASE)
			log_info ("not choosing requested address (!).");
#endif
		} else {
#if defined (DEBUG_FIND_LEASE)
			log_info ("choosing lease on requested address.");
#endif
			lease = ip_lease;
			lease -> host = (struct host_decl *)0;
		}
	}

	/* If we got a lease that matched the client identifier, we may want
	   to use it, but if we already have a lease we like, we must free
	   the lease that matched the client identifier. */
	if (uid_lease) {
		if (lease) {
			if (!packet -> raw -> ciaddr.s_addr &&
			    packet -> packet_type == DHCPREQUEST)
				dissociate_lease (uid_lease);
#if defined (DEBUG_FIND_LEASE)
			log_info ("not choosing uid lease.");
#endif
		} else {
			lease = uid_lease;
			lease -> host = (struct host_decl *)0;
#if defined (DEBUG_FIND_LEASE)
			log_info ("choosing uid lease.");
#endif
		}
	}

	/* The lease that matched the hardware address is treated likewise. */
	if (hw_lease) {
		if (lease) {
			if (!packet -> raw -> ciaddr.s_addr &&
			    packet -> packet_type == DHCPREQUEST)
				dissociate_lease (hw_lease);
#if defined (DEBUG_FIND_LEASE)
			log_info ("not choosing hardware lease.");
#endif
		} else {
			lease = hw_lease;
			lease -> host = (struct host_decl *)0;
#if defined (DEBUG_FIND_LEASE)
			log_info ("choosing hardware lease.");
#endif
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

	/* If we find an abandoned lease, but it's the one the client
	   requested, we assume that previous bugginess on the part
	   of the client, or a server database loss, caused the lease to
	   be abandoned, so we reclaim it and let the client have it. */
	if (lease && (lease -> flags & ABANDONED_LEASE) && lease == ip_lease &&
	    packet -> packet_type == DHCPREQUEST) {
		log_error ("Reclaiming REQUESTed abandoned IP address %s.",
		      piaddr (lease -> ip_addr));
		lease -> flags &= ~ABANDONED_LEASE;
	} else if (lease && (lease -> flags & ABANDONED_LEASE)) {
	/* Otherwise, if it's not the one the client requested, we do not
	   return it - instead, we claim it's ours, causing a DHCPNAK to be
	   sent if this lookup is for a DHCPREQUEST, and force the client
	   to go back through the allocation process. */
		if (ours)
			*ours = 1;
		lease = (struct lease *)0;
	}

      out:
	if (have_client_identifier)
		data_string_forget (&client_identifier, MDL);

#if defined (DEBUG_FIND_LEASE)
	if (lease)
		log_info ("Returning lease: %s.",
		      piaddr (lease -> ip_addr));
	else
		log_info ("Not returning a lease.");
#endif

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
	const unsigned char **s;
	
	mock.subnet = find_host_for_network (&hp, &mock.ip_addr, share);
	if (!mock.subnet)
		return (struct lease *)0;
	mock.next = mock.prev = (struct lease *)0;
	mock.host = hp;
	if (hp -> client_identifier.len > sizeof mock.uid_buf)
		mock.uid = dmalloc (hp -> client_identifier.len, MDL);
	else
		mock.uid = mock.uid_buf;
	if (!mock.uid)
		return (struct lease *)0;
	memcpy (mock.uid, hp -> client_identifier.data,
		hp -> client_identifier.len);
	mock.uid_len = hp -> client_identifier.len;
	mock.hardware_addr = hp -> interface;
	mock.starts = mock.timestamp = mock.ends = MIN_TIME;
	mock.flags = STATIC_LEASE;
	return &mock;
}

/* Dereference all dynamically-allocated information that may be dangling
   off of a static lease.   Otherwise, once ack_lease returns, the information
   dangling from the lease will be lost, so reference counts will be screwed
   up and memory leaks will occur. */

void static_lease_dereference (lease, file, line)
	struct lease *lease;
	const char *file;
	int line;
{
	if (!(lease -> flags & STATIC_LEASE))
		return;
	if (lease -> on_release)
		executable_statement_dereference (&lease -> on_release,
						  file, line);
	if (lease -> on_expiry)
		executable_statement_dereference (&lease -> on_expiry,
						  file, line);
	if (lease -> on_commit)
		executable_statement_dereference (&lease -> on_commit,
						  file, line);
	if (&lease -> scope)
		free_bindings (&lease -> scope, file, line);
	if (lease -> uid != lease -> uid_buf) {
		dfree (lease -> uid, file, line);
		lease -> uid = (unsigned char *)0;
	}
}

/* Look through all the pools in a list starting with the specified pool
   for a free lease.   We try to find a virgin lease if we can.   If we
   don't find a virgin lease, we try to find a non-virgin lease that's
   free.   If we can't find one of those, we try to reclaim an abandoned
   lease.   If all of these possibilities fail to pan out, we don't return
   a lease at all. */

struct lease *allocate_lease (packet, pool, ok)
	struct packet *packet;
	struct pool *pool;
	int ok;
{
	struct lease *lease, *lp;
	struct permit *permit;

	if (!pool)
		return (struct lease *)0;

	/* If we aren't elegible to try this pool, try a subsequent one. */
	if ((pool -> prohibit_list &&
	     permitted (packet, pool -> prohibit_list)) ||
	    (pool -> permit_list && !permitted (packet, pool -> permit_list)))
		return allocate_lease (packet, pool -> next, ok);

	lease = pool -> last_lease;

#if defined (FAILOVER_PROTOCOL)
	/* XXX Do we need code here to ignore PEER_IS_OWNER and just check
	   XXX tstp if we're in, e.g., PARTNER_DOWN?   Where do we deal with
	   XXX CONFLICT_DETECTED, et al? */
	/* Skip to the most expired lease in the pool that is not owned by a
	   failover peer. */
	if (lease -> pool && lease -> pool -> failover_peer) {
		while ((lease -> flags & PEER_IS_OWNER) &&
		       (lease -> ends <=
			cur_time + lease -> pool -> failover_peer -> mclt))
			lease = lease -> prev;
	}
#endif

	/* If there are no leases in the pool that have
	   expired, try the next one. */
	if (!lease || lease -> ends > cur_time)
		return allocate_lease (packet, pool -> next, ok);

	/* If we find an abandoned lease, and no other lease qualifies
	   better, take it. */
	/* XXX what if there are non-abandoned leases that are younger
	   XXX than this?   Shouldn't we hunt for those here? */
	if ((lease -> flags & ABANDONED_LEASE)) {
		/* If we already have a non-abandoned lease that we didn't
		   love, but that's okay, don't reclaim the abandoned lease. */
		if (ok)
			return allocate_lease (packet, pool -> next, ok);
		lp = allocate_lease (packet, pool -> next, 0);
		if (!lp) {
			log_error ("Reclaiming abandoned IP address %s.",
			      piaddr (lease -> ip_addr));
			lease -> flags &= ~ABANDONED_LEASE;
			return lease;
		}
		return lp;
	}

	/* If there's a lease we could take, but it had previously been
	   allocated to a different client, try for a virgin lease before
	   stealing it. */
	if (lease -> uid_len || lease -> hardware_addr.hlen) {
		/* If we're already in that boat, no need to consider
		   allocating this particular lease. */
		if (ok)
			return allocate_lease (packet, pool -> next, ok);

		lp = allocate_lease (packet, pool -> next, 1);
		if (lp)
			return lp;
		return lease;
	}

	return lease;
}

/* Determine whether or not a permit exists on a particular permit list
   that matches the specified packet, returning nonzero if so, zero if
   not. */

int permitted (packet, permit_list)
	struct packet *packet;
	struct permit *permit_list;
{
	struct permit *p;
	int i;

	for (p = permit_list; p; p = p -> next) {
		switch (p -> type) {
		      case permit_unknown_clients:
			if (!packet -> known)
				return 1;
			break;

		      case permit_known_clients:
			if (packet -> known)
				return 1;
			break;

		      case permit_authenticated_clients:
			if (packet -> authenticated)
				return 1;
			break;

		      case permit_unauthenticated_clients:
			if (!packet -> authenticated)
				return 1;
			break;

		      case permit_all_clients:
			return 1;

		      case permit_dynamic_bootp_clients:
			if (!packet -> options_valid ||
			    !packet -> packet_type)
				return 1;
			break;
			
		      case permit_class:
			for (i = 0; i < packet -> class_count; i++) {
				if (p -> class == packet -> classes [i])
					return 1;
				if (packet -> classes [i] &&
				    packet -> classes [i] -> superclass &&
				    (packet -> classes [i] -> superclass ==
				     p -> class))
					return 1;
			}
			break;
		}
	}
	return 0;
}

int locate_network (packet)
	struct packet *packet;
{
	struct iaddr ia;
	struct data_string data;
	struct subnet *subnet;
	struct option_cache *oc;

	/* See if there's a subnet selection option. */
	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_SUBNET_SELECTION);

	/* If there's no SSO and no giaddr, then use the shared_network
	   from the interface, if there is one.   If not, fail. */
	if (!oc && !packet -> raw -> giaddr.s_addr) {
		if ((packet -> shared_network =
		     packet -> interface -> shared_network))
			return 1;
		return 0;
	}

	/* If there's an SSO, and it's valid, use it to figure out the
	   subnet.    If it's not valid, fail. */
	if (oc) {
		memset (&data, 0, sizeof data);
		if (!evaluate_option_cache (&data, packet, (struct lease *)0,
					    packet -> options,
					    (struct option_state *)0,
					    &global_scope, oc, MDL)) {
			packet -> shared_network = (struct shared_network *)0;
			return 0;
		}
		if (data.len != 4) {
			packet -> shared_network = (struct shared_network *)0;
			return 0;
		}
		ia.len = 4;
		memcpy (ia.iabuf, data.data, 4);
		data_string_forget (&data, MDL);
	} else {
		ia.len = 4;
		memcpy (ia.iabuf, &packet -> raw -> giaddr, 4);
	}

	/* If we know the subnet on which the IP address lives, use it. */
	subnet = find_subnet (ia);
	if (subnet) {
		packet -> shared_network = subnet -> shared_network;
		return 1;
	}

	/* Otherwise, fail. */
	packet -> shared_network = (struct shared_network *)0;
	return 0;
}

/* Parse a Relay Agent Information option and put it at the end of the
   list of such options on the specified packet. */

int parse_agent_information_option (packet, len, data)
	struct packet *packet;
	int len;
	u_int8_t *data;
{
	struct agent_options *a, **tail;
	struct option_tag *t, *oth = 0, **ott = &oth;
	u_int8_t *op = data, *max = data + len;

	/* Parse the agent information option suboptions. */
	while (op < max) {
		/* Check for overflow. */
		if (op + 1 == max || op + op [1] + 2 > max)
			return 0;
		/* Make space for this suboption. */
 		t = (struct option_tag *)
			dmalloc (op [1] + 1 + sizeof *t, MDL);
		if (!t)
			log_fatal ("no memory for option tag data.");

		/* Link it in at the tail of the list. */
		t -> next = (struct option_tag *)0;
		*ott = t;
		ott = &t -> next;
		
		/* Copy the option data in in its raw form. */
		memcpy (t -> data, op, (unsigned)(op [1] + 2));
		op += op [1] + 2;
	}

	/* Make an agent options structure to put on the list. */
	a = (struct agent_options *)dmalloc (sizeof *a, MDL);
	if (!a)
		log_fatal ("can't allocate space for agent option structure.");

	/* Find the tail of the list. */
	for (tail = ((struct agent_options **)
		     &packet -> options -> universes [agent_universe.index]);
	     *tail; tail = &((*tail) -> next))
		;
	*tail = a;
	a -> next = (struct agent_options *)0;
	a -> first = oth;
	a -> length = len;

	return 1;
}

unsigned cons_agent_information_options (cfg_options, outpacket,
					 agentix, length)
	struct option_state *cfg_options;
	struct dhcp_packet *outpacket;
	unsigned agentix;
	unsigned length;
{
	/* We tack any agent options onto the end of the packet after
	   we've put it together. */
	if (cfg_options -> universe_count > agent_universe.index &&
	    cfg_options -> universes [agent_universe.index]) {
	    int len = 0;
	    struct agent_options *a;
	    struct option_tag *o;

	    /* Cycle through the options, appending them to the
	       buffer. */
	    for (a = ((struct agent_options *)
		      cfg_options -> universes [agent_universe.index]);
		 a; a = a -> next) {
		    if (agentix + a -> length + 3 + DHCP_FIXED_LEN <=
			dhcp_max_agent_option_packet_length) {
			    outpacket -> options [agentix++]
				    = DHO_DHCP_AGENT_OPTIONS;
			    outpacket -> options [agentix++] = a -> length;
			    for (o = a -> first; o; o = o -> next) {
				    memcpy (&outpacket -> options [agentix],
					    o -> data,
					    (unsigned)(o -> data [1] + 2));
				    agentix += o -> data [1] + 2;
			    }
		    }
	    }

	    /* Reterminate the packet. */
	    outpacket -> options [agentix++] = DHO_END;

	    /* Recompute the length, which may now be higher than the
	       client can accept but should be okay for the relay agent. */
	    return agentix + DHCP_FIXED_NON_UDP;
	}

	return length;
}
