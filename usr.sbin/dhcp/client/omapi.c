/* omapi.c

   OMAPI object interfaces for the DHCP client. */

/*
 * Copyright (c) 2000 The Internet Software Consortium
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
"$Id: omapi.c,v 1.1.1.1 2000/04/22 07:11:31 mellon Exp $ Copyright (c) 2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include <omapip/omapip_p.h>

void dhclient_db_objects_setup ()
{
	isc_result_t status;

	status = omapi_object_type_register (&dhcp_type_interface,
					     "interface",
					     dhclient_interface_set_value,
					     dhclient_interface_get_value,
					     dhclient_interface_destroy,
					     dhclient_interface_signal_handler,
					     dhclient_interface_stuff_values,
					     dhclient_interface_lookup, 
					     dhclient_interface_create,
					     dhclient_interface_remove);
	if (status != ISC_R_SUCCESS)
		log_fatal ("Can't register interface object type: %s",
			   isc_result_totext (status));

}

isc_result_t dhclient_interface_set_value  (omapi_object_t *h,
					    omapi_object_t *id,
					    omapi_data_string_t *name,
					    omapi_typed_data_t *value)
{
	struct interface_info *interface;
	isc_result_t status;
	int foo;

	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	interface = (struct interface_info *)h;

	if (!omapi_ds_strcmp (name, "name")) {
		if (value -> type == omapi_datatype_data ||
		    value -> type == omapi_datatype_string) {
			memcpy (interface -> name,
				value -> u.buffer.value,
				value -> u.buffer.len);
			interface -> name [value -> u.buffer.len] = 0;
		} else
			return ISC_R_INVALIDARG;
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


isc_result_t dhclient_interface_get_value (omapi_object_t *h,
					   omapi_object_t *id,
					   omapi_data_string_t *name,
					   omapi_value_t **value)
{
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t dhclient_interface_destroy (omapi_object_t *h,
					 const char *file, int line)
{
	struct interface_info *interface;
	isc_result_t status;

	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	interface = (struct interface_info *)h;

	
	if (interface -> ifp)
		free (interface -> ifp);
	dfree (interface, file, line);
	return ISC_R_SUCCESS;
}

isc_result_t dhclient_interface_signal_handler (omapi_object_t *h,
						const char *name, va_list ap)
{
	struct interface_info *ip, *interface;
	struct client_config *config;
	struct client_state *client;

	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	interface = (struct interface_info *)h;

	interface -> next = interfaces;
	interfaces = interface;

	discover_interfaces (DISCOVER_UNCONFIGURED);

	for (ip = interfaces; ip; ip = ip -> next) {
		/* If interfaces were specified, don't configure
		   interfaces that weren't specified! */
		if (ip -> flags & INTERFACE_RUNNING ||
		   (ip -> flags & (INTERFACE_REQUESTED |
				     INTERFACE_AUTOMATIC)) !=
		     INTERFACE_REQUESTED)
			continue;
		script_init (ip -> client,
			     "PREINIT", (struct string_list *)0);
		if (ip -> client -> alias)
			script_write_params (ip -> client, "alias_",
					     ip -> client -> alias);
		script_go (ip -> client);
	}
	
	discover_interfaces (interfaces_requested
			     ? DISCOVER_REQUESTED
			     : DISCOVER_RUNNING);

	for (ip = interfaces; ip; ip = ip -> next) {
		if (ip -> flags & INTERFACE_RUNNING)
			continue;
		ip -> flags |= INTERFACE_RUNNING;
		for (client = ip -> client; client; client = client -> next) {
			client -> state = S_INIT;
			/* Set up a timeout to start the initialization
			   process. */
			add_timeout (cur_time + random () % 5,
				     state_reboot, client);
		}
	}
	return ISC_R_SUCCESS;
}

isc_result_t dhclient_interface_stuff_values (omapi_object_t *c,
					      omapi_object_t *id,
					      omapi_object_t *h)
{
	struct interface_info *interface;
	isc_result_t status;

	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	interface = (struct interface_info *)h;

	/* Write out all the values. */

	status = omapi_connection_put_name (c, "state");
	if (status != ISC_R_SUCCESS)
		return status;
	if (interface -> flags && INTERFACE_REQUESTED)
	    status = omapi_connection_put_string (c, "up");
	else
	    status = omapi_connection_put_string (c, "down");
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

isc_result_t dhclient_interface_lookup (omapi_object_t **ip,
					omapi_object_t *id,
					omapi_object_t *ref)
{
	omapi_value_t *tv = (omapi_value_t *)0;
	isc_result_t status;
	struct interface_info *interface;

	/* First see if we were sent a handle. */
	status = omapi_get_value_str (ref, id, "handle", &tv);
	if (status == ISC_R_SUCCESS) {
		status = omapi_handle_td_lookup (ip, tv -> value);

		omapi_value_dereference (&tv, MDL);
		if (status != ISC_R_SUCCESS)
			return status;

		/* Don't return the object if the type is wrong. */
		if ((*ip) -> type != dhcp_type_interface) {
			omapi_object_dereference (ip, MDL);
			return ISC_R_INVALIDARG;
		}
	}

	/* Now look for an interface name. */
	status = omapi_get_value_str (ref, id, "name", &tv);
	if (status == ISC_R_SUCCESS) {
		for (interface = interfaces; interface;
		     interface = interface -> next) {
		    if (strncmp (interface -> name,
				 (char *)tv -> value -> u.buffer.value,
				 tv -> value -> u.buffer.len) == 0)
			    break;
		}
		omapi_value_dereference (&tv, MDL);
		if (*ip && *ip != (omapi_object_t *)interface) {
			omapi_object_dereference (ip, MDL);
			return ISC_R_KEYCONFLICT;
		} else if (!interface) {
			if (*ip)
				omapi_object_dereference (ip, MDL);
			return ISC_R_NOTFOUND;
		} else if (!*ip)
			/* XXX fix so that hash lookup itself creates
			   XXX the reference. */
			omapi_object_reference (ip,
						(omapi_object_t *)interface,
						MDL);
	}

	/* If we get to here without finding an interface, no valid key was
	   specified. */
	if (!*ip)
		return ISC_R_NOKEYS;
	return ISC_R_SUCCESS;
}

/* actually just go discover the interface */
isc_result_t dhclient_interface_create (omapi_object_t **lp,
					omapi_object_t *id)
{
 	struct interface_info *hp;
	
	hp = (struct interface_info *)dmalloc (sizeof (struct interface_info),
					       MDL);
 	if (!hp)
		return ISC_R_NOMEMORY;
 	memset (hp, 0, sizeof *hp);
	hp -> refcnt = 0;
	hp -> type = dhcp_type_interface;
 	hp -> flags = INTERFACE_REQUESTED;
	return omapi_object_reference (lp, (omapi_object_t *)hp, MDL);

}

isc_result_t dhclient_interface_remove (omapi_object_t *lp,
					omapi_object_t *id)
{
 	struct interface_info *interface, *ip, *last;

	interface = (struct interface_info *)lp;

	/* remove from interfaces */
	last = 0;
	for (ip = interfaces; ip; ip = ip -> next) {
		if (!strcmp (ip -> name, interface -> name)) {
			if (last)
				last -> next = ip -> next;
			else
				interfaces = ip -> next;
			break;
		}
		last = ip;
	}

	/* add the interface to the dummy_interface list */
	interface -> next = dummy_interfaces;
	dummy_interfaces = interface;

	/* do a DHCPRELEASE */
	do_release (interface -> client);

	/* remove the io object */
	omapi_io_destroy (interface -> outer, MDL);

	if_deregister_send (interface);
	if_deregister_receive (interface);

	return ISC_R_SUCCESS;
}
