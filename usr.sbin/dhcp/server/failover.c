/* failover.c

   Failover protocol support code... */

/*
 * Copyright (c) 1999-2000 Internet Software Consortium.
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
"$Id: failover.c,v 1.1.1.1.2.1 2000/06/22 18:00:55 minoura Exp $ Copyright (c) 1999-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include "version.h"
#include <omapip/omapip_p.h>

#if defined (FAILOVER_PROTOCOL)
static struct hash_table *failover_hash;
static dhcp_failover_state_t *failover_states;
static isc_result_t do_a_failover_option (omapi_object_t *,
					  dhcp_failover_link_t *);

void dhcp_failover_startup ()
{
	dhcp_failover_state_t *state;
	isc_result_t status;

	for (state = failover_states; state; state = state -> next) {
		dhcp_failover_state_transition (state, "startup");
		if (state -> i_am == secondary) {
			status = (dhcp_failover_listen
				  ((omapi_object_t *)state));
			if (status != ISC_R_SUCCESS) {
				add_timeout (cur_time + 90,
					     dhcp_failover_listener_restart,
					     state,
					     (tvref_t)omapi_object_reference,
					     (tvunref_t)
					     omapi_object_dereference);
			}
		} else {
			status = (dhcp_failover_link_initiate
				  ((omapi_object_t *)state));
			if (status != ISC_R_SUCCESS) {
				add_timeout (cur_time + 90,
					     dhcp_failover_reconnect, state,
					     (tvref_t)
					     dhcp_failover_state_reference,
					     (tvunref_t)
					     dhcp_failover_state_dereference);
			}
		}
		if (status != ISC_R_SUCCESS) {
			log_error ("failover peer %s: %s", state -> name,
				   isc_result_totext (status));
		}
	}
}

void dhcp_failover_write_all_states ()
{
	dhcp_failover_state_t *state;

	for (state = failover_states; state; state = state -> next) {
		write_failover_state (state);
	}
}

isc_result_t enter_failover_peer (peer)
	dhcp_failover_state_t *peer;
{
	dhcp_failover_state_t *dup = (dhcp_failover_state_t *)0;
	isc_result_t status;

	status = find_failover_peer (&dup, peer -> name, MDL);
	if (status == ISC_R_NOTFOUND) {
		if (failover_states) {
			dhcp_failover_state_reference (&peer -> next,
						      failover_states, MDL);
			dhcp_failover_state_dereference (&failover_states,
							 MDL);
		}
		dhcp_failover_state_reference (&failover_states, peer, MDL);
		return ISC_R_SUCCESS;
	}
	dhcp_failover_state_dereference (&dup, MDL);
	if (status == ISC_R_SUCCESS)
		return ISC_R_EXISTS;
	return status;
}

isc_result_t find_failover_peer (peer, name, file, line)
	dhcp_failover_state_t **peer;
	const char *name;
	const char *file;
	int line;
{
	dhcp_failover_state_t *p;

	for (p = failover_states; p; p = p -> next)
		if (!strcmp (name, p -> name))
			break;
	if (p)
		return dhcp_failover_state_reference (peer, p, file, line);
	return ISC_R_NOTFOUND;
}

/* The failover protocol has three objects associated with it.  For
   each failover partner declaration in the dhcpd.conf file, primary
   or secondary, there is a failover_state object.  For any primary or
   secondary state object that has a connection to its peer, there is
   also a failover_link object, which has its own input state seperate
   from the failover protocol state for managing the actual bytes
   coming in off the wire.  Finally, there will be one listener object
   for every distinct port number associated with a secondary
   failover_state object.  Normally all secondary failover_state
   objects are expected to listen on the same port number, so there
   need be only one listener object, but if different port numbers are
   specified for each failover object, there could be as many as one
   listener object for each secondary failover_state object. */

/* This, then, is the implemention of the failover link object. */

isc_result_t dhcp_failover_link_initiate (omapi_object_t *h)
{
	isc_result_t status;
	dhcp_failover_link_t *obj;
	omapi_value_t *value = (omapi_value_t *)0;
	dhcp_failover_state_t *state;
	omapi_object_t *o;
	int i;
	struct data_string ds;
	omapi_addr_list_t *addrs = (omapi_addr_list_t *)0;
	omapi_addr_t local_addr;

	/* Find the failover state in the object chain. */
	for (o = h; o -> outer; o = o -> outer)
		;
	for (; o; o = o -> inner) {
		if (o -> type == dhcp_type_failover_state)
			break;
	}
	if (!o)
		return ISC_R_INVALIDARG;
	state = (dhcp_failover_state_t *)o;

	obj = (dhcp_failover_link_t *)0;
	status = dhcp_failover_link_allocate (&obj, MDL);
	if (status != ISC_R_SUCCESS)
		return status;
	option_cache_reference (&obj -> peer_address, state -> address, MDL);
	obj -> peer_port = state -> port;
	dhcp_failover_state_reference (&obj -> state_object, state, MDL);

	memset (&ds, 0, sizeof ds);
	if (!evaluate_option_cache (&ds, (struct packet *)0, (struct lease *)0,
				    (struct option_state *)0,
				    (struct option_state *)0,
				    &global_scope, obj -> peer_address, MDL)) {
		dhcp_failover_link_dereference (&obj, MDL);
		return ISC_R_UNEXPECTED;
	}

	/* Make an omapi address list out of a buffer containing zero or more
	   IPv4 addresses. */
	status = omapi_addr_list_new (&addrs, ds.len / 4, MDL);
	if (status != ISC_R_SUCCESS) {
		dhcp_failover_link_dereference (&obj, MDL);
		return status;
	}

	for (i = 0; i  < addrs -> count; i++) {
		addrs -> addresses [i].addrtype = AF_INET;
		addrs -> addresses [i].addrlen = sizeof (struct in_addr);
		memcpy (addrs -> addresses [i].address,
			&ds.data [i * 4], sizeof (struct in_addr));
		addrs -> addresses [i].port = obj -> peer_port;
	}
	data_string_forget (&ds, MDL);

	/* Now figure out the local address that we're supposed to use. */
	if (!evaluate_option_cache (&ds, (struct packet *)0, (struct lease *)0,
				    (struct option_state *)0,
				    (struct option_state *)0,
				    &global_scope, state -> server_addr,
				    MDL)) {
		memset (&local_addr, 0, sizeof local_addr);
		local_addr.addrtype = AF_INET;
		local_addr.addrlen = sizeof (struct in_addr);
		if (!state -> server_identifier.len) {
			log_fatal ("failover peer %s: no identifier.",
				   state -> name);
		}
	} else {
		if (ds.len != sizeof (struct in_addr)) {
			data_string_forget (&ds, MDL);
			dhcp_failover_link_dereference (&obj, MDL);
			omapi_addr_list_dereference (&addrs, MDL);
			return ISC_R_INVALIDARG;
		}
		local_addr.addrtype = AF_INET;
		local_addr.addrlen = ds.len;
		memcpy (local_addr.address, ds.data, ds.len);
		if (!state -> server_identifier.len)
			data_string_copy (&state -> server_identifier,
					  &ds, MDL);
		data_string_forget (&ds, MDL);
		local_addr.port = 0;  /* Let the O.S. choose. */
	}

	status = omapi_connect_list ((omapi_object_t *)obj,
				     addrs, &local_addr);
	omapi_addr_list_dereference (&addrs, MDL);

	dhcp_failover_link_dereference (&obj, MDL);
	return status;
}

isc_result_t dhcp_failover_link_signal (omapi_object_t *h,
					const char *name, va_list ap)
{
	isc_result_t status;
	dhcp_failover_link_t *link;
	omapi_object_t *c;
	u_int16_t nlen;
	u_int32_t vlen;
	dhcp_failover_state_t *s, *state = (dhcp_failover_state_t *)0;

	if (h -> type != dhcp_type_failover_link) {
		/* XXX shouldn't happen.   Put an assert here? */
		return ISC_R_UNEXPECTED;
	}
	link = (dhcp_failover_link_t *)h;

	if (!strcmp (name, "connect")) {
		/* If we're primary, send the connect message. */
		status = dhcp_failover_send_connect (h);
		if (status != ISC_R_SUCCESS)
			omapi_disconnect (h -> outer, 1);
		return status;
	}

	if (!strcmp (name, "disconnect")) {
	    if (link -> state_object) {
		if (link -> state_object -> i_am == primary) {
		    add_timeout (cur_time + 5, dhcp_failover_reconnect,
				 link -> state_object,
				 (tvref_t)dhcp_failover_state_reference,
				 (tvunref_t)dhcp_failover_state_dereference);
		}
		link -> state = dhcp_flink_disconnected;
		/* Make the transition. */
		dhcp_failover_state_transition (link -> state_object, name);
	    }
	    return ISC_R_SUCCESS;
	}

	/* Not a signal we recognize? */
	if (strcmp (name, "ready")) {
		if (h -> inner && h -> inner -> type -> signal_handler)
			return (*(h -> inner -> type -> signal_handler))
				(h -> inner, name, ap);
		return ISC_R_NOTFOUND;
	}

	if (!h -> outer || h -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;
	c = h -> outer;

	/* We get here because we requested that we be woken up after
           some number of bytes were read, and that number of bytes
           has in fact been read. */
	switch (link -> state) {
	      case dhcp_flink_start:
		link -> state = dhcp_flink_message_length_wait;
		if ((omapi_connection_require (c, 2)) != ISC_R_SUCCESS)
			break;
	      case dhcp_flink_message_length_wait:
	      next_message:
		link -> state = dhcp_flink_message_wait;
		link -> imsg = dmalloc (sizeof (failover_message_t), MDL);
		if (!link -> imsg) {
		      dhcp_flink_fail:
			if (link -> imsg) {
				dfree (link -> imsg, MDL);
				link -> imsg = (failover_message_t *)0;
			}
			link -> state = dhcp_flink_disconnected;
			omapi_disconnect (c, 1);
			/* XXX just blow away the protocol state now?
			   XXX or will disconnect blow it away? */
			return ISC_R_UNEXPECTED;
		}
		memset (link -> imsg, 0, sizeof (failover_message_t));
		/* Get the length: */
		omapi_connection_get_uint16 (c, &link -> imsg_len);
		link -> imsg_count = 0;	/* Bytes read. */
		
		/* Maximum of 2048 bytes in any failover message. */
		if (link -> imsg_len > DHCP_FAILOVER_MAX_MESSAGE_SIZE)
			goto dhcp_flink_fail;

		if ((omapi_connection_require (c, link -> imsg_len - 2U)) !=
		    ISC_R_SUCCESS)
			break;
	      case dhcp_flink_message_wait:
		/* Read in the message.  At this point we have the
		   entire message in the input buffer.  For each
		   incoming value ID, set a bit in the bitmask
		   indicating that we've gotten it.  Maybe flag an
		   error message if the bit is already set.  Once
		   we're done reading, we can check the bitmask to
		   make sure that the required fields for each message
		   have been included. */

		link -> imsg_count += 2;	/* Count the length as read. */

		/* Get message type. */
		omapi_connection_copyout (&link -> imsg -> type, c, 1);
		link -> imsg_count++;

		/* Get message payload offset. */
		omapi_connection_copyout (&link -> imsg_payoff, c, 1);
		link -> imsg_count++;

		/* Get message time. */
		omapi_connection_get_uint32 (c, &link -> imsg -> time);
		link -> imsg_count += 4;

		/* Get transaction ID. */
		omapi_connection_get_uint32 (c, &link -> imsg -> xid);
		link -> imsg_count += 4;

		/* Skip over any portions of the message header that we
		   don't understand. */
		if (link -> imsg_payoff - link -> imsg_count) {
			omapi_connection_copyout ((unsigned char *)0, c,
						  (link -> imsg_payoff -
						   link -> imsg_count));
			link -> imsg_count = link -> imsg_payoff;
		}
				
		/* Now start sucking options off the wire. */
		while (link -> imsg_count < link -> imsg_len) {
			if (do_a_failover_option (c, link) != ISC_R_SUCCESS)
				goto dhcp_flink_fail;
		}

		/* If it's a connect message, try to associate it with
		   a state object. */
		/* XXX this should be authenticated! */
		if (link -> imsg -> type == FTM_CONNECT) {
		    const char *errmsg;
		    int reason;
		    /* See if we can find a failover_state object that
		       matches this connection.  This message should only
		       be received by a secondary from a primary. */
		    for (s = failover_states; s; s = s -> next) {
			if (dhcp_failover_state_match
			    (s, (u_int8_t *)&link -> imsg -> server_addr,
			     sizeof link -> imsg -> server_addr))
				state = s;
		    }		

		    /* If we can't find a failover protocol state
		       for this remote host, drop the connection */
		    if (!state) {
			    errmsg = "unknown server";
			    reason = FTR_INVALID_PARTNER;

			  badconnect:
				/* XXX Send a refusal message first?
				   XXX Look in protocol spec for guidance. */
			    log_error ("Failover CONNECT from %d.%d.%d.%d: %s",
				       ((u_int8_t *)
					(&link -> imsg -> server_addr)) [0],
				       ((u_int8_t *)
					(&link -> imsg -> server_addr)) [1],
				       ((u_int8_t *)
					(&link -> imsg -> server_addr)) [2],
				       ((u_int8_t *)
					(&link -> imsg -> server_addr)) [3],
				       errmsg);
			    dhcp_failover_send_connectack
				    ((omapi_object_t *)link, state,
				     reason, errmsg);
			    omapi_disconnect (c, 0);
			    link -> state = dhcp_flink_disconnected;
			    return ISC_R_SUCCESS;
		    }

		    if (!(link -> imsg -> options_present & FTB_HBA) ||
			link -> imsg -> hba.count != 32) {
			    errmsg = "invalid HBA";
			    reason = FTR_HBA_CONFLICT; /* XXX */
			    goto badconnect;
		    }
		    if (state -> hba)
			    dfree (state -> hba, MDL);
		    state -> hba = dmalloc (32, MDL);
		    if (!state -> hba) {
			    errmsg = "no memory";
			    reason = FTR_MISC_REJECT;
			    goto badconnect;
		    }
		    memcpy (state -> hba, link -> imsg -> hba.data, 32);

		    if (!link -> state_object)
			    dhcp_failover_state_reference
				    (&link -> state_object, state, MDL);
		    option_cache_reference
			    (&link -> peer_address, state -> address, MDL);
		}

		/* If we don't have a state object at this point, it's
		   some kind of bogus situation, so just drop the
		   connection. */
		if (!link -> state_object) {
			omapi_disconnect (c, 1);
			link -> state = dhcp_flink_disconnected;
			return ISC_R_INVALIDARG;
		}

		/* Once we have the entire message, and we've validated
		   it as best we can here, pass it to the parent. */
		omapi_signal ((omapi_object_t *)link -> state_object,
			      "message", link);
		link -> state = dhcp_flink_message_length_wait;
		dfree (link -> imsg, MDL);
		link -> imsg = (failover_message_t *)0;
		/* XXX This is dangerous because we could get into a tight
		   XXX loop reading input without servicing any other stuff.
		   XXX There needs to be a way to relinquish control but
		   XXX get it back immediately if there's no other work to
		   XXX do. */
		if ((omapi_connection_require (c, 2)) == ISC_R_SUCCESS)
			goto next_message;
		break;

	      default:
		/* XXX should never get here.   Assertion? */
		break;
	}
	return ISC_R_SUCCESS;
}

static isc_result_t do_a_failover_option (c, link)
	omapi_object_t *c;
	dhcp_failover_link_t *link;
{
	u_int16_t option_code;
	u_int16_t option_len;
	unsigned char *op;
	unsigned op_size;
	unsigned op_count;
	int i;
	isc_result_t status;
	
	if (link -> imsg_count + 2 > link -> imsg_len) {
		log_error ("FAILOVER: message overflow at option code.");
		return ISC_R_PROTOCOLERROR;
	}

	/* Get option code. */
	omapi_connection_get_uint16 (c, &option_code);
	link -> imsg_count += 2;
	
	if (link -> imsg_count + 2 > link -> imsg_len) {
		log_error ("FAILOVER: message overflow at length.");
		return ISC_R_PROTOCOLERROR;
	}

	/* Get option length. */
	omapi_connection_get_uint16 (c, &option_len);
	link -> imsg_count += 2;

	if (link -> imsg_count + option_len > link -> imsg_len) {
		log_error ("FAILOVER: message overflow at data.");
		return ISC_R_PROTOCOLERROR;
	}

	/* If it's an unknown code, skip over it. */
	if (option_code > FTO_MAX) {
#if defined (FAILOVER_PROTOCOL_DEBUG) && defined (FAILOVER_DEBUG_VERBOSE)
		log_debug ("  option code %d len %d (not recognized)",
			   option_code, option_len);
#endif
		omapi_connection_copyout ((unsigned char *)0, c, option_len);
		link -> imsg_count += option_len;
		return ISC_R_SUCCESS;
	}

	/* If it's the digest, do it now. */
	if (ft_options [option_code].type == FT_DIGEST) {
		link -> imsg_count += option_len;
		if (link -> imsg_count != link -> imsg_len) {
			log_error ("FAILOVER: digest not at end of message");
			return ISC_R_PROTOCOLERROR;
		}
#if defined (FAILOVER_PROTOCOL_DEBUG) && defined (FAILOVER_DEBUG_VERBOSE)
		log_debug ("  option %s len %d",
			   ft_options [option_code].name, option_len);
#endif
		/* For now, just dump it. */
		omapi_connection_copyout ((unsigned char *)0, c, option_len);
		return ISC_R_SUCCESS;
	}
	
	/* Only accept an option once. */
	if (link -> imsg -> options_present & ft_options [option_code].bit) {
		log_error ("FAILOVER: duplicate option %s",
			   ft_options [option_code].name);
		return ISC_R_PROTOCOLERROR;
	}

	/* Make sure the option is appropriate for this type of message.
	   Really, any option is generally allowed for any message, and the
	   cases where this is not true are too complicated to represent in
	   this way - what this code is doing is to just avoid saving the
	   value of an option we don't have any way to use, which allows
	   us to make the failover_message structure smaller. */
	if (ft_options [option_code].bit &&
	    !(fto_allowed [link -> imsg -> type] &
	      ft_options [option_code].bit)) {
		omapi_connection_copyout ((unsigned char *)0, c, option_len);
		link -> imsg_count += option_len;
		return ISC_R_SUCCESS;
	}		

	/* Figure out how many elements, how big they are, and where
	   to store them. */
	if (ft_options [option_code].num_present) {
		/* If this option takes a fixed number of elements,
		   we expect the space for them to be preallocated,
		   and we can just read the data in. */

		op = ((unsigned char *)link -> imsg) +
			ft_options [option_code].offset;
		op_size = ft_sizes [ft_options [option_code].type];
		op_count = ft_options [option_code].num_present;

		if (option_len != op_size * op_count) {
			log_error ("FAILOVER: option size (%d:%d), option %s",
				   option_len,
				   (ft_sizes [ft_options [option_code].type] *
				    ft_options [option_code].num_present),
				   ft_options [option_code].name);
			return ISC_R_PROTOCOLERROR;
		}
	} else {
		failover_option_t *fo;

		/* FT_DDNS* are special - one or two bytes of status
		   followed by the client FQDN. */
		if (ft_options [option_code].type == FT_DDNS1 ||
		    ft_options [option_code].type == FT_DDNS1) {
			ddns_fqdn_t *ddns =
				((ddns_fqdn_t *)
				 (((char *)link -> imsg) +
				  ft_options [option_code].offset));

			op_count = (ft_options [option_code].type == FT_DDNS1
				    ? 1 : 2);

			omapi_connection_copyout (&ddns -> codes [0],
						  c, op_count);
			link -> imsg_count += op_count;
			if (op_count == 1)
				ddns -> codes [1] = 0;
			op_size = 1;
			op_count = option_len - op_count;

			ddns -> length = op_count;
			ddns -> data = dmalloc (op_count, MDL);
			if (!ddns -> data) {
				log_error ("FAILOVER: no memory getting%s(%d)",
					   " DNS data ", op_count);

				/* Actually, NO_MEMORY, but if we lose here
				   we have to drop the connection. */
				return ISC_R_PROTOCOLERROR;
			}
			omapi_connection_copyout (ddns -> data, c, op_count);
			goto out;
		}

		/* A zero for num_present means that any number of
		   elements can appear, so we have to figure out how
		   many we got from the length of the option, and then
		   fill out a failover_option structure describing the
		   data. */
		op_size = ft_sizes [ft_options [option_code].type];

		/* Make sure that option data length is a multiple of the
		   size of the data type being sent. */
		if (op_size > 1 && option_len % op_size) {
			log_error ("FAILOVER: option_len %d not %s%d",
				   option_len, "multiple of ", op_size);
			return ISC_R_PROTOCOLERROR;
		}

		op_count = option_len / op_size;
		
		fo = ((failover_option_t *)
		      (((char *)link -> imsg) +
		       ft_options [option_code].offset));

		fo -> count = op_count;
		fo -> data = dmalloc (option_len, MDL);
		if (!fo -> data) {
			log_error ("FAILOVER: no memory getting %s (%d)",
				   "option data", op_count);

			return ISC_R_PROTOCOLERROR;
		}			
		op = fo -> data;
	}

	/* For single-byte message values and multi-byte values that
           don't need swapping, just read them in all at once. */
	if (op_size == 1 || ft_options [option_code].type == FT_IPADDR) {
		omapi_connection_copyout ((unsigned char *)op, c, option_len);
		link -> imsg_count += option_len;
		goto out;
	}

	/* For values that require swapping, read them in one at a time
	   using routines that swap bytes. */
	for (i = 0; i < op_count; i++) {
		switch (ft_options [option_code].type) {
		      case FT_UINT32:
			omapi_connection_get_uint32 (c, (u_int32_t *)op);
			op += 4;
			link -> imsg_count += 4;
			break;
			
		      case FT_UINT16:
			omapi_connection_get_uint16 (c, (u_int16_t *)op);
			op += 2;
			link -> imsg_count += 2;
			break;
			
		      default:
			/* Everything else should have been handled
			   already. */
			log_error ("FAILOVER: option %s: bad type %d",
				   ft_options [option_code].name,
				   ft_options [option_code].type);
			return ISC_R_PROTOCOLERROR;
		}
	}
      out:
	/* Remember that we got this option. */
	link -> imsg -> options_present |= ft_options [option_code].bit;
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_failover_link_set_value (omapi_object_t *h,
					   omapi_object_t *id,
					   omapi_data_string_t *name,
					   omapi_typed_data_t *value)
{
	if (h -> type != omapi_type_protocol)
		return ISC_R_INVALIDARG;

	/* Never valid to set these. */
	if (!omapi_ds_strcmp (name, "link-port") ||
	    !omapi_ds_strcmp (name, "link-name") ||
	    !omapi_ds_strcmp (name, "link-state"))
		return ISC_R_NOPERM;

	if (h -> inner && h -> inner -> type -> set_value)
		return (*(h -> inner -> type -> set_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_failover_link_get_value (omapi_object_t *h,
					   omapi_object_t *id,
					   omapi_data_string_t *name,
					   omapi_value_t **value)
{
	dhcp_failover_link_t *link;

	if (h -> type != omapi_type_protocol)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)h;
	
	if (!omapi_ds_strcmp (name, "link-port")) {
		return omapi_make_int_value (value, name,
					     (int)link -> peer_port, MDL);
	} else if (!omapi_ds_strcmp (name, "link-state")) {
		if (link -> state < 0 ||
		    link -> state >= dhcp_flink_state_max)
			return omapi_make_string_value (value, name,
							"invalid link state",
							MDL);
		return omapi_make_string_value
			(value, name,
			 dhcp_flink_state_names [link -> state], MDL);
	}

	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_failover_link_destroy (omapi_object_t *h,
					 const char *file, int line)
{
	dhcp_failover_link_t *link;
	if (h -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)h;
	if (link -> imsg) {
		dfree (link -> imsg, file, line);
		link -> imsg = (failover_message_t *)0;
	}
	if (link -> state_object)
		dhcp_failover_state_dereference (&link -> state_object, MDL);
	return ISC_R_SUCCESS;
}

/* Write all the published values associated with the object through the
   specified connection. */

isc_result_t dhcp_failover_link_stuff_values (omapi_object_t *c,
					      omapi_object_t *id,
					      omapi_object_t *l)
{
	dhcp_failover_link_t *link;
	isc_result_t status;

	if (l -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)l;
	
	status = omapi_connection_put_name (c, "link-port");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (int));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, link -> peer_port);
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "link-state");
	if (status != ISC_R_SUCCESS)
		return status;
	if (link -> state < 0 ||
	    link -> state >= dhcp_flink_state_max)
		status = omapi_connection_put_string (c, "invalid link state");
	else
		status = (omapi_connection_put_string
			  (c, dhcp_flink_state_names [link -> state]));
	if (status != ISC_R_SUCCESS)
		return status;

	if (link -> inner && link -> inner -> type -> stuff_values)
		return (*(link -> inner -> type -> stuff_values)) (c, id,
								link -> inner);
	return ISC_R_SUCCESS;
}

/* Set up a listener for the omapi protocol.    The handle stored points to
   a listener object, not a protocol object. */

isc_result_t dhcp_failover_listen (omapi_object_t *h)
{
	isc_result_t status;
	dhcp_failover_listener_t *obj;
	omapi_value_t *value = (omapi_value_t *)0;
	omapi_addr_t local_addr;
	unsigned long port;

	status = omapi_get_value_str (h, (omapi_object_t *)0,
				      "local-port", &value);
	if (status != ISC_R_SUCCESS)
		return status;
	if (!value -> value) {
		omapi_value_dereference (&value, MDL);
		return ISC_R_INVALIDARG;
	}
	
	status = omapi_get_int_value (&port, value -> value);
	omapi_value_dereference (&value, MDL);
	if (status != ISC_R_SUCCESS)
		return status;
	local_addr.port = port;

	status = omapi_get_value_str (h, (omapi_object_t *)0,
				      "local-address", &value);
	if (status != ISC_R_SUCCESS)
		return status;
	if (!value -> value) {
	      nogood:
		omapi_value_dereference (&value, MDL);
		return ISC_R_INVALIDARG;
	}
	
	if (value -> value -> type != omapi_datatype_data ||
	    value -> value -> u.buffer.len != sizeof (struct in_addr))
		goto nogood;

	memcpy (local_addr.address, value -> value -> u.buffer.value,
		value -> value -> u.buffer.len);
	local_addr.addrlen = value -> value -> u.buffer.len;
	local_addr.addrtype = AF_INET;

	omapi_value_dereference (&value, MDL);

	obj = (dhcp_failover_listener_t *)0;
	status = dhcp_failover_listener_allocate (&obj, MDL);
	if (status != ISC_R_SUCCESS)
		return status;
	obj -> local_port = local_addr.port;
	
	status = omapi_listen_addr ((omapi_object_t *)obj, &local_addr, 1);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_object_reference (&h -> outer,
					 (omapi_object_t *)obj, MDL);
	if (status != ISC_R_SUCCESS) {
		dhcp_failover_listener_dereference (&obj, MDL);
		return status;
	}
	status = omapi_object_reference (&obj -> inner, h, MDL);
	if (status != ISC_R_SUCCESS) {
		dhcp_failover_listener_dereference (&obj, MDL);
		return status;
	}

	return dhcp_failover_listener_dereference (&obj, MDL);
}

/* Signal handler for protocol listener - if we get a connect signal,
   create a new protocol connection, otherwise pass the signal down. */

isc_result_t dhcp_failover_listener_signal (omapi_object_t *o,
					    const char *name, va_list ap)
{
	isc_result_t status;
	omapi_connection_object_t *c;
	dhcp_failover_link_t *obj;
	dhcp_failover_listener_t *p;
	dhcp_failover_state_t *s, *state = (dhcp_failover_state_t *)0;

	if (!o || o -> type != dhcp_type_failover_listener)
		return ISC_R_INVALIDARG;
	p = (dhcp_failover_listener_t *)o;

	/* Not a signal we recognize? */
	if (strcmp (name, "connect")) {
		if (p -> inner && p -> inner -> type -> signal_handler)
			return (*(p -> inner -> type -> signal_handler))
				(p -> inner, name, ap);
		return ISC_R_NOTFOUND;
	}

	c = va_arg (ap, omapi_connection_object_t *);
	if (!c || c -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	obj = (dhcp_failover_link_t *)0;
	status = dhcp_failover_link_allocate (&obj, MDL);
	if (status != ISC_R_SUCCESS)
		return status;
	obj -> peer_port = ntohs (c -> remote_addr.sin_port);

	status = omapi_object_reference (&obj -> outer,
					 (omapi_object_t *)c, MDL);
	if (status != ISC_R_SUCCESS) {
	      lose:
		dhcp_failover_link_dereference (&obj, MDL);
		omapi_disconnect ((omapi_object_t *)c, 1);
		return status;
	}

	status = omapi_object_reference (&c -> inner,
					 (omapi_object_t *)obj, MDL);
	if (status != ISC_R_SUCCESS)
		goto lose;

	return dhcp_failover_link_dereference (&obj, MDL);
}

isc_result_t dhcp_failover_listener_set_value (omapi_object_t *h,
						omapi_object_t *id,
						omapi_data_string_t *name,
						omapi_typed_data_t *value)
{
	if (h -> type != dhcp_type_failover_listener)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> set_value)
		return (*(h -> inner -> type -> set_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_failover_listener_get_value (omapi_object_t *h,
						omapi_object_t *id,
						omapi_data_string_t *name,
						omapi_value_t **value)
{
	if (h -> type != dhcp_type_failover_listener)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_failover_listener_destroy (omapi_object_t *h,
					      const char *file, int line)
{
	if (h -> type != dhcp_type_failover_listener)
		return ISC_R_INVALIDARG;
	return ISC_R_SUCCESS;
}

/* Write all the published values associated with the object through the
   specified connection. */

isc_result_t dhcp_failover_listener_stuff (omapi_object_t *c,
					   omapi_object_t *id,
					   omapi_object_t *p)
{
	int i;

	if (p -> type != dhcp_type_failover_listener)
		return ISC_R_INVALIDARG;

	if (p -> inner && p -> inner -> type -> stuff_values)
		return (*(p -> inner -> type -> stuff_values)) (c, id,
								p -> inner);
	return ISC_R_SUCCESS;
}

/* Set up master state machine for the failover protocol. */

isc_result_t dhcp_failover_register (omapi_object_t *h)
{
	isc_result_t status;
	dhcp_failover_state_t *obj;
	unsigned long port;
	omapi_value_t *value = (omapi_value_t *)0;

	status = omapi_get_value_str (h, (omapi_object_t *)0,
				      "local-port", &value);
	if (status != ISC_R_SUCCESS)
		return status;
	if (!value -> value) {
		omapi_value_dereference (&value, MDL);
		return ISC_R_INVALIDARG;
	}
	
	status = omapi_get_int_value (&port, value -> value);
	omapi_value_dereference (&value, MDL);
	if (status != ISC_R_SUCCESS)
		return status;

	obj = (dhcp_failover_state_t *)0;
	dhcp_failover_state_allocate (&obj, MDL);
	obj -> listen_port = port;
	
	status = omapi_listen ((omapi_object_t *)obj, port, 1);
	if (status != ISC_R_SUCCESS) {
		dhcp_failover_state_dereference (&obj, MDL);
		return status;
	}

	status = omapi_object_reference (&h -> outer, (omapi_object_t *)obj,
					 MDL);
	if (status != ISC_R_SUCCESS) {
		dhcp_failover_state_dereference (&obj, MDL);
		return status;
	}
	status = omapi_object_reference (&obj -> inner, h, MDL);
	dhcp_failover_state_dereference (&obj, MDL);
	return status;
}

/* Signal handler for protocol state machine. */

isc_result_t dhcp_failover_state_signal (omapi_object_t *o,
					 const char *name, va_list ap)
{
	isc_result_t status;
	omapi_connection_object_t *c;
	omapi_protocol_object_t *obj;
	dhcp_failover_state_t *state;
	dhcp_failover_link_t *link;
	char *peer_name;

	if (!o || o -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	state = (dhcp_failover_state_t *)o;

	/* Not a signal we recognize? */
	if (strcmp (name, "disconnect") &&
	    strcmp (name, "message")) {
		if (state -> inner && state -> inner -> type -> signal_handler)
			return (*(state -> inner -> type -> signal_handler))
				(state -> inner, name, ap);
		return ISC_R_NOTFOUND;
	}

	/* Handle connect signals by seeing what state we're in
	   and potentially doing a state transition. */
	if (!strcmp (name, "disconnect")) {
		link = va_arg (ap, dhcp_failover_link_t *);

		dhcp_failover_link_dereference (&state -> link_to_peer, MDL);
		dhcp_failover_state_transition (state, "disconnect");
		if (state -> i_am == primary)
			add_timeout (cur_time + 90, dhcp_failover_reconnect,
				     state,
				     (tvref_t)dhcp_failover_state_reference,
				     (tvunref_t)
				     dhcp_failover_state_dereference);
	} else if (!strcmp (name, "message")) {
		link = va_arg (ap, dhcp_failover_link_t *);

		if (link -> imsg -> type == FTM_CONNECT) {
			/* If we already have a link to the peer, it must be
			   dead, so drop it.
			   XXX Is this the right thing to do? */
			if (state -> link_to_peer) {
				omapi_disconnect ((omapi_object_t *)
						  state -> link_to_peer, 1);
				dhcp_failover_link_dereference
					(&state -> link_to_peer, MDL);
				dhcp_failover_state_transition (state,
								"disconnect");
			}
			dhcp_failover_link_reference (&state -> link_to_peer,
						      link, MDL);
			status = (dhcp_failover_send_connectack
				  ((omapi_object_t *)link, state, 0, 0));
			if (status != ISC_R_SUCCESS) {
				dhcp_failover_link_dereference
					(&state -> link_to_peer, MDL);
				omapi_disconnect (link -> outer, 1);
				return ISC_R_SUCCESS;
			}
			dhcp_failover_state_transition (state, "connect");
		} else if (link -> imsg -> type == FTM_CONNECTACK) {
		    const char *errmsg;
		    int reason;
		    if (link -> imsg -> reject_reason) {
			log_error ("Failover CONNECT to %d.%d.%d.%d%s%s",
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [0],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [1],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [2],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [3],
				   " rejected: ",
				   (dhcp_failover_reject_reason_print
				    (link -> imsg -> reject_reason)));
			/* XXX print message from peer if peer sent message. */
			omapi_disconnect (link -> outer, 1);
			return ISC_R_SUCCESS;
		    }
				      
		    if (!dhcp_failover_state_match
			(state,
			 (u_int8_t *)&link -> imsg -> server_addr,
			 sizeof link -> imsg -> server_addr)) {
			errmsg = "unknown server";
			reason = FTR_INVALID_PARTNER;
		      badconnectack:
			log_error ("Failover CONNECTACK from %d.%d.%d.%d: %s",
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [0],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [1],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [2],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [3],
				   errmsg);
			dhcp_failover_send_disconnect ((omapi_object_t *)link,
						       reason, errmsg);
			omapi_disconnect (link -> outer, 0);
			return ISC_R_SUCCESS;
		    }

		    if (state -> link_to_peer) {
			errmsg = "already connected";
			reason = FTR_DUP_CONNECTION;
			goto badconnectack;
		    }

		    dhcp_failover_link_reference (&state -> link_to_peer,
						  link, MDL);
		    dhcp_failover_state_transition (state, "connect");
		    add_timeout (cur_time +
				 (int)state -> max_transmit_idle,
				 dhcp_failover_send_contact, state,
				 (tvref_t)dhcp_failover_state_reference,
				 (tvunref_t)dhcp_failover_state_dereference);
		    add_timeout (cur_time +
				 (int)state -> max_response_delay,
				 dhcp_failover_timeout, state,
				 (tvref_t)dhcp_failover_state_reference,
				 (tvunref_t)dhcp_failover_state_dereference);
		} else if (link -> imsg -> type == FTM_DISCONNECT) {
		    if (link -> imsg -> reject_reason) {
			log_error ("Failover DISCONNECT from %d.%d.%d.%d%s%s",
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [0],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [1],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [2],
				   ((u_int8_t *)
				    (&link -> imsg -> server_addr)) [3],
				   ": ",
				   (dhcp_failover_reject_reason_print
				    (link -> imsg -> reject_reason)));
		    }
		    omapi_disconnect (link -> outer, 1);
		} else if (link -> imsg -> type == FTM_BNDUPD) {
			dhcp_failover_process_bind_update (state,
							   link -> imsg);
		} else if (link -> imsg -> type == FTM_BNDACK) {
			dhcp_failover_process_bind_ack (state, link -> imsg);
		} else if (link -> imsg -> type == FTM_POOLREQ) {
			dhcp_failover_pool_rebalance (state);
		} else if (link -> imsg -> type == FTM_POOLRESP) {
			log_info ("pool response: %ld leases",
				  (unsigned long)
				  link -> imsg -> addresses_transferred);
		}
	}

	/* Add a timeout so that if the partner doesn't send another message
	   for the maximum transmit idle time plus a grace of one second,
	   we close the connection. */
	if (state -> my_state == normal)
		add_timeout (cur_time +
			     (int)state -> max_response_delay,
			     dhcp_failover_timeout, state,
			     (tvref_t)dhcp_failover_state_reference,
			     (tvunref_t)dhcp_failover_state_dereference);

	/* Handle all the events we care about... */
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_failover_state_transition (dhcp_failover_state_t *state,
					     const char *name)
{
	/* XXX Check these state transitions against the spec! */
	if (!strcmp (name, "disconnect")) {
		if (state -> link_to_peer) {
		    if (state -> link_to_peer -> state_object)
			dhcp_failover_state_dereference
				(&state -> link_to_peer -> state_object, MDL);
		    dhcp_failover_link_dereference (&state -> link_to_peer,
						    MDL);
		}
		cancel_timeout (dhcp_failover_send_contact, state);
		cancel_timeout (dhcp_failover_timeout, state);

		switch (state -> my_state) {
		      case potential_conflict_nic:
		      case partner_down:
		      case communications_interrupted:
			/* Already in the right state? */
			return ISC_R_SUCCESS;
		
		      case potential_conflict:
			return dhcp_failover_set_state
				(state, potential_conflict_nic);
				
		      case recover:
			/* XXX I don't think it makes sense to make a
			   XXX transition from recover to communications-
			   XXX interrupted, because then when the connect
			   XXX occurred, we'd make a transition into
			   XXX normal, not recover. */
		      case normal:
			return dhcp_failover_set_state
				(state, communications_interrupted);

			/* XXX ??? */
		      case unknown_state:
		      default:
			return dhcp_failover_set_state
				(state, potential_conflict_nic);

		}
	} else if (!strcmp (name, "connect")) {
		switch (state -> my_state) {
		      case communications_interrupted:
			return dhcp_failover_set_state (state, normal);

		      case partner_down:
			return dhcp_failover_set_state (state, recover);
		
		      case potential_conflict_nic:
			return dhcp_failover_set_state (state,
							potential_conflict);

			/* We should never be in these states when we make
			   a "connect" transition. */
		      case recover:
		      case potential_conflict:
		      case normal:
			return ISC_R_INVALIDARG;

			/* We should never be in the unknown_state or an
			   unknown state either. */
		      case unknown_state:
		      default:
			return dhcp_failover_set_state (state,
							potential_conflict);
		}
	} else if (!strcmp (name, "startup")) {
		switch (state -> my_state) {
		      case communications_interrupted:
		      case partner_down:
		      case potential_conflict_nic:
			return ISC_R_SUCCESS;

		      case normal:
		      case recover:
			return dhcp_failover_set_state
				(state, communications_interrupted);

		      case potential_conflict:
			return dhcp_failover_set_state
				(state, potential_conflict_nic);

		      case unknown_state:
			return dhcp_failover_set_state
				(state, communications_interrupted);

		      default:
			return dhcp_failover_set_state
				(state, potential_conflict_nic);
		}
	}
	return ISC_R_INVALIDARG;
}

isc_result_t dhcp_failover_set_state (dhcp_failover_state_t *state,
				      enum failover_state new_state)
{
    enum failover_state saved_state;
    TIME saved_stos;

    /* First make the transition out of the current state. */
    switch (state -> my_state) {
      case normal:
      case recover:
      case potential_conflict:
	/* Any updates that haven't been acked yet, we have to
	   resend, just in case. */
	if (state -> ack_queue_tail) {
	    struct lease *lp;

	    /* Zap the flags. */
	    for (lp = state -> ack_queue_head; lp; lp = lp -> next_pending)
		    lp -> flags = ((lp -> flags & ~ON_ACK_QUEUE) |
				   ON_UPDATE_QUEUE);

	    /* Now hook the ack queue to the beginning of the update
	       queue. */
	    if (state -> update_queue_head) {
		    lease_reference (&state -> ack_queue_tail -> next_pending,
				     state -> update_queue_head, MDL);
		    lease_dereference (&state -> update_queue_head, MDL);
	    }
	    lease_reference (&state -> update_queue_head,
			     state -> ack_queue_head, MDL);
	    if (!state -> update_queue_tail)
		    lease_reference (&state -> update_queue_tail,
				     state -> ack_queue_tail, MDL);
	    lease_dereference (&state -> ack_queue_tail, MDL);
	    lease_dereference (&state -> ack_queue_head, MDL);
	    state -> cur_unacked_updates = 0;
	}
	cancel_timeout (dhcp_failover_keepalive, state);
	break;

      case partner_down:
      case communications_interrupted:
      case potential_conflict_nic:
      default:
	break;
    }

    /* Tentatively make the transition. */
    saved_state = state -> my_state;
    saved_stos = state -> my_stos;
    state -> my_stos = cur_time;
    state -> my_state = new_state;

    if (!write_failover_state (state) || !commit_leases ()) {
	    /* XXX What to do?   What to do? */
	    log_error ("Unable to record current failover state for %s",
		       state -> name);
	    /* XXX for now, we don't make the state transition, but this is
	       XXX kind of a scary choice. */
	    state -> my_state = saved_state;
	    state -> my_stos = saved_stos;
	    return ISC_R_IOERROR;
    }

    log_info ("failover peer %s moves from %s to %s",
	      state -> name, dhcp_failover_state_name_print (saved_state),
	      dhcp_failover_state_name_print (state -> my_state));
    
    switch (new_state)
    {
      case normal:
      case recover:
      case potential_conflict:
	/* If we go into communications-interrupted and then back into
	   potential-conflict, do we need to start over, or just continue
	   with the reconciliation process?    This came up at one of the
	   teleconferences, so it's probably answered in the draft. */
	if (state -> max_flying_updates > state -> cur_unacked_updates &&
	    state -> update_queue_head) {
		dhcp_failover_send_updates (state);
	}
	dhcp_failover_state_pool_check (state);
	break;
      default:
	break;
    }

    return ISC_R_SUCCESS;
}

int dhcp_failover_pool_rebalance (dhcp_failover_state_t *state)
{
	int lts;
	int leases_queued = 0;
	struct lease *lp = (struct lease *)0;
	struct lease *next = (struct lease *)0;
	struct shared_network *s;
	struct pool *p;
	int polarity;
	binding_state_t peer_lease_state;
	binding_state_t my_lease_state;
	struct lease **lq;

	if (state -> my_state != normal || state -> i_am == secondary)
		return 0;

	for (s = shared_networks; s; s = s -> next) {
	    for (p = s -> pools; p; p = p -> next) {
		if (p -> failover_peer != state)
		    continue;
		log_info ("pool %lx total %d  free %d  backup %d",
			  (unsigned long)p, p -> lease_count,
			  p -> free_leases, p -> backup_leases);

		/* Right now we're giving the peer half of the free leases.
		   If we have more leases than the peer (i.e., more than
		   half), then the number of leases we have, less the number
		   of leases the peer has, will be how many more leases we
		   have than the peer has.   So if we send half that number
		   to the peer, we should be even. */
		if (p -> failover_peer -> i_am == primary) {
			lts = (p -> free_leases - p -> backup_leases) / 2;
			peer_lease_state = FTS_BACKUP;
			my_lease_state = FTS_FREE;
			lq = &p -> free;
		} else {
			lts = (p -> backup_leases - p -> free_leases) / 2;
			peer_lease_state = FTS_FREE;
			my_lease_state = FTS_BACKUP;
			lq = &p -> backup;
		}

		if (lts > 1) {
		    lease_reference (&lp, *lq, MDL);

		    while (lp && lts) {
			/* Remember the next lease in the list. */
			if (next)
			    lease_dereference (&next, MDL);
			if (lp -> next)
			    lease_reference (&next, lp -> next, MDL);

			--lts;
			++leases_queued;
			lp -> next_binding_state = peer_lease_state;
			lp -> tstp = cur_time;
			lp -> starts = cur_time;
			if (!supersede_lease (lp, (struct lease *)0, 1, 1, 0))
			{
			    log_info ("can't commit lease %s on giveaway",
				      piaddr (lp -> ip_addr));
			}

			lease_dereference (&lp, MDL);
			if (next)
				lease_reference (&lp, next, MDL);
		    }
		    if (next)
			lease_dereference (&next, MDL);
		    if (lp)
			lease_dereference (&lp, MDL);

		}
		if (lts > 1) {
			log_info ("lease imbalance - lts = %d", lts);
			leases_queued -= lts;
		}
	    }
	}
	dhcp_failover_send_poolresp (state, leases_queued);
	dhcp_failover_send_updates (state);
	return leases_queued;
}

int dhcp_failover_pool_check (struct pool *pool)
{
	int lts;
	struct lease *lp;

	if (!pool -> failover_peer ||
	    pool -> failover_peer -> i_am == primary ||
	    pool -> failover_peer -> my_state != normal)
		return 0;

	if (pool -> failover_peer -> i_am == primary)
		lts = (pool -> backup_leases - pool -> free_leases) / 2;
	else
		lts = (pool -> free_leases - pool -> backup_leases) / 2;

	log_info ("pool %lx total %d  free %d  backup %d  lts %d",
		  (unsigned long)pool, pool -> lease_count,
		  pool -> free_leases, pool -> backup_leases, lts);

	if (lts > 1) {
		/* XXX What about multiple pools? */
		dhcp_failover_send_poolreq (pool -> failover_peer);
		return 1;
	}
	return 0;
}

int dhcp_failover_state_pool_check (dhcp_failover_state_t *state)
{
	struct lease *lp;
	struct shared_network *s;
	struct pool *p;

	for (s = shared_networks; s; s = s -> next) {
		for (p = s -> pools; p; p = p -> next) {
			if (p -> failover_peer != state)
				continue;
			/* Only need to request rebalance on one pool. */
			if (dhcp_failover_pool_check (p))
				return 1;
		}
	}
	return 0;
}

isc_result_t dhcp_failover_send_updates (dhcp_failover_state_t *state)
{
	struct lease *lp = (struct lease *)0;
	isc_result_t status;

	/* Can't update peer if we're not talking to it! */
	if (state -> my_state != normal &&
	    state -> my_state != recover &&
	    state -> my_state != potential_conflict)
		return ISC_R_SUCCESS;

	while (state -> max_flying_updates > state -> cur_unacked_updates &&
	       state -> update_queue_head) {
		/* Grab the head of the update queue. */
		lease_reference (&lp, state -> update_queue_head, MDL);

		/* Send the update to the peer. */
		status = dhcp_failover_send_bind_update (state, lp);
		if (status != ISC_R_SUCCESS) {
			lease_dereference (&lp, MDL);
			return status;
		}
		lp -> flags &= ~ON_UPDATE_QUEUE;

		/* Take it off the head of the update queue and put the next
		   item in the update queue at the head. */
		lease_dereference (&state -> update_queue_head, MDL);
		if (lp -> next_pending) {
			lease_reference (&state -> update_queue_head,
					 lp -> next_pending, MDL);
			lease_dereference (&lp -> next_pending, MDL);
		} else {
			lease_dereference (&state -> update_queue_tail, MDL);
		}

		if (state -> ack_queue_head) {
			lease_reference
				(&state -> ack_queue_tail -> next_pending,
				 lp, MDL);
			lease_dereference (&state -> ack_queue_tail, MDL);
		} else {
			lease_reference (&state -> ack_queue_head, lp, MDL);
		}
		lease_reference (&state -> ack_queue_tail, lp, MDL);
		lp -> flags |= ON_ACK_QUEUE;
		lease_dereference (&lp, MDL);

		/* Count the object as an unacked update. */
		state -> cur_unacked_updates++;
	}
	return ISC_R_SUCCESS;
}

/* Queue an update for a lease.   Always returns 1 at this point - it's
   not an error for this to be called on a lease for which there's no
   failover peer. */

int dhcp_failover_queue_update (struct lease *lease, int immediate)
{
	dhcp_failover_state_t *state;

	if (!lease -> pool ||
	    !lease -> pool -> failover_peer)
		return 1;

	/* If it's already on the update queue, leave it there. */
	if (lease -> flags & ON_UPDATE_QUEUE)
		return 1;

	/* Get the failover state structure for this lease. */
	state = lease -> pool -> failover_peer;

	/* If it's on the ack queue, take it off. */
	if (lease -> flags & ON_ACK_QUEUE)
		dhcp_failover_ack_queue_remove (state, lease);

	if (state -> update_queue_head) {
		if (state -> update_queue_tail -> next_pending)
			lease_dereference
				(&state -> update_queue_tail -> next_pending,
				 MDL);
		lease_reference (&state -> update_queue_tail -> next_pending,
				 lease, MDL);
		lease_dereference (&state -> update_queue_tail, MDL);
	} else {
		lease_reference (&state -> update_queue_head, lease, MDL);
	}
	lease_reference (&state -> update_queue_tail, lease, MDL);
	lease -> flags |= ON_UPDATE_QUEUE;
	if (immediate)
		dhcp_failover_send_updates (state);
	return 1;
}

void dhcp_failover_ack_queue_remove (dhcp_failover_state_t *state,
				     struct lease *lease)
{
	struct lease *lp;

	if (state -> ack_queue_head == lease) {
		lease_dereference (&state -> ack_queue_head, MDL);
		if (lease -> next_pending) {
			lease_reference (&state -> ack_queue_head,
					 lease -> next_pending, MDL);
			lease_dereference (&lease -> next_pending, MDL);
		} else {
			lease_dereference (&state -> ack_queue_tail, MDL);
		}
		lease -> flags &= ~ON_ACK_QUEUE;
		state -> cur_unacked_updates--;
		return;
	}
	for (lp = state -> ack_queue_head;
	     lp && lp -> next_pending != lease; lp = lp -> next_pending)
		;
	if (lp) {
		lease_dereference (&lp -> next_pending, MDL);
		if (lease -> next_pending) {
			lease_reference (&lp -> next_pending,
					 lease -> next_pending, MDL);
			lease_dereference (&lease -> next_pending, MDL);
		} else {
			lease_dereference (&state -> ack_queue_tail, MDL);
			lease_reference (&state -> ack_queue_tail, lp, MDL);
		}
	}
	lease -> flags &= ~ON_ACK_QUEUE;
	state -> cur_unacked_updates--;
}

isc_result_t dhcp_failover_state_set_value (omapi_object_t *h,
					    omapi_object_t *id,
					    omapi_data_string_t *name,
					    omapi_typed_data_t *value)
{
	if (h -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> set_value)
		return (*(h -> inner -> type -> set_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

void dhcp_failover_keepalive (void *vs)
{
	dhcp_failover_state_t *state = vs;
}

void dhcp_failover_reconnect (void *vs)
{
	dhcp_failover_state_t *state = vs;
	isc_result_t status;

	status = dhcp_failover_link_initiate ((omapi_object_t *)state);
	if (status != ISC_R_SUCCESS) {
		log_info ("failover peer %s: %s", state -> name,
			  isc_result_totext (status));
		add_timeout (cur_time + 90,
			     dhcp_failover_listener_restart, state,
			     (tvref_t)dhcp_failover_state_reference,
			     (tvunref_t)dhcp_failover_state_dereference);
	}
}

void dhcp_failover_listener_restart (void *vs)
{
	dhcp_failover_state_t *state = vs;
	isc_result_t status;

	status = dhcp_failover_listen ((omapi_object_t *)state);
	if (status != ISC_R_SUCCESS) {
		log_info ("failover peer %s: %s", state -> name,
			  isc_result_totext (status));
		add_timeout (cur_time + 90,
			     dhcp_failover_listener_restart, state,
			     (tvref_t)dhcp_failover_state_reference,
			     (tvunref_t)dhcp_failover_state_dereference);
	}
}

isc_result_t dhcp_failover_state_get_value (omapi_object_t *h,
					    omapi_object_t *id,
					    omapi_data_string_t *name,
					    omapi_value_t **value)
{
	dhcp_failover_state_t *s;
	struct option_cache *oc;
	struct data_string ds;
	isc_result_t status;

	if (h -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	s = (dhcp_failover_state_t *)h;
	
	if (!omapi_ds_strcmp (name, "name")) {
		if (s -> name)
			return omapi_make_string_value (value,
							name, s -> name, MDL);
		return ISC_R_NOTFOUND;
	} else if (!omapi_ds_strcmp (name, "partner-address")) {
		oc = s -> address;
	      getaddr:
		memset (&ds, 0, sizeof ds);
		if (!evaluate_option_cache (&ds, (struct packet *)0,
					    (struct lease *)0,
					    (struct option_state *)0,
					    (struct option_state *)0,
					    &global_scope, oc, MDL)) {
			return ISC_R_NOTFOUND;
		}
		status = omapi_make_const_value (value,
						 name, ds.data, ds.len, MDL);
		/* Disgusting kludge: */
		if (oc == s -> server_addr && !s -> server_identifier.len)
			data_string_copy (&s -> server_identifier, &ds, MDL);
		data_string_forget (&ds, MDL);
		return status;
	} else if (!omapi_ds_strcmp (name, "local-address")) {
		oc = s -> server_addr;
		goto getaddr;
	} else if (!omapi_ds_strcmp (name, "partner-port")) {
		return omapi_make_int_value (value, name, s -> port, MDL);
	} else if (!omapi_ds_strcmp (name, "local-port")) {
		return omapi_make_int_value (value,
					     name, s -> listen_port, MDL);
	} else if (!omapi_ds_strcmp (name, "max-outstanding-updates")) {
		return omapi_make_uint_value (value, name,
					      s -> max_flying_updates, MDL);
	} else if (!omapi_ds_strcmp (name, "mclt")) {
		return omapi_make_uint_value (value, name, s -> mclt, MDL);
	} else if (!omapi_ds_strcmp (name, "load-balance-max-secs")) {
		return omapi_make_int_value (value, name,
					     s -> load_balance_max_secs, MDL);
	} else if (!omapi_ds_strcmp (name, "load-balance-hba")) {
		if (s -> hba)
			return omapi_make_const_value (value, name,
						       s -> hba, 32, MDL);
		return ISC_R_NOTFOUND;
	} else if (!omapi_ds_strcmp (name, "partner-state")) {
		return omapi_make_uint_value (value, name,
					     s -> partner_state, MDL);
	} else if (!omapi_ds_strcmp (name, "local-state")) {
		return omapi_make_uint_value (value, name,
					      s -> my_state, MDL);
	} else if (!omapi_ds_strcmp (name, "partner-stos")) {
		return omapi_make_int_value (value, name,
					     s -> partner_stos, MDL);
	} else if (!omapi_ds_strcmp (name, "local-stos")) {
		return omapi_make_int_value (value, name,
					     s -> my_stos, MDL);
	} else if (!omapi_ds_strcmp (name, "hierarchy")) {
		return omapi_make_uint_value (value, name, s -> i_am, MDL);
	} else if (!omapi_ds_strcmp (name, "last-packet-sent")) {
		return omapi_make_int_value (value, name,
					     s -> last_packet_sent, MDL);
	} else if (!omapi_ds_strcmp (name, "last-timestamp-received")) {
		return omapi_make_int_value (value, name,
					     s -> last_timestamp_received,
					     MDL);
	} else if (!omapi_ds_strcmp (name, "skew")) {
		return omapi_make_int_value (value, name, s -> skew, MDL);
	} else if (!omapi_ds_strcmp (name, "max-transmit-idle")) {
		return omapi_make_uint_value (value, name,
					     s -> max_transmit_idle, MDL);
	} else if (!omapi_ds_strcmp (name, "max-response-delay")) {
		return omapi_make_uint_value (value, name,
					     s -> max_response_delay, MDL);
	} else if (!omapi_ds_strcmp (name, "cur-unacked-updates")) {
		return omapi_make_int_value (value, name,
					     s -> cur_unacked_updates, MDL);
	}
		
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_failover_state_destroy (omapi_object_t *h,
					      const char *file, int line)
{
	dhcp_failover_state_t *s;

	if (h -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	s = (dhcp_failover_state_t *)h;
	if (s -> link_to_peer)
		dhcp_failover_link_dereference (&s -> link_to_peer, MDL);
	if (s -> name)
		dfree (s -> name, MDL);
	if (s -> address)
		option_cache_dereference (&s -> address, MDL);
	if (s -> server_addr)
		option_cache_dereference (&s -> server_addr, MDL);
	
	return ISC_R_SUCCESS;
}

/* Write all the published values associated with the object through the
   specified connection. */

isc_result_t dhcp_failover_state_stuff (omapi_object_t *c,
					omapi_object_t *id,
					omapi_object_t *h)
{
	dhcp_failover_state_t *s;
	omapi_connection_object_t *conn;
	isc_result_t status;

	if (c -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;
	conn = (omapi_connection_object_t *)c;

	if (h -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	s = (dhcp_failover_state_t *)h;
	
	status = omapi_connection_put_name (c, "name");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_string (c, s -> name);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "partner-address");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof s -> address);
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_copyin (c, (u_int8_t *)&s -> address,
					  sizeof s -> address);
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "partner-port");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, (u_int32_t)s -> port);
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "local-address");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof s -> server_addr);
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_copyin (c, (u_int8_t *)&s -> server_addr,
					  sizeof s -> server_addr);
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "local-port");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, (u_int32_t)s -> listen_port);
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "max-outstanding-updates");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, s -> max_flying_updates);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "mclt");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, s -> mclt);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "load-balance-max-secs");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_put_uint32
		  (c, (u_int32_t)s -> load_balance_max_secs));
	if (status != ISC_R_SUCCESS)
		return status;

	
	if (s -> hba) {
		status = omapi_connection_put_name (c, "load-balance-hba");
		if (status != ISC_R_SUCCESS)
			return status;
		status = omapi_connection_put_uint32 (c, 32);
		if (status != ISC_R_SUCCESS)
			return status;
		status = omapi_connection_copyin (c, s -> hba, 32);
		if (status != ISC_R_SUCCESS)
			return status;
	}

	status = omapi_connection_put_name (c, "partner-state");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, s -> partner_state);
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "local-state");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, s -> my_state);
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "partner-stos");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, (u_int32_t)s -> partner_stos);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "local-stos");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, (u_int32_t)s -> my_stos);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "hierarchy");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, s -> i_am);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "last-packet-sent");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_put_uint32
		  (c, (u_int32_t)s -> last_packet_sent));
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "last-timestamp-received");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_put_uint32
		  (c, (u_int32_t)s -> last_timestamp_received));
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "skew");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, (u_int32_t)s -> skew);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "max-transmit-idle");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_put_uint32
		  (c, (u_int32_t)s -> max_transmit_idle));
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "max-response-delay");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_put_uint32
		  (c, (u_int32_t)s -> max_response_delay));
	if (status != ISC_R_SUCCESS)
		return status;
	
	status = omapi_connection_put_name (c, "cur-unacked-updates");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (u_int32_t));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_put_uint32
		  (c, (u_int32_t)s -> cur_unacked_updates));
	if (status != ISC_R_SUCCESS)
		return status;

	if (h -> inner && h -> inner -> type -> stuff_values)
		return (*(h -> inner -> type -> stuff_values)) (c, id,
								h -> inner);
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_failover_state_lookup (omapi_object_t **sp,
					 omapi_object_t *id,
					 omapi_object_t *ref)
{
	omapi_value_t *tv = (omapi_value_t *)0;
	isc_result_t status;
	dhcp_failover_state_t *s;

	/* First see if we were sent a handle. */
	status = omapi_get_value_str (ref, id, "handle", &tv);
	if (status == ISC_R_SUCCESS) {
		status = omapi_handle_td_lookup (sp, tv -> value);

		omapi_value_dereference (&tv, MDL);
		if (status != ISC_R_SUCCESS)
			return status;

		/* Don't return the object if the type is wrong. */
		if ((*sp) -> type != dhcp_type_failover_state) {
			omapi_object_dereference (sp, MDL);
			return ISC_R_INVALIDARG;
		}
	}

	/* Look the failover state up by peer name. */
	status = omapi_get_value_str (ref, id, "peer_name", &tv);
	if (status == ISC_R_SUCCESS) {
		for (s = failover_states; s; s = s -> next) {
			unsigned l = strlen (s -> name);
			if (l == tv -> value -> u.buffer.len ||
			    !memcmp (s -> name,
				     tv -> value -> u.buffer.value, l))
				break;
		}
		omapi_value_dereference (&tv, MDL);

		/* If we already have a lease, and it's not the same one,
		   then the query was invalid. */
		if (*sp && *sp != (omapi_object_t *)s) {
			omapi_object_dereference (sp, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!s) {
			if (*sp)
				omapi_object_dereference (sp, MDL);
			return ISC_R_NOTFOUND;
		} else if (!*sp)
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (sp, (omapi_object_t *)s, MDL);
	}

	/* If we get to here without finding a lease, no valid key was
	   specified. */
	if (!*sp)
		return ISC_R_NOKEYS;
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_failover_state_create (omapi_object_t **sp,
					 omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dhcp_failover_state_remove (omapi_object_t *sp,
					 omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

int dhcp_failover_state_match (dhcp_failover_state_t *state,
			       u_int8_t *addr, unsigned addrlen)
{
	struct option_cache *oc;
	struct data_string ds;
	int i;
	
	memset (&ds, 0, sizeof ds);
	if (evaluate_option_cache (&ds, (struct packet *)0,
				   (struct lease *)0,
				   (struct option_state *)0,
				   (struct option_state *)0,
				   &global_scope,
				   state -> address, MDL)) {
		for (i = 0; i + addrlen - 1 < ds.len; i += addrlen) {
			if (!memcmp (&ds.data [i],
				     addr, addrlen)) {
				data_string_forget (&ds, MDL);
				return 1;
			}
		}
		data_string_forget (&ds, MDL);
	}
	return 0;
}

const char *dhcp_failover_reject_reason_print (int reason)
{
    switch (reason) {
      case FTR_ILLEGAL_IP_ADDR:
	return "Illegal IP address (not part of any address pool).";

      case FTR_FATAL_CONFLICT:
	return "Fatal conflict exists: address in use by other client.";

      case FTR_MISSING_BINDINFO:
	return "Missing binding information.";

      case FTR_TIMEMISMATCH:
	return "Connection rejected, time mismatch too great.";

      case FTR_INVALID_MCLT:
	return "Connection rejected, invalid MCLT.";

      case FTR_MISC_REJECT:
	return "Connection rejected, unknown reason.";

      case FTR_DUP_CONNECTION:
	return "Connection rejected, duplicate connection.";

      case FTR_INVALID_PARTNER:
	return "Connection rejected, invalid failover partner.";

      case FTR_TLS_UNSUPPORTED:
	return "TLS not supported.";

      case FTR_TLS_UNCONFIGURED:
	return "TLS supported but not configured.";

      case FTR_TLS_REQUIRED:
	return "TLS required but not supported by partner.";

      case FTR_DIGEST_UNSUPPORTED:
	return "Message digest not supported.";

      case FTR_DIGEST_UNCONFIGURED:
	return "Message digest not configured.";

      case FTR_VERSION_MISMATCH:
	return "Protocol version mismatch.";

      case FTR_MISSING_BIND_INFO:
	return "Missing binding information.";

      case FTR_OUTDATED_BIND_INFO:
	return "Outdated binding information.";

      case FTR_LESS_CRIT_BIND_INFO:
	return "Less critical binding information.";

      case FTR_NO_TRAFFIC:
	return "No traffic within sufficient time.";

      case FTR_HBA_CONFLICT:
	return "Hash bucket assignment conflict.";

      default:
      case FTR_UNKNOWN:
	return "Unknown: Error occurred but does not match any reason code.";
    }
}

const char *dhcp_failover_state_name_print (enum failover_state state)
{
	switch (state) {
	      default:
	      case unknown_state:
		return "unknown-state";

	      case partner_down:
		return "partner-down";

	      case normal:
		return "normal";

	      case communications_interrupted:
		return "communications-interrupted";

	      case potential_conflict_nic:
		return "potential-conflict-nic";

	      case potential_conflict:
		return "potential-conflict";

	      case recover:
		return "recover";
	}
}

failover_option_t *dhcp_failover_option_printf (unsigned code,
						char *obuf,
						unsigned *obufix,
						unsigned obufmax,
						const char *fmt, ...)
{
	va_list va;
	char tbuf [256];

	va_start (va, fmt);
#if defined (HAVE_SNPRINTF)
	/* Presumably if we have snprintf, we also have
	   vsnprintf. */
	vsnprintf (tbuf, sizeof tbuf, fmt, va);
#else
	vsprintf (tbuf, fmt, va);
#endif
	va_end (va);

	return dhcp_failover_make_option (code, obuf, obufix, obufmax,
					  strlen (tbuf), tbuf);
}

failover_option_t *dhcp_failover_make_option (unsigned code,
					      char *obuf, unsigned *obufix,
					      unsigned obufmax, ...)
{
	va_list va;
	struct failover_option_info *info;
	int i;
	unsigned size, count;
	unsigned val;
	u_int8_t *iaddr;
	unsigned ilen;
	u_int8_t *bval;
	char *txt;
#if defined (DEBUG_FAILOVER_MESSAGES)
	char tbuf [256];
#endif

	/* Note that the failover_option structure is used differently on
	   input than on output - on input, count is an element count, and
	   on output it's the number of bytes total in the option, including
	   the option code and option length. */
	failover_option_t option, *op;


	/* Bogus option code? */
	if (code < 1 || code > FTO_MAX || ft_options [code].type == FT_UNDEF) {
		return &null_failover_option;
	}
	info = &ft_options [code];
		
	va_start (va, obufmax);

	/* Get the number of elements and the size of the buffer we need
	   to allocate. */
	if (info -> type == FT_DDNS || info -> type == FT_DDNS1) {
		count = info -> type == FT_DDNS ? 1 : 2;
		size = va_arg (va, int) + count;
	} else {
		/* Find out how many items in this list. */
		if (info -> num_present)
			count = info -> num_present;
		else
			count = va_arg (va, int);

		/* Figure out size. */
		switch (info -> type) {
		      case FT_UINT8:
		      case FT_BYTES:
		      case FT_DIGEST:
			size = count;
			break;

		      case FT_TEXT_OR_BYTES:
		      case FT_TEXT:
			txt = va_arg (va, char *);
			size = count;
			break;

		      case FT_IPADDR:
			ilen = va_arg (va, unsigned);
			size = count * ilen;
			break;

		      case FT_UINT32:
			size = count * 4;
			break;

		      case FT_UINT16:
			size = count * 2;
			break;

		      default:
			/* shouldn't get here. */
			log_fatal ("bogus type in failover_make_option: %d",
				   info -> type);
			break;
		}
	}
	
	size += 4;

	/* Allocate a buffer for the option. */
	option.count = size;
	option.data = dmalloc (option.count, MDL);
	if (!option.data)
		return &null_failover_option;

	/* Put in the option code and option length. */
	putUShort (option.data, code);
	putUShort (&option.data [2], size - 4);

#if defined (DEBUG_FAILOVER_MESSAGES)	
	sprintf (tbuf, " (%s<%d>", info -> name, option.count);
	failover_print (obuf, obufix, obufmax, tbuf);
#endif

	/* Now put in the data. */
	switch (info -> type) {
	      case FT_UINT8:
		for (i = 0; i < count; i++) {
			val = va_arg (va, unsigned);
#if defined (DEBUG_FAILOVER_MESSAGES)
			sprintf (tbuf, " %d", val);
			failover_print (obuf, obufix, obufmax, tbuf);
#endif
			option.data [i + 4] = val;
		}
		break;

	      case FT_IPADDR:
		for (i = 0; i < count; i++) {
			iaddr = va_arg (va, u_int8_t *);
			if (ilen != 4) {
				dfree (option.data, MDL);
				log_error ("IP addrlen=%d, should be 4.",
					   ilen);
				return &null_failover_option;
			}
				
#if defined (DEBUG_FAILOVER_MESSAGES)
			sprintf (tbuf, " %u.%u.%u.%u", iaddr [0], iaddr [1],
				 iaddr [2], iaddr [3]);
			failover_print (obuf, obufix, obufmax, tbuf);
#endif
			memcpy (&option.data [4 + i * ilen], iaddr, ilen);
		}
		break;

	      case FT_UINT32:
		for (i = 0; i < count; i++) {
			val = va_arg (va, unsigned);
#if defined (DEBUG_FAILOVER_MESSAGES)
			sprintf (tbuf, " %d", val);
			failover_print (obuf, obufix, obufmax, tbuf);
#endif
			putULong (&option.data [4 + i * 4], val);
		}
		break;

	      case FT_BYTES:
	      case FT_DIGEST:
		bval = va_arg (va, u_int8_t *);
#if defined (DEBUG_FAILOVER_MESSAGES)
		for (i = 0; i < count; i++) {
			sprintf (tbuf, " %d", bval [i]);
			failover_print (obuf, obufix, obufmax, tbuf);
		}
#endif
		memcpy (&option.data [4], bval, count);
		break;

		/* On output, TEXT_OR_BYTES is _always_ text, and always NUL
		   terminated.  Note that the caller should be careful not to
		   provide a format and data that amount to more than 256 bytes
		   of data, since it will be truncated on platforms that
		   support snprintf, and will mung the stack on those platforms
		   that do not support snprintf.  Also, callers should not pass
		   data acquired from the network without specifically checking
		   it to make sure it won't bash the stack. */
	      case FT_TEXT_OR_BYTES:
	      case FT_TEXT:
#if defined (DEBUG_FAILOVER_MESSAGES)
		sprintf (tbuf, "\"%s\"", txt);
		failover_print (obuf, obufix, obufmax, tbuf);
#endif
		memcpy (&option.data [4], txt, count);
		break;

	      case FT_DDNS:
	      case FT_DDNS1:
		option.data [4] = va_arg (va, unsigned);
		if (count == 2)
			option.data [5] = va_arg (va, unsigned);
		bval = va_arg (va, u_int8_t *);
		memcpy (&option.data [4 + count], bval, size - count - 4);
#if defined (DEBUG_FAILOVER_MESSAGES)
		for (i = 4; i < size; i++) {
			sprintf (tbuf, " %d", option.data [i]);
			failover_print (obuf, obufix, obufmax, tbuf);
		}
#endif
		break;

	      case FT_UINT16:
		for (i = 0; i < count; i++) {
			val = va_arg (va, u_int32_t);
#if defined (DEBUG_FAILOVER_MESSAGES)
			sprintf (tbuf, " %d", val);
			failover_print (obuf, obufix, obufmax, tbuf);
#endif
			putUShort (&option.data [4 + i * 2], val);
		}
		break;

	      case FT_UNDEF:
	      default:
		break;
	}

	failover_print (obuf, obufix, obufmax, ")");

	/* Now allocate a place to store what we just set up. */
	op = dmalloc (sizeof (failover_option_t), MDL);
	if (!op) {
		dfree (option.data, MDL);
		return &null_failover_option;
	}

	*op = option;
	return op;
}

/* Send a failover message header. */

isc_result_t dhcp_failover_put_message (dhcp_failover_link_t *link,
					omapi_object_t *connection,
					int msg_type, ...)
{
	unsigned count = 0;
	unsigned size = 0;
	int bad_option = 0;
	int opix = 0;
	va_list list;
	failover_option_t *option;
	unsigned char *opbuf;
	isc_result_t status = ISC_R_SUCCESS;
	unsigned char cbuf;

	/* Run through the argument list once to compute the length of
	   the option portion of the message. */
	va_start (list, msg_type);
	while ((option = va_arg (list, failover_option_t *))) {
		if (option != &skip_failover_option)
			size += option -> count;
		if (option == &null_failover_option)
			bad_option = 1;
	}
	va_end (list);

	/* Allocate an option buffer, unless we got an error. */
	if (!bad_option) {
		opbuf = dmalloc (size, MDL);
		if (!opbuf)
			status = ISC_R_NOMEMORY;
	}

	va_start (list, msg_type);
	while ((option = va_arg (list, failover_option_t *))) {
		if (option == &skip_failover_option)
		    continue;
		if (!bad_option && opbuf)
			memcpy (&opbuf [opix],
				option -> data, option -> count);
		if (option != &null_failover_option &&
		    option != &skip_failover_option) {
			opix += option -> count;
			dfree (option -> data, MDL);
			dfree (option, MDL);
		}
	}

	if (bad_option)
		return ISC_R_INVALIDARG;

	/* Now send the message header. */

	/* Message length. */
	status = omapi_connection_put_uint16 (connection, size + 12);
	if (status != ISC_R_SUCCESS)
		goto err;

	/* Message type. */
	cbuf = msg_type;
	status = omapi_connection_copyin (connection, &cbuf, 1);
	if (status != ISC_R_SUCCESS)
		goto err;

	/* Payload offset. */
	cbuf = 12;
	status = omapi_connection_copyin (connection, &cbuf, 1);
	if (status != ISC_R_SUCCESS)
		goto err;

	/* Current time. */
	status = omapi_connection_put_uint32 (connection, (u_int32_t)cur_time);
	if (status != ISC_R_SUCCESS)
		goto err;

	/* Transaction ID. */
	status = omapi_connection_put_uint32 (connection, link -> xid++);
	if (status != ISC_R_SUCCESS)
		goto err;

	
	/* Payload. */
	status = omapi_connection_copyin (connection, opbuf, size);
	if (status != ISC_R_SUCCESS)
		goto err;
	dfree (opbuf, MDL);
	if (link -> state_object &&
	    link -> state_object -> link_to_peer == link)
		add_timeout (cur_time +
			     (int)link -> state_object -> max_transmit_idle,
			     dhcp_failover_send_contact, link -> state_object,
			     (tvref_t)dhcp_failover_state_reference,
			     (tvunref_t)dhcp_failover_state_dereference);
	return status;

      err:
	dfree (opbuf, MDL);
	omapi_disconnect (connection, 1);
	return status;
}

void dhcp_failover_timeout (void *vstate)
{
	dhcp_failover_state_t *state = vstate;
	dhcp_failover_link_t *link;
	isc_result_t status;

	if (!state || state -> type != dhcp_type_failover_state)
		return;
	if (state -> my_state != normal)
		return;
	link = state -> link_to_peer;
	if (!link ||
	    !link -> outer ||
	    link -> outer -> type != omapi_type_connection)
		return;

	log_error ("timeout waiting for failover peer %s", state -> name);

	/* If we haven't gotten a timely response, blow away the connection.
	   This will cause the state to change automatically. */
	omapi_disconnect (link -> outer, 1);
}

void dhcp_failover_send_contact (void *vstate)
{
	dhcp_failover_state_t *state = vstate;
	dhcp_failover_link_t *link;
	isc_result_t status;

#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(contact");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!state || state -> type != dhcp_type_failover_state)
		return;
	link = state -> link_to_peer;
	if (!link ||
	    !link -> outer ||
	    link -> outer -> type != omapi_type_connection)
		return;

	status = (dhcp_failover_put_message
		  (link, link -> outer,
		   FTM_CONTACT,
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return;
}

/* Send a connect message. */

isc_result_t dhcp_failover_send_connect (omapi_object_t *l)
{
	dhcp_failover_link_t *link;
	dhcp_failover_state_t *state;
	isc_result_t status;
	char hba [32];
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(connect");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!l || l -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)l;
	state = link -> state_object;
	if (!l -> outer || l -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	if (state -> hba) {
		int i;
		for (i = 0; i < 32; i++)
			hba [i] = ~state -> hba [i];
	}

	status = (dhcp_failover_put_message
		  (link, l -> outer,
		   FTM_CONNECT,
		   dhcp_failover_make_option (FTO_SERVER_ADDR, FMA,
					      state -> server_identifier.len,
					      state -> server_identifier.data),
		   dhcp_failover_make_option (FTO_MAX_UNACKED, FMA,
					      state -> max_flying_updates),
		   dhcp_failover_make_option (FTO_RECEIVE_TIMER, FMA,
					      state -> max_response_delay),
		   dhcp_failover_option_printf (FTO_VENDOR_CLASS, FMA,
						"isc-%s", DHCP_VERSION),
		   dhcp_failover_make_option (FTO_PROTOCOL_VERSION, FMA,
					      DHCP_FAILOVER_VERSION),
		   dhcp_failover_make_option (FTO_TLS_REQUEST, FMA,
					      0, 0),
		   dhcp_failover_make_option (FTO_MCLT, FMA,
					      state -> mclt),
		   (state -> hba
		    ? dhcp_failover_make_option (FTO_HBA, FMA, 32, hba)
		    : &skip_failover_option),
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return status;
}

isc_result_t dhcp_failover_send_connectack (omapi_object_t *l,
					    dhcp_failover_state_t *state,
					    int reason, const char *errmsg)
{
	dhcp_failover_link_t *link;
	isc_result_t status;
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(connectack");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!l || l -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)l;
	if (!l -> outer || l -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	status = (dhcp_failover_put_message
		  (link, l -> outer,
		   FTM_CONNECTACK,
		   (state
		    ? (dhcp_failover_make_option
		       (FTO_SERVER_ADDR, FMA,
			state -> server_identifier.len,
			state -> server_identifier.data))
		    : &skip_failover_option),
		   (state
		    ? dhcp_failover_make_option (FTO_MAX_UNACKED, FMA,
						 state -> max_flying_updates)
		    : &skip_failover_option),
		   (state
		    ? dhcp_failover_make_option (FTO_RECEIVE_TIMER, FMA,
						 state -> max_response_delay)
		    : &skip_failover_option),
		   dhcp_failover_option_printf (FTO_VENDOR_CLASS, FMA,
						"isc-%s", DHCP_VERSION),
		   dhcp_failover_make_option (FTO_PROTOCOL_VERSION, FMA,
					      DHCP_FAILOVER_VERSION),
		   dhcp_failover_make_option (FTO_TLS_REQUEST, FMA,
					      0, 0),
		   (reason
		    ? dhcp_failover_make_option (FTO_REJECT_REASON,
						 FMA, reason)
		    : &skip_failover_option),
		   (errmsg
		    ? dhcp_failover_make_option (FTO_MESSAGE, FMA,
						 strlen (errmsg), errmsg)
		    : &skip_failover_option),
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return status;
}

isc_result_t dhcp_failover_send_disconnect (omapi_object_t *l,
					    int reason,
					    const char *message)
{
	dhcp_failover_link_t *link;
	dhcp_failover_state_t *state;
	isc_result_t status;
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(disconnect");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!l || l -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)l;
	state = link -> state_object;
	if (!l -> outer || l -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	if (!message && reason)
		message = dhcp_failover_reject_reason_print (reason);

	status = (dhcp_failover_put_message
		  (link, l -> outer,
		   FTM_DISCONNECT,
		   dhcp_failover_make_option (FTO_REJECT_REASON,
					      FMA, reason),
		   (message
		    ? dhcp_failover_make_option (FTO_MESSAGE, FMA,
						 strlen (message), message)
		    : &skip_failover_option),
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return status;
}

/* Send a Bind Update message. */

isc_result_t dhcp_failover_send_bind_update (dhcp_failover_state_t *state,
					     struct lease *lease)
{
	dhcp_failover_link_t *link;
	isc_result_t status;
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(bndupd");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!state -> link_to_peer ||
	    state -> link_to_peer -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)state -> link_to_peer;

	if (!link -> outer || link -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	/* Send the update. */
	status = (dhcp_failover_put_message
		  (link, link -> outer,
		   FTM_BNDUPD,
		   dhcp_failover_make_option (FTO_ASSIGNED_IP_ADDRESS, FMA,
					      lease -> ip_addr.len,
					      lease -> ip_addr.iabuf),
		   dhcp_failover_make_option (FTO_BINDING_STATUS, FMA,
					      lease -> binding_state),
		   lease -> uid_len
		   ? dhcp_failover_make_option (FTO_CLIENT_IDENTIFIER, FMA,
						lease -> uid_len,
						lease -> uid)
		   : &skip_failover_option,
		   lease -> hardware_addr.hlen
		   ? dhcp_failover_make_option (FTO_CHADDR, FMA,
						lease -> hardware_addr.hlen,
						lease -> hardware_addr.hbuf)
		   : &skip_failover_option,
		   dhcp_failover_make_option (FTO_LEASE_EXPIRY, FMA,
					      lease -> ends),
		   dhcp_failover_make_option (FTO_POTENTIAL_EXPIRY, FMA,
					      lease -> tstp),
		   dhcp_failover_make_option (FTO_STOS, FMA,
					      lease -> starts),
		   dhcp_failover_make_option (FTO_CLTT, FMA,
					      lease -> cltt),
		   &skip_failover_option,	/* XXX DDNS */
		   &skip_failover_option,	/* XXX request options */
		   &skip_failover_option,	/* XXX reply options */
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return status;
}

/* Send a Bind ACK message. */

isc_result_t dhcp_failover_send_bind_ack (dhcp_failover_state_t *state,
					  struct lease *lease,
					  failover_message_t *msg,
					  int reason, const char *message)
{
	dhcp_failover_link_t *link;
	isc_result_t status;
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(bndack");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!state -> link_to_peer ||
	    state -> link_to_peer -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)state -> link_to_peer;

	if (!link -> outer || link -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	if (!message && reason)
		message = dhcp_failover_reject_reason_print (reason);

	/* Send the update. */
	status = (dhcp_failover_put_message
		  (link, link -> outer,
		   FTM_BNDACK,
		   dhcp_failover_make_option (FTO_ASSIGNED_IP_ADDRESS, FMA,
					      sizeof msg -> assigned_addr,
					      &msg -> assigned_addr),
		   dhcp_failover_make_option (FTO_BINDING_STATUS, FMA,
					      msg -> binding_status),
		   (msg -> options_present & FTB_CLIENT_IDENTIFIER)
		   ? dhcp_failover_make_option (FTO_CLIENT_IDENTIFIER, FMA,
						msg -> client_identifier.count,
						msg -> client_identifier.data)
		   : &skip_failover_option,
		   (msg -> options_present & FTB_CHADDR)
		   ? dhcp_failover_make_option (FTO_CHADDR, FMA,
						msg -> chaddr.count,
						msg -> chaddr.data)
		   : &skip_failover_option,
		   dhcp_failover_make_option (FTO_LEASE_EXPIRY, FMA,
					      msg -> expiry),
		   dhcp_failover_make_option (FTO_POTENTIAL_EXPIRY, FMA,
					      msg -> potential_expiry),
		   dhcp_failover_make_option (FTO_STOS, FMA,
					      msg -> stos),
		   dhcp_failover_make_option (FTO_CLTT, FMA,
					      msg -> client_ltt),
		   reason
		   ? dhcp_failover_make_option (FTO_REJECT_REASON,
						FMA, reason)
		   : &skip_failover_option,
		   (message
		    ? dhcp_failover_make_option (FTO_MESSAGE, FMA,
						 strlen (message), message)
		    : &skip_failover_option),
		   &skip_failover_option,	/* XXX DDNS */
		   &skip_failover_option,	/* XXX request options */
		   &skip_failover_option,	/* XXX reply options */
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return status;
}

isc_result_t dhcp_failover_send_poolreq (dhcp_failover_state_t *state)
{
	dhcp_failover_link_t *link;
	isc_result_t status;
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(poolreq");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!state -> link_to_peer ||
	    state -> link_to_peer -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)state -> link_to_peer;

	if (!link -> outer || link -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	status = (dhcp_failover_put_message
		  (link, link -> outer,
		   FTM_POOLREQ,
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return status;
}

isc_result_t dhcp_failover_send_poolresp (dhcp_failover_state_t *state,
					  int leases)
{
	dhcp_failover_link_t *link;
	isc_result_t status;
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(poolresp");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!state -> link_to_peer ||
	    state -> link_to_peer -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)state -> link_to_peer;

	if (!link -> outer || link -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	status = (dhcp_failover_put_message
		  (link, link -> outer,
		   FTM_POOLREQ,
		   dhcp_failover_make_option (FTO_ADDRESSES_TRANSFERRED, FMA,
					      leases),
		   (failover_option_t *)0));

#if defined (DEBUG_FAILOVER_MESSAGES)
	if (status != ISC_R_SUCCESS)
		failover_print (FMA, " (failed)");
	failover_print (FMA, ")");
	if (obufix) {
		log_debug ("%s", obuf);
	}
#endif
	return status;
}

isc_result_t dhcp_failover_process_bind_update (dhcp_failover_state_t *state,
					       failover_message_t *msg)
{
	struct lease *lt, *lease;
	struct iaddr ia;
	int reason = FTR_MISC_REJECT;
	const char *message;
	int new_binding_state;

	ia.len = sizeof msg -> assigned_addr;
	memcpy (ia.iabuf, &msg -> assigned_addr, ia.len);

	lease = (struct lease *)0;
	lt = (struct lease *)0;
	if (!find_lease_by_ip_addr (&lease, ia, MDL)) {
		message = "unknown IP address";
		reason = FTR_ILLEGAL_IP_ADDR;
		goto bad;
	}

	/* XXX check for conflicts. */

	/* Install the new info. */
	if (!lease_copy (&lt, lease, MDL)) {
		message = "no memory";
		goto bad;
	}

	if (msg -> options_present & FTB_CHADDR) {
		if (msg -> chaddr.count > sizeof lt -> hardware_addr.hbuf) {
			message = "chaddr to long";
			goto bad;
		}
		lt -> hardware_addr.hlen = msg -> chaddr.count;
		memcpy (lt -> hardware_addr.hbuf, msg -> chaddr.data,
			msg -> chaddr.count);
	}		

	if (msg -> options_present & FTB_CLIENT_IDENTIFIER) {
		lt -> uid_len = msg -> client_identifier.count;
		if (lt -> uid_len > sizeof lt -> uid_buf) {
			lt -> uid_max = lt -> uid_len;
			lt -> uid = dmalloc (lt -> uid_len, MDL);
			if (!lt -> uid) {
				message = "no memory";
				goto bad;
			}
		} else {
			lt -> uid_max = sizeof lt -> uid_buf;
			lt -> uid = lt -> uid_buf;
		}
		memcpy (lt -> uid,
			msg -> client_identifier.data, lt -> uid_len);
	}
		
	/* XXX Times may need to be adjusted based on clock skew! */
	if (msg -> options_present & FTB_STOS) {
		lt -> starts = msg -> stos;
	}
	if (msg -> options_present & FTB_LEASE_EXPIRY) {
		lt -> ends = msg -> expiry;
	}
	if (msg -> options_present & FTB_CLTT) {
		lt -> cltt = msg -> client_ltt;
	}
	if (msg -> options_present & FTB_POTENTIAL_EXPIRY) {
		lt -> tsfp = msg -> potential_expiry;
	}

	if (msg -> options_present & FTB_BINDING_STATUS) {
		/* Check the requested transition to make sure it's
		   valid. */
		new_binding_state = (binding_state_transition_check
				     (lease, state, msg -> binding_status));
		if (new_binding_state != msg -> binding_status) {
			dhcp_failover_send_bind_ack
				(state, lease, msg, FTR_FATAL_CONFLICT,
				 "invalid binding state transition");
		}
		lt -> next_binding_state = new_binding_state;
	}

	/* Try to install the new information. */
	if (!supersede_lease (lease, lt, 1, 0, 0)) {
		message = "database update failed";
	      bad:
		dhcp_failover_send_bind_ack (state, lease, msg,
					     reason, message);
	} else
		dhcp_failover_send_bind_ack (state, lease, msg, 0, 0);
	if (lt)
		lease_dereference (&lt, MDL);
	if (lease)
		lease_dereference (&lease, MDL);

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_failover_process_bind_ack (dhcp_failover_state_t *state,
					     failover_message_t *msg)
{
	struct lease *lt = (struct lease *)0;
	struct lease *lease = (struct lease *)0;
	struct iaddr ia;
	const char *message = "no memory";

	ia.len = sizeof msg -> assigned_addr;
	memcpy (ia.iabuf, &msg -> assigned_addr, ia.len);

	if (!find_lease_by_ip_addr (&lease, ia, MDL)) {
		message = "no such lease";
		goto bad;
	}

	/* XXX check for conflicts. */
	if (msg -> options_present & FTB_REJECT_REASON) {
		log_error ("bind update on %s from %s rejected: %.*s",
			   piaddr (ia), state -> name,
			   (int)((msg -> options_present & FTB_MESSAGE)
				 ? msg -> message.count
				 : strlen (dhcp_failover_reject_reason_print
					   (msg -> reject_reason))),
			   (msg -> options_present & FTB_MESSAGE)
			   ? (const char *)(msg -> message.data)
			   : (dhcp_failover_reject_reason_print
			      (msg -> reject_reason)));
		goto unqueue;
	}

	/* Install the new info. */
	if (!lease_copy (&lt, lease, MDL)) {
		lease_dereference (&lease, MDL);
		goto bad;
	}

	/* XXX Times may need to be adjusted based on clock skew! */
	if (msg -> options_present & FTB_POTENTIAL_EXPIRY) {
		lt -> tsfp = msg -> potential_expiry;
	}

	/* Try to install the new information. */
	supersede_lease (lease, lt, 1, 0, 0);

      unqueue:
	dhcp_failover_ack_queue_remove (state, lease);

	/* If there are updates pending, we've created space to send at
	   least one. */
	dhcp_failover_send_updates (state);

      out:
	lease_dereference (&lease, MDL);
	if (lt)
		lease_dereference (&lt, MDL);

	return ISC_R_SUCCESS;

      bad:
	log_info ("bind update on %s from %s: %s.",
		  piaddr (ia), state -> name, message);
	goto out;
}

#if defined (DEBUG_FAILOVER_MESSAGES)
/* Print hunks of failover messages, doing line breaks as appropriate.
   Note that this assumes syslog is being used, rather than, e.g., the
   Windows NT logging facility, where just dumping the whole message in
   one hunk would be more appropriate. */

void failover_print (char *obuf,
		     unsigned *obufix, unsigned obufmax, const char *s)
{
	int len = strlen (s);

	while (len + *obufix + 1 >= obufmax) {
		log_debug ("%s", obuf);
		if (!*obufix) {
			log_debug ("%s", s);
			*obufix = 0;
			return;
		}
		*obufix = 0;
	}
	strcpy (&obuf [*obufix], s);
	*obufix += len;
}	
#endif /* defined (DEBUG_FAILOVER_MESSAGES) */

/* Taken from draft-ietf-dhc-loadb-01.txt: */
/* A "mixing table" of 256 distinct values, in pseudo-random order. */
unsigned char loadb_mx_tbl[256] = {
    251, 175, 119, 215,  81,  14,  79, 191, 103,  49,
    181, 143, 186, 157,   0, 232,  31,  32,  55,  60,
    152,  58,  17, 237, 174,  70, 160, 144, 220,  90,
    57,  223,  59,   3,  18, 140, 111, 166, 203, 196,
    134, 243, 124,  95, 222, 179, 197,  65, 180,  48,
     36,  15, 107,  46, 233, 130, 165,  30, 123, 161,
    209,  23,  97,  16,  40,  91, 219,  61, 100,  10,
    210, 109, 250, 127,  22, 138,  29, 108, 244,  67,
    207,   9, 178, 204,  74,  98, 126, 249, 167, 116,
    34,   77, 193, 200, 121,   5,  20, 113,  71,  35,
    128,  13, 182,  94,  25, 226, 227, 199,  75,  27,
     41, 245, 230, 224,  43, 225, 177,  26, 155, 150,
    212, 142, 218, 115, 241,  73,  88, 105,  39, 114,
     62, 255, 192, 201, 145, 214, 168, 158, 221, 148,
    154, 122,  12,  84,  82, 163,  44, 139, 228, 236,
    205, 242, 217,  11, 187, 146, 159,  64,  86, 239,
    195,  42, 106, 198, 118, 112, 184, 172,  87,   2,
    173, 117, 176, 229, 247, 253, 137, 185,  99, 164,
    102, 147,  45,  66, 231,  52, 141, 211, 194, 206,
    246, 238,  56, 110,  78, 248,  63, 240, 189,  93,
     92,  51,  53, 183,  19, 171,  72,  50,  33, 104,
    101,  69,   8, 252,  83, 120,  76, 135,  85,  54,
    202, 125, 188, 213,  96, 235, 136, 208, 162, 129,
    190, 132, 156,  38,  47,   1,   7, 254,  24,   4,
    216, 131,  89,  21,  28, 133,  37, 153, 149,  80,
    170,  68,   6, 169, 234, 151 };

static unsigned char loadb_p_hash (const unsigned char *, unsigned);
static unsigned char loadb_p_hash (const unsigned char *key, unsigned len)
{
        unsigned char hash = len;
        int i;
        for(i = len; i > 0;  )
		hash = loadb_mx_tbl [hash ^ (key [--i])];
        return hash;
}

int load_balance_mine (struct packet *packet, dhcp_failover_state_t *state)
{
	struct option_cache *oc;
	struct data_string ds;
	unsigned char hbaix;
	int hm;

	if (state -> load_balance_max_secs < ntohs (packet -> raw -> secs)) {
		return 1;
	}

	/* If we don't have a hash bucket array, we can't tell if this
	   one's ours, so we assume it's not. */
	if (!state -> hba)
		return 0;

	oc = lookup_option (&dhcp_universe, packet -> options,
			    DHO_DHCP_CLIENT_IDENTIFIER);
	memset (&ds, 0, sizeof ds);
	if (oc &&
	    evaluate_option_cache (&ds, packet, (struct lease *)0,
				   packet -> options, (struct option_state *)0,
				   &global_scope, oc, MDL)) {
		hbaix = loadb_p_hash (ds.data, ds.len);
	} else {
		hbaix = loadb_p_hash (packet -> raw -> chaddr,
				      packet -> raw -> hlen);
	}
	hm = (state -> hba [hbaix / 8] & (1 << (hbaix & 3)));
	if (state -> i_am == primary)
		return hm;
	else
		return !hm;
}

binding_state_t binding_state_transition_check (struct lease *lease,
						dhcp_failover_state_t *state,
						binding_state_t binding_state)
{
	/* If there is no transition, it's no problem. */
	if (binding_state == lease -> binding_state)
		return binding_state;

	/* This is really only dealing with what to do with bind updates when
	   we're in the normal state - if we were down and came back, and the
	   peer is in partner_down state, then we should take whatever it
	   sends, as long as it hasn't done anything illegal.   What about
	   when we're in potential_conflict? */
	/* Also, we should sanity check things here - the partner shouldn't
	   be allowed to set a lease to the EXPIRED state when it hasn't
	   expired, for example. */
	/* Note that tsfp had better be set from the latest bind update
	   _before_ this function is called! */
	switch (lease -> binding_state) {
	      case FTS_FREE:
	      case FTS_ABANDONED:
		switch (binding_state) {
		      case FTS_ACTIVE:
		      case FTS_ABANDONED:
		      case FTS_BACKUP:
		      case FTS_RESERVED:
		      case FTS_BOOTP:
			/* If the lease was free, and our peer is primary,
			   then it can make it active, or abandoned, or
			   backup.    Abandoned is treated like free in
			   this case. */
			if (state -> i_am == secondary)
				return binding_state;

			/* Otherwise, it can't do any sort of state
			   transition. */
		      case FTS_FREE: /* for compiler */
		      case FTS_EXPIRED:
		      case FTS_RELEASED:
		      case FTS_RESET:
			return FTS_FREE;
		}
	      case FTS_ACTIVE:
	      case FTS_RESERVED:
	      case FTS_BOOTP:
		/* The secondary can't change the state of an active
		   lease. */
		if (state -> i_am == primary)
			return FTS_ACTIVE;

		/* So this is only for transitions made by the primary: */
		switch (binding_state) {
		      case FTS_FREE:
		      case FTS_BACKUP:
			/* Can't set a lease to free or backup until the
			   peer agrees that it's expired. */
			if (lease -> tsfp > cur_time)
				return FTS_ACTIVE;
			return binding_state;

		      case FTS_EXPIRED:
			if (lease -> ends > cur_time)
				return lease -> binding_state;

		      case FTS_RESERVED:
		      case FTS_BOOTP:
		      case FTS_RELEASED:
		      case FTS_ABANDONED:
		      case FTS_RESET:
		      case FTS_ACTIVE:
			return binding_state;

		}
	      case FTS_EXPIRED:
		switch (binding_state) {
		      case FTS_FREE:
		      case FTS_BACKUP:
			/* Can't set a lease to free or backup until the
			   peer agrees that it's expired. */
			if (lease -> tsfp > cur_time)
				return FTS_ACTIVE;
			return binding_state;

		      case FTS_RESERVED:
		      case FTS_BOOTP:
		      case FTS_ACTIVE:
		      case FTS_RELEASED:
		      case FTS_ABANDONED:
		      case FTS_RESET:
		      case FTS_EXPIRED:
			return binding_state;
		}
	      case FTS_RELEASED:
		switch (binding_state) {
		      case FTS_FREE:
		      case FTS_BACKUP:
			/* Can't set a lease to free or backup until the
			   peer agrees that it's expired. */
			if (lease -> tsfp > cur_time)
				return FTS_ACTIVE;
			return binding_state;

		      case FTS_RESERVED:
		      case FTS_BOOTP:
		      case FTS_EXPIRED:
		      case FTS_ABANDONED:
		      case FTS_RESET:
		      case FTS_ACTIVE:
		      case FTS_RELEASED:
			return binding_state;
		}
	      case FTS_RESET:
		switch (binding_state) {
		      case FTS_FREE:
		      case FTS_BACKUP:
			/* Can't set a lease to free or backup until the
			   peer agrees that it's expired. */
			if (lease -> tsfp > cur_time)
				return FTS_ACTIVE;
			return binding_state;

		      case FTS_ACTIVE:
		      case FTS_RESERVED:
		      case FTS_BOOTP:
		      case FTS_EXPIRED:
		      case FTS_RELEASED:
		      case FTS_ABANDONED:
		      case FTS_RESET:
			return binding_state;
		}
	      case FTS_BACKUP:
		switch (binding_state) {
		      case FTS_ACTIVE:
		      case FTS_ABANDONED:
		      case FTS_FREE:
		      case FTS_RESERVED:
		      case FTS_BOOTP:
			/* If the lease was in backup, and our peer is
			   secondary, then it can make it active, or
			   abandoned, or free. */
			if (state -> i_am == primary)
				return binding_state;

			/* Otherwise, it can't do any sort of state
			   transition. */
		      case FTS_EXPIRED:
		      case FTS_RELEASED:
		      case FTS_RESET:
			return lease -> binding_state;

		      case FTS_BACKUP:
			return FTS_BACKUP;
		}
	}
	/*NOTREACHED*/
	return lease -> binding_state;
}

int lease_mine_to_extend (struct lease *lease)
{
	dhcp_failover_state_t *peer;

	if (lease && lease -> pool &&
	    lease -> pool -> failover_peer) {
		peer = lease -> pool -> failover_peer;
/* XXX This does't seem right - either peer can extend a lease to MCLT. */
		if (peer -> my_state == normal) { /* XXX */
			switch (lease -> binding_state) {
			      case FTS_ACTIVE:
			      case FTS_ABANDONED:
			      case FTS_RESET:
			      case FTS_RELEASED:
			      case FTS_EXPIRED:
			      case FTS_FREE:
			      case FTS_BOOTP:
			      case FTS_RESERVED:
				if (peer -> i_am == secondary)
					return 0;
				break;
			      case FTS_BACKUP:
				if (peer -> i_am == primary)
					return 0;
				break;
			}
		}
	}
	return 1;
}

OMAPI_OBJECT_ALLOC (dhcp_failover_state, dhcp_failover_state_t,
		    dhcp_type_failover_state)
OMAPI_OBJECT_ALLOC (dhcp_failover_listener, dhcp_failover_listener_t,
		    dhcp_type_failover_listener)
OMAPI_OBJECT_ALLOC (dhcp_failover_link, dhcp_failover_link_t,
		    dhcp_type_failover_link)
#endif /* defined (FAILOVER_PROTOCOL) */
