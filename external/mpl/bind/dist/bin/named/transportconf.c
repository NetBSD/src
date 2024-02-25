/*	$NetBSD: transportconf.c,v 1.2.2.2 2024/02/25 15:43:06 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>

#include <isc/buffer.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/name.h>
#include <dns/transport.h>

#include <isccfg/cfg.h>

#include <named/log.h>
#include <named/transportconf.h>

#define create_name(id, name)                                      \
	isc_buffer_t namesrc, namebuf;                             \
	char namedata[DNS_NAME_FORMATSIZE + 1];                    \
	dns_name_init(name, NULL);                                 \
	isc_buffer_constinit(&namesrc, id, strlen(id));            \
	isc_buffer_add(&namesrc, strlen(id));                      \
	isc_buffer_init(&namebuf, namedata, sizeof(namedata));     \
	result = (dns_name_fromtext(name, &namesrc, dns_rootname,  \
				    DNS_NAME_DOWNCASE, &namebuf)); \
	if (result != ISC_R_SUCCESS) {                             \
		goto failure;                                      \
	}

#define parse_transport_option(map, transport, name, setter)      \
	{                                                         \
		const cfg_obj_t *obj = NULL;                      \
		cfg_map_get(map, name, &obj);                     \
		if (obj != NULL) {                                \
			setter(transport, cfg_obj_asstring(obj)); \
		}                                                 \
	}

#define parse_transport_tls_versions(map, transport, name, setter)                \
	{                                                                         \
		const cfg_obj_t *obj = NULL;                                      \
		cfg_map_get(map, name, &obj);                                     \
		if (obj != NULL) {                                                \
			{                                                         \
				uint32_t tls_protos = 0;                          \
				const cfg_listelt_t *proto = NULL;                \
				INSIST(obj != NULL);                              \
				for (proto = cfg_list_first(obj); proto != 0;     \
				     proto = cfg_list_next(proto))                \
				{                                                 \
					const cfg_obj_t *tls_proto_obj =          \
						cfg_listelt_value(proto);         \
					const char *tls_sver =                    \
						cfg_obj_asstring(                 \
							tls_proto_obj);           \
					const isc_tls_protocol_version_t ver =    \
						isc_tls_protocol_name_to_version( \
							tls_sver);                \
					INSIST(ver !=                             \
					       ISC_TLS_PROTO_VER_UNDEFINED);      \
					INSIST(isc_tls_protocol_supported(        \
						ver));                            \
					tls_protos |= ver;                        \
				}                                                 \
				if (tls_protos != 0) {                            \
					setter(transport, tls_protos);            \
				}                                                 \
			}                                                         \
		}                                                                 \
	}

#define parse_transport_bool_option(map, transport, name, setter)  \
	{                                                          \
		const cfg_obj_t *obj = NULL;                       \
		cfg_map_get(map, name, &obj);                      \
		if (obj != NULL) {                                 \
			setter(transport, cfg_obj_asboolean(obj)); \
		}                                                  \
	}

static isc_result_t
add_doh_transports(const cfg_obj_t *transportlist, dns_transport_list_t *list) {
	const cfg_obj_t *doh = NULL;
	const char *dohid = NULL;
	isc_result_t result;

	for (const cfg_listelt_t *element = cfg_list_first(transportlist);
	     element != NULL; element = cfg_list_next(element))
	{
		dns_name_t dohname;
		dns_transport_t *transport;

		doh = cfg_listelt_value(element);
		dohid = cfg_obj_asstring(cfg_map_getname(doh));

		create_name(dohid, &dohname);

		transport = dns_transport_new(&dohname, DNS_TRANSPORT_HTTP,
					      list);

		dns_transport_set_tlsname(transport, dohid);
		parse_transport_option(doh, transport, "key-file",
				       dns_transport_set_keyfile);
		parse_transport_option(doh, transport, "cert-file",
				       dns_transport_set_certfile);
		parse_transport_tls_versions(doh, transport, "protocols",
					     dns_transport_set_tls_versions);
		parse_transport_option(doh, transport, "ciphers",
				       dns_transport_set_ciphers);
		parse_transport_bool_option(
			doh, transport, "prefer-server-ciphers",
			dns_transport_set_prefer_server_ciphers)
			parse_transport_option(doh, transport, "ca-file",
					       dns_transport_set_cafile);
		parse_transport_option(doh, transport, "remote-hostname",
				       dns_transport_set_remote_hostname);
	}

	return (ISC_R_SUCCESS);
failure:
	cfg_obj_log(doh, named_g_lctx, ISC_LOG_ERROR,
		    "configuring DoH '%s': %s", dohid,
		    isc_result_totext(result));

	return (result);
}

static isc_result_t
add_tls_transports(const cfg_obj_t *transportlist, dns_transport_list_t *list) {
	const cfg_obj_t *tls = NULL;
	const char *tlsid = NULL;
	isc_result_t result;

	for (const cfg_listelt_t *element = cfg_list_first(transportlist);
	     element != NULL; element = cfg_list_next(element))
	{
		dns_name_t tlsname;
		dns_transport_t *transport;

		tls = cfg_listelt_value(element);
		tlsid = cfg_obj_asstring(cfg_map_getname(tls));

		if (!strcmp(tlsid, "ephemeral")) {
			result = ISC_R_UNEXPECTEDTOKEN;
			goto failure;
		}

		create_name(tlsid, &tlsname);

		transport = dns_transport_new(&tlsname, DNS_TRANSPORT_TLS,
					      list);

		dns_transport_set_tlsname(transport, tlsid);
		parse_transport_option(tls, transport, "key-file",
				       dns_transport_set_keyfile);
		parse_transport_option(tls, transport, "cert-file",
				       dns_transport_set_certfile);
		parse_transport_tls_versions(tls, transport, "protocols",
					     dns_transport_set_tls_versions);
		parse_transport_option(tls, transport, "ciphers",
				       dns_transport_set_ciphers);
		parse_transport_bool_option(
			tls, transport, "prefer-server-ciphers",
			dns_transport_set_prefer_server_ciphers)
			parse_transport_option(tls, transport, "ca-file",
					       dns_transport_set_cafile);
		parse_transport_option(tls, transport, "remote-hostname",
				       dns_transport_set_remote_hostname);
	}

	return (ISC_R_SUCCESS);
failure:
	cfg_obj_log(tls, named_g_lctx, ISC_LOG_ERROR,
		    "configuring tls '%s': %s", tlsid,
		    isc_result_totext(result));

	return (result);
}

#define CHECK(f)                             \
	if ((result = f) != ISC_R_SUCCESS) { \
		goto failure;                \
	}

static isc_result_t
transport_list_fromconfig(const cfg_obj_t *config, dns_transport_list_t *list) {
	const cfg_obj_t *obj = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	if (result == ISC_R_SUCCESS &&
	    cfg_map_get(config, "tls", &obj) == ISC_R_SUCCESS)
	{
		result = add_tls_transports(obj, list);
		obj = NULL;
	}

	if (result == ISC_R_SUCCESS &&
	    cfg_map_get(config, "doh", &obj) == ISC_R_SUCCESS)
	{
		result = add_doh_transports(obj, list);
		obj = NULL;
	}

	return (result);
}

static void
transport_list_add_ephemeral(dns_transport_list_t *list) {
	isc_result_t result;
	dns_name_t tlsname;
	dns_transport_t *transport;

	create_name("ephemeral", &tlsname);

	transport = dns_transport_new(&tlsname, DNS_TRANSPORT_TLS, list);
	dns_transport_set_tlsname(transport, "ephemeral");

	return;
failure:
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

isc_result_t
named_transports_fromconfig(const cfg_obj_t *config, const cfg_obj_t *vconfig,
			    isc_mem_t *mctx, dns_transport_list_t **listp) {
	isc_result_t result;
	dns_transport_list_t *list = dns_transport_list_new(mctx);

	REQUIRE(listp != NULL && *listp == NULL);

	transport_list_add_ephemeral(list);

	if (config != NULL) {
		result = transport_list_fromconfig(config, list);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}
	}

	if (vconfig != NULL) {
		config = cfg_tuple_get(vconfig, "options");
		transport_list_fromconfig(config, list);
	}

	*listp = list;
	return (ISC_R_SUCCESS);
failure:
	dns_transport_list_detach(&list);
	return (result);
}
