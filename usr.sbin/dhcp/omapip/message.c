/* message.c

   Subroutines for dealing with message objects. */

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

#include <omapip/omapip_p.h>

OMAPI_OBJECT_ALLOC (omapi_message,
		    omapi_message_object_t, omapi_type_message)

omapi_message_object_t *omapi_registered_messages;

isc_result_t omapi_message_new (omapi_object_t **o, const char *file, int line)
{
	omapi_message_object_t *m;
	omapi_object_t *g;
	isc_result_t status;

	m = dmalloc (sizeof *m, file, line);
	if (!m)
		return ISC_R_NOMEMORY;
	memset (m, 0, sizeof *m);
	m -> type = omapi_type_message;
	rc_register (file, line, &m, m, m -> refcnt);
	m -> refcnt = 1;

	g = (omapi_object_t *)0;
	status = omapi_generic_new (&g, file, line);
	if (status != ISC_R_SUCCESS) {
		dfree (m, file, line);
		return status;
	}
	status = omapi_object_reference (&m -> inner, g, file, line);
	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&m, file, line);
		omapi_object_dereference (&g, file, line);
		return status;
	}
	status = omapi_object_reference (&g -> outer,
					 (omapi_object_t *)m, file, line);

	if (status != ISC_R_SUCCESS) {
		omapi_object_dereference ((omapi_object_t **)&m, file, line);
		omapi_object_dereference (&g, file, line);
		return status;
	}

	status = omapi_object_reference (o, (omapi_object_t *)m, file, line);
	omapi_object_dereference ((omapi_object_t **)&m, file, line);
	omapi_object_dereference (&g, file, line);
	if (status != ISC_R_SUCCESS)
		return status;

	return status;
}

isc_result_t omapi_message_set_value (omapi_object_t *h,
				      omapi_object_t *id,
				      omapi_data_string_t *name,
				      omapi_typed_data_t *value)
{
	omapi_message_object_t *m;
	isc_result_t status;

	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)h;

	/* Can't set authlen. */

	/* Can set authenticator, but the value must be typed data. */
	if (!omapi_ds_strcmp (name, "authenticator")) {
		if (m -> authenticator)
			omapi_typed_data_dereference (&m -> authenticator,
						      MDL);
		omapi_typed_data_reference (&m -> authenticator, value, MDL);
		return ISC_R_SUCCESS;

	} else if (!omapi_ds_strcmp (name, "object")) {
		if (value -> type != omapi_datatype_object)
			return ISC_R_INVALIDARG;
		if (m -> object)
			omapi_object_dereference (&m -> object, MDL);
		omapi_object_reference (&m -> object, value -> u.object, MDL);
		return ISC_R_SUCCESS;

	} else if (!omapi_ds_strcmp (name, "notify-object")) {
		if (value -> type != omapi_datatype_object)
			return ISC_R_INVALIDARG;
		if (m -> notify_object)
			omapi_object_dereference (&m -> notify_object, MDL);
		omapi_object_reference (&m -> notify_object,
					value -> u.object, MDL);
		return ISC_R_SUCCESS;

	/* Can set authid, but it has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "authid")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> authid = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Can set op, but it has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "op")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> op = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Handle also has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "handle")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> h = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Transaction ID has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "id")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> id = value -> u.integer;
		return ISC_R_SUCCESS;

	/* Remote transaction ID has to be an integer. */
	} else if (!omapi_ds_strcmp (name, "rid")) {
		if (value -> type != omapi_datatype_int)
			return ISC_R_INVALIDARG;
		m -> rid = value -> u.integer;
		return ISC_R_SUCCESS;
	}

	/* Try to find some inner object that can take the value. */
	if (h -> inner && h -> inner -> type -> set_value) {
		status = ((*(h -> inner -> type -> set_value))
			  (h -> inner, id, name, value));
		if (status == ISC_R_SUCCESS)
			return status;
	}
			  
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_message_get_value (omapi_object_t *h,
				      omapi_object_t *id,
				      omapi_data_string_t *name,
				      omapi_value_t **value)
{
	omapi_message_object_t *m;
	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)h;

	/* Look for values that are in the message data structure. */
	if (!omapi_ds_strcmp (name, "authlen"))
		return omapi_make_int_value (value, name, (int)m -> authlen,
					     MDL);
	else if (!omapi_ds_strcmp (name, "authenticator")) {
		if (m -> authenticator)
			return omapi_make_value (value, name,
						 m -> authenticator, MDL);
		else
			return ISC_R_NOTFOUND;
	} else if (!omapi_ds_strcmp (name, "authid")) {
		return omapi_make_int_value (value,
					     name, (int)m -> authid, MDL);
	} else if (!omapi_ds_strcmp (name, "op")) {
		return omapi_make_int_value (value, name, (int)m -> op, MDL);
	} else if (!omapi_ds_strcmp (name, "handle")) {
		return omapi_make_int_value (value, name, (int)m -> h, MDL);
	} else if (!omapi_ds_strcmp (name, "id")) {
		return omapi_make_int_value (value, name, (int)m -> id, MDL);
	} else if (!omapi_ds_strcmp (name, "rid")) {
		return omapi_make_int_value (value, name, (int)m -> rid, MDL);
	}

	/* See if there's an inner object that has the value. */
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_message_destroy (omapi_object_t *h,
				    const char *file, int line)
{
	int i;

	omapi_message_object_t *m;
	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)h;
	if (m -> authenticator) {
		omapi_typed_data_dereference (&m -> authenticator, file, line);
	}
	if (!m -> prev && omapi_registered_messages != m)
		omapi_message_unregister (h);
	if (m -> id_object)
		omapi_object_dereference ((omapi_object_t **)&m -> id_object,
					  file, line);
	if (m -> object)
		omapi_object_dereference ((omapi_object_t **)&m -> object,
					  file, line);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_message_signal_handler (omapi_object_t *h,
					   const char *name, va_list ap)
{
	omapi_message_object_t *m;
	if (h -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)h;
	
	if (!strcmp (name, "status") && 
	    (m -> object || m -> notify_object)) {
		if (m -> object)
			return ((m -> object -> type -> signal_handler))
				(m -> object, name, ap);
		else
			return ((m -> notify_object -> type -> signal_handler))
				(m -> notify_object, name, ap);
	}
	if (h -> inner && h -> inner -> type -> signal_handler)
		return (*(h -> inner -> type -> signal_handler)) (h -> inner,
								  name, ap);
	return ISC_R_NOTFOUND;
}

/* Write all the published values associated with the object through the
   specified connection. */

isc_result_t omapi_message_stuff_values (omapi_object_t *c,
					 omapi_object_t *id,
					 omapi_object_t *m)
{
	int i;

	if (m -> type != omapi_type_message)
		return ISC_R_INVALIDARG;

	if (m -> inner && m -> inner -> type -> stuff_values)
		return (*(m -> inner -> type -> stuff_values)) (c, id,
								m -> inner);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_message_register (omapi_object_t *mo)
{
	omapi_message_object_t *m;

	if (mo -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)mo;
	
	/* Already registered? */
	if (m -> prev || m -> next || omapi_registered_messages == m)
		return ISC_R_INVALIDARG;

	if (omapi_registered_messages) {
		omapi_object_reference
			((omapi_object_t **)&m -> next,
			 (omapi_object_t *)omapi_registered_messages, MDL);
		omapi_object_reference
			((omapi_object_t **)&omapi_registered_messages -> prev,
			 (omapi_object_t *)m, MDL);
		omapi_object_dereference
			((omapi_object_t **)&omapi_registered_messages, MDL);
	}
	omapi_object_reference
		((omapi_object_t **)&omapi_registered_messages,
		 (omapi_object_t *)m, MDL);
	return ISC_R_SUCCESS;;
}

isc_result_t omapi_message_unregister (omapi_object_t *mo)
{
	omapi_message_object_t *m;
	omapi_message_object_t *n;

	if (mo -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	m = (omapi_message_object_t *)mo;
	
	/* Not registered? */
	if (!m -> prev && omapi_registered_messages != m)
		return ISC_R_INVALIDARG;

	n = (omapi_message_object_t *)0;
	if (m -> next) {
		omapi_object_reference ((omapi_object_t **)&n,
					(omapi_object_t *)m -> next, MDL);
		omapi_object_dereference ((omapi_object_t **)&m -> next, MDL);
	}
	if (m -> prev) {
		omapi_message_object_t *tmp = (omapi_message_object_t *)0;
		omapi_object_reference ((omapi_object_t **)&tmp,
					(omapi_object_t *)m -> prev, MDL);
		omapi_object_dereference ((omapi_object_t **)&m -> prev, MDL);
		if (tmp -> next)
			omapi_object_dereference
				((omapi_object_t **)&tmp -> next, MDL);
		if (n)
			omapi_object_reference
				((omapi_object_t **)&tmp -> next,
				 (omapi_object_t *)n, MDL);
		omapi_object_dereference ((omapi_object_t **)&tmp, MDL);
	} else {
		omapi_object_dereference
			((omapi_object_t **)&omapi_registered_messages, MDL);
		if (n)
			omapi_object_reference
				((omapi_object_t **)&omapi_registered_messages,
				 (omapi_object_t *)n, MDL);
	}
	if (n)
		omapi_object_dereference ((omapi_object_t **)&n, MDL);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_message_process (omapi_object_t *mo, omapi_object_t *po)
{
	omapi_message_object_t *message, *m;
	omapi_object_t *object = (omapi_object_t *)0;
	omapi_value_t *tv = (omapi_value_t *)0;
	unsigned long create, update, exclusive;
	unsigned long wsi;
	isc_result_t status, waitstatus;
	omapi_object_type_t *type;

	if (mo -> type != omapi_type_message)
		return ISC_R_INVALIDARG;
	message = (omapi_message_object_t *)mo;

	if (message -> rid) {
		for (m = omapi_registered_messages; m; m = m -> next)
			if (m -> id == message -> rid)
				break;
		/* If we don't have a real message corresponding to
		   the message ID to which this message claims it is a
		   response, something's fishy. */
		if (!m)
			return ISC_R_NOTFOUND;
	} else
		m = (omapi_message_object_t *)0;

	switch (message -> op) {
	      case OMAPI_OP_OPEN:
		if (m) {
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0, ISC_R_INVALIDARG,
				 message -> id, "OPEN can't be a response");
		}

		/* Get the type of the requested object, if one was
		   specified. */
		status = omapi_get_value_str (mo, (omapi_object_t *)0,
					      "type", &tv);
		if (status == ISC_R_SUCCESS &&
		    (tv -> value -> type == omapi_datatype_data ||
		     tv -> value -> type == omapi_datatype_string)) {
			for (type = omapi_object_types;
			     type; type = type -> next)
				if (!omapi_td_strcmp (tv -> value,
						      type -> name))
					break;
		} else
			type = (omapi_object_type_t *)0;
		if (tv)
			omapi_value_dereference (&tv, MDL);

		/* Get the create flag. */
		status = omapi_get_value_str (mo,
					      (omapi_object_t *)0,
					      "create", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&create, tv -> value);
			omapi_value_dereference (&tv, MDL);
			if (status != ISC_R_SUCCESS) {
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 status, message -> id,
					 "invalid create flag value");
			}
		} else
			create = 0;

		/* Get the update flag. */
		status = omapi_get_value_str (mo,
					      (omapi_object_t *)0,
					      "update", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&update, tv -> value);
			omapi_value_dereference (&tv, MDL);
			if (status != ISC_R_SUCCESS) {
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 status, message -> id,
					 "invalid update flag value");
			}
		} else
			update = 0;

		/* Get the exclusive flag. */
		status = omapi_get_value_str (mo,
					      (omapi_object_t *)0,
					      "exclusive", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&exclusive, tv -> value);
			omapi_value_dereference (&tv, MDL);
			if (status != ISC_R_SUCCESS) {
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 status, message -> id,
					 "invalid exclusive flag value");
			}
		} else
			exclusive = 0;

		/* If we weren't given a type, look the object up with
                   the handle. */
		if (!type) {
			if (create) {
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 ISC_R_INVALIDARG, message -> id,
					 "type required on create");
			}
			goto refresh;
		}

		/* If the type doesn't provide a lookup method, we can't
		   look up the object. */
		if (!type -> lookup) {
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 ISC_R_NOTIMPLEMENTED, message -> id,
				 "unsearchable object type");
		}

		if (!message -> object) {
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 ISC_R_NOTFOUND, message -> id,
				 "no lookup key specified");
		}
		status = (*(type -> lookup)) (&object, (omapi_object_t *)0,
					      message -> object);

		if (status != ISC_R_SUCCESS &&
		    status != ISC_R_NOTFOUND &&
		    status != ISC_R_NOKEYS) {
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 status, message -> id,
				 "object lookup failed");
		}

		/* If we didn't find the object and we aren't supposed to
		   create it, return an error. */
		if (status == ISC_R_NOTFOUND && !create) {
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 ISC_R_NOTFOUND, message -> id,
				 "no object matches specification");
		}			

		/* If we found an object, we're supposed to be creating an
		   object, and we're not supposed to have found an object,
		   return an error. */
		if (status == ISC_R_SUCCESS && create && exclusive) {
			omapi_object_dereference (&object, MDL);
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 ISC_R_EXISTS, message -> id,
				 "specified object already exists");
		}

		/* If we're creating the object, do it now. */
		if (!object) {
			status = omapi_object_create (&object,
						      (omapi_object_t *)0,
						      type);
			if (status != ISC_R_SUCCESS) {
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 status, message -> id,
					 "can't create new object");
			}
		}

		/* If we're updating it, do so now. */
		if (create || update) {
			status = omapi_object_update (object,
						      (omapi_object_t *)0,
						      message -> object,
						      message -> h);
			if (status != ISC_R_SUCCESS) {
				omapi_object_dereference (&object, MDL);
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 status, message -> id,
					 "can't update object");
			}
		}
		
		/* Now send the new contents of the object back in
		   response. */
		goto send;

	      case OMAPI_OP_REFRESH:
	      refresh:
		status = omapi_handle_lookup (&object, message -> h);
		if (status != ISC_R_SUCCESS) {
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 status, message -> id,
				 "no matching handle");
		}
	      send:		
		status = omapi_protocol_send_update (po, (omapi_object_t *)0,
						     message -> id, object);
		omapi_object_dereference (&object, MDL);
		return status;

	      case OMAPI_OP_UPDATE:
		if (m -> object) {
			omapi_object_reference (&object, m -> object, MDL);
		} else {
			status = omapi_handle_lookup (&object, message -> h);
			if (status != ISC_R_SUCCESS) {
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 status, message -> id,
					 "no matching handle");
			}
		}

		status = omapi_object_update (object, (omapi_object_t *)0,
					      message -> object,
					      message -> h);
		if (status != ISC_R_SUCCESS) {
			omapi_object_dereference (&object, MDL);
			if (!message -> rid)
				return omapi_protocol_send_status
					(po, (omapi_object_t *)0,
					 status, message -> id,
					 "can't update object");
			if (m)
				omapi_signal ((omapi_object_t *)m,
					      "status", status,
					      (omapi_typed_data_t *)0);
			return ISC_R_SUCCESS;
		}
		if (!message -> rid)
			status = omapi_protocol_send_status
				(po, (omapi_object_t *)0, ISC_R_SUCCESS,
				 message -> id, (char *)0);
		if (m)
			omapi_signal ((omapi_object_t *)m,
				      "status", ISC_R_SUCCESS,
				      (omapi_typed_data_t *)0);
		return status;

	      case OMAPI_OP_NOTIFY:
		return omapi_protocol_send_status
			(po, (omapi_object_t *)0, ISC_R_NOTIMPLEMENTED,
			 message -> id, "notify not implemented yet");

	      case OMAPI_OP_STATUS:
		/* The return status of a request. */
		if (!m)
			return ISC_R_UNEXPECTED;

		/* Get the wait status. */
		status = omapi_get_value_str (mo,
					      (omapi_object_t *)0,
					      "result", &tv);
		if (status == ISC_R_SUCCESS) {
			status = omapi_get_int_value (&wsi, tv -> value);
			waitstatus = wsi;
			omapi_value_dereference (&tv, MDL);
			if (status != ISC_R_SUCCESS)
				waitstatus = ISC_R_UNEXPECTED;
		} else
			waitstatus = ISC_R_UNEXPECTED;

		status = omapi_get_value_str (mo,
					      (omapi_object_t *)0,
					      "message", &tv);
		omapi_signal ((omapi_object_t *)m, "status", waitstatus, tv);
		if (status == ISC_R_SUCCESS)
			omapi_value_dereference (&tv, MDL);
		return ISC_R_SUCCESS;

	      case OMAPI_OP_DELETE:
		status = omapi_handle_lookup (&object, message -> h);
		if (status != ISC_R_SUCCESS) {
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 status, message -> id,
				 "no matching handle");
		}

		if (!object -> type -> remove)
			return omapi_protocol_send_status
				(po, (omapi_object_t *)0,
				 ISC_R_NOTIMPLEMENTED, message -> id,
				 "no remove method for object");

		status = (*(object -> type -> remove)) (object,
							(omapi_object_t *)0);
		omapi_object_dereference (&object, MDL);

		return omapi_protocol_send_status (po, (omapi_object_t *)0,
						   status, message -> id,
						   (char *)0);
	}
	return ISC_R_NOTIMPLEMENTED;
}
