/* omapi.c

   OMAPI object interfaces for the DHCP server. */

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

/* Many, many thanks to Brian Murrell and BCtel for this code - BCtel
   provided the funding that resulted in this code and the entire
   OMAPI support library being written, and Brian helped brainstorm
   and refine the requirements.  To the extent that this code is
   useful, you have Brian and BCtel to thank.  Any limitations in the
   code are a result of mistakes on my part.  -- Ted Lemon */

#ifndef lint
static char copyright[] =
"$Id: omapi.c,v 1.3 2000/06/24 06:50:04 mellon Exp $ Copyright (c) 1999-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include <omapip/omapip_p.h>

omapi_object_type_t *dhcp_type_lease;
omapi_object_type_t *dhcp_type_pool;
omapi_object_type_t *dhcp_type_class;
omapi_object_type_t *dhcp_type_host;
#if defined (FAILOVER_PROTOCOL)
omapi_object_type_t *dhcp_type_failover_state;
omapi_object_type_t *dhcp_type_failover_link;
omapi_object_type_t *dhcp_type_failover_listener;
#endif

void dhcp_db_objects_setup ()
{
	isc_result_t status;

	status = omapi_object_type_register (&dhcp_type_lease,
					     "lease",
					     dhcp_lease_set_value,
					     dhcp_lease_get_value,
					     dhcp_lease_destroy,
					     dhcp_lease_signal_handler,
					     dhcp_lease_stuff_values,
					     dhcp_lease_lookup, 
					     dhcp_lease_create,
					     dhcp_lease_remove,
#if defined (COMPACT_LEASES)
					     dhcp_lease_free,
#else
					     0,
#endif
					     0,
					     sizeof (struct lease));
	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register lease object type: %s",
			   isc_result_totext (status));

	status = omapi_object_type_register (&dhcp_type_class,
					     "class",
					     dhcp_class_set_value,
					     dhcp_class_get_value,
					     dhcp_class_destroy,
					     dhcp_class_signal_handler,
					     dhcp_class_stuff_values,
					     dhcp_class_lookup, 
					     dhcp_class_create,
					     dhcp_class_remove, 0, 0,
					     sizeof (struct class));
	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register class object type: %s",
			   isc_result_totext (status));

	status = omapi_object_type_register (&dhcp_type_pool,
					     "pool",
					     dhcp_pool_set_value,
					     dhcp_pool_get_value,
					     dhcp_pool_destroy,
					     dhcp_pool_signal_handler,
					     dhcp_pool_stuff_values,
					     dhcp_pool_lookup, 
					     dhcp_pool_create,
					     dhcp_pool_remove, 0, 0,
					     sizeof (struct pool));

	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register pool object type: %s",
			   isc_result_totext (status));

	status = omapi_object_type_register (&dhcp_type_host,
					     "host",
					     dhcp_host_set_value,
					     dhcp_host_get_value,
					     dhcp_host_destroy,
					     dhcp_host_signal_handler,
					     dhcp_host_stuff_values,
					     dhcp_host_lookup, 
					     dhcp_host_create,
					     dhcp_host_remove, 0, 0,
					     sizeof (struct host_decl));

	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register host object type: %s",
			   isc_result_totext (status));

#if defined (FAILOVER_PROTOCOL)
	status = omapi_object_type_register (&dhcp_type_failover_state,
					     "failover-state",
					     dhcp_failover_state_set_value,
					     dhcp_failover_state_get_value,
					     dhcp_failover_state_destroy,
					     dhcp_failover_state_signal,
					     dhcp_failover_state_stuff,
					     dhcp_failover_state_lookup, 
					     dhcp_failover_state_create,
					     dhcp_failover_state_remove,
					     0, 0,
					     sizeof (dhcp_failover_state_t));

	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register failover state object type: %s",
			   isc_result_totext (status));

	status = omapi_object_type_register (&dhcp_type_failover_link,
					     "failover-link",
					     dhcp_failover_link_set_value,
					     dhcp_failover_link_get_value,
					     dhcp_failover_link_destroy,
					     dhcp_failover_link_signal,
					     dhcp_failover_link_stuff_values,
					     0, 0, 0, 0, 0,
					     sizeof (dhcp_failover_link_t));

	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register failover link object type: %s",
			   isc_result_totext (status));

	status = omapi_object_type_register (&dhcp_type_failover_listener,
					     "failover-listener",
					     dhcp_failover_listener_set_value,
					     dhcp_failover_listener_get_value,
					     dhcp_failover_listener_destroy,
					     dhcp_failover_listener_signal,
					     dhcp_failover_listener_stuff,
					     0, 0, 0, 0, 0,
					     sizeof
					     (dhcp_failover_listener_t));

	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register failover listener object type: %s",
			   isc_result_totext (status));
#endif /* FAILOVER_PROTOCOL */
}

isc_result_t dhcp_lease_set_value  (omapi_object_t *h,
				    omapi_object_t *id,
				    omapi_data_string_t *name,
				    omapi_typed_data_t *value)
{
	struct lease *lease;
	isc_result_t status;
	int foo;

	if (h -> type != dhcp_type_lease)
		return ISC_R_INVALIDARG;
	lease = (struct lease *)h;

	/* We're skipping a lot of things it might be interesting to
	   set - for now, we just make it possible to whack the state. */
	if (!omapi_ds_strcmp (name, "state")) {
		unsigned long bar;
		status = omapi_get_int_value (&bar, value);
		if (status != ISC_R_SUCCESS)
			return status;

		if (bar < 1 || bar > FTS_BOOTP)
			return ISC_R_INVALIDARG;
		if (lease -> binding_state != bar) {
			lease -> next_binding_state = bar;
			if (supersede_lease (lease, 0, 1, 1, 1))
				return ISC_R_SUCCESS;
			return ISC_R_IOERROR;
		}
		return ISC_R_UNCHANGED;
	}

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> set_value) {
		status = ((*(h -> inner -> type -> set_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS || status == ISC_R_UNCHANGED)
			return status;
	}
			  
	return ISC_R_NOTFOUND;
}


isc_result_t dhcp_lease_get_value (omapi_object_t *h, omapi_object_t *id,
				   omapi_data_string_t *name,
				   omapi_value_t **value)
{
	struct lease *lease;
	isc_result_t status;

	if (h -> type != dhcp_type_lease)
		return ISC_R_INVALIDARG;
	lease = (struct lease *)h;

	if (!omapi_ds_strcmp (name, "state"))
		return omapi_make_int_value (value, name,
					     (int)lease -> binding_state, MDL);
	else if (!omapi_ds_strcmp (name, "ip-address"))
		return omapi_make_const_value (value, name,
					       lease -> ip_addr.iabuf,
					       lease -> ip_addr.len, MDL);
	else if (!omapi_ds_strcmp (name, "dhcp-client-identifier")) {
		return omapi_make_const_value (value, name,
					       lease -> uid,
					       lease -> uid_len, MDL);
	} else if (!omapi_ds_strcmp (name, "hostname")) {
		if (lease -> hostname)
			return omapi_make_string_value
				(value, name, lease -> hostname, MDL);
		return ISC_R_NOTFOUND;
	} else if (!omapi_ds_strcmp (name, "client-hostname")) {
		if (lease -> client_hostname)
			return omapi_make_string_value
				(value, name, lease -> client_hostname, MDL);
		return ISC_R_NOTFOUND;
	} else if (!omapi_ds_strcmp (name, "host")) {
		if (lease -> host)
			return omapi_make_handle_value
				(value, name,
				 ((omapi_object_t *)lease -> host), MDL);
	} else if (!omapi_ds_strcmp (name, "subnet"))
		return omapi_make_handle_value (value, name,
						((omapi_object_t *)
						 lease -> subnet), MDL);
	else if (!omapi_ds_strcmp (name, "pool"))
		return omapi_make_handle_value (value, name,
						((omapi_object_t *)
						 lease -> pool), MDL);
	else if (!omapi_ds_strcmp (name, "billing-class")) {
		if (lease -> billing_class)
			return omapi_make_handle_value
				(value, name,
				 ((omapi_object_t *)lease -> billing_class),
				 MDL);
		return ISC_R_NOTFOUND;
	} else if (!omapi_ds_strcmp (name, "hardware-address"))
		return omapi_make_const_value
			(value, name, &lease -> hardware_addr.hbuf [1],
			 (unsigned)(lease -> hardware_addr.hlen - 1), MDL);
	else if (!omapi_ds_strcmp (name, "hardware-type"))
		return omapi_make_int_value (value, name,
					     lease -> hardware_addr.hbuf [0],
					     MDL);

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> get_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_lease_destroy (omapi_object_t *h, const char *file, int line)
{
	struct lease *lease;
	isc_result_t status;

	if (h -> type != dhcp_type_lease)
		return ISC_R_INVALIDARG;
	lease = (struct lease *)h;

	uid_hash_delete (lease);
	hw_hash_delete (lease);
	if (lease -> billing_class)
		class_dereference
			(&lease -> billing_class, file, line);
	if (lease -> uid && lease -> uid != &lease -> uid_buf [0]) {
		dfree (lease -> uid, MDL);
		lease -> uid = &lease -> uid_buf [0];
		lease -> uid_len = 0;
	}
	if (lease -> hostname) {
		dfree (lease -> hostname, MDL);
		lease -> hostname = (char *)0;
	}
	if (lease -> client_hostname) {
		dfree (lease -> client_hostname, MDL);
		lease -> hostname = (char *)0;
	}
	if (lease -> host)
		host_dereference (&lease -> host, file, line);
	if (lease -> subnet)
		subnet_dereference (&lease -> subnet, file, line);
	if (lease -> pool)
		pool_dereference (&lease -> pool, file, line);
	if (lease -> on_expiry)
		executable_statement_dereference (&lease -> on_expiry,
						  file, line);
	if (lease -> on_commit)
		executable_statement_dereference (&lease -> on_commit,
						  file, line);
	if (lease -> on_release)
		executable_statement_dereference (&lease -> on_release,
						  file, line);
	if (lease -> state) {
		data_string_forget (&lease -> state -> parameter_request_list,
				    file, line);
		free_lease_state (lease -> state, file, line);
		lease -> state = (struct lease_state *)0;

		cancel_timeout (lease_ping_timeout, lease);
		--outstanding_pings; /* XXX */
	}
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_lease_signal_handler (omapi_object_t *h,
					const char *name, va_list ap)
{
	struct lease *lease;
	isc_result_t status;
	int updatep = 0;

	if (h -> type != dhcp_type_lease)
		return ISC_R_INVALIDARG;
	lease = (struct lease *)h;

	if (!strcmp (name, "updated")) {
		if (lease -> hardware_addr.hlen == 0 ||
		    lease -> hardware_addr.hlen > 17)
			return ISC_R_INVALIDARG;
		if (!write_lease (lease) || !commit_leases ()
#if defined (FAILOVER_PROTOCOL)
		    || !dhcp_failover_queue_update (lease, 1)
#endif
			) {
			return ISC_R_IOERROR;
		}
		updatep = 1;
	}

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> signal_handler))
			  (h -> inner, name, ap));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	if (updatep)
		return ISC_R_SUCCESS;
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_lease_stuff_values (omapi_object_t *c,
				      omapi_object_t *id,
				      omapi_object_t *h)
{
	struct lease *lease;
	isc_result_t status;

	if (h -> type != dhcp_type_lease)
		return ISC_R_INVALIDARG;
	lease = (struct lease *)h;

	/* Write out all the values. */

	status = omapi_connection_put_name (c, "state");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (int));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, lease -> binding_state);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "ip-address");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, lease -> ip_addr.len);
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_copyin (c, lease -> ip_addr.iabuf,
					  lease -> ip_addr.len);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "dhcp-client-identifier");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, lease -> uid_len);
	if (status != ISC_R_SUCCESS)
		return status;
	if (lease -> uid_len) {
		status = omapi_connection_copyin (c, lease -> uid,
						  lease -> uid_len);
		if (status != ISC_R_SUCCESS)
			return status;
	}

	status = omapi_connection_put_name (c, "hostname");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_string (c, lease -> hostname);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "client-hostname");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_string (c, lease -> client_hostname);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "host");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_handle (c,
					      (omapi_object_t *)lease -> host);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "subnet");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_handle
		(c, (omapi_object_t *)lease -> subnet);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "pool");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_handle (c,
					      (omapi_object_t *)lease -> pool);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "billing-class");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_handle
		(c, (omapi_object_t *)lease -> billing_class);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "hardware-address");
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_put_uint32
		  (c, (unsigned long)(lease -> hardware_addr.hlen - 1)));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_copyin
		  (c, &lease -> hardware_addr.hbuf [1],
		   (unsigned long)(lease -> hardware_addr.hlen - 1)));
	
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "hardware-type");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (int));
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c,
					      lease -> hardware_addr.hbuf [0]);
	if (status != ISC_R_SUCCESS)
		return status;


	status = omapi_connection_put_name (c, "ends");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (TIME));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_copyin
		  (c, (const unsigned char *)&(lease -> ends), sizeof(TIME)));
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_connection_put_name (c, "starts");
	if (status != ISC_R_SUCCESS)
		return status;
	status = omapi_connection_put_uint32 (c, sizeof (TIME));
	if (status != ISC_R_SUCCESS)
		return status;
	status = (omapi_connection_copyin
		  (c,
		   (const unsigned char *)&(lease -> starts), sizeof (TIME)));
	if (status != ISC_R_SUCCESS)
		return status;

	/* Write out the inner object, if any. */
	if (h -> inner && h -> inner -> type -> stuff_values) {
		status = ((*(h -> inner -> type -> stuff_values))
			  (c, id, h -> inner));
		if (status == ISC_R_SUCCESS)
			return status;
	}

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_lease_lookup (omapi_object_t **lp,
				omapi_object_t *id, omapi_object_t *ref)
{
	omapi_value_t *tv = (omapi_value_t *)0;
	isc_result_t status;
	struct lease *lease;

	/* First see if we were sent a handle. */
	status = omapi_get_value_str (ref, id, "handle", &tv);
	if (status == ISC_R_SUCCESS) {
		status = omapi_handle_td_lookup (lp, tv -> value);

		omapi_value_dereference (&tv, MDL);
		if (status != ISC_R_SUCCESS)
			return status;

		/* Don't return the object if the type is wrong. */
		if ((*lp) -> type != dhcp_type_lease) {
			omapi_object_dereference (lp, MDL);
			return ISC_R_INVALIDARG;
		}
	}

	/* Now look for an IP address. */
	status = omapi_get_value_str (ref, id, "ip-address", &tv);
	if (status == ISC_R_SUCCESS) {
		lease = (struct lease *)0;
		lease_hash_lookup (&lease, lease_ip_addr_hash,
				   tv -> value -> u.buffer.value,
				   tv -> value -> u.buffer.len, MDL);

		omapi_value_dereference (&tv, MDL);

		/* If we already have a lease, and it's not the same one,
		   then the query was invalid. */
		if (*lp && *lp != (omapi_object_t *)lease) {
			omapi_object_dereference (lp, MDL);
			lease_dereference (&lease, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!lease) {
			if (*lp)
				omapi_object_dereference (lp, MDL);
			return ISC_R_NOTFOUND;
		} else if (!*lp) {
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (lp,
						(omapi_object_t *)lease, MDL);
			lease_dereference (&lease, MDL);
		}
	}

	/* Now look for a client identifier. */
	status = omapi_get_value_str (ref, id, "dhcp-client-identifier", &tv);
	if (status == ISC_R_SUCCESS) {
		lease = (struct lease *)0;
		lease_hash_lookup (&lease, lease_uid_hash,
				   tv -> value -> u.buffer.value,
				   tv -> value -> u.buffer.len, MDL);
		omapi_value_dereference (&tv, MDL);
			
		if (*lp && *lp != (omapi_object_t *)lease) {
			omapi_object_dereference (lp, MDL);
			lease_dereference (&lease, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!lease) {
			if (*lp)
			    omapi_object_dereference (lp, MDL);
			return ISC_R_NOTFOUND;
		} else if (lease -> n_uid) {
			if (*lp)
			    omapi_object_dereference (lp, MDL);
			return ISC_R_MULTIPLE;
		} else if (!*lp) {
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (lp,
						(omapi_object_t *)lease, MDL);
			lease_dereference (&lease, MDL);
		}
	}

	/* Now look for a hardware address. */
	status = omapi_get_value_str (ref, id, "hardware-address", &tv);
	if (status == ISC_R_SUCCESS) {
		lease = (struct lease *)0;
		lease_hash_lookup (&lease, lease_hw_addr_hash,
				   tv -> value -> u.buffer.value,
				   tv -> value -> u.buffer.len, MDL);
		omapi_value_dereference (&tv, MDL);
			
		if (*lp && *lp != (omapi_object_t *)lease) {
			omapi_object_dereference (lp, MDL);
			lease_dereference (&lease, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!lease) {
			if (*lp)
			    omapi_object_dereference (lp, MDL);
			return ISC_R_NOTFOUND;
		} else if (lease -> n_hw) {
			if (*lp)
			    omapi_object_dereference (lp, MDL);
			lease_dereference (&lease, MDL);
			return ISC_R_MULTIPLE;
		} else if (!*lp) {
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (lp,
						(omapi_object_t *)lease, MDL);
			lease_dereference (&lease, MDL);
		}
	}

	/* If we get to here without finding a lease, no valid key was
	   specified. */
	if (!*lp)
		return ISC_R_NOKEYS;
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_lease_create (omapi_object_t **lp,
				omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dhcp_lease_remove (omapi_object_t *lp,
				omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dhcp_host_set_value  (omapi_object_t *h,
				   omapi_object_t *id,
				   omapi_data_string_t *name,
				   omapi_typed_data_t *value)
{
	struct host_decl *host;
	isc_result_t status;
	int foo;

	if (h -> type != dhcp_type_host)
		return ISC_R_INVALIDARG;
	host = (struct host_decl *)h;

	/* XXX For now, we can only set these values on new host objects. 
	   XXX Soon, we need to be able to update host objects. */
	if (!omapi_ds_strcmp (name, "name")) {
		if (host -> name)
			return ISC_R_EXISTS;
		if (value -> type == omapi_datatype_data ||
		    value -> type == omapi_datatype_string) {
			host -> name = dmalloc (value -> u.buffer.len + 1,
						MDL);
			if (!host -> name)
				return ISC_R_NOMEMORY;
			memcpy (host -> name,
				value -> u.buffer.value,
				value -> u.buffer.len);
			host -> name [value -> u.buffer.len] = 0;
		} else
			return ISC_R_INVALIDARG;
		return ISC_R_SUCCESS;
	}

	if (!omapi_ds_strcmp (name, "group")) {
		if (value -> type == omapi_datatype_data ||
		    value -> type == omapi_datatype_string) {
			struct group_object *group;
			group = (struct group_object *)0;
			group_hash_lookup (&group, group_name_hash,
					   (char *)value -> u.buffer.value,
					   value -> u.buffer.len, MDL);
			if (!group || (group -> flags & GROUP_OBJECT_DELETED))
				return ISC_R_NOTFOUND;
			group_reference (&host -> group, group -> group, MDL);
			if (host -> named_group)
				group_object_dereference (&host -> named_group,
							  MDL);
			group_object_reference (&host -> named_group,
						group, MDL);
			group_object_dereference (&group, MDL);
		} else
			return ISC_R_INVALIDARG;
		return ISC_R_SUCCESS;
	}

	if (!omapi_ds_strcmp (name, "hardware-address")) {
		if (host -> interface.hlen)
			return ISC_R_EXISTS;
		if (value -> type == omapi_datatype_data ||
		    value -> type == omapi_datatype_string) {
			if (value -> u.buffer.len >
			    (sizeof host -> interface.hbuf) - 1)
				return ISC_R_INVALIDARG;
			memcpy (&host -> interface.hbuf [1],
				value -> u.buffer.value,
				value -> u.buffer.len);
			host -> interface.hlen = value -> u.buffer.len + 1;
		} else
			return ISC_R_INVALIDARG;
		return ISC_R_SUCCESS;
	}

	if (!omapi_ds_strcmp (name, "hardware-type")) {
		int type;
		if (value -> type == omapi_datatype_data &&
		    value -> u.buffer.len == sizeof type) {
			if (value -> u.buffer.len > sizeof type)
				return ISC_R_INVALIDARG;
			memcpy (&type,
				value -> u.buffer.value,
				value -> u.buffer.len);
		} else if (value -> type == omapi_datatype_int)
			type = value -> u.integer;
		else
			return ISC_R_INVALIDARG;
		host -> interface.hbuf [0] = type;
		return ISC_R_SUCCESS;
	}

	if (!omapi_ds_strcmp (name, "dhcp-client-identifier")) {
		if (host -> client_identifier.data)
			return ISC_R_EXISTS;
		if (value -> type == omapi_datatype_data ||
		    value -> type == omapi_datatype_string) {
		    if (!buffer_allocate (&host -> client_identifier.buffer,
					  value -> u.buffer.len, MDL))
			    return ISC_R_NOMEMORY;
		    host -> client_identifier.data =
			    &host -> client_identifier.buffer -> data [0];
		    memcpy (host -> client_identifier.buffer -> data,
			    value -> u.buffer.value,
			    value -> u.buffer.len);
		    host -> client_identifier.len = value -> u.buffer.len;
		} else
		    return ISC_R_INVALIDARG;
		return ISC_R_SUCCESS;
	}

	if (!omapi_ds_strcmp (name, "ip-address")) {
		if (host -> fixed_addr)
			return ISC_R_EXISTS;
		if (value -> type == omapi_datatype_data ||
		    value -> type == omapi_datatype_string) {
			struct data_string ds;
			memset (&ds, 0, sizeof ds);
			ds.len = value -> u.buffer.len;
			if (!buffer_allocate (&ds.buffer, ds.len, MDL))
				return ISC_R_NOMEMORY;
			ds.data = (&ds.buffer -> data [0]);
			memcpy (ds.buffer -> data,
				value -> u.buffer.value, ds.len);
			if (!option_cache (&host -> fixed_addr,
					   &ds, (struct expression *)0,
					   (struct option *)0)) {
				data_string_forget (&ds, MDL);
				return ISC_R_NOMEMORY;
			}
			data_string_forget (&ds, MDL);
		} else
			return ISC_R_INVALIDARG;
		return ISC_R_SUCCESS;
	}

	if (!omapi_ds_strcmp (name, "statements")) {
		if (!host -> group) {
			if (!clone_group (&host -> group, root_group, MDL))
				return ISC_R_NOMEMORY;
		} else {
			if (host -> group -> statements &&
			    (!host -> named_group ||
			     host -> group != host -> named_group -> group) &&
			    host -> group != root_group)
				return ISC_R_EXISTS;
			if (!clone_group (&host -> group, host -> group, MDL))
				return ISC_R_NOMEMORY;
		}
		if (!host -> group)
			return ISC_R_NOMEMORY;
		if (value -> type == omapi_datatype_data ||
		    value -> type == omapi_datatype_string) {
			struct parse *parse;
			int lose = 0;
			parse = (struct parse *)0;
			status = new_parse (&parse, -1,
					    (char *)value -> u.buffer.value,
					    value -> u.buffer.len,
					    "network client");
			if (status != ISC_R_SUCCESS)
				return status;
			if (!(parse_executable_statements
			      (&host -> group -> statements, parse, &lose,
			       context_any))) {
				end_parse (&parse);
				return ISC_R_BADPARSE;
			}
			end_parse (&parse);
		} else
			return ISC_R_INVALIDARG;
		return ISC_R_SUCCESS;
	}

	/* The "known" flag isn't supported in the database yet, but it's
	   legitimate. */
	if (!omapi_ds_strcmp (name, "known")) {
		return ISC_R_SUCCESS;
	}

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> set_value) {
		status = ((*(h -> inner -> type -> set_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS || status == ISC_R_UNCHANGED)
			return status;
	}
			  
	return ISC_R_NOTFOUND;
}


isc_result_t dhcp_host_get_value (omapi_object_t *h, omapi_object_t *id,
				   omapi_data_string_t *name,
				   omapi_value_t **value)
{
	struct host_decl *host;
	isc_result_t status;
	struct data_string ip_addrs;

	if (h -> type != dhcp_type_host)
		return ISC_R_INVALIDARG;
	host = (struct host_decl *)h;

	if (!omapi_ds_strcmp (name, "ip-addresses")) {
	    memset (&ip_addrs, 0, sizeof ip_addrs);
	    if (host -> fixed_addr &&
		evaluate_option_cache (&ip_addrs, (struct packet *)0,
				       (struct lease *)0,
				       (struct option_state *)0,
				       (struct option_state *)0,
				       &global_scope,
				       host -> fixed_addr, MDL)) {
		    status = omapi_make_const_value (value, name,
						     ip_addrs.data,
						     ip_addrs.len, MDL);
		    data_string_forget (&ip_addrs, MDL);
		    return status;
	    }
	    return ISC_R_NOTFOUND;
	}

	if (!omapi_ds_strcmp (name, "dhcp-client-identifier")) {
		if (!host -> client_identifier.len)
			return ISC_R_NOTFOUND;
		return omapi_make_const_value (value, name,
					       host -> client_identifier.data,
					       host -> client_identifier.len,
					       MDL);
	}

	if (!omapi_ds_strcmp (name, "name"))
		return omapi_make_string_value (value, name, host -> name,
						MDL);

	if (!omapi_ds_strcmp (name, "hardware-address")) {
		if (!host -> interface.hlen)
			return ISC_R_NOTFOUND;
		return (omapi_make_const_value
			(value, name, &host -> interface.hbuf [1],
			 (unsigned long)(host -> interface.hlen - 1), MDL));
	}

	if (!omapi_ds_strcmp (name, "hardware-type")) {
		if (!host -> interface.hlen)
			return ISC_R_NOTFOUND;
		return omapi_make_int_value (value, name,
					     host -> interface.hbuf [0], MDL);
	}

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> get_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_host_destroy (omapi_object_t *h, const char *file, int line)
{
	struct host_decl *host;
	isc_result_t status;

	if (h -> type != dhcp_type_host)
		return ISC_R_INVALIDARG;
	host = (struct host_decl *)h;

	/* Currently, this is completely hopeless - have to complete
           reference counting support for server OMAPI objects. */
	/* XXX okay, that's completed - now what? */

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_host_signal_handler (omapi_object_t *h,
				       const char *name, va_list ap)
{
	struct host_decl *host;
	isc_result_t status;
	int updatep = 0;

	if (h -> type != dhcp_type_host)
		return ISC_R_INVALIDARG;
	host = (struct host_decl *)h;

	if (!strcmp (name, "updated")) {
		if ((host -> interface.hlen == 0 ||
		     host -> interface.hbuf [0] == 0 ||
		     host -> interface.hlen > 17) &&
		    !host -> client_identifier.len)
			return ISC_R_INVALIDARG;

		if (!host -> name) {
			char hnbuf [64];
			sprintf (hnbuf, "nh%08lx%08lx",
				 cur_time, (unsigned long)host);
			host -> name = dmalloc (strlen (hnbuf) + 1, MDL);
			if (!host -> name)
				return ISC_R_NOMEMORY;
			strcpy (host -> name, hnbuf);
		}

#ifdef DEBUG_OMAPI
		log_debug ("OMAPI added host %s", host -> name);
#endif
		status = enter_host (host, 1, 1);
		if (status != ISC_R_SUCCESS)
			return status;
		updatep = 1;
	}

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> signal_handler))
			  (h -> inner, name, ap));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	if (updatep)
		return ISC_R_SUCCESS;
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_host_stuff_values (omapi_object_t *c,
				      omapi_object_t *id,
				      omapi_object_t *h)
{
	struct host_decl *host;
	isc_result_t status;
	struct data_string ip_addrs;

	if (h -> type != dhcp_type_host)
		return ISC_R_INVALIDARG;
	host = (struct host_decl *)h;

	/* Write out all the values. */

	memset (&ip_addrs, 0, sizeof ip_addrs);
	if (host -> fixed_addr &&
	    evaluate_option_cache (&ip_addrs, (struct packet *)0,
				   (struct lease *)0,
				   (struct option_state *)0,
				   (struct option_state *)0,
				   &global_scope,
				   host -> fixed_addr, MDL)) {
		status = omapi_connection_put_name (c, "ip-address");
		if (status != ISC_R_SUCCESS)
			return status;
		status = omapi_connection_put_uint32 (c, ip_addrs.len);
		if (status != ISC_R_SUCCESS)
			return status;
		status = omapi_connection_copyin (c,
						  ip_addrs.data, ip_addrs.len);
		if (status != ISC_R_SUCCESS)
			return status;
	}

	if (host -> client_identifier.len) {
		status = omapi_connection_put_name (c,
						    "dhcp-client-identifier");
		if (status != ISC_R_SUCCESS)
			return status;
		status = (omapi_connection_put_uint32
			  (c, host -> client_identifier.len));
		if (status != ISC_R_SUCCESS)
			return status;
		status = (omapi_connection_copyin
			  (c,
			   host -> client_identifier.data,
			   host -> client_identifier.len));
		if (status != ISC_R_SUCCESS)
			return status;
	}

	if (host -> name) {
		status = omapi_connection_put_name (c, "name");
		if (status != ISC_R_SUCCESS)
			return status;
		status = omapi_connection_put_string (c, host -> name);
		if (status != ISC_R_SUCCESS)
			return status;
	}

	if (host -> interface.hlen) {
		status = omapi_connection_put_name (c, "hardware-address");
		if (status != ISC_R_SUCCESS)
			return status;
		status = (omapi_connection_put_uint32
			  (c, (unsigned long)(host -> interface.hlen - 1)));
		if (status != ISC_R_SUCCESS)
			return status;
		status = (omapi_connection_copyin
			  (c, &host -> interface.hbuf [1],
			   (unsigned long)(host -> interface.hlen)));
		if (status != ISC_R_SUCCESS)
			return status;

		status = omapi_connection_put_name (c, "hardware-type");
		if (status != ISC_R_SUCCESS)
			return status;
		status = omapi_connection_put_uint32 (c, sizeof (int));
		if (status != ISC_R_SUCCESS)
			return status;
		status = (omapi_connection_put_uint32
			  (c, host -> interface.hbuf [0]));
		if (status != ISC_R_SUCCESS)
			return status;
	}

	/* Write out the inner object, if any. */
	if (h -> inner && h -> inner -> type -> stuff_values) {
		status = ((*(h -> inner -> type -> stuff_values))
			  (c, id, h -> inner));
		if (status == ISC_R_SUCCESS)
			return status;
	}

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_host_lookup (omapi_object_t **lp,
				omapi_object_t *id, omapi_object_t *ref)
{
	omapi_value_t *tv = (omapi_value_t *)0;
	isc_result_t status;
	struct host_decl *host;

	/* First see if we were sent a handle. */
	status = omapi_get_value_str (ref, id, "handle", &tv);
	if (status == ISC_R_SUCCESS) {
		status = omapi_handle_td_lookup (lp, tv -> value);

		omapi_value_dereference (&tv, MDL);
		if (status != ISC_R_SUCCESS)
			return status;

		/* Don't return the object if the type is wrong. */
		if ((*lp) -> type != dhcp_type_host) {
			omapi_object_dereference (lp, MDL);
			return ISC_R_INVALIDARG;
		}
		if (((struct host_decl *)(*lp)) -> flags & HOST_DECL_DELETED) {
			omapi_object_dereference (lp, MDL);
		}
	}

	/* Now look for a client identifier. */
	status = omapi_get_value_str (ref, id, "dhcp-client-identifier", &tv);
	if (status == ISC_R_SUCCESS) {
		host = (struct host_decl *)0;
		host_hash_lookup (&host, host_uid_hash,
				  tv -> value -> u.buffer.value,
				  tv -> value -> u.buffer.len, MDL);
		omapi_value_dereference (&tv, MDL);
			
		if (*lp && *lp != (omapi_object_t *)host) {
			omapi_object_dereference (lp, MDL);
			if (host)
				host_dereference (&host, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!host || (host -> flags & HOST_DECL_DELETED)) {
			if (*lp)
			    omapi_object_dereference (lp, MDL);
			if (host)
				host_dereference (&host, MDL);
			return ISC_R_NOTFOUND;
		} else if (!*lp) {
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (lp,
						(omapi_object_t *)host, MDL);
			host_dereference (&host, MDL);
		}
	}

	/* Now look for a hardware address. */
	status = omapi_get_value_str (ref, id, "hardware-address", &tv);
	if (status == ISC_R_SUCCESS) {
		host = (struct host_decl *)0;
		host_hash_lookup (&host, host_hw_addr_hash,
				  tv -> value -> u.buffer.value,
				  tv -> value -> u.buffer.len, MDL);
		omapi_value_dereference (&tv, MDL);
			
		if (*lp && *lp != (omapi_object_t *)host) {
			omapi_object_dereference (lp, MDL);
			if (host)
				host_dereference (&host, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!host || (host -> flags & HOST_DECL_DELETED)) {
			if (*lp)
			    omapi_object_dereference (lp, MDL);
			if (host)
				host_dereference (&host, MDL);
			return ISC_R_NOTFOUND;
		} else if (!*lp) {
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (lp,
						(omapi_object_t *)host, MDL);
			host_dereference (&host, MDL);
		}
	}

	/* Now look for an ip address. */
	status = omapi_get_value_str (ref, id, "ip-address", &tv);
	if (status == ISC_R_SUCCESS) {
		struct lease *l;

		/* first find the lease for this ip address */
		l = (struct lease *)0;
		lease_hash_lookup (&l, lease_ip_addr_hash,
				   tv -> value -> u.buffer.value,
				   tv -> value -> u.buffer.len, MDL);
		omapi_value_dereference (&tv, MDL);

		if (!l && !*lp)
			return ISC_R_NOTFOUND;

		if (l) {
			/* now use that to get a host */
			host = (struct host_decl *)0;
			host_hash_lookup (&host, host_hw_addr_hash,
					  l -> hardware_addr.hbuf,
					  l -> hardware_addr.hlen, MDL);
			
			if (host && *lp && *lp != (omapi_object_t *)host) {
			    omapi_object_dereference (lp, MDL);
			    if (host)
				host_dereference (&host, MDL);
			    return ISC_R_KEYCONFLICT;
			} else if (!host || (host -> flags &
					     HOST_DECL_DELETED)) {
			    if (host)
				host_dereference (&host, MDL);
			    if (!*lp)
				    return ISC_R_NOTFOUND;
			} else if (!*lp) {
				/* XXX fix so that hash lookup itself creates
				   XXX the reference. */
			    omapi_object_reference (lp, (omapi_object_t *)host,
						    MDL);
			    host_dereference (&host, MDL);
			}
			lease_dereference (&l, MDL);
		}
	}

	/* Now look for a name. */
	status = omapi_get_value_str (ref, id, "name", &tv);
	if (status == ISC_R_SUCCESS) {
		host = (struct host_decl *)0;
		host_hash_lookup (&host, host_name_hash,
				  tv -> value -> u.buffer.value,
				  tv -> value -> u.buffer.len, MDL);
		omapi_value_dereference (&tv, MDL);
			
		if (*lp && *lp != (omapi_object_t *)host) {
			omapi_object_dereference (lp, MDL);
			if (host)
			    host_dereference (&host, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!host || (host -> flags & HOST_DECL_DELETED)) {
			if (host)
			    host_dereference (&host, MDL);
			return ISC_R_NOTFOUND;	
		} else if (!*lp) {
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (lp,
						(omapi_object_t *)host, MDL);
			host_dereference (&host, MDL);
		}
	}

	/* If we get to here without finding a host, no valid key was
	   specified. */
	if (!*lp)
		return ISC_R_NOKEYS;
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_host_create (omapi_object_t **lp,
			       omapi_object_t *id)
{
	struct host_decl *hp;
	isc_result_t status;
	hp = (struct host_decl *)0;
	status = host_allocate (&hp, MDL);
	if (status != ISC_R_SUCCESS)
		return status;
	group_reference (&hp -> group, root_group, MDL);
	hp -> flags = HOST_DECL_DYNAMIC;
	status = omapi_object_reference (lp, (omapi_object_t *)hp, MDL);
	host_dereference (&hp, MDL);
	return status;
}

isc_result_t dhcp_host_remove (omapi_object_t *lp,
			       omapi_object_t *id)
{
	struct host_decl *hp;
	if (lp -> type != dhcp_type_host)
		return ISC_R_INVALIDARG;
	hp = (struct host_decl *)lp;

#ifdef DEBUG_OMAPI
	log_debug ("OMAPI delete host %s", hp -> name);
#endif
	delete_host (hp, 1);
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_pool_set_value  (omapi_object_t *h,
				   omapi_object_t *id,
				   omapi_data_string_t *name,
				   omapi_typed_data_t *value)
{
	struct pool *pool;
	isc_result_t status;
	int foo;

	if (h -> type != dhcp_type_pool)
		return ISC_R_INVALIDARG;
	pool = (struct pool *)h;

	/* No values to set yet. */

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> set_value) {
		status = ((*(h -> inner -> type -> set_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS || status == ISC_R_UNCHANGED)
			return status;
	}
			  
	return ISC_R_NOTFOUND;
}


isc_result_t dhcp_pool_get_value (omapi_object_t *h, omapi_object_t *id,
				  omapi_data_string_t *name,
				  omapi_value_t **value)
{
	struct pool *pool;
	isc_result_t status;

	if (h -> type != dhcp_type_pool)
		return ISC_R_INVALIDARG;
	pool = (struct pool *)h;

	/* No values to get yet. */

	/* Try to find some inner object that can provide the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> get_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_pool_destroy (omapi_object_t *h, const char *file, int line)
{
	struct pool *pool;
	isc_result_t status;

	if (h -> type != dhcp_type_pool)
		return ISC_R_INVALIDARG;
	pool = (struct pool *)h;

	/* Can't destroy pools yet. */

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_pool_signal_handler (omapi_object_t *h,
				       const char *name, va_list ap)
{
	struct pool *pool;
	isc_result_t status;
	int updatep = 0;

	if (h -> type != dhcp_type_pool)
		return ISC_R_INVALIDARG;
	pool = (struct pool *)h;

	/* Can't write pools yet. */

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> signal_handler))
			  (h -> inner, name, ap));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	if (updatep)
		return ISC_R_SUCCESS;
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_pool_stuff_values (omapi_object_t *c,
				     omapi_object_t *id,
				     omapi_object_t *h)
{
	struct pool *pool;
	isc_result_t status;

	if (h -> type != dhcp_type_pool)
		return ISC_R_INVALIDARG;
	pool = (struct pool *)h;

	/* Can't stuff pool values yet. */

	/* Write out the inner object, if any. */
	if (h -> inner && h -> inner -> type -> stuff_values) {
		status = ((*(h -> inner -> type -> stuff_values))
			  (c, id, h -> inner));
		if (status == ISC_R_SUCCESS)
			return status;
	}

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_pool_lookup (omapi_object_t **lp,
			       omapi_object_t *id, omapi_object_t *ref)
{
	omapi_value_t *tv = (omapi_value_t *)0;
	isc_result_t status;
	struct pool *pool;

	/* Can't look up pools yet. */

	/* If we get to here without finding a pool, no valid key was
	   specified. */
	if (!*lp)
		return ISC_R_NOKEYS;
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_pool_create (omapi_object_t **lp,
			       omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dhcp_pool_remove (omapi_object_t *lp,
			       omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dhcp_class_set_value  (omapi_object_t *h,
				    omapi_object_t *id,
				    omapi_data_string_t *name,
				    omapi_typed_data_t *value)
{
	struct class *class;
	isc_result_t status;
	int foo;

	if (h -> type != dhcp_type_class)
		return ISC_R_INVALIDARG;
	class = (struct class *)h;

	/* No values to set yet. */

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> set_value) {
		status = ((*(h -> inner -> type -> set_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS || status == ISC_R_UNCHANGED)
			return status;
	}
			  
	return ISC_R_NOTFOUND;
}


isc_result_t dhcp_class_get_value (omapi_object_t *h, omapi_object_t *id,
				   omapi_data_string_t *name,
				   omapi_value_t **value)
{
	struct class *class;
	isc_result_t status;

	if (h -> type != dhcp_type_class)
		return ISC_R_INVALIDARG;
	class = (struct class *)h;

	/* No values to get yet. */

	/* Try to find some inner object that can provide the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> get_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_class_destroy (omapi_object_t *h, const char *file, int line)
{
	struct class *class;
	isc_result_t status;

	if (h -> type != dhcp_type_class)
		return ISC_R_INVALIDARG;
	class = (struct class *)h;

	/* Can't destroy classs yet. */

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_class_signal_handler (omapi_object_t *h,
					const char *name, va_list ap)
{
	struct class *class;
	isc_result_t status;
	int updatep = 0;

	if (h -> type != dhcp_type_class)
		return ISC_R_INVALIDARG;
	class = (struct class *)h;

	/* Can't write classs yet. */

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> get_value) {
		status = ((*(h -> inner -> type -> signal_handler))
			  (h -> inner, name, ap));
		if (status == ISC_R_SUCCESS)
			return status;
	}
	if (updatep)
		return ISC_R_SUCCESS;
	return ISC_R_NOTFOUND;
}

isc_result_t dhcp_class_stuff_values (omapi_object_t *c,
				      omapi_object_t *id,
				      omapi_object_t *h)
{
	struct class *class;
	isc_result_t status;

	if (h -> type != dhcp_type_class)
		return ISC_R_INVALIDARG;
	class = (struct class *)h;

	/* Can't stuff class values yet. */

	/* Write out the inner object, if any. */
	if (h -> inner && h -> inner -> type -> stuff_values) {
		status = ((*(h -> inner -> type -> stuff_values))
			  (c, id, h -> inner));
		if (status == ISC_R_SUCCESS)
			return status;
	}

	return ISC_R_SUCCESS;
}

isc_result_t dhcp_class_lookup (omapi_object_t **lp,
				omapi_object_t *id, omapi_object_t *ref)
{
	omapi_value_t *tv = (omapi_value_t *)0;
	isc_result_t status;
	struct class *class;

	/* Can't look up classs yet. */

	/* If we get to here without finding a class, no valid key was
	   specified. */
	if (!*lp)
		return ISC_R_NOKEYS;
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_class_create (omapi_object_t **lp,
				omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dhcp_class_remove (omapi_object_t *lp,
				omapi_object_t *id)
{
	return ISC_R_NOTIMPLEMENTED;
}

/* vim: set tabstop=8: */
