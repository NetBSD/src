/*
 * WPA Supplicant / dbus-based control interface
 * Copyright (c) 2006, Dan Williams <dcbw@redhat.com> and Red Hat, Inc.
 * Copyright (c) 2009-2010, Witold Sowa <witold.sowa@gmail.com>
 * Copyright (c) 2009, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "common/ieee802_11_defs.h"
#include "eap_peer/eap_methods.h"
#include "eapol_supp/eapol_supp_sm.h"
#include "rsn_supp/wpa.h"
#include "../config.h"
#include "../wpa_supplicant_i.h"
#include "../driver_i.h"
#include "../notify.h"
#include "../wpas_glue.h"
#include "../bss.h"
#include "../scan.h"
#include "../ctrl_iface.h"
#include "dbus_new_helpers.h"
#include "dbus_new.h"
#include "dbus_new_handlers.h"
#include "dbus_dict_helpers.h"

extern int wpa_debug_level;
extern int wpa_debug_show_keys;
extern int wpa_debug_timestamp;

static const char *debug_strings[] = {
	"excessive", "msgdump", "debug", "info", "warning", "error", NULL
};


/**
 * wpas_dbus_error_unknown_error - Return a new InvalidArgs error message
 * @message: Pointer to incoming dbus message this error refers to
 * @arg: Optional string appended to error message
 * Returns: a dbus error message
 *
 * Convenience function to create and return an UnknownError
 */
DBusMessage * wpas_dbus_error_unknown_error(DBusMessage *message,
					    const char *arg)
{
	/*
	 * This function can be called as a result of a failure
	 * within internal getter calls, which will call this function
	 * with a NULL message parameter.  However, dbus_message_new_error
	 * looks very unkindly (i.e, abort()) on a NULL message, so
	 * in this case, we should not call it.
	 */
	if (message == NULL) {
		wpa_printf(MSG_INFO, "dbus: wpas_dbus_error_unknown_error "
			   "called with NULL message (arg=%s)",
			   arg ? arg : "N/A");
		return NULL;
	}

	return dbus_message_new_error(message, WPAS_DBUS_ERROR_UNKNOWN_ERROR,
				      arg);
}


/**
 * wpas_dbus_error_iface_unknown - Return a new invalid interface error message
 * @message: Pointer to incoming dbus message this error refers to
 * Returns: A dbus error message
 *
 * Convenience function to create and return an invalid interface error
 */
static DBusMessage * wpas_dbus_error_iface_unknown(DBusMessage *message)
{
	return dbus_message_new_error(message, WPAS_DBUS_ERROR_IFACE_UNKNOWN,
				      "wpa_supplicant knows nothing about "
				      "this interface.");
}


/**
 * wpas_dbus_error_network_unknown - Return a new NetworkUnknown error message
 * @message: Pointer to incoming dbus message this error refers to
 * Returns: a dbus error message
 *
 * Convenience function to create and return an invalid network error
 */
static DBusMessage * wpas_dbus_error_network_unknown(DBusMessage *message)
{
	return dbus_message_new_error(message, WPAS_DBUS_ERROR_NETWORK_UNKNOWN,
				      "There is no such a network in this "
				      "interface.");
}


/**
 * wpas_dbus_error_invalid_args - Return a new InvalidArgs error message
 * @message: Pointer to incoming dbus message this error refers to
 * Returns: a dbus error message
 *
 * Convenience function to create and return an invalid options error
 */
DBusMessage * wpas_dbus_error_invalid_args(DBusMessage *message,
					  const char *arg)
{
	DBusMessage *reply;

	reply = dbus_message_new_error(message, WPAS_DBUS_ERROR_INVALID_ARGS,
				       "Did not receive correct message "
				       "arguments.");
	if (arg != NULL)
		dbus_message_append_args(reply, DBUS_TYPE_STRING, &arg,
					 DBUS_TYPE_INVALID);

	return reply;
}


static const char *dont_quote[] = {
	"key_mgmt", "proto", "pairwise", "auth_alg", "group", "eap",
	"opensc_engine_path", "pkcs11_engine_path", "pkcs11_module_path",
	"bssid", NULL
};

static dbus_bool_t should_quote_opt(const char *key)
{
	int i = 0;
	while (dont_quote[i] != NULL) {
		if (os_strcmp(key, dont_quote[i]) == 0)
			return FALSE;
		i++;
	}
	return TRUE;
}

/**
 * get_iface_by_dbus_path - Get a new network interface
 * @global: Pointer to global data from wpa_supplicant_init()
 * @path: Pointer to a dbus object path representing an interface
 * Returns: Pointer to the interface or %NULL if not found
 */
static struct wpa_supplicant * get_iface_by_dbus_path(
	struct wpa_global *global, const char *path)
{
	struct wpa_supplicant *wpa_s;

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_strcmp(wpa_s->dbus_new_path, path) == 0)
			return wpa_s;
	}
	return NULL;
}


/**
 * set_network_properties - Set properties of a configured network
 * @wpa_s: wpa_supplicant structure for a network interface
 * @ssid: wpa_ssid structure for a configured network
 * @iter: DBus message iterator containing dictionary of network
 * properties to set.
 * @error: On failure, an error describing the failure
 * Returns: TRUE if the request succeeds, FALSE if it failed
 *
 * Sets network configuration with parameters given id DBus dictionary
 */
dbus_bool_t set_network_properties(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid,
				   DBusMessageIter *iter,
				   DBusError *error)
{
	struct wpa_dbus_dict_entry entry = { .type = DBUS_TYPE_STRING };
	DBusMessageIter	iter_dict;
	char *value = NULL;

	if (!wpa_dbus_dict_open_read(iter, &iter_dict, error))
		return FALSE;

	while (wpa_dbus_dict_has_dict_entry(&iter_dict)) {
		size_t size = 50;
		int ret;

		if (!wpa_dbus_dict_get_entry(&iter_dict, &entry))
			goto error;

		value = NULL;
		if (entry.type == DBUS_TYPE_ARRAY &&
		    entry.array_type == DBUS_TYPE_BYTE) {
			if (entry.array_len <= 0)
				goto error;

			size = entry.array_len * 2 + 1;
			value = os_zalloc(size);
			if (value == NULL)
				goto error;

			ret = wpa_snprintf_hex(value, size,
					       (u8 *) entry.bytearray_value,
					       entry.array_len);
			if (ret <= 0)
				goto error;
		} else if (entry.type == DBUS_TYPE_STRING) {
			if (should_quote_opt(entry.key)) {
				size = os_strlen(entry.str_value);
				if (size <= 0)
					goto error;

				size += 3;
				value = os_zalloc(size);
				if (value == NULL)
					goto error;

				ret = os_snprintf(value, size, "\"%s\"",
						  entry.str_value);
				if (ret < 0 || (size_t) ret != (size - 1))
					goto error;
			} else {
				value = os_strdup(entry.str_value);
				if (value == NULL)
					goto error;
			}
		} else if (entry.type == DBUS_TYPE_UINT32) {
			value = os_zalloc(size);
			if (value == NULL)
				goto error;

			ret = os_snprintf(value, size, "%u",
					  entry.uint32_value);
			if (ret <= 0)
				goto error;
		} else if (entry.type == DBUS_TYPE_INT32) {
			value = os_zalloc(size);
			if (value == NULL)
				goto error;

			ret = os_snprintf(value, size, "%d",
					  entry.int32_value);
			if (ret <= 0)
				goto error;
		} else
			goto error;

		if (wpa_config_set(ssid, entry.key, value, 0) < 0)
			goto error;

		if ((os_strcmp(entry.key, "psk") == 0 &&
		     value[0] == '"' && ssid->ssid_len) ||
		    (strcmp(entry.key, "ssid") == 0 && ssid->passphrase))
			wpa_config_update_psk(ssid);
		else if (os_strcmp(entry.key, "priority") == 0)
			wpa_config_update_prio_list(wpa_s->conf);

		os_free(value);
		wpa_dbus_dict_entry_clear(&entry);
	}

	return TRUE;

error:
	os_free(value);
	wpa_dbus_dict_entry_clear(&entry);
	dbus_set_error_const(error, DBUS_ERROR_INVALID_ARGS,
			     "invalid message format");
	return FALSE;
}


/**
 * wpas_dbus_simple_property_getter - Get basic type property
 * @iter: Message iter to use when appending arguments
 * @type: DBus type of property (must be basic type)
 * @val: pointer to place holding property value
 * @error: On failure an error describing the failure
 * Returns: TRUE if the request was successful, FALSE if it failed
 *
 * Generic getter for basic type properties. Type is required to be basic.
 */
dbus_bool_t wpas_dbus_simple_property_getter(DBusMessageIter *iter,
					     const int type,
					     const void *val,
					     DBusError *error)
{
	DBusMessageIter variant_iter;

	if (!dbus_type_is_basic(type)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
		               "%s: given type is not basic", __func__);
		return FALSE;
	}

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
	                                      wpa_dbus_type_as_string(type),
	                                      &variant_iter))
		goto error;

	if (!dbus_message_iter_append_basic(&variant_iter, type, val))
		goto error;

	if (!dbus_message_iter_close_container(iter, &variant_iter))
		goto error;

	return TRUE;

error:
	dbus_set_error(error, DBUS_ERROR_FAILED,
	               "%s: error constructing reply", __func__);
	return FALSE;
}


/**
 * wpas_dbus_simple_property_setter - Set basic type property
 * @message: Pointer to incoming dbus message
 * @type: DBus type of property (must be basic type)
 * @val: pointer to place where value being set will be stored
 * Returns: TRUE if the request was successful, FALSE if it failed
 *
 * Generic setter for basic type properties. Type is required to be basic.
 */
dbus_bool_t wpas_dbus_simple_property_setter(DBusMessageIter *iter,
					     DBusError *error,
					     const int type, void *val)
{
	DBusMessageIter variant_iter;

	if (!dbus_type_is_basic(type)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
			       "%s: given type is not basic", __func__);
		return FALSE;
	}

	/* Look at the new value */
	dbus_message_iter_recurse(iter, &variant_iter);
	if (dbus_message_iter_get_arg_type(&variant_iter) != type) {
		dbus_set_error_const(error, DBUS_ERROR_FAILED,
				     "wrong property type");
		return FALSE;
	}
	dbus_message_iter_get_basic(&variant_iter, val);

	return TRUE;
}


/**
 * wpas_dbus_simple_array_property_getter - Get array type property
 * @iter: Pointer to incoming dbus message iterator
 * @type: DBus type of property array elements (must be basic type)
 * @array: pointer to array of elements to put into response message
 * @array_len: length of above array
 * @error: a pointer to an error to fill on failure
 * Returns: TRUE if the request succeeded, FALSE if it failed
 *
 * Generic getter for array type properties. Array elements type is
 * required to be basic.
 */
dbus_bool_t wpas_dbus_simple_array_property_getter(DBusMessageIter *iter,
						   const int type,
						   const void *array,
						   size_t array_len,
						   DBusError *error)
{
	DBusMessageIter variant_iter, array_iter;
	char type_str[] = "a?"; /* ? will be replaced with subtype letter; */
	const char *sub_type_str;
	size_t element_size, i;

	if (!dbus_type_is_basic(type)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
		               "%s: given type is not basic", __func__);
		return FALSE;
	}

	sub_type_str = wpa_dbus_type_as_string(type);
	type_str[1] = sub_type_str[0];

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
					      type_str, &variant_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
		               "%s: failed to construct message 1", __func__);
		return FALSE;
	}

	if (!dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY,
					      sub_type_str, &array_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
		               "%s: failed to construct message 2", __func__);
		return FALSE;
	}

	switch(type) {
	case DBUS_TYPE_BYTE:
	case DBUS_TYPE_BOOLEAN:
		element_size = 1;
		break;
	case DBUS_TYPE_INT16:
	case DBUS_TYPE_UINT16:
		element_size = sizeof(uint16_t);
		break;
	case DBUS_TYPE_INT32:
	case DBUS_TYPE_UINT32:
		element_size = sizeof(uint32_t);
		break;
	case DBUS_TYPE_INT64:
	case DBUS_TYPE_UINT64:
		element_size = sizeof(uint64_t);
		break;
	case DBUS_TYPE_DOUBLE:
		element_size = sizeof(double);
		break;
	case DBUS_TYPE_STRING:
	case DBUS_TYPE_OBJECT_PATH:
		element_size = sizeof(char *);
		break;
	default:
		dbus_set_error(error, DBUS_ERROR_FAILED,
		               "%s: unknown element type %d", __func__, type);
		return FALSE;
	}

	for (i = 0; i < array_len; i++) {
		dbus_message_iter_append_basic(&array_iter, type,
					       array + i * element_size);
	}

	if (!dbus_message_iter_close_container(&variant_iter, &array_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
		               "%s: failed to construct message 3", __func__);
		return FALSE;
	}

	if (!dbus_message_iter_close_container(iter, &variant_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
		               "%s: failed to construct message 4", __func__);
		return FALSE;
	}

	return TRUE;
}


/**
 * wpas_dbus_simple_array_array_property_getter - Get array array type property
 * @iter: Pointer to incoming dbus message iterator
 * @type: DBus type of property array elements (must be basic type)
 * @array: pointer to array of elements to put into response message
 * @array_len: length of above array
 * @error: a pointer to an error to fill on failure
 * Returns: TRUE if the request succeeded, FALSE if it failed
 *
 * Generic getter for array type properties. Array elements type is
 * required to be basic.
 */
dbus_bool_t wpas_dbus_simple_array_array_property_getter(DBusMessageIter *iter,
							 const int type,
							 struct wpabuf **array,
							 size_t array_len,
							 DBusError *error)
{
	DBusMessageIter variant_iter, array_iter;
	char type_str[] = "aa?";
	char inner_type_str[] = "a?";
	const char *sub_type_str;
	size_t i;

	if (!dbus_type_is_basic(type)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
			       "%s: given type is not basic", __func__);
		return FALSE;
	}

	sub_type_str = wpa_dbus_type_as_string(type);
	type_str[2] = sub_type_str[0];
	inner_type_str[1] = sub_type_str[0];

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
					      type_str, &variant_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
			       "%s: failed to construct message 1", __func__);
		return FALSE;
	}
	if (!dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY,
					      inner_type_str, &array_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
			       "%s: failed to construct message 2", __func__);
		return FALSE;
	}

	for (i = 0; i < array_len; i++) {
		wpa_dbus_dict_bin_array_add_element(&array_iter,
						    wpabuf_head(array[i]),
						    wpabuf_len(array[i]));

	}

	if (!dbus_message_iter_close_container(&variant_iter, &array_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
			       "%s: failed to close message 2", __func__);
		return FALSE;
	}

	if (!dbus_message_iter_close_container(iter, &variant_iter)) {
		dbus_set_error(error, DBUS_ERROR_FAILED,
			       "%s: failed to close message 1", __func__);
		return FALSE;
	}

	return TRUE;
}


/**
 * wpas_dbus_handler_create_interface - Request registration of a network iface
 * @message: Pointer to incoming dbus message
 * @global: %wpa_supplicant global data structure
 * Returns: The object path of the new interface object,
 *          or a dbus error message with more information
 *
 * Handler function for "CreateInterface" method call. Handles requests
 * by dbus clients to register a network interface that wpa_supplicant
 * will manage.
 */
DBusMessage * wpas_dbus_handler_create_interface(DBusMessage *message,
						 struct wpa_global *global)
{
	DBusMessageIter iter_dict;
	DBusMessage *reply = NULL;
	DBusMessageIter iter;
	struct wpa_dbus_dict_entry entry;
	char *driver = NULL;
	char *ifname = NULL;
	char *confname = NULL;
	char *bridge_ifname = NULL;

	dbus_message_iter_init(message, &iter);

	if (!wpa_dbus_dict_open_read(&iter, &iter_dict, NULL))
		goto error;
	while (wpa_dbus_dict_has_dict_entry(&iter_dict)) {
		if (!wpa_dbus_dict_get_entry(&iter_dict, &entry))
			goto error;
		if (!strcmp(entry.key, "Driver") &&
		    (entry.type == DBUS_TYPE_STRING)) {
			driver = os_strdup(entry.str_value);
			wpa_dbus_dict_entry_clear(&entry);
			if (driver == NULL)
				goto error;
		} else if (!strcmp(entry.key, "Ifname") &&
			   (entry.type == DBUS_TYPE_STRING)) {
			ifname = os_strdup(entry.str_value);
			wpa_dbus_dict_entry_clear(&entry);
			if (ifname == NULL)
				goto error;
		} else if (!strcmp(entry.key, "ConfigFile") &&
			   (entry.type == DBUS_TYPE_STRING)) {
			confname = os_strdup(entry.str_value);
			wpa_dbus_dict_entry_clear(&entry);
			if (confname == NULL)
				goto error;
		} else if (!strcmp(entry.key, "BridgeIfname") &&
			   (entry.type == DBUS_TYPE_STRING)) {
			bridge_ifname = os_strdup(entry.str_value);
			wpa_dbus_dict_entry_clear(&entry);
			if (bridge_ifname == NULL)
				goto error;
		} else {
			wpa_dbus_dict_entry_clear(&entry);
			goto error;
		}
	}

	if (ifname == NULL)
		goto error; /* Required Ifname argument missing */

	/*
	 * Try to get the wpa_supplicant record for this iface, return
	 * an error if we already control it.
	 */
	if (wpa_supplicant_get_iface(global, ifname) != NULL) {
		reply = dbus_message_new_error(message,
					       WPAS_DBUS_ERROR_IFACE_EXISTS,
					       "wpa_supplicant already "
					       "controls this interface.");
	} else {
		struct wpa_supplicant *wpa_s;
		struct wpa_interface iface;
		os_memset(&iface, 0, sizeof(iface));
		iface.driver = driver;
		iface.ifname = ifname;
		iface.confname = confname;
		iface.bridge_ifname = bridge_ifname;
		/* Otherwise, have wpa_supplicant attach to it. */
		if ((wpa_s = wpa_supplicant_add_iface(global, &iface))) {
			const char *path = wpa_s->dbus_new_path;
			reply = dbus_message_new_method_return(message);
			dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH,
			                         &path, DBUS_TYPE_INVALID);
		} else {
			reply = wpas_dbus_error_unknown_error(
				message, "wpa_supplicant couldn't grab this "
				"interface.");
		}
	}

out:
	os_free(driver);
	os_free(ifname);
	os_free(bridge_ifname);
	return reply;

error:
	reply = wpas_dbus_error_invalid_args(message, NULL);
	goto out;
}


/**
 * wpas_dbus_handler_remove_interface - Request deregistration of an interface
 * @message: Pointer to incoming dbus message
 * @global: wpa_supplicant global data structure
 * Returns: a dbus message containing a UINT32 indicating success (1) or
 *          failure (0), or returns a dbus error message with more information
 *
 * Handler function for "removeInterface" method call.  Handles requests
 * by dbus clients to deregister a network interface that wpa_supplicant
 * currently manages.
 */
DBusMessage * wpas_dbus_handler_remove_interface(DBusMessage *message,
						 struct wpa_global *global)
{
	struct wpa_supplicant *wpa_s;
	char *path;
	DBusMessage *reply = NULL;

	dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH, &path,
			      DBUS_TYPE_INVALID);

	wpa_s = get_iface_by_dbus_path(global, path);
	if (wpa_s == NULL)
		reply = wpas_dbus_error_iface_unknown(message);
	else if (wpa_supplicant_remove_iface(global, wpa_s)) {
		reply = wpas_dbus_error_unknown_error(
			message, "wpa_supplicant couldn't remove this "
			"interface.");
	}

	return reply;
}


/**
 * wpas_dbus_handler_get_interface - Get the object path for an interface name
 * @message: Pointer to incoming dbus message
 * @global: %wpa_supplicant global data structure
 * Returns: The object path of the interface object,
 *          or a dbus error message with more information
 *
 * Handler function for "getInterface" method call.
 */
DBusMessage * wpas_dbus_handler_get_interface(DBusMessage *message,
					      struct wpa_global *global)
{
	DBusMessage *reply = NULL;
	const char *ifname;
	const char *path;
	struct wpa_supplicant *wpa_s;

	dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &ifname,
			      DBUS_TYPE_INVALID);

	wpa_s = wpa_supplicant_get_iface(global, ifname);
	if (wpa_s == NULL)
		return wpas_dbus_error_iface_unknown(message);

	path = wpa_s->dbus_new_path;
	reply = dbus_message_new_method_return(message);
	if (reply == NULL)
		return dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					      NULL);
	if (!dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &path,
				      DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		return dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					      NULL);
	}

	return reply;
}


/**
 * wpas_dbus_getter_debug_level - Get debug level
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "DebugLevel" property.
 */
dbus_bool_t wpas_dbus_getter_debug_level(DBusMessageIter *iter,
					 DBusError *error,
					 void *user_data)
{
	const char *str;
	int idx = wpa_debug_level;

	if (idx < 0)
		idx = 0;
	if (idx > 5)
		idx = 5;
	str = debug_strings[idx];
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						&str, error);
}


/**
 * wpas_dbus_getter_debug_timestamp - Get debug timestamp
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "DebugTimestamp" property.
 */
dbus_bool_t wpas_dbus_getter_debug_timestamp(DBusMessageIter *iter,
                                             DBusError *error,
                                             void *user_data)
{
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_BOOLEAN,
						&wpa_debug_timestamp, error);

}


/**
 * wpas_dbus_getter_debug_show_keys - Get debug show keys
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "DebugShowKeys" property.
 */
dbus_bool_t wpas_dbus_getter_debug_show_keys(DBusMessageIter *iter,
					     DBusError *error,
					     void *user_data)
{
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_BOOLEAN,
						&wpa_debug_show_keys, error);

}

/**
 * wpas_dbus_setter_debug_level - Set debug level
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter for "DebugLevel" property.
 */
dbus_bool_t wpas_dbus_setter_debug_level(DBusMessageIter *iter,
					 DBusError *error, void *user_data)
{
	struct wpa_global *global = user_data;
	const char *str = NULL;
	int i, val = -1;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_STRING,
					      &str))
		return FALSE;

	for (i = 0; debug_strings[i]; i++)
		if (os_strcmp(debug_strings[i], str) == 0) {
			val = i;
			break;
		}

	if (val < 0 ||
	    wpa_supplicant_set_debug_params(global, val, wpa_debug_timestamp,
					    wpa_debug_show_keys)) {
		dbus_set_error_const(error, DBUS_ERROR_FAILED, "wrong debug "
				     "level value");
		return FALSE;
	}

	return TRUE;
}


/**
 * wpas_dbus_setter_debug_timestamp - Set debug timestamp
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter for "DebugTimestamp" property.
 */
dbus_bool_t wpas_dbus_setter_debug_timestamp(DBusMessageIter *iter,
					     DBusError *error,
					     void *user_data)
{
	struct wpa_global *global = user_data;
	dbus_bool_t val;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_BOOLEAN,
					      &val))
		return FALSE;

	wpa_supplicant_set_debug_params(global, wpa_debug_level, val ? 1 : 0,
					wpa_debug_show_keys);
	return TRUE;
}


/**
 * wpas_dbus_setter_debug_show_keys - Set debug show keys
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter for "DebugShowKeys" property.
 */
dbus_bool_t wpas_dbus_setter_debug_show_keys(DBusMessageIter *iter,
					     DBusError *error,
					     void *user_data)
{
	struct wpa_global *global = user_data;
	dbus_bool_t val;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_BOOLEAN,
					      &val))
		return FALSE;

	wpa_supplicant_set_debug_params(global, wpa_debug_level,
					wpa_debug_timestamp,
					val ? 1 : 0);
	return TRUE;
}


/**
 * wpas_dbus_getter_interfaces - Request registered interfaces list
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Interfaces" property. Handles requests
 * by dbus clients to return list of registered interfaces objects
 * paths
 */
dbus_bool_t wpas_dbus_getter_interfaces(DBusMessageIter *iter,
					DBusError *error,
					void *user_data)
{
	struct wpa_global *global = user_data;
	struct wpa_supplicant *wpa_s;
	const char **paths;
	unsigned int i = 0, num = 0;
	dbus_bool_t success;

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next)
		num++;

	paths = os_zalloc(num * sizeof(char*));
	if (!paths) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next)
		paths[i++] = wpa_s->dbus_new_path;

	success = wpas_dbus_simple_array_property_getter(iter,
							 DBUS_TYPE_OBJECT_PATH,
							 paths, num, error);

	os_free(paths);
	return success;
}


/**
 * wpas_dbus_getter_eap_methods - Request supported EAP methods list
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "EapMethods" property. Handles requests
 * by dbus clients to return list of strings with supported EAP methods
 */
dbus_bool_t wpas_dbus_getter_eap_methods(DBusMessageIter *iter,
					 DBusError *error, void *user_data)
{
	char **eap_methods;
	size_t num_items = 0;
	dbus_bool_t success;

	eap_methods = eap_get_names_as_string_array(&num_items);
	if (!eap_methods) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	success = wpas_dbus_simple_array_property_getter(iter,
							 DBUS_TYPE_STRING,
							 eap_methods,
							 num_items, error);

	while (num_items)
		os_free(eap_methods[--num_items]);
	os_free(eap_methods);
	return success;
}


static int wpas_dbus_get_scan_type(DBusMessage *message, DBusMessageIter *var,
				   char **type, DBusMessage **reply)
{
	if (dbus_message_iter_get_arg_type(var) != DBUS_TYPE_STRING) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
			   "Type must be a string");
		*reply = wpas_dbus_error_invalid_args(
			message, "Wrong Type value type. String required");
		return -1;
	}
	dbus_message_iter_get_basic(var, type);
	return 0;
}


static int wpas_dbus_get_scan_ssids(DBusMessage *message, DBusMessageIter *var,
				    struct wpa_driver_scan_params *params,
				    DBusMessage **reply)
{
	struct wpa_driver_scan_ssid *ssids = params->ssids;
	size_t ssids_num = 0;
	u8 *ssid;
	DBusMessageIter array_iter, sub_array_iter;
	char *val;
	int len;

	if (dbus_message_iter_get_arg_type(var) != DBUS_TYPE_ARRAY) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: ssids "
			   "must be an array of arrays of bytes");
		*reply = wpas_dbus_error_invalid_args(
			message, "Wrong SSIDs value type. Array of arrays of "
			"bytes required");
		return -1;
	}

	dbus_message_iter_recurse(var, &array_iter);

	if (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_ARRAY ||
	    dbus_message_iter_get_element_type(&array_iter) != DBUS_TYPE_BYTE)
	{
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: ssids "
			   "must be an array of arrays of bytes");
		*reply = wpas_dbus_error_invalid_args(
			message, "Wrong SSIDs value type. Array of arrays of "
			"bytes required");
		return -1;
	}

	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_ARRAY)
	{
		if (ssids_num >= WPAS_MAX_SCAN_SSIDS) {
			wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
				   "Too many ssids specified on scan dbus "
				   "call");
			*reply = wpas_dbus_error_invalid_args(
				message, "Too many ssids specified. Specify "
				"at most four");
			return -1;
		}

		dbus_message_iter_recurse(&array_iter, &sub_array_iter);

		dbus_message_iter_get_fixed_array(&sub_array_iter, &val, &len);

		if (len > MAX_SSID_LEN) {
			wpa_printf(MSG_DEBUG,
				   "wpas_dbus_handler_scan[dbus]: "
				   "SSID too long (len=%d max_len=%d)",
				   len, MAX_SSID_LEN);
			*reply = wpas_dbus_error_invalid_args(
				message, "Invalid SSID: too long");
			return -1;
		}

		if (len != 0) {
			ssid = os_malloc(len);
			if (ssid == NULL) {
				wpa_printf(MSG_DEBUG,
					   "wpas_dbus_handler_scan[dbus]: "
					   "out of memory. Cannot allocate "
					   "memory for SSID");
				*reply = dbus_message_new_error(
					message, DBUS_ERROR_NO_MEMORY, NULL);
				return -1;
			}
			os_memcpy(ssid, val, len);
		} else {
			/* Allow zero-length SSIDs */
			ssid = NULL;
		}

		ssids[ssids_num].ssid = ssid;
		ssids[ssids_num].ssid_len = len;

		dbus_message_iter_next(&array_iter);
		ssids_num++;
	}

	params->num_ssids = ssids_num;
	return 0;
}


static int wpas_dbus_get_scan_ies(DBusMessage *message, DBusMessageIter *var,
				  struct wpa_driver_scan_params *params,
				  DBusMessage **reply)
{
	u8 *ies = NULL, *nies;
	int ies_len = 0;
	DBusMessageIter array_iter, sub_array_iter;
	char *val;
	int len;

	if (dbus_message_iter_get_arg_type(var) != DBUS_TYPE_ARRAY) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: ies must "
			   "be an array of arrays of bytes");
		*reply = wpas_dbus_error_invalid_args(
			message, "Wrong IEs value type. Array of arrays of "
			"bytes required");
		return -1;
	}

	dbus_message_iter_recurse(var, &array_iter);

	if (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_ARRAY ||
	    dbus_message_iter_get_element_type(&array_iter) != DBUS_TYPE_BYTE)
	{
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: ies must "
			   "be an array of arrays of bytes");
		*reply = wpas_dbus_error_invalid_args(
			message, "Wrong IEs value type. Array required");
		return -1;
	}

	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_ARRAY)
	{
		dbus_message_iter_recurse(&array_iter, &sub_array_iter);

		dbus_message_iter_get_fixed_array(&sub_array_iter, &val, &len);
		if (len == 0) {
			dbus_message_iter_next(&array_iter);
			continue;
		}

		nies = os_realloc(ies, ies_len + len);
		if (nies == NULL) {
			wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
				   "out of memory. Cannot allocate memory for "
				   "IE");
			os_free(ies);
			*reply = dbus_message_new_error(
				message, DBUS_ERROR_NO_MEMORY, NULL);
			return -1;
		}
		ies = nies;
		os_memcpy(ies + ies_len, val, len);
		ies_len += len;

		dbus_message_iter_next(&array_iter);
	}

	params->extra_ies = ies;
	params->extra_ies_len = ies_len;
	return 0;
}


static int wpas_dbus_get_scan_channels(DBusMessage *message,
				       DBusMessageIter *var,
				       struct wpa_driver_scan_params *params,
				       DBusMessage **reply)
{
	DBusMessageIter array_iter, sub_array_iter;
	int *freqs = NULL, *nfreqs;
	int freqs_num = 0;

	if (dbus_message_iter_get_arg_type(var) != DBUS_TYPE_ARRAY) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
			   "Channels must be an array of structs");
		*reply = wpas_dbus_error_invalid_args(
			message, "Wrong Channels value type. Array of structs "
			"required");
		return -1;
	}

	dbus_message_iter_recurse(var, &array_iter);

	if (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_STRUCT) {
		wpa_printf(MSG_DEBUG,
			   "wpas_dbus_handler_scan[dbus]: Channels must be an "
			   "array of structs");
		*reply = wpas_dbus_error_invalid_args(
			message, "Wrong Channels value type. Array of structs "
			"required");
		return -1;
	}

	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT)
	{
		int freq, width;

		dbus_message_iter_recurse(&array_iter, &sub_array_iter);

		if (dbus_message_iter_get_arg_type(&sub_array_iter) !=
		    DBUS_TYPE_UINT32) {
			wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
				   "Channel must by specified by struct of "
				   "two UINT32s %c",
				   dbus_message_iter_get_arg_type(
					   &sub_array_iter));
			*reply = wpas_dbus_error_invalid_args(
				message, "Wrong Channel struct. Two UINT32s "
				"required");
			os_free(freqs);
			return -1;
		}
		dbus_message_iter_get_basic(&sub_array_iter, &freq);

		if (!dbus_message_iter_next(&sub_array_iter) ||
		    dbus_message_iter_get_arg_type(&sub_array_iter) !=
		    DBUS_TYPE_UINT32) {
			wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
				   "Channel must by specified by struct of "
				   "two UINT32s");
			*reply = wpas_dbus_error_invalid_args(
				message,
				"Wrong Channel struct. Two UINT32s required");
			os_free(freqs);
			return -1;
		}

		dbus_message_iter_get_basic(&sub_array_iter, &width);

#define FREQS_ALLOC_CHUNK 32
		if (freqs_num % FREQS_ALLOC_CHUNK == 0) {
			nfreqs = os_realloc(freqs, sizeof(int) *
					    (freqs_num + FREQS_ALLOC_CHUNK));
			if (nfreqs == NULL)
				os_free(freqs);
			freqs = nfreqs;
		}
		if (freqs == NULL) {
			wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
				   "out of memory. can't allocate memory for "
				   "freqs");
			*reply = dbus_message_new_error(
				message, DBUS_ERROR_NO_MEMORY, NULL);
			return -1;
		}

		freqs[freqs_num] = freq;

		freqs_num++;
		dbus_message_iter_next(&array_iter);
	}

	nfreqs = os_realloc(freqs,
			    sizeof(int) * (freqs_num + 1));
	if (nfreqs == NULL)
		os_free(freqs);
	freqs = nfreqs;
	if (freqs == NULL) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
			   "out of memory. Can't allocate memory for freqs");
		*reply = dbus_message_new_error(
			message, DBUS_ERROR_NO_MEMORY, NULL);
		return -1;
	}
	freqs[freqs_num] = 0;

	params->freqs = freqs;
	return 0;
}


/**
 * wpas_dbus_handler_scan - Request a wireless scan on an interface
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: NULL indicating success or DBus error message on failure
 *
 * Handler function for "Scan" method call of a network device. Requests
 * that wpa_supplicant perform a wireless scan as soon as possible
 * on a particular wireless interface.
 */
DBusMessage * wpas_dbus_handler_scan(DBusMessage *message,
				     struct wpa_supplicant *wpa_s)
{
	DBusMessage *reply = NULL;
	DBusMessageIter iter, dict_iter, entry_iter, variant_iter;
	char *key = NULL, *type = NULL;
	struct wpa_driver_scan_params params;
	size_t i;

	os_memset(&params, 0, sizeof(params));

	dbus_message_iter_init(message, &iter);

	dbus_message_iter_recurse(&iter, &dict_iter);

	while (dbus_message_iter_get_arg_type(&dict_iter) ==
			DBUS_TYPE_DICT_ENTRY) {
		dbus_message_iter_recurse(&dict_iter, &entry_iter);
		dbus_message_iter_get_basic(&entry_iter, &key);
		dbus_message_iter_next(&entry_iter);
		dbus_message_iter_recurse(&entry_iter, &variant_iter);

		if (os_strcmp(key, "Type") == 0) {
			if (wpas_dbus_get_scan_type(message, &variant_iter,
						    &type, &reply) < 0)
				goto out;
		} else if (os_strcmp(key, "SSIDs") == 0) {
			if (wpas_dbus_get_scan_ssids(message, &variant_iter,
						     &params, &reply) < 0)
				goto out;
		} else if (os_strcmp(key, "IEs") == 0) {
			if (wpas_dbus_get_scan_ies(message, &variant_iter,
						   &params, &reply) < 0)
				goto out;
		} else if (os_strcmp(key, "Channels") == 0) {
			if (wpas_dbus_get_scan_channels(message, &variant_iter,
							&params, &reply) < 0)
				goto out;
		} else {
			wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
				   "Unknown argument %s", key);
			reply = wpas_dbus_error_invalid_args(message, key);
			goto out;
		}

		dbus_message_iter_next(&dict_iter);
	}

	if (!type) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
			   "Scan type not specified");
		reply = wpas_dbus_error_invalid_args(message, key);
		goto out;
	}

	if (!os_strcmp(type, "passive")) {
		if (params.num_ssids || params.extra_ies_len) {
			wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
				   "SSIDs or IEs specified for passive scan.");
			reply = wpas_dbus_error_invalid_args(
				message, "You can specify only Channels in "
				"passive scan");
			goto out;
		} else if (params.freqs && params.freqs[0]) {
			wpa_supplicant_trigger_scan(wpa_s, &params);
		} else {
			wpa_s->scan_req = 2;
			wpa_supplicant_req_scan(wpa_s, 0, 0);
		}
	} else if (!os_strcmp(type, "active")) {
		if (!params.num_ssids) {
			/* Add wildcard ssid */
			params.num_ssids++;
		}
		wpa_supplicant_trigger_scan(wpa_s, &params);
	} else {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_scan[dbus]: "
			   "Unknown scan type: %s", type);
		reply = wpas_dbus_error_invalid_args(message,
						     "Wrong scan type");
		goto out;
	}

out:
	for (i = 0; i < WPAS_MAX_SCAN_SSIDS; i++)
		os_free((u8 *) params.ssids[i].ssid);
	os_free((u8 *) params.extra_ies);
	os_free(params.freqs);
	return reply;
}


/*
 * wpas_dbus_handler_disconnect - Terminate the current connection
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: NotConnected DBus error message if already not connected
 * or NULL otherwise.
 *
 * Handler function for "Disconnect" method call of network interface.
 */
DBusMessage * wpas_dbus_handler_disconnect(DBusMessage *message,
					   struct wpa_supplicant *wpa_s)
{
	if (wpa_s->current_ssid != NULL) {
		wpa_s->disconnected = 1;
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);

		return NULL;
	}

	return dbus_message_new_error(message, WPAS_DBUS_ERROR_NOT_CONNECTED,
				      "This interface is not connected");
}


/**
 * wpas_dbus_new_iface_add_network - Add a new configured network
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: A dbus message containing the object path of the new network
 *
 * Handler function for "AddNetwork" method call of a network interface.
 */
DBusMessage * wpas_dbus_handler_add_network(DBusMessage *message,
					    struct wpa_supplicant *wpa_s)
{
	DBusMessage *reply = NULL;
	DBusMessageIter	iter;
	struct wpa_ssid *ssid = NULL;
	char path_buf[WPAS_DBUS_OBJECT_PATH_MAX], *path = path_buf;
	DBusError error;

	dbus_message_iter_init(message, &iter);

	ssid = wpa_config_add_network(wpa_s->conf);
	if (ssid == NULL) {
		wpa_printf(MSG_ERROR, "wpas_dbus_handler_add_network[dbus]: "
			   "can't add new interface.");
		reply = wpas_dbus_error_unknown_error(
			message,
			"wpa_supplicant could not add "
			"a network on this interface.");
		goto err;
	}
	wpas_notify_network_added(wpa_s, ssid);
	ssid->disabled = 1;
	wpa_config_set_network_defaults(ssid);

	dbus_error_init(&error);
	if (!set_network_properties(wpa_s, ssid, &iter, &error)) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_handler_add_network[dbus]:"
			   "control interface couldn't set network "
			   "properties");
		reply = wpas_dbus_reply_new_from_error(message, &error,
						       DBUS_ERROR_INVALID_ARGS,
						       "Failed to add network");
		dbus_error_free(&error);
		goto err;
	}

	/* Construct the object path for this network. */
	os_snprintf(path, WPAS_DBUS_OBJECT_PATH_MAX,
		    "%s/" WPAS_DBUS_NEW_NETWORKS_PART "/%d",
		    wpa_s->dbus_new_path, ssid->id);

	reply = dbus_message_new_method_return(message);
	if (reply == NULL) {
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto err;
	}
	if (!dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &path,
				      DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto err;
	}

	return reply;

err:
	if (ssid) {
		wpas_notify_network_removed(wpa_s, ssid);
		wpa_config_remove_network(wpa_s->conf, ssid->id);
	}
	return reply;
}


/**
 * wpas_dbus_handler_remove_network - Remove a configured network
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: NULL on success or dbus error on failure
 *
 * Handler function for "RemoveNetwork" method call of a network interface.
 */
DBusMessage * wpas_dbus_handler_remove_network(DBusMessage *message,
					       struct wpa_supplicant *wpa_s)
{
	DBusMessage *reply = NULL;
	const char *op;
	char *iface = NULL, *net_id = NULL;
	int id;
	struct wpa_ssid *ssid;

	dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH, &op,
			      DBUS_TYPE_INVALID);

	/* Extract the network ID and ensure the network */
	/* is actually a child of this interface */
	iface = wpas_dbus_new_decompose_object_path(op, 0, &net_id, NULL);
	if (iface == NULL || os_strcmp(iface, wpa_s->dbus_new_path) != 0) {
		reply = wpas_dbus_error_invalid_args(message, op);
		goto out;
	}

	id = strtoul(net_id, NULL, 10);
	if (errno == EINVAL) {
		reply = wpas_dbus_error_invalid_args(message, op);
		goto out;
	}

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL) {
		reply = wpas_dbus_error_network_unknown(message);
		goto out;
	}

	wpas_notify_network_removed(wpa_s, ssid);

	if (wpa_config_remove_network(wpa_s->conf, id) < 0) {
		wpa_printf(MSG_ERROR,
			   "wpas_dbus_handler_remove_network[dbus]: "
			   "error occurred when removing network %d", id);
		reply = wpas_dbus_error_unknown_error(
			message, "error removing the specified network on "
			"this interface.");
		goto out;
	}

	if (ssid == wpa_s->current_ssid)
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);

out:
	os_free(iface);
	os_free(net_id);
	return reply;
}


static void remove_network(void *arg, struct wpa_ssid *ssid)
{
	struct wpa_supplicant *wpa_s = arg;

	wpas_notify_network_removed(wpa_s, ssid);

	if (wpa_config_remove_network(wpa_s->conf, ssid->id) < 0) {
		wpa_printf(MSG_ERROR,
			   "wpas_dbus_handler_remove_all_networks[dbus]: "
			   "error occurred when removing network %d",
			   ssid->id);
		return;
	}

	if (ssid == wpa_s->current_ssid)
		wpa_supplicant_disassociate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
}


/**
 * wpas_dbus_handler_remove_all_networks - Remove all configured networks
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: NULL on success or dbus error on failure
 *
 * Handler function for "RemoveAllNetworks" method call of a network interface.
 */
DBusMessage * wpas_dbus_handler_remove_all_networks(
	DBusMessage *message, struct wpa_supplicant *wpa_s)
{
	/* NB: could check for failure and return an error */
	wpa_config_foreach_network(wpa_s->conf, remove_network, wpa_s);
	return NULL;
}


/**
 * wpas_dbus_handler_select_network - Attempt association with a network
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: NULL on success or dbus error on failure
 *
 * Handler function for "SelectNetwork" method call of network interface.
 */
DBusMessage * wpas_dbus_handler_select_network(DBusMessage *message,
					       struct wpa_supplicant *wpa_s)
{
	DBusMessage *reply = NULL;
	const char *op;
	char *iface = NULL, *net_id = NULL;
	int id;
	struct wpa_ssid *ssid;

	dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH, &op,
			      DBUS_TYPE_INVALID);

	/* Extract the network ID and ensure the network */
	/* is actually a child of this interface */
	iface = wpas_dbus_new_decompose_object_path(op, 0, &net_id, NULL);
	if (iface == NULL || os_strcmp(iface, wpa_s->dbus_new_path) != 0) {
		reply = wpas_dbus_error_invalid_args(message, op);
		goto out;
	}

	id = strtoul(net_id, NULL, 10);
	if (errno == EINVAL) {
		reply = wpas_dbus_error_invalid_args(message, op);
		goto out;
	}

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL) {
		reply = wpas_dbus_error_network_unknown(message);
		goto out;
	}

	/* Finally, associate with the network */
	wpa_supplicant_select_network(wpa_s, ssid);

out:
	os_free(iface);
	os_free(net_id);
	return reply;
}


/**
 * wpas_dbus_handler_network_reply - Reply to a NetworkRequest signal
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: NULL on success or dbus error on failure
 *
 * Handler function for "NetworkReply" method call of network interface.
 */
DBusMessage * wpas_dbus_handler_network_reply(DBusMessage *message,
					      struct wpa_supplicant *wpa_s)
{
#ifdef IEEE8021X_EAPOL
	DBusMessage *reply = NULL;
	const char *op, *field, *value;
	char *iface = NULL, *net_id = NULL;
	int id;
	struct wpa_ssid *ssid;

	if (!dbus_message_get_args(message, NULL,
	                           DBUS_TYPE_OBJECT_PATH, &op,
	                           DBUS_TYPE_STRING, &field,
	                           DBUS_TYPE_STRING, &value,
			           DBUS_TYPE_INVALID))
		return wpas_dbus_error_invalid_args(message, NULL);

	/* Extract the network ID and ensure the network */
	/* is actually a child of this interface */
	iface = wpas_dbus_new_decompose_object_path(op, 0, &net_id, NULL);
	if (iface == NULL || os_strcmp(iface, wpa_s->dbus_new_path) != 0) {
		reply = wpas_dbus_error_invalid_args(message, op);
		goto out;
	}

	id = strtoul(net_id, NULL, 10);
	if (errno == EINVAL) {
		reply = wpas_dbus_error_invalid_args(message, net_id);
		goto out;
	}

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL) {
		reply = wpas_dbus_error_network_unknown(message);
		goto out;
	}

	if (wpa_supplicant_ctrl_iface_ctrl_rsp_handle(wpa_s, ssid,
						      field, value) < 0)
		reply = wpas_dbus_error_invalid_args(message, field);
	else {
		/* Tell EAP to retry immediately */
		eapol_sm_notify_ctrl_response(wpa_s->eapol);
	}

out:
	os_free(iface);
	os_free(net_id);
	return reply;
#else /* IEEE8021X_EAPOL */
	wpa_printf(MSG_DEBUG, "CTRL_IFACE: 802.1X not included");
	return wpas_dbus_error_unknown_error(message, "802.1X not included");
#endif /* IEEE8021X_EAPOL */
}


/**
 * wpas_dbus_handler_add_blob - Store named binary blob (ie, for certificates)
 * @message: Pointer to incoming dbus message
 * @wpa_s: %wpa_supplicant data structure
 * Returns: A dbus message containing an error on failure or NULL on success
 *
 * Asks wpa_supplicant to internally store a binary blobs.
 */
DBusMessage * wpas_dbus_handler_add_blob(DBusMessage *message,
					 struct wpa_supplicant *wpa_s)
{
	DBusMessage *reply = NULL;
	DBusMessageIter	iter, array_iter;

	char *blob_name;
	u8 *blob_data;
	int blob_len;
	struct wpa_config_blob *blob = NULL;

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &blob_name);

	if (wpa_config_get_blob(wpa_s->conf, blob_name)) {
		return dbus_message_new_error(message,
					      WPAS_DBUS_ERROR_BLOB_EXISTS,
					      NULL);
	}

	dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &array_iter);

	dbus_message_iter_get_fixed_array(&array_iter, &blob_data, &blob_len);

	blob = os_zalloc(sizeof(*blob));
	if (!blob) {
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto err;
	}

	blob->data = os_malloc(blob_len);
	if (!blob->data) {
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto err;
	}
	os_memcpy(blob->data, blob_data, blob_len);

	blob->len = blob_len;
	blob->name = os_strdup(blob_name);
	if (!blob->name) {
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto err;
	}

	wpa_config_set_blob(wpa_s->conf, blob);
	wpas_notify_blob_added(wpa_s, blob->name);

	return reply;

err:
	if (blob) {
		os_free(blob->name);
		os_free(blob->data);
		os_free(blob);
	}
	return reply;
}


/**
 * wpas_dbus_handler_get_blob - Get named binary blob (ie, for certificates)
 * @message: Pointer to incoming dbus message
 * @wpa_s: %wpa_supplicant data structure
 * Returns: A dbus message containing array of bytes (blob)
 *
 * Gets one wpa_supplicant's binary blobs.
 */
DBusMessage * wpas_dbus_handler_get_blob(DBusMessage *message,
					 struct wpa_supplicant *wpa_s)
{
	DBusMessage *reply = NULL;
	DBusMessageIter	iter, array_iter;

	char *blob_name;
	const struct wpa_config_blob *blob;

	dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &blob_name,
			      DBUS_TYPE_INVALID);

	blob = wpa_config_get_blob(wpa_s->conf, blob_name);
	if (!blob) {
		return dbus_message_new_error(message,
					      WPAS_DBUS_ERROR_BLOB_UNKNOWN,
					      "Blob id not set");
	}

	reply = dbus_message_new_method_return(message);
	if (!reply) {
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto out;
	}

	dbus_message_iter_init_append(reply, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
					      DBUS_TYPE_BYTE_AS_STRING,
					      &array_iter)) {
		dbus_message_unref(reply);
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto out;
	}

	if (!dbus_message_iter_append_fixed_array(&array_iter, DBUS_TYPE_BYTE,
						  &(blob->data), blob->len)) {
		dbus_message_unref(reply);
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto out;
	}

	if (!dbus_message_iter_close_container(&iter, &array_iter)) {
		dbus_message_unref(reply);
		reply = dbus_message_new_error(message, DBUS_ERROR_NO_MEMORY,
					       NULL);
		goto out;
	}

out:
	return reply;
}


/**
 * wpas_remove_handler_remove_blob - Remove named binary blob
 * @message: Pointer to incoming dbus message
 * @wpa_s: %wpa_supplicant data structure
 * Returns: NULL on success or dbus error
 *
 * Asks wpa_supplicant to internally remove a binary blobs.
 */
DBusMessage * wpas_dbus_handler_remove_blob(DBusMessage *message,
					    struct wpa_supplicant *wpa_s)
{
	DBusMessage *reply = NULL;
	char *blob_name;

	dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &blob_name,
			      DBUS_TYPE_INVALID);

	if (wpa_config_remove_blob(wpa_s->conf, blob_name)) {
		return dbus_message_new_error(message,
					      WPAS_DBUS_ERROR_BLOB_UNKNOWN,
					      "Blob id not set");
	}
	wpas_notify_blob_removed(wpa_s, blob_name);

	return reply;

}

/*
 * wpas_dbus_handler_flush_bss - Flush the BSS cache
 * @message: Pointer to incoming dbus message
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: NULL
 *
 * Handler function for "FlushBSS" method call of network interface.
 */
DBusMessage * wpas_dbus_handler_flush_bss(DBusMessage *message,
					  struct wpa_supplicant *wpa_s)
{
	dbus_uint32_t age;

	dbus_message_get_args(message, NULL, DBUS_TYPE_UINT32, &age,
			      DBUS_TYPE_INVALID);

	if (age == 0)
		wpa_bss_flush(wpa_s);
	else
		wpa_bss_flush_by_age(wpa_s, age);

	return NULL;
}


/**
 * wpas_dbus_getter_capabilities - Return interface capabilities
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Capabilities" property of an interface.
 */
dbus_bool_t wpas_dbus_getter_capabilities(DBusMessageIter *iter,
					  DBusError *error, void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	struct wpa_driver_capa capa;
	int res;
	DBusMessageIter iter_dict, iter_dict_entry, iter_dict_val, iter_array,
		variant_iter;
	const char *scans[] = { "active", "passive", "ssid" };

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
					      "a{sv}", &variant_iter))
		goto nomem;

	if (!wpa_dbus_dict_open_write(&variant_iter, &iter_dict))
		goto nomem;

	res = wpa_drv_get_capa(wpa_s, &capa);

	/***** pairwise cipher */
	if (res < 0) {
		const char *args[] = {"ccmp", "tkip", "none"};
		if (!wpa_dbus_dict_append_string_array(
			    &iter_dict, "Pairwise", args,
			    sizeof(args) / sizeof(char*)))
			goto nomem;
	} else {
		if (!wpa_dbus_dict_begin_string_array(&iter_dict, "Pairwise",
						      &iter_dict_entry,
						      &iter_dict_val,
						      &iter_array))
			goto nomem;

		if (capa.enc & WPA_DRIVER_CAPA_ENC_CCMP) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "ccmp"))
				goto nomem;
		}

		if (capa.enc & WPA_DRIVER_CAPA_ENC_TKIP) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "tkip"))
				goto nomem;
		}

		if (capa.key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_WPA_NONE) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "none"))
				goto nomem;
		}

		if (!wpa_dbus_dict_end_string_array(&iter_dict,
						    &iter_dict_entry,
						    &iter_dict_val,
						    &iter_array))
			goto nomem;
	}

	/***** group cipher */
	if (res < 0) {
		const char *args[] = {
			"ccmp", "tkip", "wep104", "wep40"
		};
		if (!wpa_dbus_dict_append_string_array(
			    &iter_dict, "Group", args,
			    sizeof(args) / sizeof(char*)))
			goto nomem;
	} else {
		if (!wpa_dbus_dict_begin_string_array(&iter_dict, "Group",
						      &iter_dict_entry,
						      &iter_dict_val,
						      &iter_array))
			goto nomem;

		if (capa.enc & WPA_DRIVER_CAPA_ENC_CCMP) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "ccmp"))
				goto nomem;
		}

		if (capa.enc & WPA_DRIVER_CAPA_ENC_TKIP) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "tkip"))
				goto nomem;
		}

		if (capa.enc & WPA_DRIVER_CAPA_ENC_WEP104) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wep104"))
				goto nomem;
		}

		if (capa.enc & WPA_DRIVER_CAPA_ENC_WEP40) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wep40"))
				goto nomem;
		}

		if (!wpa_dbus_dict_end_string_array(&iter_dict,
						    &iter_dict_entry,
						    &iter_dict_val,
						    &iter_array))
			goto nomem;
	}

	/***** key management */
	if (res < 0) {
		const char *args[] = {
			"wpa-psk", "wpa-eap", "ieee8021x", "wpa-none",
#ifdef CONFIG_WPS
			"wps",
#endif /* CONFIG_WPS */
			"none"
		};
		if (!wpa_dbus_dict_append_string_array(
			    &iter_dict, "KeyMgmt", args,
			    sizeof(args) / sizeof(char*)))
			goto nomem;
	} else {
		if (!wpa_dbus_dict_begin_string_array(&iter_dict, "KeyMgmt",
						      &iter_dict_entry,
						      &iter_dict_val,
						      &iter_array))
			goto nomem;

		if (!wpa_dbus_dict_string_array_add_element(&iter_array,
							    "none"))
			goto nomem;

		if (!wpa_dbus_dict_string_array_add_element(&iter_array,
							    "ieee8021x"))
			goto nomem;

		if (capa.key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA |
				     WPA_DRIVER_CAPA_KEY_MGMT_WPA2)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wpa-eap"))
				goto nomem;

			if (capa.key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_FT)
				if (!wpa_dbus_dict_string_array_add_element(
					    &iter_array, "wpa-ft-eap"))
					goto nomem;

/* TODO: Ensure that driver actually supports sha256 encryption. */
#ifdef CONFIG_IEEE80211W
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wpa-eap-sha256"))
				goto nomem;
#endif /* CONFIG_IEEE80211W */
		}

		if (capa.key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
				     WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wpa-psk"))
				goto nomem;

			if (capa.key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_FT_PSK)
				if (!wpa_dbus_dict_string_array_add_element(
					    &iter_array, "wpa-ft-psk"))
					goto nomem;

/* TODO: Ensure that driver actually supports sha256 encryption. */
#ifdef CONFIG_IEEE80211W
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wpa-psk-sha256"))
				goto nomem;
#endif /* CONFIG_IEEE80211W */
		}

		if (capa.key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_WPA_NONE) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wpa-none"))
				goto nomem;
		}


#ifdef CONFIG_WPS
		if (!wpa_dbus_dict_string_array_add_element(&iter_array,
							    "wps"))
			goto nomem;
#endif /* CONFIG_WPS */

		if (!wpa_dbus_dict_end_string_array(&iter_dict,
						    &iter_dict_entry,
						    &iter_dict_val,
						    &iter_array))
			goto nomem;
	}

	/***** WPA protocol */
	if (res < 0) {
		const char *args[] = { "rsn", "wpa" };
		if (!wpa_dbus_dict_append_string_array(
			    &iter_dict, "Protocol", args,
			    sizeof(args) / sizeof(char*)))
			goto nomem;
	} else {
		if (!wpa_dbus_dict_begin_string_array(&iter_dict, "Protocol",
						      &iter_dict_entry,
						      &iter_dict_val,
						      &iter_array))
			goto nomem;

		if (capa.key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
				     WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "rsn"))
				goto nomem;
		}

		if (capa.key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA |
				     WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "wpa"))
				goto nomem;
		}

		if (!wpa_dbus_dict_end_string_array(&iter_dict,
						    &iter_dict_entry,
						    &iter_dict_val,
						    &iter_array))
			goto nomem;
	}

	/***** auth alg */
	if (res < 0) {
		const char *args[] = { "open", "shared", "leap" };
		if (!wpa_dbus_dict_append_string_array(
			    &iter_dict, "AuthAlg", args,
			    sizeof(args) / sizeof(char*)))
			goto nomem;
	} else {
		if (!wpa_dbus_dict_begin_string_array(&iter_dict, "AuthAlg",
						      &iter_dict_entry,
						      &iter_dict_val,
						      &iter_array))
			goto nomem;

		if (capa.auth & (WPA_DRIVER_AUTH_OPEN)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "open"))
				goto nomem;
		}

		if (capa.auth & (WPA_DRIVER_AUTH_SHARED)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "shared"))
				goto nomem;
		}

		if (capa.auth & (WPA_DRIVER_AUTH_LEAP)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "leap"))
				goto nomem;
		}

		if (!wpa_dbus_dict_end_string_array(&iter_dict,
						    &iter_dict_entry,
						    &iter_dict_val,
						    &iter_array))
			goto nomem;
	}

	/***** Scan */
	if (!wpa_dbus_dict_append_string_array(&iter_dict, "Scan", scans,
					       sizeof(scans) / sizeof(char *)))
		goto nomem;

	/***** Modes */
	if (!wpa_dbus_dict_begin_string_array(&iter_dict, "Modes",
					      &iter_dict_entry,
					      &iter_dict_val,
					      &iter_array))
		goto nomem;

	if (!wpa_dbus_dict_string_array_add_element(
			    &iter_array, "infrastructure"))
		goto nomem;

	if (!wpa_dbus_dict_string_array_add_element(
			    &iter_array, "ad-hoc"))
		goto nomem;

	if (res >= 0) {
		if (capa.flags & (WPA_DRIVER_FLAGS_AP)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "ap"))
				goto nomem;
		}

		if (capa.flags & (WPA_DRIVER_FLAGS_P2P_CAPABLE)) {
			if (!wpa_dbus_dict_string_array_add_element(
				    &iter_array, "p2p"))
				goto nomem;
		}
	}

	if (!wpa_dbus_dict_end_string_array(&iter_dict,
					    &iter_dict_entry,
					    &iter_dict_val,
					    &iter_array))
		goto nomem;
	/***** Modes end */

	if (res >= 0) {
		dbus_int32_t max_scan_ssid = capa.max_scan_ssids;

		if (!wpa_dbus_dict_append_int32(&iter_dict, "MaxScanSSID",
						max_scan_ssid))
			goto nomem;
	}

	if (!wpa_dbus_dict_close_write(&variant_iter, &iter_dict))
		goto nomem;
	if (!dbus_message_iter_close_container(iter, &variant_iter))
		goto nomem;

	return TRUE;

nomem:
	dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
	return FALSE;
}


/**
 * wpas_dbus_getter_state - Get interface state
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "State" property.
 */
dbus_bool_t wpas_dbus_getter_state(DBusMessageIter *iter, DBusError *error,
				   void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	const char *str_state;
	char *state_ls, *tmp;
	dbus_bool_t success = FALSE;

	str_state = wpa_supplicant_state_txt(wpa_s->wpa_state);

	/* make state string lowercase to fit new DBus API convention
	 */
	state_ls = tmp = os_strdup(str_state);
	if (!tmp) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}
	while (*tmp) {
		*tmp = tolower(*tmp);
		tmp++;
	}

	success = wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						   &state_ls, error);

	os_free(state_ls);

	return success;
}


/**
 * wpas_dbus_new_iface_get_scanning - Get interface scanning state
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "scanning" property.
 */
dbus_bool_t wpas_dbus_getter_scanning(DBusMessageIter *iter, DBusError *error,
                                      void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_bool_t scanning = wpa_s->scanning ? TRUE : FALSE;

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_BOOLEAN,
						&scanning, error);
}


/**
 * wpas_dbus_getter_ap_scan - Control roaming mode
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter function for "ApScan" property.
 */
dbus_bool_t wpas_dbus_getter_ap_scan(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_uint32_t ap_scan = wpa_s->conf->ap_scan;

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_UINT32,
						&ap_scan, error);
}


/**
 * wpas_dbus_setter_ap_scan - Control roaming mode
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter function for "ApScan" property.
 */
dbus_bool_t wpas_dbus_setter_ap_scan(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_uint32_t ap_scan;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_UINT32,
					      &ap_scan))
		return FALSE;

	if (wpa_supplicant_set_ap_scan(wpa_s, ap_scan)) {
		dbus_set_error_const(error, DBUS_ERROR_FAILED,
				     "ap_scan must be 0, 1, or 2");
		return FALSE;
	}
	return TRUE;
}


/**
 * wpas_dbus_getter_fast_reauth - Control fast
 * reauthentication (TLS session resumption)
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter function for "FastReauth" property.
 */
dbus_bool_t wpas_dbus_getter_fast_reauth(DBusMessageIter *iter,
					 DBusError *error,
					 void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_bool_t fast_reauth = wpa_s->conf->fast_reauth ? TRUE : FALSE;

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_BOOLEAN,
						&fast_reauth, error);
}


/**
 * wpas_dbus_setter_fast_reauth - Control fast
 * reauthentication (TLS session resumption)
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter function for "FastReauth" property.
 */
dbus_bool_t wpas_dbus_setter_fast_reauth(DBusMessageIter *iter,
				     DBusError *error,
				     void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_bool_t fast_reauth;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_BOOLEAN,
					      &fast_reauth))
		return FALSE;

	wpa_s->conf->fast_reauth = fast_reauth;
	return TRUE;
}


/**
 * wpas_dbus_getter_bss_expire_age - Get BSS entry expiration age
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter function for "BSSExpireAge" property.
 */
dbus_bool_t wpas_dbus_getter_bss_expire_age(DBusMessageIter *iter,
					    DBusError *error,
					    void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_uint32_t expire_age = wpa_s->conf->bss_expiration_age;

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_UINT32,
						&expire_age, error);
}


/**
 * wpas_dbus_setter_bss_expire_age - Control BSS entry expiration age
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter function for "BSSExpireAge" property.
 */
dbus_bool_t wpas_dbus_setter_bss_expire_age(DBusMessageIter *iter,
					    DBusError *error,
					    void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_uint32_t expire_age;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_UINT32,
					      &expire_age))
		return FALSE;

	if (wpa_supplicant_set_bss_expiration_age(wpa_s, expire_age)) {
		dbus_set_error_const(error, DBUS_ERROR_FAILED,
				     "BSSExpireAge must be >= 10");
		return FALSE;
	}
	return TRUE;
}


/**
 * wpas_dbus_getter_bss_expire_count - Get BSS entry expiration scan count
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter function for "BSSExpireCount" property.
 */
dbus_bool_t wpas_dbus_getter_bss_expire_count(DBusMessageIter *iter,
					      DBusError *error,
					      void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_uint32_t expire_count = wpa_s->conf->bss_expiration_age;

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_UINT32,
						&expire_count, error);
}


/**
 * wpas_dbus_setter_bss_expire_count - Control BSS entry expiration scan count
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter function for "BSSExpireCount" property.
 */
dbus_bool_t wpas_dbus_setter_bss_expire_count(DBusMessageIter *iter,
					      DBusError *error,
					      void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	dbus_uint32_t expire_count;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_UINT32,
					      &expire_count))
		return FALSE;

	if (wpa_supplicant_set_bss_expiration_count(wpa_s, expire_count)) {
		dbus_set_error_const(error, DBUS_ERROR_FAILED,
				     "BSSExpireCount must be > 0");
		return FALSE;
	}
	return TRUE;
}


/**
 * wpas_dbus_getter_country - Control country code
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter function for "Country" property.
 */
dbus_bool_t wpas_dbus_getter_country(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	char country[3];
	char *str = country;

	country[0] = wpa_s->conf->country[0];
	country[1] = wpa_s->conf->country[1];
	country[2] = '\0';

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						&str, error);
}


/**
 * wpas_dbus_setter_country - Control country code
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter function for "Country" property.
 */
dbus_bool_t wpas_dbus_setter_country(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	const char *country;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_STRING,
					      &country))
		return FALSE;

	if (!country[0] || !country[1]) {
		dbus_set_error_const(error, DBUS_ERROR_FAILED,
				     "invalid country code");
		return FALSE;
	}

	if (wpa_s->drv_priv != NULL && wpa_drv_set_country(wpa_s, country)) {
		wpa_printf(MSG_DEBUG, "Failed to set country");
		dbus_set_error_const(error, DBUS_ERROR_FAILED,
				     "failed to set country code");
		return FALSE;
	}

	wpa_s->conf->country[0] = country[0];
	wpa_s->conf->country[1] = country[1];
	return TRUE;
}


/**
 * wpas_dbus_getter_ifname - Get interface name
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Ifname" property.
 */
dbus_bool_t wpas_dbus_getter_ifname(DBusMessageIter *iter, DBusError *error,
				    void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	const char *ifname = wpa_s->ifname;

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						&ifname, error);
}


/**
 * wpas_dbus_getter_driver - Get interface name
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Driver" property.
 */
dbus_bool_t wpas_dbus_getter_driver(DBusMessageIter *iter, DBusError *error,
				    void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	const char *driver;

	if (wpa_s->driver == NULL || wpa_s->driver->name == NULL) {
		wpa_printf(MSG_DEBUG, "wpas_dbus_getter_driver[dbus]: "
			   "wpa_s has no driver set");
		dbus_set_error(error, DBUS_ERROR_FAILED, "%s: no driver set",
			       __func__);
		return FALSE;
	}

	driver = wpa_s->driver->name;
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						&driver, error);
}


/**
 * wpas_dbus_getter_current_bss - Get current bss object path
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "CurrentBSS" property.
 */
dbus_bool_t wpas_dbus_getter_current_bss(DBusMessageIter *iter,
					 DBusError *error,
					 void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	char path_buf[WPAS_DBUS_OBJECT_PATH_MAX], *bss_obj_path = path_buf;

	if (wpa_s->current_bss)
		os_snprintf(bss_obj_path, WPAS_DBUS_OBJECT_PATH_MAX,
			    "%s/" WPAS_DBUS_NEW_BSSIDS_PART "/%u",
			    wpa_s->dbus_new_path, wpa_s->current_bss->id);
	else
		os_snprintf(bss_obj_path, WPAS_DBUS_OBJECT_PATH_MAX, "/");

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_OBJECT_PATH,
						&bss_obj_path, error);
}


/**
 * wpas_dbus_getter_current_network - Get current network object path
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "CurrentNetwork" property.
 */
dbus_bool_t wpas_dbus_getter_current_network(DBusMessageIter *iter,
					     DBusError *error,
					     void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	char path_buf[WPAS_DBUS_OBJECT_PATH_MAX], *net_obj_path = path_buf;

	if (wpa_s->current_ssid)
		os_snprintf(net_obj_path, WPAS_DBUS_OBJECT_PATH_MAX,
			    "%s/" WPAS_DBUS_NEW_NETWORKS_PART "/%u",
			    wpa_s->dbus_new_path, wpa_s->current_ssid->id);
	else
		os_snprintf(net_obj_path, WPAS_DBUS_OBJECT_PATH_MAX, "/");

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_OBJECT_PATH,
						&net_obj_path, error);
}


/**
 * wpas_dbus_getter_current_auth_mode - Get current authentication type
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "CurrentAuthMode" property.
 */
dbus_bool_t wpas_dbus_getter_current_auth_mode(DBusMessageIter *iter,
					       DBusError *error,
					       void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	const char *eap_mode;
	const char *auth_mode;
	char eap_mode_buf[WPAS_DBUS_AUTH_MODE_MAX];

	if (wpa_s->wpa_state != WPA_COMPLETED) {
		auth_mode = "INACTIVE";
	} else if (wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		eap_mode = wpa_supplicant_get_eap_mode(wpa_s);
		os_snprintf(eap_mode_buf, WPAS_DBUS_AUTH_MODE_MAX,
			    "EAP-%s", eap_mode);
		auth_mode = eap_mode_buf;

	} else {
		auth_mode = wpa_key_mgmt_txt(wpa_s->key_mgmt,
					     wpa_s->current_ssid->proto);
	}

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						&auth_mode, error);
}


/**
 * wpas_dbus_getter_bridge_ifname - Get interface name
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "BridgeIfname" property.
 */
dbus_bool_t wpas_dbus_getter_bridge_ifname(DBusMessageIter *iter,
					   DBusError *error,
					   void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	const char *bridge_ifname;

	bridge_ifname = wpa_s->bridge_ifname ? wpa_s->bridge_ifname : "";
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						&bridge_ifname, error);
}


/**
 * wpas_dbus_getter_bsss - Get array of BSSs objects
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "BSSs" property.
 */
dbus_bool_t wpas_dbus_getter_bsss(DBusMessageIter *iter, DBusError *error,
				  void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	struct wpa_bss *bss;
	char **paths;
	unsigned int i = 0;
	dbus_bool_t success = FALSE;

	paths = os_zalloc(wpa_s->num_bss * sizeof(char *));
	if (!paths) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	/* Loop through scan results and append each result's object path */
	dl_list_for_each(bss, &wpa_s->bss_id, struct wpa_bss, list_id) {
		paths[i] = os_zalloc(WPAS_DBUS_OBJECT_PATH_MAX);
		if (paths[i] == NULL) {
			dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY,
					     "no memory");
			goto out;
		}
		/* Construct the object path for this BSS. */
		os_snprintf(paths[i++], WPAS_DBUS_OBJECT_PATH_MAX,
			    "%s/" WPAS_DBUS_NEW_BSSIDS_PART "/%u",
			    wpa_s->dbus_new_path, bss->id);
	}

	success = wpas_dbus_simple_array_property_getter(iter,
							 DBUS_TYPE_OBJECT_PATH,
							 paths, wpa_s->num_bss,
							 error);

out:
	while (i)
		os_free(paths[--i]);
	os_free(paths);
	return success;
}


/**
 * wpas_dbus_getter_networks - Get array of networks objects
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Networks" property.
 */
dbus_bool_t wpas_dbus_getter_networks(DBusMessageIter *iter, DBusError *error,
				      void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	struct wpa_ssid *ssid;
	char **paths;
	unsigned int i = 0, num = 0;
	dbus_bool_t success = FALSE;

	if (wpa_s->conf == NULL) {
		wpa_printf(MSG_ERROR, "%s[dbus]: An error occurred getting "
			   "networks list.", __func__);
		dbus_set_error(error, DBUS_ERROR_FAILED, "%s: an error "
			       "occurred getting the networks list", __func__);
		return FALSE;
	}

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next)
		if (!network_is_persistent_group(ssid))
			num++;

	paths = os_zalloc(num * sizeof(char *));
	if (!paths) {
		dbus_set_error(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	/* Loop through configured networks and append object path of each */
	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (network_is_persistent_group(ssid))
			continue;
		paths[i] = os_zalloc(WPAS_DBUS_OBJECT_PATH_MAX);
		if (paths[i] == NULL) {
			dbus_set_error(error, DBUS_ERROR_NO_MEMORY, "no memory");
			goto out;
		}

		/* Construct the object path for this network. */
		os_snprintf(paths[i++], WPAS_DBUS_OBJECT_PATH_MAX,
			    "%s/" WPAS_DBUS_NEW_NETWORKS_PART "/%d",
			    wpa_s->dbus_new_path, ssid->id);
	}

	success = wpas_dbus_simple_array_property_getter(iter,
							 DBUS_TYPE_OBJECT_PATH,
							 paths, num, error);

out:
	while (i)
		os_free(paths[--i]);
	os_free(paths);
	return success;
}


/**
 * wpas_dbus_getter_blobs - Get all blobs defined for this interface
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Blobs" property.
 */
dbus_bool_t wpas_dbus_getter_blobs(DBusMessageIter *iter, DBusError *error,
				   void *user_data)
{
	struct wpa_supplicant *wpa_s = user_data;
	DBusMessageIter variant_iter, dict_iter, entry_iter, array_iter;
	struct wpa_config_blob *blob;

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
					      "a{say}", &variant_iter) ||
	    !dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY,
					      "{say}", &dict_iter)) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	blob = wpa_s->conf->blobs;
	while (blob) {
		if (!dbus_message_iter_open_container(&dict_iter,
						      DBUS_TYPE_DICT_ENTRY,
						      NULL, &entry_iter) ||
		    !dbus_message_iter_append_basic(&entry_iter,
						    DBUS_TYPE_STRING,
						    &(blob->name)) ||
		    !dbus_message_iter_open_container(&entry_iter,
						      DBUS_TYPE_ARRAY,
						      DBUS_TYPE_BYTE_AS_STRING,
						      &array_iter) ||
		    !dbus_message_iter_append_fixed_array(&array_iter,
							  DBUS_TYPE_BYTE,
							  &(blob->data),
							  blob->len) ||
		    !dbus_message_iter_close_container(&entry_iter,
						       &array_iter) ||
		    !dbus_message_iter_close_container(&dict_iter,
						       &entry_iter)) {
			dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY,
					     "no memory");
			return FALSE;
		}

		blob = blob->next;
	}

	if (!dbus_message_iter_close_container(&variant_iter, &dict_iter) ||
	    !dbus_message_iter_close_container(iter, &variant_iter)) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	return TRUE;
}


static struct wpa_bss * get_bss_helper(struct bss_handler_args *args,
				       DBusError *error, const char *func_name)
{
	struct wpa_bss *res = wpa_bss_get_id(args->wpa_s, args->id);

	if (!res) {
		wpa_printf(MSG_ERROR, "%s[dbus]: no bss with id %d found",
		           func_name, args->id);
		dbus_set_error(error, DBUS_ERROR_FAILED,
			       "%s: BSS %d not found",
			       func_name, args->id);
	}

	return res;
}


/**
 * wpas_dbus_getter_bss_bssid - Return the BSSID of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "BSSID" property.
 */
dbus_bool_t wpas_dbus_getter_bss_bssid(DBusMessageIter *iter, DBusError *error,
				       void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	return wpas_dbus_simple_array_property_getter(iter, DBUS_TYPE_BYTE,
						      res->bssid, ETH_ALEN,
						      error);
}


/**
 * wpas_dbus_getter_bss_ssid - Return the SSID of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "SSID" property.
 */
dbus_bool_t wpas_dbus_getter_bss_ssid(DBusMessageIter *iter, DBusError *error,
				      void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	return wpas_dbus_simple_array_property_getter(iter, DBUS_TYPE_BYTE,
						      res->ssid, res->ssid_len,
						      error);
}


/**
 * wpas_dbus_getter_bss_privacy - Return the privacy flag of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Privacy" property.
 */
dbus_bool_t wpas_dbus_getter_bss_privacy(DBusMessageIter *iter,
					 DBusError *error, void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;
	dbus_bool_t privacy;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	privacy = (res->caps & IEEE80211_CAP_PRIVACY) ? TRUE : FALSE;
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_BOOLEAN,
						&privacy, error);
}


/**
 * wpas_dbus_getter_bss_mode - Return the mode of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Mode" property.
 */
dbus_bool_t wpas_dbus_getter_bss_mode(DBusMessageIter *iter, DBusError *error,
				      void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;
	const char *mode;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	if (res->caps & IEEE80211_CAP_IBSS)
		mode = "ad-hoc";
	else
		mode = "infrastructure";

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_STRING,
						&mode, error);
}


/**
 * wpas_dbus_getter_bss_level - Return the signal strength of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Level" property.
 */
dbus_bool_t wpas_dbus_getter_bss_signal(DBusMessageIter *iter,
					DBusError *error, void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;
	s16 level;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	level = (s16) res->level;
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_INT16,
						&level, error);
}


/**
 * wpas_dbus_getter_bss_frequency - Return the frequency of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Frequency" property.
 */
dbus_bool_t wpas_dbus_getter_bss_frequency(DBusMessageIter *iter,
					   DBusError *error, void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;
	u16 freq;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	freq = (u16) res->freq;
	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_UINT16,
						&freq, error);
}


static int cmp_u8s_desc(const void *a, const void *b)
{
	return (*(u8 *) b - *(u8 *) a);
}


/**
 * wpas_dbus_getter_bss_rates - Return available bit rates of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Rates" property.
 */
dbus_bool_t wpas_dbus_getter_bss_rates(DBusMessageIter *iter,
				       DBusError *error, void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;
	u8 *ie_rates = NULL;
	u32 *real_rates;
	int rates_num, i;
	dbus_bool_t success = FALSE;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	rates_num = wpa_bss_get_bit_rates(res, &ie_rates);
	if (rates_num < 0)
		return FALSE;

	qsort(ie_rates, rates_num, 1, cmp_u8s_desc);

	real_rates = os_malloc(sizeof(u32) * rates_num);
	if (!real_rates) {
		os_free(ie_rates);
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	for (i = 0; i < rates_num; i++)
		real_rates[i] = ie_rates[i] * 500000;

	success = wpas_dbus_simple_array_property_getter(iter, DBUS_TYPE_UINT32,
							 real_rates, rates_num,
							 error);

	os_free(ie_rates);
	os_free(real_rates);
	return success;
}


static dbus_bool_t wpas_dbus_get_bss_security_prop(DBusMessageIter *iter,
						   struct wpa_ie_data *ie_data,
						   DBusError *error)
{
	DBusMessageIter iter_dict, variant_iter;
	const char *group;
	const char *pairwise[2]; /* max 2 pairwise ciphers is supported */
	const char *key_mgmt[7]; /* max 7 key managements may be supported */
	int n;

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
					      "a{sv}", &variant_iter))
		goto nomem;

	if (!wpa_dbus_dict_open_write(&variant_iter, &iter_dict))
		goto nomem;

	/* KeyMgmt */
	n = 0;
	if (ie_data->key_mgmt & WPA_KEY_MGMT_PSK)
		key_mgmt[n++] = "wpa-psk";
	if (ie_data->key_mgmt & WPA_KEY_MGMT_FT_PSK)
		key_mgmt[n++] = "wpa-ft-psk";
	if (ie_data->key_mgmt & WPA_KEY_MGMT_PSK_SHA256)
		key_mgmt[n++] = "wpa-psk-sha256";
	if (ie_data->key_mgmt & WPA_KEY_MGMT_IEEE8021X)
		key_mgmt[n++] = "wpa-eap";
	if (ie_data->key_mgmt & WPA_KEY_MGMT_FT_IEEE8021X)
		key_mgmt[n++] = "wpa-ft-eap";
	if (ie_data->key_mgmt & WPA_KEY_MGMT_IEEE8021X_SHA256)
		key_mgmt[n++] = "wpa-eap-sha256";
	if (ie_data->key_mgmt & WPA_KEY_MGMT_NONE)
		key_mgmt[n++] = "wpa-none";

	if (!wpa_dbus_dict_append_string_array(&iter_dict, "KeyMgmt",
					       key_mgmt, n))
		goto nomem;

	/* Group */
	switch (ie_data->group_cipher) {
	case WPA_CIPHER_WEP40:
		group = "wep40";
		break;
	case WPA_CIPHER_TKIP:
		group = "tkip";
		break;
	case WPA_CIPHER_CCMP:
		group = "ccmp";
		break;
	case WPA_CIPHER_WEP104:
		group = "wep104";
		break;
	default:
		group = "";
		break;
	}

	if (!wpa_dbus_dict_append_string(&iter_dict, "Group", group))
		goto nomem;

	/* Pairwise */
	n = 0;
	if (ie_data->pairwise_cipher & WPA_CIPHER_TKIP)
		pairwise[n++] = "tkip";
	if (ie_data->pairwise_cipher & WPA_CIPHER_CCMP)
		pairwise[n++] = "ccmp";

	if (!wpa_dbus_dict_append_string_array(&iter_dict, "Pairwise",
					       pairwise, n))
		goto nomem;

	/* Management group (RSN only) */
	if (ie_data->proto == WPA_PROTO_RSN) {
		switch (ie_data->mgmt_group_cipher) {
#ifdef CONFIG_IEEE80211W
		case WPA_CIPHER_AES_128_CMAC:
			group = "aes128cmac";
			break;
#endif /* CONFIG_IEEE80211W */
		default:
			group = "";
			break;
		}

		if (!wpa_dbus_dict_append_string(&iter_dict, "MgmtGroup",
						 group))
			goto nomem;
	}

	if (!wpa_dbus_dict_close_write(&variant_iter, &iter_dict))
		goto nomem;
	if (!dbus_message_iter_close_container(iter, &variant_iter))
		goto nomem;

	return TRUE;

nomem:
	dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
	return FALSE;
}


/**
 * wpas_dbus_getter_bss_wpa - Return the WPA options of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "WPA" property.
 */
dbus_bool_t wpas_dbus_getter_bss_wpa(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;
	struct wpa_ie_data wpa_data;
	const u8 *ie;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	os_memset(&wpa_data, 0, sizeof(wpa_data));
	ie = wpa_bss_get_vendor_ie(res, WPA_IE_VENDOR_TYPE);
	if (ie) {
		if (wpa_parse_wpa_ie(ie, 2 + ie[1], &wpa_data) < 0) {
			dbus_set_error_const(error, DBUS_ERROR_FAILED,
					     "failed to parse WPA IE");
			return FALSE;
		}
	}

	return wpas_dbus_get_bss_security_prop(iter, &wpa_data, error);
}


/**
 * wpas_dbus_getter_bss_rsn - Return the RSN options of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "RSN" property.
 */
dbus_bool_t wpas_dbus_getter_bss_rsn(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;
	struct wpa_ie_data wpa_data;
	const u8 *ie;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	os_memset(&wpa_data, 0, sizeof(wpa_data));
	ie = wpa_bss_get_ie(res, WLAN_EID_RSN);
	if (ie) {
		if (wpa_parse_wpa_ie(ie, 2 + ie[1], &wpa_data) < 0) {
			dbus_set_error_const(error, DBUS_ERROR_FAILED,
					     "failed to parse RSN IE");
			return FALSE;
		}
	}

	return wpas_dbus_get_bss_security_prop(iter, &wpa_data, error);
}


/**
 * wpas_dbus_getter_bss_ies - Return all IEs of a BSS
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "IEs" property.
 */
dbus_bool_t wpas_dbus_getter_bss_ies(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct bss_handler_args *args = user_data;
	struct wpa_bss *res;

	res = get_bss_helper(args, error, __func__);
	if (!res)
		return FALSE;

	return wpas_dbus_simple_array_property_getter(iter, DBUS_TYPE_BYTE,
						      res + 1, res->ie_len,
						      error);
}


/**
 * wpas_dbus_getter_enabled - Check whether network is enabled or disabled
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "enabled" property of a configured network.
 */
dbus_bool_t wpas_dbus_getter_enabled(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct network_handler_args *net = user_data;
	dbus_bool_t enabled = net->ssid->disabled ? FALSE : TRUE;

	return wpas_dbus_simple_property_getter(iter, DBUS_TYPE_BOOLEAN,
						&enabled, error);
}


/**
 * wpas_dbus_setter_enabled - Mark a configured network as enabled or disabled
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter for "Enabled" property of a configured network.
 */
dbus_bool_t wpas_dbus_setter_enabled(DBusMessageIter *iter, DBusError *error,
				     void *user_data)
{
	struct network_handler_args *net = user_data;
	struct wpa_supplicant *wpa_s;
	struct wpa_ssid *ssid;
	dbus_bool_t enable;

	if (!wpas_dbus_simple_property_setter(iter, error, DBUS_TYPE_BOOLEAN,
					      &enable))
		return FALSE;

	wpa_s = net->wpa_s;
	ssid = net->ssid;

	if (enable)
		wpa_supplicant_enable_network(wpa_s, ssid);
	else
		wpa_supplicant_disable_network(wpa_s, ssid);

	return TRUE;
}


/**
 * wpas_dbus_getter_network_properties - Get options for a configured network
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Getter for "Properties" property of a configured network.
 */
dbus_bool_t wpas_dbus_getter_network_properties(DBusMessageIter *iter,
						DBusError *error,
						void *user_data)
{
	struct network_handler_args *net = user_data;
	DBusMessageIter	variant_iter, dict_iter;
	char **iterator;
	char **props = wpa_config_get_all(net->ssid, 1);
	dbus_bool_t success = FALSE;

	if (!props) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		return FALSE;
	}

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "a{sv}",
					      &variant_iter) ||
	    !wpa_dbus_dict_open_write(&variant_iter, &dict_iter)) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		goto out;
	}

	iterator = props;
	while (*iterator) {
		if (!wpa_dbus_dict_append_string(&dict_iter, *iterator,
						 *(iterator + 1))) {
			dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY,
					     "no memory");
			goto out;
		}
		iterator += 2;
	}


	if (!wpa_dbus_dict_close_write(&variant_iter, &dict_iter) ||
	    !dbus_message_iter_close_container(iter, &variant_iter)) {
		dbus_set_error_const(error, DBUS_ERROR_NO_MEMORY, "no memory");
		goto out;
	}

	success = TRUE;

out:
	iterator = props;
	while (*iterator) {
		os_free(*iterator);
		iterator++;
	}
	os_free(props);
	return success;
}


/**
 * wpas_dbus_setter_network_properties - Set options for a configured network
 * @iter: Pointer to incoming dbus message iter
 * @error: Location to store error on failure
 * @user_data: Function specific data
 * Returns: TRUE on success, FALSE on failure
 *
 * Setter for "Properties" property of a configured network.
 */
dbus_bool_t wpas_dbus_setter_network_properties(DBusMessageIter *iter,
						DBusError *error,
						void *user_data)
{
	struct network_handler_args *net = user_data;
	struct wpa_ssid *ssid = net->ssid;
	DBusMessageIter	variant_iter;

	dbus_message_iter_recurse(iter, &variant_iter);
	return set_network_properties(net->wpa_s, ssid, &variant_iter, error);
}
