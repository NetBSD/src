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
"$Id: failover.c,v 1.1.1.1 2000/04/22 07:12:05 mellon Exp $ Copyright (c) 1999-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include "version.h"
#include <omapip/omapip_p.h>

#if defined (FAILOVER_PROTOCOL)
static struct hash_table *failover_hash;
static dhcp_failover_state_t *failover_states;
static isc_result_t do_a_failover_option (omapi_object_t *,
					  dhcp_failover_link_t *);

void enter_failover_peer (peer)
	dhcp_failover_state_t *peer;
{
}

isc_result_t find_failover_peer (peer, name)
	dhcp_failover_state_t **peer;
	char *name;
{
	dhcp_failover_state_t *p;

	for (p = failover_states; p; p = p -> next)
		if (!strcmp (name, p -> name))
			break;
	if (p)
		return omapi_object_reference ((omapi_object_t **)peer,
					       (omapi_object_t *)p, MDL);
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
	unsigned long port;
	omapi_value_t *value = (omapi_value_t *)0;
	dhcp_failover_state_t *state;
	omapi_object_t *o;
	int i;
	struct data_string ds;

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

	obj = (dhcp_failover_link_t *)dmalloc (sizeof *obj, MDL);
	if (!obj)
		return ISC_R_NOMEMORY;
	memset (obj, 0, sizeof *obj);
	obj -> refcnt = 1;
	obj -> type = dhcp_type_failover_link;
	option_cache_reference (&obj -> peer_address, state -> address, MDL);
	obj -> peer_port = state -> port;

	memset (&ds, 0, sizeof ds);
	if (!evaluate_option_cache (&ds, (struct packet *)0, (struct lease *)0,
				    (struct option_state *)0,
				    (struct option_state *)0,
				    &global_scope, obj -> peer_address, MDL)) {
		option_cache_dereference (&obj -> peer_address, MDL);
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return ISC_R_UNEXPECTED;
	}

	/* Kludge around the fact that omapi_connect expects an ASCII string
	   representing the address to which to connect. */
	status = ISC_R_INVALIDARG;
	for (i = 0; i + 4 <= ds.len; i += 4) {
		char peer_name [40];
		sprintf (peer_name, "%d.%d.%d.%d",
			 ds.data [i], ds.data [i + 1],
			 ds.data [i + 2], ds.data [i + 3]);
		status = omapi_connect ((omapi_object_t *)obj,
					peer_name, port);
		if (status == ISC_R_SUCCESS)
			break;
	}
	data_string_forget (&ds, MDL);

	/* If we didn't connect to any of the addresses, give up. */
	if (status != ISC_R_SUCCESS) {
	      lose:
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		option_cache_dereference (&obj -> peer_address, MDL);
		return status;
	}
	status = omapi_object_reference (&h -> outer,
					 (omapi_object_t *)obj, MDL);
	if (status != ISC_R_SUCCESS)
		goto lose;
	status = omapi_object_reference (&obj -> inner, h, MDL);
	if (status != ISC_R_SUCCESS)
		goto lose;

	/* Send the introductory message. */
	status = dhcp_failover_send_connect ((omapi_object_t *)obj);
	if (status != ISC_R_SUCCESS)
		goto lose;

	omapi_object_dereference ((omapi_object_t **)&obj, MDL);
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_failover_link_signal (omapi_object_t *h,
					const char *name, va_list ap)
{
	isc_result_t status;
	dhcp_failover_link_t *link;
	omapi_object_t *c;
	u_int16_t nlen;
	u_int32_t vlen;

	if (h -> type != dhcp_type_failover_link) {
		/* XXX shouldn't happen.   Put an assert here? */
		return ISC_R_UNEXPECTED;
	}
	link = (dhcp_failover_link_t *)h;

	/* Not a signal we recognize? */
	if (strcmp (name, "ready")) {
		if (h -> inner && h -> inner -> type -> signal_handler)
			return (*(h -> inner -> type -> signal_handler)) (h,
									  name,
									  ap);
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
		memset (link -> imsg, 0, sizeof (link -> imsg));
		/* Get the length: */
		omapi_connection_get_uint16 (c, &link -> imsg_len);
		link -> imsg_count = 0;	/* Bytes read. */
		
		/* Maximum of 2048 bytes in any failover message. */
		if (link -> imsg_len > DHCP_FAILOVER_MAX_MESSAGE_SIZE)
			goto dhcp_flink_fail;

		if ((omapi_connection_require (c, link -> imsg_len)) !=
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
				
		/* Get transaction ID. */
		omapi_connection_get_uint32 (c, &link -> imsg -> xid);
		link -> imsg_count += 4;

		/* Now start sucking options off the wire. */
		while (link -> imsg_count < link -> imsg_len) {
			if (do_a_failover_option (c, link) != ISC_R_SUCCESS)
				goto dhcp_flink_fail;
		}

		/* Once we have the entire message, and we've validated
		   it as best we can here, pass it to the parent. */
		omapi_signal_in (h -> outer, "message", link);
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
	
	if (link -> imsg_count + 2 > link -> imsg_len) {
		log_error ("FAILOVER: message overflow at option code.");
		return ISC_R_PROTOCOLERROR;
	}

	/* Get option code. */
	omapi_connection_get_uint16 (c, &option_code);
	link -> imsg_count += 2;
	
	/* Get option length. */
	omapi_connection_get_uint16 (c, &option_len);
	link -> imsg_count += 2;

	if (link -> imsg_count + option_len > link -> imsg_len) {
		log_error ("FAILOVER: message overflow at %s",
			   " length.");
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
	    !(fto_allowed [option_code] & ft_options [option_code].bit)) {
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

		op = ((unsigned char *)&link -> imsg) +
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
				 (((char *)&link -> imsg) +
				  ft_options [option_code].offset));

			op_count = (ft_options [option_code].type == FT_DDNS1
				    ? 1 : 2);

			omapi_connection_copyout (&ddns -> codes [0],
						  c, op_count);
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
		      (((char *)&link -> imsg) +
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
		goto out;
	}

	/* For values that require swapping, read them in one at a time
	   using routines that swap bytes. */
	for (i = 0; i < op_count; i++) {
		switch (ft_options [option_code].type) {
		      case FT_UINT32:
			omapi_connection_get_uint32 (c, (u_int32_t *)op);
			op += 4;
			break;
			
		      case FT_UINT16:
			omapi_connection_get_uint16 (c, (u_int16_t *)op);
			op += 2;
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

	obj = (dhcp_failover_listener_t *)dmalloc (sizeof *obj, MDL);
	if (!obj)
		return ISC_R_NOMEMORY;
	memset (obj, 0, sizeof *obj);
	obj -> refcnt = 1;
	obj -> type = dhcp_type_failover_listener;
	obj -> local_port = port;
	
	status = omapi_listen ((omapi_object_t *)obj, port, 1);
	omapi_object_dereference ((omapi_object_t **)&obj, MDL);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_object_reference (&h -> outer,
					 (omapi_object_t *)obj, MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return status;
	}
	status = omapi_object_reference (&obj -> inner, h, MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return status;
	}

	return status;
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
	dhcp_failover_state_t *state;

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

	/* See if we can find a secondary failover_state object that
	   matches this connection. */
	for (state = failover_states; state; state = state -> next) {
		int i;
		struct data_string ds;

		memset (&ds, 0, sizeof ds);
		if (evaluate_option_cache (&ds, (struct packet *)0,
					   (struct lease *)0,
					   (struct option_state *)0,
					   (struct option_state *)0,
					   &global_scope,
					   state -> address, MDL)) {
			for (i = 0; i < ds.len; i += 4) {
				if (!memcmp (&ds.data [i],
					     &c -> remote_addr, 4))
					break;
			}
		}
	}		

	/* If we can't find a failover protocol state for this remote
	   host, drop the connection */
	if (!state) {
		/* XXX Send a refusal message first?
		   XXX Look in protocol spec for guidance. */
		/* XXX An error message from a connect signal should
		   XXX drop the connection - make sure this is what
		   XXX actually happens! */
		return ISC_R_INVALIDARG;
	}

	obj = (dhcp_failover_link_t *)dmalloc (sizeof *obj, MDL);
	if (!obj)
		return ISC_R_NOMEMORY;
	memset (obj, 0, sizeof *obj);
	obj -> refcnt = 1;
	obj -> type = dhcp_type_failover_link;
	option_cache_reference (&obj -> peer_address, state -> address, MDL);
	obj -> peer_port = ntohs (c -> remote_addr.sin_port);

	status = omapi_object_reference (&obj -> outer,
					 (omapi_object_t *)c, MDL);
	if (status != ISC_R_SUCCESS) {
	      lose:
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		omapi_disconnect ((omapi_object_t *)c, 1);
		return status;
	}

	status = omapi_object_reference (&c -> inner,
					 (omapi_object_t *)obj, MDL);
	if (status != ISC_R_SUCCESS)
		goto lose;

	/* Notify the master state machine of the arrival of a new
           connection. */
	status = omapi_signal_in ((omapi_object_t *)state, "connect", obj);
	if (status != ISC_R_SUCCESS)
		goto lose;

	omapi_object_dereference ((omapi_object_t **)&obj, MDL);
	return status;
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

	obj = (dhcp_failover_state_t *)dmalloc (sizeof *obj, MDL);
	if (!obj)
		return ISC_R_NOMEMORY;
	memset (obj, 0, sizeof *obj);
	obj -> refcnt = 1;
	obj -> type = dhcp_type_failover_state;
	obj -> listen_port = port;
	
	status = omapi_listen ((omapi_object_t *)obj, port, 1);
	omapi_object_dereference ((omapi_object_t **)&obj, MDL);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_object_reference (&h -> outer, (omapi_object_t *)obj,
					 MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return status;
	}
	status = omapi_object_reference (&obj -> inner, h, MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&obj, MDL);
		return status;
	}

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
	char *peer_name;

	if (!o || o -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	state = (dhcp_failover_state_t *)o;

	/* Not a signal we recognize? */
	if (strcmp (name, "connect") &&
	    strcmp (name, "disconnect") &&
	    strcmp (name, "message")) {
		if (state -> inner && state -> inner -> type -> signal_handler)
			return (*(state -> inner -> type -> signal_handler))
				(state -> inner, name, ap);
		return ISC_R_NOTFOUND;
	}

	/* Handle all the events we care about... */
	return ISC_R_SUCCESS;
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

isc_result_t dhcp_failover_state_get_value (omapi_object_t *h,
						omapi_object_t *id,
						omapi_data_string_t *name,
						omapi_value_t **value)
{
	if (h -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_failover_state_destroy (omapi_object_t *h,
					      const char *file, int line)
{
	if (h -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;
	return ISC_R_SUCCESS;
}

/* Write all the published values associated with the object through the
   specified connection. */

isc_result_t dhcp_failover_state_stuff (omapi_object_t *c,
					omapi_object_t *id,
					omapi_object_t *p)
{
	int i;

	if (p -> type != dhcp_type_failover_state)
		return ISC_R_INVALIDARG;

	if (p -> inner && p -> inner -> type -> stuff_values)
		return (*(p -> inner -> type -> stuff_values)) (c, id,
								p -> inner);
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

failover_option_t *dhcp_failover_make_option (unsigned code,
					      char *obuf, unsigned *obufix,
					      unsigned obufmax, ...)
{
	va_list va;
	struct failover_option_info *info;
	int i;
	unsigned size, count;
	unsigned val;
	struct iaddr *ival;
	u_int8_t *bval;
	char *fmt;
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
			fmt = va_arg (va, char *);
#if defined (HAVE_SNPRINTF)
			/* Presumably if we have snprintf, we also have
                           vsnprintf. */
			vsnprintf (tbuf, sizeof tbuf, fmt, va);
#else
			vsprintf (tbuf, fmt, va);
#endif
			size = strlen (tbuf);
			count = size;
			break;

		      case FT_IPADDR:
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
	putUShort (&option.data [4], size - 4);

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
			ival = va_arg (va, struct iaddr *);
			if (ival -> len != 4) {
				dfree (option.data, MDL);
				log_error ("IP addrlen=%d, should be 4.",
					   ival -> len);
				return &null_failover_option;
			}
				
#if defined (DEBUG_FAILOVER_MESSAGES)
			sprintf (tbuf, " %s", piaddr (*ival));
			failover_print (obuf, obufix, obufmax, tbuf);
#endif
			memcpy (&option.data [4 + i * 4], ival -> iabuf, 4);
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
		failover_print (obuf, obufix, obufmax, " \"");
		failover_print (obuf, obufix, obufmax, tbuf);
		failover_print (obuf, obufix, obufmax, "\"");
#endif
		memcpy (&option.data [4], tbuf, count);
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
		opix += option -> count;
		dfree (option -> data, MDL);
		dfree (option, MDL);
	}

	if (bad_option || !opbuf)
		return status;

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
	return status;

      err:
	dfree (opbuf, MDL);
	omapi_disconnect (connection, 1);
	return status;
}	

/* Send a connect message. */

isc_result_t dhcp_failover_send_connect (omapi_object_t *l)
{
	dhcp_failover_link_t *link;
	dhcp_failover_state_t *state;
	isc_result_t status;
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

	status = (dhcp_failover_put_message
		  (link, l -> outer,
		   FTM_CONNECT,
		   dhcp_failover_make_option (FTO_SERVER_ADDR, FMA,
					      &state -> server_addr),
		   dhcp_failover_make_option (FTO_MAX_UNACKED, FMA,
					      state -> max_flying_updates),
		   dhcp_failover_make_option (FTO_RECEIVE_TIMER, FMA,
					      state -> max_response_delay),
		   dhcp_failover_make_option (FTO_VENDOR_CLASS, FMA,
					      "isc-%s", DHCP_VERSION),
		   dhcp_failover_make_option (FTO_PROTOCOL_VERSION, FMA,
					      DHCP_FAILOVER_VERSION),
		   dhcp_failover_make_option (FTO_TLS_REQUEST, FMA,
					      0, 0),
		   dhcp_failover_make_option (FTO_MCLT, FMA,
					      state -> mclt),
		   dhcp_failover_make_option (FTO_HBA, FMA,
					      state -> hba),
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

isc_result_t dhcp_failover_update_peer (struct lease *lease, int releasep)
{
	dhcp_failover_link_t *link;
	dhcp_failover_state_t *state;
	isc_result_t status;
#if defined (DEBUG_FAILOVER_MESSAGES)	
	char obuf [64];
	unsigned obufix = 0;
	
# define FMA obuf, &obufix, sizeof obuf
	failover_print (FMA, "(bndupd");
#else
# define FMA (unsigned char *)0, (unsigned *)0, 0
#endif

	if (!lease -> pool ||
	    !lease -> pool -> failover_peer ||
	    !lease -> pool -> failover_peer -> outer)
		return ISC_R_INVALIDARG;

	state = lease -> pool -> failover_peer;
	if (state -> outer -> type != dhcp_type_failover_link)
		return ISC_R_INVALIDARG;
	link = (dhcp_failover_link_t *)state -> outer;

	if (!link -> outer || link -> outer -> type != omapi_type_connection)
		return ISC_R_INVALIDARG;

	status = (dhcp_failover_put_message
		  (link, link -> outer,
		   FTM_BNDUPD,
		   dhcp_failover_make_option (FTO_ASSIGNED_IP_ADDRESS, FMA,
					      lease -> ip_addr.iabuf),
		   dhcp_failover_make_option (FTO_BINDING_STATUS, FMA,
					      0 /* ??? */),
		   lease -> uid_len
		   ? dhcp_failover_make_option (FTO_CLIENT_IDENTIFIER, FMA,
						lease -> uid,
						lease -> uid_len)
		   : &skip_failover_option,
		   dhcp_failover_make_option (FTO_CHADDR, FMA,
					      lease -> hardware_addr.hlen + 1,
					      lease -> hardware_addr.hbuf),
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

#if defined (DEBUG_FAILOVER_MESSAGES)
/* Print hunks of failover messages, doing line breaks as appropriate.
   Note that this assumes syslog is being used, rather than, e.g., the
   Windows NT logging facility, where just dumping the whole message in
   one hunk would be more appropriate. */

void failover_print (char *obuf,
		     unsigned *obufix, unsigned obufmax, const char *s)
{
	int len = strlen (s);

	if (len + *obufix + 1 >= obufmax) {
		if (!*obufix) {
			log_debug ("%s", s);
			return;
		}
		log_debug ("%s", obuf);
		*obufix = 0;
	} else {
		strcpy (&obuf [*obufix], s);
		obufix += len;
	}
}	
#endif /* defined (DEBUG_FAILOVER_MESSAGES) */

void update_partner (struct lease *lease)
{
}
#endif /* defined (FAILOVER_PROTOCOL) */
