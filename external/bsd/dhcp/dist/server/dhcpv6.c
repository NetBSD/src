/*	$NetBSD: dhcpv6.c,v 1.7 2018/04/07 21:19:32 christos Exp $	*/

/*
 * Copyright (C) 2006-2017 by Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: dhcpv6.c,v 1.7 2018/04/07 21:19:32 christos Exp $");


/*! \file server/dhcpv6.c */

#include "dhcpd.h"

#ifdef DHCPv6

#ifdef DHCP4o6
static void forw_dhcpv4_query(struct packet *packet);
static void send_dhcpv4_response(struct data_string *raw);

static void recv_dhcpv4_query(struct data_string *raw);
static void dhcp4o6_dhcpv4_query(struct data_string *reply_ret,
				 struct packet *packet);

struct udp_data4o6 {
	u_int16_t src_port;
	u_int8_t  rsp_opt_exist;
	u_int8_t  reserved;
};

static int offset_data4o6 = 36; /* 16+16+4 */
#endif

/*
 * We use print_hex_1() to output DUID values. We could actually output
 * the DUID with more information... MAC address if using type 1 or 3,
 * and so on. However, RFC 3315 contains Grave Warnings against actually
 * attempting to understand a DUID.
 */

/*
 * TODO: gettext() or other method of localization for the messages
 *       for status codes (and probably for log formats eventually)
 * TODO: refactoring (simplify, simplify, simplify)
 * TODO: support multiple shared_networks on each interface (this
 *       will allow the server to issue multiple IPv6 addresses to
 *       a single interface)
 */

/*
 * DHCPv6 Reply workflow assist.  A Reply packet is built by various
 * different functions; this gives us one location where we keep state
 * regarding a reply.
 */
struct reply_state {
	/* root level persistent state */
	struct shared_network *shared;
	struct host_decl *host;
	struct subnet *subnet; /* Used to match fixed-addrs to subnet scopes. */
	struct option_state *opt_state;
	struct packet *packet;
	struct data_string client_id;

	/* IA level persistent state */
	unsigned ia_count;
	unsigned pd_count;
	unsigned client_resources;
	isc_boolean_t resources_included;
	isc_boolean_t static_lease;
	unsigned static_prefixes;
	struct ia_xx *ia;
	struct ia_xx *old_ia;
	struct option_state *reply_ia;
	struct data_string fixed;
	struct iaddrcidrnet fixed_pref; /* static prefix for logging */

	/* IAADDR/PREFIX level persistent state */
	struct iasubopt *lease;

	/*
	 * "t1", "t2", preferred, and valid lifetimes records for calculating
	 * t1 and t2 (min/max).
	 */
	u_int32_t renew, rebind, min_prefer, min_valid;

	/* Client-requested valid and preferred lifetimes. */
	u_int32_t client_valid, client_prefer;

	/* Chosen values to transmit for valid and preferred lifetimes. */
	u_int32_t send_valid, send_prefer;

	/* Preferred prefix length (-1 is any). */
	int preflen;

	/* Index into the data field that has been consumed. */
	unsigned cursor;

	/* Space for the on commit statements for a fixed host */
	struct on_star on_star;

	union reply_buffer {
		unsigned char data[65536];
		struct dhcpv6_packet reply;
	} buf;
};

/*
 * Prototypes local to this file.
 */
static int get_encapsulated_IA_state(struct option_state **enc_opt_state,
				     struct data_string *enc_opt_data,
				     struct packet *packet,
				     struct option_cache *oc,
				     int offset);
static void build_dhcpv6_reply(struct data_string *, struct packet *);
static isc_result_t shared_network_from_packet6(struct shared_network **shared,
						struct packet *packet);
static void seek_shared_host(struct host_decl **hp,
			     struct shared_network *shared);
static isc_boolean_t fixed_matches_shared(struct host_decl *host,
					  struct shared_network *shared);
static isc_result_t reply_process_ia_na(struct reply_state *reply,
					struct option_cache *ia);
static isc_result_t reply_process_ia_ta(struct reply_state *reply,
					struct option_cache *ia);
static isc_result_t reply_process_addr(struct reply_state *reply,
				       struct option_cache *addr);
static isc_boolean_t address_is_owned(struct reply_state *reply,
				      struct iaddr *addr);
static isc_boolean_t temporary_is_available(struct reply_state *reply,
					    struct iaddr *addr);
static isc_result_t find_client_temporaries(struct reply_state *reply);
static isc_result_t reply_process_try_addr(struct reply_state *reply,
					   struct iaddr *addr);
static isc_result_t find_client_address(struct reply_state *reply);
static isc_result_t reply_process_is_addressed(struct reply_state *reply,
					       struct binding_scope **scope,
					       struct group *group);
static isc_result_t reply_process_send_addr(struct reply_state *reply,
					    struct iaddr *addr);
static struct iasubopt *lease_compare(struct iasubopt *alpha,
				      struct iasubopt *beta);
static isc_result_t reply_process_ia_pd(struct reply_state *reply,
					struct option_cache *ia_pd);
static struct group *find_group_by_prefix(struct reply_state *reply);
static isc_result_t reply_process_prefix(struct reply_state *reply,
					 struct option_cache *pref);
static isc_boolean_t prefix_is_owned(struct reply_state *reply,
				     struct iaddrcidrnet *pref);
static isc_result_t find_client_prefix(struct reply_state *reply);
static isc_result_t reply_process_try_prefix(struct reply_state *reply,
					     struct iaddrcidrnet *pref);
static isc_result_t reply_process_is_prefixed(struct reply_state *reply,
					      struct binding_scope **scope,
					      struct group *group);
static isc_result_t reply_process_send_prefix(struct reply_state *reply,
					      struct iaddrcidrnet *pref);
static struct iasubopt *prefix_compare(struct reply_state *reply,
				       struct iasubopt *alpha,
				       struct iasubopt *beta);
static void schedule_lease_timeout_reply(struct reply_state *reply);

static int eval_prefix_mode(int thislen, int preflen, int prefix_mode);
static isc_result_t pick_v6_prefix_helper(struct reply_state *reply,
					  int prefix_mode);

static void unicast_reject(struct data_string *reply_ret, struct packet *packet,
		  const struct data_string *client_id,
		  const struct data_string *server_id);

static isc_boolean_t is_unicast_option_defined(struct packet *packet);
static isc_result_t shared_network_from_requested_addr (struct shared_network
							**shared,
							struct packet* packet);
static isc_result_t get_first_ia_addr_val (struct packet* packet, int addr_type,
					   struct iaddr* iaddr);

static void
set_reply_tee_times(struct reply_state* reply, unsigned ia_cursor);

static const char *iasubopt_plen_str(struct iasubopt *lease);
static int release_on_roam(struct reply_state *reply);

static int reuse_lease6(struct reply_state *reply, struct iasubopt *lease);
static void shorten_lifetimes(struct reply_state *reply, struct iasubopt *lease,
			      time_t age, int threshold);
static void write_to_packet(struct reply_state *reply, unsigned ia_cursor);
static const char *iasubopt_plen_str(struct iasubopt *lease);

#ifdef NSUPDATE
static void ddns_update_static6(struct reply_state* reply);
#endif

#ifdef DHCP4o6
/*
 * \brief Omapi I/O handler
 *
 * The inter-process communication receive handler.
 * Get the message, put it into the raw data_string
 * and call \ref send_dhcpv4_response() (DHCPv6 side) or
 * \ref recv_dhcpv4_query() (DHCPv4 side)
 *
 * \param h the OMAPI object
 * \return a result for I/O success or error (used by the I/O subsystem)
 */
isc_result_t dhcpv4o6_handler(omapi_object_t *h) {
	char buf[65536];
	struct data_string raw;
	int cc;

	if (h->type != dhcp4o6_type)
		return DHCP_R_INVALIDARG;

	cc = recv(dhcp4o6_fd, buf, sizeof(buf), 0);

	if (cc < DHCP_FIXED_NON_UDP + offset_data4o6)
		return ISC_R_UNEXPECTED;
	memset(&raw, 0, sizeof(raw));
	if (!buffer_allocate(&raw.buffer, cc, MDL)) {
		log_error("dhcpv4o6_handler: no memory buffer.");
		return ISC_R_NOMEMORY;
	}
	raw.data = raw.buffer->data;
	raw.len = cc;
	memcpy(raw.buffer->data, buf, cc);

	if (local_family == AF_INET6) {
		send_dhcpv4_response(&raw);
	} else {
		recv_dhcpv4_query(&raw);
	}

	data_string_forget(&raw, MDL);

	return ISC_R_SUCCESS;
}

/*
 * \brief Send the DHCPv4-response back to the DHCPv6 side
 *  (DHCPv6 server function)
 *
 * Format: interface:16 + address:16 + udp:4 + DHCPv6 DHCPv4-response message
 *
 * \param raw the IPC message content
 */
static void send_dhcpv4_response(struct data_string *raw) {
	struct interface_info *ip;
	char name[16 + 1];
	struct sockaddr_in6 to_addr;
	char pbuf[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	struct udp_data4o6 udp_data;
	int send_ret;

	memset(name, 0, sizeof(name));
	memcpy(name, raw->data, 16);
	for (ip = interfaces; ip != NULL; ip = ip->next) {
		if (!strcmp(name, ip->name))
			break;
	}
	if (ip == NULL) {
		log_error("send_dhcpv4_response: can't find interface %s.",
			  name);
		return;
	}

	memset(&to_addr, 0, sizeof(to_addr));
	to_addr.sin6_family = AF_INET6;
	memcpy(&to_addr.sin6_addr, raw->data + 16, 16);
	memset(&udp_data, 0, sizeof(udp_data));
	memcpy(&udp_data, raw->data + 32, 4);
	if ((raw->data[36] == DHCPV6_RELAY_FORW) ||
	    (raw->data[36] == DHCPV6_RELAY_REPL)) {
		if (udp_data.rsp_opt_exist) {
			to_addr.sin6_port = udp_data.src_port;
		} else {
			to_addr.sin6_port = local_port;
		}
	} else {
		to_addr.sin6_port = remote_port;
	}

	log_info("send_dhcpv4_response(): sending %s on %s to %s port %d",
		 dhcpv6_type_names[raw->data[36]],
		 name,
		 inet_ntop(AF_INET6, raw->data + 16, pbuf, sizeof(pbuf)),
		 ntohs(to_addr.sin6_port));

	send_ret = send_packet6(ip, raw->data + 36, raw->len - 36, &to_addr);
	if (send_ret < 0) {
		log_error("send_dhcpv4_response: send_packet6(): %m");
	} else if (send_ret != raw->len - 36) {
		log_error("send_dhcpv4_response: send_packet6() "
			  "sent %d of %d bytes",
			  send_ret, raw->len - 36);
	}
}
#endif /* DHCP4o6 */

/*
 * Schedule lease timeouts for all of the iasubopts in the reply.
 * This is currently used to schedule timeouts for soft leases.
 */

static void
schedule_lease_timeout_reply(struct reply_state *reply) {
	struct iasubopt *tmp;
	int i;

	/* sanity check the reply */
	if ((reply == NULL) || (reply->ia == NULL) || (reply->ia->iasubopt == NULL))
		return;

	/* walk through the list, scheduling as we go */
	for (i = 0 ; i < reply->ia->num_iasubopt ; i++) {
		tmp = reply->ia->iasubopt[i];
		schedule_lease_timeout(tmp->ipv6_pool);
	}
}

/*
 * This function returns the time since DUID time start for the
 * given time_t value.
 */
static u_int32_t
duid_time(time_t when) {
	/*
	 * This time is modulo 2^32.
	 */
	while ((when - DUID_TIME_EPOCH) > 4294967295u) {
		/* use 2^31 to avoid spurious compiler warnings */
		when -= 2147483648u;
		when -= 2147483648u;
	}

	return when - DUID_TIME_EPOCH;
}


/*
 * Server DUID.
 *
 * This must remain the same for the lifetime of this server, because
 * clients return the server DUID that we sent them in Request packets.
 *
 * We pick the server DUID like this:
 *
 * 1. Check dhcpd.conf - any value the administrator has configured
 *    overrides any possible values.
 * 2. Check the leases.txt - we want to use the previous value if
 *    possible.
 * 3. Check if dhcpd.conf specifies a type of server DUID to use,
 *    and generate that type.
 * 4. Generate a type 1 (time + hardware address) DUID.
 */
static struct data_string server_duid;

/*
 * Check if the server_duid has been set.
 */
isc_boolean_t
server_duid_isset(void) {
	return (server_duid.data != NULL);
}

/*
 * Return the server_duid.
 */
void
copy_server_duid(struct data_string *ds, const char *file, int line) {
	data_string_copy(ds, &server_duid, file, line);
}

/*
 * Set the server DUID to a specified value. This is used when
 * the server DUID is stored in persistent memory (basically the
 * leases.txt file).
 */
void
set_server_duid(struct data_string *new_duid) {
	/* INSIST(new_duid != NULL); */
	/* INSIST(new_duid->data != NULL); */

	if (server_duid_isset()) {
		data_string_forget(&server_duid, MDL);
	}
	data_string_copy(&server_duid, new_duid, MDL);
}


/*
 * Set the server DUID based on the D6O_SERVERID option. This handles
 * the case where the administrator explicitly put it in the dhcpd.conf
 * file.
 */
isc_result_t
set_server_duid_from_option(void) {
	struct option_state *opt_state;
	struct option_cache *oc;
	struct data_string option_duid;
	isc_result_t ret_val;

	opt_state = NULL;
	if (!option_state_allocate(&opt_state, MDL)) {
		log_fatal("No memory for server DUID.");
	}

	execute_statements_in_scope(NULL, NULL, NULL, NULL, NULL,
				    opt_state, &global_scope, root_group,
				    NULL, NULL);

	oc = lookup_option(&dhcpv6_universe, opt_state, D6O_SERVERID);
	if (oc == NULL) {
		ret_val = ISC_R_NOTFOUND;
	} else {
		memset(&option_duid, 0, sizeof(option_duid));
		if (!evaluate_option_cache(&option_duid, NULL, NULL, NULL,
					   opt_state, NULL, &global_scope,
					   oc, MDL)) {
			ret_val = ISC_R_UNEXPECTED;
		} else {
			set_server_duid(&option_duid);
			data_string_forget(&option_duid, MDL);
			ret_val = ISC_R_SUCCESS;
		}
	}

	option_state_dereference(&opt_state, MDL);

	return ret_val;
}

/*
 * DUID layout, as defined in RFC 3315, section 9.
 *
 * We support type 1 (hardware address plus time) and type 3 (hardware
 * address).
 *
 * We can support type 2 for specific vendors in the future, if they
 * publish the specification. And of course there may be additional
 * types later.
 */
static int server_duid_type = DUID_LLT;

/*
 * Set the DUID type.
 */
void
set_server_duid_type(int type) {
	server_duid_type = type;
}

/*
 * Generate a new server DUID. This is done if there was no DUID in
 * the leases.txt or in the dhcpd.conf file.
 */
isc_result_t
generate_new_server_duid(void) {
	struct interface_info *p;
	u_int32_t time_val;
	struct data_string generated_duid;

	/*
	 * Verify we have a type that we support.
	 */
	if ((server_duid_type != DUID_LL) && (server_duid_type != DUID_LLT)) {
		log_error("Invalid DUID type %d specified, "
			  "only LL and LLT types supported", server_duid_type);
		return DHCP_R_INVALIDARG;
	}

	/*
	 * Find an interface with a hardware address.
	 * Any will do. :)
	 */
	for (p = interfaces; p != NULL; p = p->next) {
		if (p->hw_address.hlen > 0) {
			break;
		}
	}
	if (p == NULL) {
		return ISC_R_UNEXPECTED;
	}

	/*
	 * Build our DUID.
	 */
	memset(&generated_duid, 0, sizeof(generated_duid));
	if (server_duid_type == DUID_LLT) {
		time_val = duid_time(time(NULL));
		generated_duid.len = 8 + p->hw_address.hlen - 1;
		if (!buffer_allocate(&generated_duid.buffer,
				     generated_duid.len, MDL)) {
			log_fatal("No memory for server DUID.");
		}
		generated_duid.data = generated_duid.buffer->data;
		putUShort(generated_duid.buffer->data, DUID_LLT);
		putUShort(generated_duid.buffer->data + 2,
			  p->hw_address.hbuf[0]);
		putULong(generated_duid.buffer->data + 4, time_val);
		memcpy(generated_duid.buffer->data + 8,
		       p->hw_address.hbuf+1, p->hw_address.hlen-1);
	} else if (server_duid_type == DUID_LL) {
		generated_duid.len = 4 + p->hw_address.hlen - 1;
		if (!buffer_allocate(&generated_duid.buffer,
				     generated_duid.len, MDL)) {
			log_fatal("No memory for server DUID.");
		}
		generated_duid.data = generated_duid.buffer->data;
		putUShort(generated_duid.buffer->data, DUID_LL);
		putUShort(generated_duid.buffer->data + 2,
			  p->hw_address.hbuf[0]);
		memcpy(generated_duid.buffer->data + 4,
		       p->hw_address.hbuf+1, p->hw_address.hlen-1);
	} else {
		log_fatal("Unsupported server DUID type %d.", server_duid_type);
	}

	set_server_duid(&generated_duid);
	data_string_forget(&generated_duid, MDL);

	return ISC_R_SUCCESS;
}

/*
 * Get the client identifier from the packet.
 */
isc_result_t
get_client_id(struct packet *packet, struct data_string *client_id) {
	struct option_cache *oc;

	/*
	 * Verify our client_id structure is empty.
	 */
	if ((client_id->data != NULL) || (client_id->len != 0)) {
		return DHCP_R_INVALIDARG;
	}

	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_CLIENTID);
	if (oc == NULL) {
		return ISC_R_NOTFOUND;
	}

	if (!evaluate_option_cache(client_id, packet, NULL, NULL,
				   packet->options, NULL,
				   &global_scope, oc, MDL)) {
		return ISC_R_FAILURE;
	}

	return ISC_R_SUCCESS;
}

/*
 * Message validation, defined in RFC 3315, sections 15.2, 15.5, 15.7:
 *
 *    Servers MUST discard any Solicit messages that do not include a
 *    Client Identifier option or that do include a Server Identifier
 *    option.
 */
static int
valid_client_msg(struct packet *packet, struct data_string *client_id) {
	int ret_val;
	struct option_cache *oc;
	struct data_string data;

	ret_val = 0;
	memset(client_id, 0, sizeof(*client_id));
	memset(&data, 0, sizeof(data));

	switch (get_client_id(packet, client_id)) {
		case ISC_R_SUCCESS:
			break;
		case ISC_R_NOTFOUND:
			log_debug("Discarding %s from %s; "
				  "client identifier missing",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  piaddr(packet->client_addr));
			goto exit;
		default:
			log_error("Error processing %s from %s; "
				  "unable to evaluate Client Identifier",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  piaddr(packet->client_addr));
			goto exit;
	}

	/*
	 * Required by RFC 3315, section 15.
	 */
	if (packet->unicast) {
		log_debug("Discarding %s from %s; packet sent unicast "
			  "(CLIENTID %s)",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr),
			  print_hex_1(client_id->len, client_id->data, 60));
		goto exit;
	}


	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_SERVERID);
	if (oc != NULL) {
		if (evaluate_option_cache(&data, packet, NULL, NULL,
					  packet->options, NULL,
					  &global_scope, oc, MDL)) {
			log_debug("Discarding %s from %s; "
				  "server identifier found "
				  "(CLIENTID %s, SERVERID %s)",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  piaddr(packet->client_addr),
				  print_hex_1(client_id->len,
				  	      client_id->data, 60),
				  print_hex_2(data.len,
				  	      data.data, 60));
		} else {
			log_debug("Discarding %s from %s; "
				  "server identifier found "
				  "(CLIENTID %s)",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  print_hex_1(client_id->len,
				  	      client_id->data, 60),
				  piaddr(packet->client_addr));
		}
		goto exit;
	}

	/* looks good */
	ret_val = 1;

exit:
	if (data.len > 0) {
		data_string_forget(&data, MDL);
	}
	if (!ret_val) {
		if (client_id->len > 0) {
			data_string_forget(client_id, MDL);
		}
	}
	return ret_val;
}

/*
 * Response validation, defined in RFC 3315, sections 15.4, 15.6, 15.8,
 * 15.9 (slightly different wording, but same meaning):
 *
 *   Servers MUST discard any received Request message that meet any of
 *   the following conditions:
 *
 *   -  the message does not include a Server Identifier option.
 *   -  the contents of the Server Identifier option do not match the
 *      server's DUID.
 *   -  the message does not include a Client Identifier option.
 */
static int
valid_client_resp(struct packet *packet,
		  struct data_string *client_id,
		  struct data_string *server_id)
{
	int ret_val;
	struct option_cache *oc;

	/* INSIST((duid.data != NULL) && (duid.len > 0)); */

	ret_val = 0;
	memset(client_id, 0, sizeof(*client_id));
	memset(server_id, 0, sizeof(*server_id));

	switch (get_client_id(packet, client_id)) {
		case ISC_R_SUCCESS:
			break;
		case ISC_R_NOTFOUND:
			log_debug("Discarding %s from %s; "
				  "client identifier missing",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  piaddr(packet->client_addr));
			goto exit;
		default:
			log_error("Error processing %s from %s; "
				  "unable to evaluate Client Identifier",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  piaddr(packet->client_addr));
			goto exit;
	}

	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_SERVERID);
	if (oc == NULL) {
		log_debug("Discarding %s from %s: "
			  "server identifier missing (CLIENTID %s)",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr),
			  print_hex_1(client_id->len, client_id->data, 60));
		goto exit;
	}
	if (!evaluate_option_cache(server_id, packet, NULL, NULL,
				   packet->options, NULL,
				   &global_scope, oc, MDL)) {
		log_error("Error processing %s from %s; "
			  "unable to evaluate Server Identifier (CLIENTID %s)",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr),
			  print_hex_1(client_id->len, client_id->data, 60));
		goto exit;
	}
	if ((server_duid.len != server_id->len) ||
	    (memcmp(server_duid.data, server_id->data, server_duid.len) != 0)) {
		log_debug("Discarding %s from %s; "
			  "not our server identifier "
			  "(CLIENTID %s, SERVERID %s, server DUID %s)",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr),
			  print_hex_1(client_id->len, client_id->data, 60),
			  print_hex_2(server_id->len, server_id->data, 60),
			  print_hex_3(server_duid.len, server_duid.data, 60));
		goto exit;
	}

	/* looks good */
	ret_val = 1;

exit:
	if (!ret_val) {
		if (server_id->len > 0) {
			data_string_forget(server_id, MDL);
		}
		if (client_id->len > 0) {
			data_string_forget(client_id, MDL);
		}
	}
	return ret_val;
}

/*
 * Information request validation, defined in RFC 3315, section 15.12:
 *
 *   Servers MUST discard any received Information-request message that
 *   meets any of the following conditions:
 *
 *   -  The message includes a Server Identifier option and the DUID in
 *      the option does not match the server's DUID.
 *
 *   -  The message includes an IA option.
 */
static int
valid_client_info_req(struct packet *packet, struct data_string *server_id) {
	int ret_val;
	struct option_cache *oc;
	struct data_string client_id;
	char client_id_str[80];	/* print_hex_1() uses maximum 60 characters,
				   plus a few more for extra information */

	ret_val = 0;
	memset(server_id, 0, sizeof(*server_id));
	memset(&client_id, 0, sizeof(client_id));

	/*
	 * Make a string that we can print out to give more
	 * information about the client if we need to.
	 *
	 * By RFC 3315, Section 18.1.5 clients SHOULD have a
	 * client-id on an Information-request packet, but it
	 * is not strictly necessary.
	 */
	if (get_client_id(packet, &client_id) == ISC_R_SUCCESS) {
		snprintf(client_id_str, sizeof(client_id_str), " (CLIENTID %s)",
			 print_hex_1(client_id.len, client_id.data, 60));
		data_string_forget(&client_id, MDL);
	} else {
		client_id_str[0] = '\0';
	}

	/*
	 * Required by RFC 3315, section 15.
	 */
	if (packet->unicast) {
		log_debug("Discarding %s from %s; packet sent unicast%s",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr), client_id_str);
		goto exit;
	}

	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_NA);
	if (oc != NULL) {
		log_debug("Discarding %s from %s; "
			  "IA_NA option present%s",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr), client_id_str);
		goto exit;
	}
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_TA);
	if (oc != NULL) {
		log_debug("Discarding %s from %s; "
			  "IA_TA option present%s",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr), client_id_str);
		goto exit;
	}
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_PD);
	if (oc != NULL) {
		log_debug("Discarding %s from %s; "
			  "IA_PD option present%s",
			  dhcpv6_type_names[packet->dhcpv6_msg_type],
			  piaddr(packet->client_addr), client_id_str);
		goto exit;
	}

	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_SERVERID);
	if (oc != NULL) {
		if (!evaluate_option_cache(server_id, packet, NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("Error processing %s from %s; "
				  "unable to evaluate Server Identifier%s",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  piaddr(packet->client_addr), client_id_str);
			goto exit;
		}
		if ((server_duid.len != server_id->len) ||
		    (memcmp(server_duid.data, server_id->data,
		    	    server_duid.len) != 0)) {
			log_debug("Discarding %s from %s; "
				  "not our server identifier "
				  "(SERVERID %s, server DUID %s)%s",
				  dhcpv6_type_names[packet->dhcpv6_msg_type],
				  piaddr(packet->client_addr),
				  print_hex_1(server_id->len,
				  	      server_id->data, 60),
				  print_hex_2(server_duid.len,
				  	      server_duid.data, 60),
				  client_id_str);
			goto exit;
		}
	}

	/* looks good */
	ret_val = 1;

exit:
	if (!ret_val) {
		if (server_id->len > 0) {
			data_string_forget(server_id, MDL);
		}
	}
	return ret_val;
}

/*
 * Options that we want to send, in addition to what was requested
 * via the ORO.
 */
static const int required_opts[] = {
	D6O_CLIENTID,
	D6O_SERVERID,
	D6O_STATUS_CODE,
	D6O_PREFERENCE,
	0
};
static const int required_opts_solicit[] = {
	D6O_CLIENTID,
	D6O_SERVERID,
	D6O_IA_NA,
	D6O_IA_TA,
	D6O_IA_PD,
	D6O_RAPID_COMMIT,
	D6O_STATUS_CODE,
	D6O_RECONF_ACCEPT,
	D6O_PREFERENCE,
	0
};
static const int required_opts_agent[] = {
	D6O_INTERFACE_ID,
#if defined(RELAY_PORT)
	D6O_RELAY_SOURCE_PORT,
#endif
	D6O_RELAY_MSG,
	0
};
static const int required_opts_IA[] = {
	D6O_IAADDR,
	D6O_STATUS_CODE,
	0
};
static const int required_opts_IA_PD[] = {
	D6O_IAPREFIX,
	D6O_STATUS_CODE,
	0
};
static const int required_opts_STATUS_CODE[] = {
	D6O_STATUS_CODE,
	0
};
#ifdef DHCP4o6
static const int required_opts_4o6[] = {
	D6O_DHCPV4_MSG,
	0
};
#endif

static const int unicast_reject_opts[] = {
	D6O_CLIENTID,
	D6O_SERVERID,
	D6O_STATUS_CODE,
	0
};


/*
 * Extracts from packet contents an IA_* option, storing the IA structure
 * in its entirety in enc_opt_data, and storing any decoded DHCPv6 options
 * in enc_opt_state for later lookup and evaluation.  The 'offset' indicates
 * where in the IA_* the DHCPv6 options commence.
 */
static int
get_encapsulated_IA_state(struct option_state **enc_opt_state,
			  struct data_string *enc_opt_data,
			  struct packet *packet,
			  struct option_cache *oc,
			  int offset)
{
	/*
	 * Get the raw data for the encapsulated options.
	 */
	memset(enc_opt_data, 0, sizeof(*enc_opt_data));
	if (!evaluate_option_cache(enc_opt_data, packet,
				   NULL, NULL, packet->options, NULL,
				   &global_scope, oc, MDL)) {
		log_error("get_encapsulated_IA_state: "
			  "error evaluating raw option.");
		return 0;
	}
	if (enc_opt_data->len < offset) {
		log_error("get_encapsulated_IA_state: raw option too small.");
		data_string_forget(enc_opt_data, MDL);
		return 0;
	}

	/*
	 * Now create the option state structure, and pass it to the
	 * function that parses options.
	 */
	*enc_opt_state = NULL;
	if (!option_state_allocate(enc_opt_state, MDL)) {
		log_error("get_encapsulated_IA_state: no memory for options.");
		data_string_forget(enc_opt_data, MDL);
		return 0;
	}
	if (!parse_option_buffer(*enc_opt_state,
				 enc_opt_data->data + offset,
				 enc_opt_data->len - offset,
				 &dhcpv6_universe)) {
		log_error("get_encapsulated_IA_state: error parsing options.");
		option_state_dereference(enc_opt_state, MDL);
		data_string_forget(enc_opt_data, MDL);
		return 0;
	}

	return 1;
}

static int
set_status_code(u_int16_t status_code, const char *status_message,
		struct option_state *opt_state)
{
	struct data_string d;
	int ret_val;

	memset(&d, 0, sizeof(d));
	d.len = sizeof(status_code) + strlen(status_message);
	if (!buffer_allocate(&d.buffer, d.len, MDL)) {
		log_fatal("set_status_code: no memory for status code.");
	}
	d.data = d.buffer->data;
	putUShort(d.buffer->data, status_code);
	memcpy(d.buffer->data + sizeof(status_code),
	       status_message, d.len - sizeof(status_code));
	if (!save_option_buffer(&dhcpv6_universe, opt_state,
				d.buffer, (unsigned char *)d.data, d.len,
				D6O_STATUS_CODE, 0)) {
		log_error("set_status_code: error saving status code.");
		ret_val = 0;
	} else {
		ret_val = 1;
	}
	data_string_forget(&d, MDL);
	return ret_val;
}

static void check_pool6_threshold(struct reply_state *reply,
			   struct iasubopt *lease)
{
	struct ipv6_pond *pond;
	isc_uint64_t used, count, high_threshold;
	int poolhigh = 0, poollow = 0;
	char *shared_name = "no name";
	char tmp_addr[INET6_ADDRSTRLEN];

	if ((lease->ipv6_pool == NULL) || (lease->ipv6_pool->ipv6_pond == NULL))
		return;
	pond = lease->ipv6_pool->ipv6_pond;

	/* If the address range is too large to track, just skip all this. */
	if (pond->jumbo_range == 1) {
		return;
	}

	count = pond->num_total;
	used = pond->num_active;

	/* get network name for logging */
	if ((pond->shared_network != NULL) &&
	    (pond->shared_network->name != NULL)) {
		shared_name = pond->shared_network->name;
	}

	/* The logged flag indicates if we have already crossed the high
	 * threshold and emitted a log message.  If it is set we check to
	 * see if we have re-crossed the low threshold and need to reset
	 * things.  When we cross the high threshold we determine what
	 * the low threshold is and save it into the low_threshold value.
	 * When we cross that threshold we reset the logged flag and
	 * the low_threshold to 0 which allows the high threshold message
	 * to be emitted once again.
	 * if we haven't recrossed the boundry we don't need to do anything.
	 */
	if (pond->logged !=0) {
		if (used <= pond->low_threshold) {
			pond->low_threshold = 0;
			pond->logged = 0;
			log_error("Pool threshold reset - shared subnet: %s; "
				  "address: %s; low threshold %llu/%llu.",
				  shared_name,
				  inet_ntop(AF_INET6, &lease->addr,
					    tmp_addr, sizeof(tmp_addr)),
				  used, count);
		}
		return;
	}

	/* find the high threshold */
	if (get_option_int(&poolhigh, &server_universe, reply->packet, NULL,
			   NULL, reply->packet->options, reply->opt_state,
			   reply->opt_state, &lease->scope,
			   SV_LOG_THRESHOLD_HIGH, MDL) == 0) {
		/* no threshold bail out */
		return;
	}

	/* We do have a threshold for this pool, see if its valid */
	if ((poolhigh <= 0) || (poolhigh > 100)) {
		/* not valid */
		return;
	}

	/* we have a valid value, have we exceeded it */
	high_threshold = FIND_POND6_PERCENT(count, poolhigh);
	if (used < high_threshold) {
		/* nope, no more to do */
		return;
	}

	/* we've exceeded it, output a message */
	log_error("Pool threshold exceeded - shared subnet: %s; "
		  "address: %s; high threshold %d%% %llu/%llu.",
		  shared_name,
		  inet_ntop(AF_INET6, &lease->addr, tmp_addr, sizeof(tmp_addr)),
		  poolhigh, used, count);

	/* handle the low threshold now, if we don't
	 * have one we default to 0. */
	if ((get_option_int(&poollow, &server_universe, reply->packet, NULL,
			    NULL, reply->packet->options, reply->opt_state,
			    reply->opt_state, &lease->scope,
			    SV_LOG_THRESHOLD_LOW, MDL) == 0) ||
	    (poollow > 100)) {
		poollow = 0;
	}

	/*
	 * If the low theshold is higher than the high threshold we continue to log
	 * If it isn't then we set the flag saying we already logged and determine
	 * what the reset threshold is.
	 */
	if (poollow < poolhigh) {
		pond->logged = 1;
		pond->low_threshold = FIND_POND6_PERCENT(count, poollow);
	}
}

/*
 * We have a set of operations we do to set up the reply packet, which
 * is the same for many message types.
 */
static int
start_reply(struct packet *packet,
	    const struct data_string *client_id,
	    const struct data_string *server_id,
	    struct option_state **opt_state,
	    struct dhcpv6_packet *reply)
{
	struct option_cache *oc;
	const unsigned char *server_id_data;
	int server_id_len;

	/*
	 * Build our option state for reply.
	 */
	*opt_state = NULL;
	if (!option_state_allocate(opt_state, MDL)) {
		log_error("start_reply: no memory for option_state.");
		return 0;
	}
	execute_statements_in_scope(NULL, packet, NULL, NULL,
				    packet->options, *opt_state,
				    &global_scope, root_group, NULL, NULL);

	/*
	 * A small bit of special handling for Solicit messages.
	 *
	 * We could move the logic into a flag, but for now just check
	 * explicitly.
	 */
	if (packet->dhcpv6_msg_type == DHCPV6_SOLICIT) {
		reply->msg_type = DHCPV6_ADVERTISE;

		/*
		 * If:
		 * - this message type supports rapid commit (Solicit), and
		 * - the server is configured to supply a rapid commit, and
		 * - the client requests a rapid commit,
		 * Then we add a rapid commit option, and send Reply (instead
		 * of an Advertise).
		 */
		oc = lookup_option(&dhcpv6_universe,
				   *opt_state, D6O_RAPID_COMMIT);
		if (oc != NULL) {
			oc = lookup_option(&dhcpv6_universe,
					   packet->options, D6O_RAPID_COMMIT);
			if (oc != NULL) {
				/* Rapid-commit in action. */
				reply->msg_type = DHCPV6_REPLY;
			} else {
				/* Don't want a rapid-commit in advertise. */
				delete_option(&dhcpv6_universe,
					      *opt_state, D6O_RAPID_COMMIT);
			}
		}
	} else {
		reply->msg_type = DHCPV6_REPLY;
		/* Delete the rapid-commit from the sent options. */
		oc = lookup_option(&dhcpv6_universe,
				   *opt_state, D6O_RAPID_COMMIT);
		if (oc != NULL) {
			delete_option(&dhcpv6_universe,
				      *opt_state, D6O_RAPID_COMMIT);
		}
	}

	/*
	 * Use the client's transaction identifier for the reply.
	 */
	memcpy(reply->transaction_id, packet->dhcpv6_transaction_id,
	       sizeof(reply->transaction_id));

	/*
	 * RFC 3315, section 18.2 says we need server identifier and
	 * client identifier.
	 *
	 * If the server ID is defined via the configuration file, then
	 * it will already be present in the option state at this point,
	 * so we don't need to set it.
	 *
	 * If we have a server ID passed in from the caller,
	 * use that, otherwise use the global DUID.
	 */
	oc = lookup_option(&dhcpv6_universe, *opt_state, D6O_SERVERID);
	if (oc == NULL) {
		if (server_id == NULL) {
			server_id_data = server_duid.data;
			server_id_len = server_duid.len;
		} else {
			server_id_data = server_id->data;
			server_id_len = server_id->len;
		}
		if (!save_option_buffer(&dhcpv6_universe, *opt_state,
					NULL, (unsigned char *)server_id_data,
					server_id_len, D6O_SERVERID, 0)) {
				log_error("start_reply: "
					  "error saving server identifier.");
				return 0;
		}
	}

	if (client_id->buffer != NULL) {
		if (!save_option_buffer(&dhcpv6_universe, *opt_state,
					client_id->buffer,
					(unsigned char *)client_id->data,
					client_id->len,
					D6O_CLIENTID, 0)) {
			log_error("start_reply: error saving "
				  "client identifier.");
			return 0;
		}
	}

	/*
	 * If the client accepts reconfiguration, let it know that we
	 * will send them.
	 *
	 * Note: we don't actually do this yet, but DOCSIS requires we
	 *       claim to.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options,
			   D6O_RECONF_ACCEPT);
	if (oc != NULL) {
		if (!save_option_buffer(&dhcpv6_universe, *opt_state,
					NULL, (unsigned char *)"", 0,
					D6O_RECONF_ACCEPT, 0)) {
			log_error("start_reply: "
				  "error saving RECONF_ACCEPT option.");
			option_state_dereference(opt_state, MDL);
			return 0;
		}
	}

	return 1;
}

/*
 * Try to get the IPv6 address the client asked for from the
 * pool.
 *
 * addr is the result (should be a pointer to NULL on entry)
 * pool is the pool to search in
 * requested_addr is the address the client wants
 */
static isc_result_t
try_client_v6_address(struct iasubopt **addr,
		      struct ipv6_pool *pool,
		      const struct data_string *requested_addr)
{
	struct in6_addr tmp_addr;
	isc_result_t result;

	if (requested_addr->len < sizeof(tmp_addr)) {
		return DHCP_R_INVALIDARG;
	}
	memcpy(&tmp_addr, requested_addr->data, sizeof(tmp_addr));
	if (IN6_IS_ADDR_UNSPECIFIED(&tmp_addr)) {
		return ISC_R_FAILURE;
	}

	/*
	 * The address is not covered by this (or possibly any) dynamic
	 * range.
	 */
	if (!ipv6_in_pool(&tmp_addr, pool)) {
		return ISC_R_ADDRNOTAVAIL;
	}

	if (lease6_exists(pool, &tmp_addr)) {
		return ISC_R_ADDRINUSE;
	}

	result = iasubopt_allocate(addr, MDL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	(*addr)->addr = tmp_addr;
	(*addr)->plen = 0;

	/* Default is soft binding for 2 minutes. */
	result = add_lease6(pool, *addr, cur_time + 120);
	if (result != ISC_R_SUCCESS) {
		iasubopt_dereference(addr, MDL);
	}
	return result;
}

/*!
 *
 * \brief  Get an IPv6 address for the client.
 *
 * Attempt to find a usable address for the client.  We walk through
 * the ponds checking for permit and deny then through the pools
 * seeing if they have an available address.
 *
 * \param reply = the state structure for the current work on this request
 *                if we create a lease we return it using reply->lease
 *
 * \return
 * ISC_R_SUCCESS = we were able to find an address and are returning a
 *                 pointer to the lease
 * ISC_R_NORESOURCES = there don't appear to be any free addresses.  This
 *                     is probabalistic.  We don't exhaustively try the
 *                     address range, instead we hash the duid and if
 *                     the address derived from the hash is in use we
 *                     hash the address.  After a number of failures we
 *                     conclude the pool is basically full.
 */
static isc_result_t
pick_v6_address(struct reply_state *reply)
{
	struct ipv6_pool *p = NULL;
	struct ipv6_pond *pond;
	int i;
	int start_pool;
	unsigned int attempts;
	char tmp_buf[INET6_ADDRSTRLEN];
	struct iasubopt **addr = &reply->lease;
        isc_uint64_t total = 0;
        isc_uint64_t active = 0;
        isc_uint64_t abandoned = 0;
	int jumbo_range = 0;
	char *shared_name = (reply->shared->name ?
			     reply->shared->name : "(no name)");

	/*
	 * Do a quick walk through of the ponds and pools
	 * to see if we have any NA address pools
	 */
	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (pond->ipv6_pools == NULL)
			continue;

		for (i = 0; (p = pond->ipv6_pools[i]) != NULL; i++) {
			if (p->pool_type == D6O_IA_NA)
				break;
		}
		if (p != NULL)
			break;
	}

	/* If we get here and p is NULL we have no useful pools */
	if (p == NULL) {
		log_debug("Unable to pick client address: "
			  "no IPv6 pools on this shared network");
		return ISC_R_NORESOURCES;
	}

	/*
	 * We have at least one pool that could provide an address
	 * Now we walk through the ponds and pools again and check
	 * to see if the client is permitted and if an address is
	 * available
	 *
	 * Within a given pond we start looking at the last pool we
	 * allocated from, unless it had a collision trying to allocate
	 * an address. This will tend to move us into less-filled pools.
	 */

	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		isc_result_t result = ISC_R_FAILURE;

		if (((pond->prohibit_list != NULL) &&
		     (permitted(reply->packet, pond->prohibit_list))) ||
		    ((pond->permit_list != NULL) &&
		     (!permitted(reply->packet, pond->permit_list))))
			continue;

#ifdef EUI_64
		/* If pond is EUI-64 but client duid isn't a valid EUI-64
		 * id, then skip this pond */
		if (pond->use_eui_64 &&
		    !valid_eui_64_duid(&reply->ia->iaid_duid, IAID_LEN)) {
			continue;
		}
#endif

		start_pool = pond->last_ipv6_pool;
		i = start_pool;
		do {
			p = pond->ipv6_pools[i];
			if (p->pool_type == D6O_IA_NA) {
#ifdef EUI_64
				if (pond->use_eui_64) {
					result =
					create_lease6_eui_64(p, addr,
					              &reply->ia->iaid_duid,
					              cur_time + 120);
				}
				else
#endif
				{
					result =
					create_lease6(p, addr, &attempts,
						      &reply->ia->iaid_duid,
					              cur_time + 120);

				}

				if (result == ISC_R_SUCCESS) {
					/*
					 * Record the pool used (or next one if
					 * there was a collision).
					 */
					if (attempts > 1) {
						i++;
						if (pond->ipv6_pools[i]
						    == NULL) {
							i = 0;
						}
					}

					pond->last_ipv6_pool = i;

					log_debug("Picking pool address %s",
						  inet_ntop(AF_INET6,
						  &((*addr)->addr),
						  tmp_buf, sizeof(tmp_buf)));
					return (ISC_R_SUCCESS);
				}
			}

			i++;
			if (pond->ipv6_pools[i] == NULL) {
				i = 0;
			}
		} while (i != start_pool);

		if (result == ISC_R_NORESOURCES) {
			jumbo_range += pond->jumbo_range;
			total += pond->num_total;
			active += pond->num_active;
			abandoned += pond->num_abandoned;
		}
	}

	/*
	 * If we failed to pick an IPv6 address from any of the subnets.
	 * Presumably that means we have no addresses for the client.
	 */
	if (jumbo_range != 0) {
		log_debug("Unable to pick client address: "
			  "no addresses available  - shared network %s: "
			  " 2^64-1 < total, %llu active,  %llu abandoned",
			  shared_name, active - abandoned, abandoned);
	} else {
		log_debug("Unable to pick client address: "
			  "no addresses available  - shared network %s: "
			  "%llu total, %llu active,  %llu abandoned",
			  shared_name, total, active - abandoned, abandoned);
	}

	return ISC_R_NORESOURCES;
}

/*
 * Try to get the IPv6 prefix the client asked for from the
 * prefix pool.
 *
 * pref is the result (should be a pointer to NULL on entry)
 * pool is the prefix pool to search in
 * requested_pref is the address the client wants
 */
static isc_result_t
try_client_v6_prefix(struct iasubopt **pref,
		     struct ipv6_pool *pool,
		     const struct data_string *requested_pref)
{
	u_int8_t tmp_plen;
	struct in6_addr tmp_pref;
	struct iaddr ia;
	isc_result_t result;

	if (requested_pref->len < sizeof(tmp_plen) + sizeof(tmp_pref)) {
		return DHCP_R_INVALIDARG;
	}

	tmp_plen = (int) requested_pref->data[0];
	if ((tmp_plen < 3) || (tmp_plen > 128)) {
		return ISC_R_FAILURE;
	}

	memcpy(&tmp_pref, requested_pref->data + 1, sizeof(tmp_pref));
	if (IN6_IS_ADDR_UNSPECIFIED(&tmp_pref)) {
		return ISC_R_FAILURE;
	}

	ia.len = 16;
	memcpy(&ia.iabuf, &tmp_pref, 16);
	if (!is_cidr_mask_valid(&ia, (int) tmp_plen)) {
		return ISC_R_FAILURE;
	}

	if (!ipv6_in_pool(&tmp_pref, pool) ||
	    ((int)tmp_plen != pool->units)) {
		return ISC_R_ADDRNOTAVAIL;
	}

	if (prefix6_exists(pool, &tmp_pref, tmp_plen)) {
		return ISC_R_ADDRINUSE;
	}

	result = iasubopt_allocate(pref, MDL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	(*pref)->addr = tmp_pref;
	(*pref)->plen = tmp_plen;

	/* Default is soft binding for 2 minutes. */
	result = add_lease6(pool, *pref, cur_time + 120);
	if (result != ISC_R_SUCCESS) {
		iasubopt_dereference(pref, MDL);
	}

	return result;
}

/*!
 *
 * \brief  Get an IPv6 prefix for the client.
 *
 * Attempt to find a usable prefix for the client.  Based upon the prefix
 * length mode and the plen supplied by the client (if one), we make one
 * or more calls to pick_v6_prefix_helper() to find a prefix as follows:
 *
 * PLM_IGNORE or client specifies a plen of zero, use the first available
 * prefix regardless of it's length.
 *
 * PLM_PREFER – look for an exact match to client's plen first, if none
 * found, use the first available prefix of any length
 *
 * PLM_EXACT – look for an exact match first, if none found then fail. This
 * is the default behavior.
 *
 * PLM_MAXIMUM  - look for an exact match first, then the first available whose
 * prefix length is less than client's plen, otherwise fail.
 *
 * PLM_MINIMUM  - look for an exact match first, then the first available whose
 * prefix length is greater than client's plen, otherwise fail.
 *
 * Note that the selection mode is configurable at the global scope only via
 * prefix-len-mode.
 *
 * \param reply = the state structure for the current work on this request
 *                if we create a lease we return it using reply->lease
 *
 * \return
 * ISC_R_SUCCESS = we were able to find an prefix and are returning a
 *                 pointer to the lease
 * ISC_R_NORESOURCES = there don't appear to be any free addresses.  This
 *                     is probabalistic.  We don't exhaustively try the
 *                     address range, instead we hash the duid and if
 *                     the address derived from the hash is in use we
 *                     hash the address.  After a number of failures we
 *                     conclude the pool is basically full.
 */
static isc_result_t
pick_v6_prefix(struct reply_state *reply) {
        struct ipv6_pool *p = NULL;
        struct ipv6_pond *pond;
        int i;
	isc_result_t result;

	/*
	 * Do a quick walk through of the ponds and pools
	 * to see if we have any prefix pools
	*/
	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (pond->ipv6_pools == NULL)
			continue;

		for (i = 0; (p = pond->ipv6_pools[i]) != NULL; i++) {
			if (p->pool_type == D6O_IA_PD)
				break;
		}
		if (p != NULL)
			break;
	}

	/* If we get here and p is NULL we have no useful pools */
	if (p == NULL) {
		log_debug("Unable to pick client prefix: "
			  "no IPv6 pools on this shared network");
		return ISC_R_NORESOURCES;
	}

	if (reply->preflen <= 0) {
		/* If we didn't get a plen (-1) or client plen is 0, then just
		 * select first available (same as PLM_INGORE) */
		result = pick_v6_prefix_helper(reply, PLM_IGNORE);
	} else {
		switch (prefix_length_mode) {
		case PLM_PREFER:
			/* First we look for an exact match, if not found
			 * then first available */
			result = pick_v6_prefix_helper(reply, PLM_EXACT);
			if (result != ISC_R_SUCCESS) {
				result = pick_v6_prefix_helper(reply,
							      PLM_IGNORE);
			}
			break;

		case PLM_EXACT:
			/* Match exactly or fail */
			result = pick_v6_prefix_helper(reply, PLM_EXACT);
			break;

		case PLM_MINIMUM:
		case PLM_MAXIMUM:
			/* First we look for an exact match, if not found
			 * then first available by mode */
			result = pick_v6_prefix_helper(reply, PLM_EXACT);
			if (result != ISC_R_SUCCESS) {
				result = pick_v6_prefix_helper(reply,
							    prefix_length_mode);
			}
			break;

		default:
			/* First available */
			result = pick_v6_prefix_helper(reply, PLM_IGNORE);
			break;
		}
	}

	if (result == ISC_R_SUCCESS) {
		char tmp_buf[INET6_ADDRSTRLEN];

		log_debug("Picking pool prefix %s/%u",
			  inet_ntop(AF_INET6, &(reply->lease->addr),
				    tmp_buf, sizeof(tmp_buf)),
				    (unsigned)(reply->lease->plen));
		return (ISC_R_SUCCESS);
	}

	/*
	 * If we failed to pick an IPv6 prefix
	 * Presumably that means we have no prefixes for the client.
	*/
	log_debug("Unable to pick client prefix: no prefixes available");
	return ISC_R_NORESOURCES;
}

/*!
 *
 * \brief  Get an IPv6 prefix for the client based upon selection mode.
 *
 * We walk through the ponds checking for permit and deny. If a pond is
 * permissable to use, loop through its PD pools checking prefix lengths
 * against the client plen based on the prefix length mode, looking for
 * available prefixes.
 *
 * \param reply = the state structure for the current work on this request
 *                if we create a lease we return it using reply->lease
 * \prefix_mode = selection mode to use
 *
 * \return
 * ISC_R_SUCCESS = we were able to find a prefix and are returning a
 *                 pointer to the lease
 * ISC_R_NORESOURCES = there don't appear to be any free addresses.  This
 *                     is probabalistic.  We don't exhaustively try the
 *                     address range, instead we hash the duid and if
 *                     the address derived from the hash is in use we
 *                     hash the address.  After a number of failures we
 *                     conclude the pool is basically full.
 */
isc_result_t
pick_v6_prefix_helper(struct reply_state *reply, int prefix_mode) {
	struct ipv6_pool *p = NULL;
	struct ipv6_pond *pond;
	int i;
	unsigned int attempts;
	struct iasubopt **pref = &reply->lease;

	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (((pond->prohibit_list != NULL) &&
		     (permitted(reply->packet, pond->prohibit_list))) ||
		    ((pond->permit_list != NULL) &&
		     (!permitted(reply->packet, pond->permit_list))))
			continue;

		for (i = 0; (p = pond->ipv6_pools[i]) != NULL; i++) {
			if ((p->pool_type == D6O_IA_PD) &&
			    (eval_prefix_mode(p->units, reply->preflen,
					      prefix_mode) == 1) &&
			    (create_prefix6(p, pref, &attempts,
					    &reply->ia->iaid_duid,
					    cur_time + 120) == ISC_R_SUCCESS)) {
				return (ISC_R_SUCCESS);
			}
		}
	}

	return ISC_R_NORESOURCES;
}

/*!
 *
 * \brief Test a prefix length against another based on prefix length mode
 *
 * \param len - prefix length to test
 * \param preflen - preferred prefix length against which to test
 * \param prefix_mode - prefix selection mode with which to test
 *
 * Note that the case of preferred length of 0 is not short-cut here as it
 * is assumed to be done at a higher level.
 *
 * \return 1 if the given length is usable based upon mode and a preferred
 * length, 0 if not.
 */
int
eval_prefix_mode(int len, int preflen, int prefix_mode) {
	int use_it = 1;
	switch (prefix_mode) {
	case PLM_EXACT:
		use_it = (len == preflen);
		break;
	case PLM_MINIMUM:
		/* they asked for a prefix length no "shorter" than preflen */
		use_it = (len >= preflen);
		break;
	case PLM_MAXIMUM:
		/* they asked for a prefix length no "longer" than preflen */
		use_it = (len <= preflen);
		break;
	default:
		/* otherwise use it */
		break;
	}

	return (use_it);
}

/*
 *! \file server/dhcpv6.c
 *
 * \brief construct a reply containing information about a client's lease
 *
 * lease_to_client() is called from several messages to construct a
 * reply that contains all that we know about the client's correct lease
 * (or projected lease).
 *
 * Solicit - "Soft" binding, ignore unknown addresses or bindings, just
 *	     send what we "may" give them on a request.
 *
 * Request - "Hard" binding, but ignore supplied addresses (just provide what
 *	     the client should really use).
 *
 * Renew   - "Hard" binding, but client-supplied addresses are 'real'.  Error
 * Rebind    out any "wrong" addresses the client sends.  This means we send
 *	     an empty IA_NA with a status code of NoBinding or NotOnLink or
 *	     possibly send the address with zeroed lifetimes.
 *
 * Information-Request - No binding.
 *
 * The basic structure is to traverse the client-supplied data first, and
 * validate and echo back any contents that can be.  If the client-supplied
 * data does not error out (on renew/rebind as above), but we did not send
 * any addresses, attempt to allocate one.
 *
 * At the end of the this function we call commit_leases_timed() to
 * fsync and rotate the file as necessary.  commit_leases_timed() will
 * check that we have written at least one lease to the file and that
 * some time has passed before doing any fsync or file rewrite so we
 * don't bother tracking if we did a write_ia during this function.
 */
/* TODO: look at client hints for lease times */

static void
lease_to_client(struct data_string *reply_ret,
		struct packet *packet,
		const struct data_string *client_id,
		const struct data_string *server_id)
{
	static struct reply_state reply;
	struct option_cache *oc;
	struct data_string packet_oro;
	int i;

	memset(&packet_oro, 0, sizeof(packet_oro));

	/* Locate the client.  */
	if (shared_network_from_packet6(&reply.shared,
					packet) != ISC_R_SUCCESS)
		goto exit;

	/*
	 * Initialize the reply.
	 */
	packet_reference(&reply.packet, packet, MDL);
	data_string_copy(&reply.client_id, client_id, MDL);

	if (!start_reply(packet, client_id, server_id, &reply.opt_state,
			 &reply.buf.reply))
		goto exit;

	/* Set the write cursor to just past the reply header. */
	reply.cursor = REPLY_OPTIONS_INDEX;

	/*
	 * Get the ORO from the packet, if any.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_ORO);
	if (oc != NULL) {
		if (!evaluate_option_cache(&packet_oro, packet,
					   NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("lease_to_client: error evaluating ORO.");
			goto exit;
		}
	}

	/*
	 * Find a host record that matches the packet, if any, and is
	 * valid for the shared network the client is on.
	 */
	if (find_hosts6(&reply.host, packet, client_id, MDL)) {
		packet->known = 1;
		seek_shared_host(&reply.host, reply.shared);
	}

	/* Process the client supplied IA's onto the reply buffer. */
	reply.ia_count = 0;
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_NA);

	for (; oc != NULL ; oc = oc->next) {
		isc_result_t status;

		/* Start counting resources (addresses) offered. */
		reply.client_resources = 0;
		reply.resources_included = ISC_FALSE;

		status = reply_process_ia_na(&reply, oc);

		/*
		 * We continue to try other IA's whether we can address
		 * this one or not.  Any other result is an immediate fail.
		 */
		if ((status != ISC_R_SUCCESS) &&
		    (status != ISC_R_NORESOURCES))
			goto exit;
	}
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_TA);
	for (; oc != NULL ; oc = oc->next) {
		isc_result_t status;

		/* Start counting resources (addresses) offered. */
		reply.client_resources = 0;
		reply.resources_included = ISC_FALSE;

		status = reply_process_ia_ta(&reply, oc);

		/*
		 * We continue to try other IA's whether we can address
		 * this one or not.  Any other result is an immediate fail.
		 */
		if ((status != ISC_R_SUCCESS) &&
		    (status != ISC_R_NORESOURCES))
			goto exit;
	}

	/* Same for IA_PD's. */
	reply.pd_count = 0;
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_PD);
	for (; oc != NULL ; oc = oc->next) {
		isc_result_t status;

		/* Start counting resources (prefixes) offered. */
		reply.client_resources = 0;
		reply.resources_included = ISC_FALSE;

		status = reply_process_ia_pd(&reply, oc);

		/*
		 * We continue to try other IA_PD's whether we can address
		 * this one or not.  Any other result is an immediate fail.
		 */
		if ((status != ISC_R_SUCCESS) &&
		    (status != ISC_R_NORESOURCES))
			goto exit;
	}

	/*
	 * Make no reply if we gave no resources and is not
	 * for Information-Request.
	 */
	if ((reply.ia_count == 0) && (reply.pd_count == 0)) {
		if (reply.packet->dhcpv6_msg_type !=
					    DHCPV6_INFORMATION_REQUEST)
			goto exit;

		/*
		 * Because we only execute statements on a per-IA basis,
		 * we need to execute statements in any non-IA reply to
		 * source configuration.
		 */
		execute_statements_in_scope(NULL, reply.packet, NULL, NULL,
					    reply.packet->options,
					    reply.opt_state, &global_scope,
					    reply.shared->group, root_group,
					    NULL);

		/* Execute statements from class scopes. */
		for (i = reply.packet->class_count; i > 0; i--) {
			execute_statements_in_scope(NULL, reply.packet,
						    NULL, NULL,
						    reply.packet->options,
						    reply.opt_state,
						    &global_scope,
						    reply.packet->classes[i - 1]->group,
						    reply.shared->group, NULL);
		}

		/* Bring in any configuration from a host record. */
		if (reply.host != NULL)
			execute_statements_in_scope(NULL, reply.packet,
						    NULL, NULL,
						    reply.packet->options,
						    reply.opt_state,
						    &global_scope,
						    reply.host->group,
						    reply.shared->group, NULL);
	}

	/*
	 * RFC3315 section 17.2.2 (Solicit):
	 *
	 * If the server will not assign any addresses to any IAs in a
	 * subsequent Request from the client, the server MUST send an
	 * Advertise message to the client that includes only a Status
	 * Code option with code NoAddrsAvail and a status message for
	 * the user, a Server Identifier option with the server's DUID,
	 * and a Client Identifier option with the client's DUID.
	 *
	 * This has been updated by an errata such that the server
	 * can always send an IA.
	 *
	 * Section 18.2.1 (Request):
	 *
	 * If the server cannot assign any addresses to an IA in the
	 * message from the client, the server MUST include the IA in
	 * the Reply message with no addresses in the IA and a Status
	 * Code option in the IA containing status code NoAddrsAvail.
	 *
	 * Section 18.1.8 (Client Behavior):
	 *
	 * Leave unchanged any information about addresses the client has
	 * recorded in the IA but that were not included in the IA from
	 * the server.
	 * Sends a Renew/Rebind if the IA is not in the Reply message.
	 */

	/*
	 * Having stored the client's IA's, store any options that
	 * will fit in the remaining space.
	 */
	reply.cursor += store_options6((char *)reply.buf.data + reply.cursor,
				       sizeof(reply.buf) - reply.cursor,
				       reply.opt_state, reply.packet,
				       required_opts_solicit,
				       &packet_oro);

	/* Return our reply to the caller. */
	reply_ret->len = reply.cursor;
	reply_ret->buffer = NULL;
	if (!buffer_allocate(&reply_ret->buffer, reply.cursor, MDL)) {
		log_fatal("No memory to store Reply.");
	}
	memcpy(reply_ret->buffer->data, reply.buf.data, reply.cursor);
	reply_ret->data = reply_ret->buffer->data;

	/* If appropriate commit and rotate the lease file */
	(void) commit_leases_timed();

      exit:
	/* Cleanup. */
	if (reply.shared != NULL)
		shared_network_dereference(&reply.shared, MDL);
	if (reply.host != NULL)
		host_dereference(&reply.host, MDL);
	if (reply.opt_state != NULL)
		option_state_dereference(&reply.opt_state, MDL);
	if (reply.packet != NULL)
		packet_dereference(&reply.packet, MDL);
	if (reply.client_id.data != NULL)
		data_string_forget(&reply.client_id, MDL);
	if (packet_oro.buffer != NULL)
		data_string_forget(&packet_oro, MDL);
	reply.renew = reply.rebind = reply.min_prefer = reply.min_valid = 0;
	reply.cursor = 0;
}

/* Process a client-supplied IA_NA.  This may append options to the tail of
 * the reply packet being built in the reply_state structure.
 */
static isc_result_t
reply_process_ia_na(struct reply_state *reply, struct option_cache *ia) {
	isc_result_t status = ISC_R_SUCCESS;
	u_int32_t iaid;
	unsigned ia_cursor;
	struct option_state *packet_ia;
	struct option_cache *oc;
	struct data_string ia_data, data;

	/* Initialize values that will get cleaned up on return. */
	packet_ia = NULL;
	memset(&ia_data, 0, sizeof(ia_data));
	memset(&data, 0, sizeof(data));
	/*
	 * Note that find_client_address() may set reply->lease.
	 */

	/* Make sure there is at least room for the header. */
	if ((reply->cursor + IA_NA_OFFSET + 4) > sizeof(reply->buf)) {
		log_error("reply_process_ia_na: Reply too long for IA.");
		return ISC_R_NOSPACE;
	}


	/* Fetch the IA_NA contents. */
	if (!get_encapsulated_IA_state(&packet_ia, &ia_data, reply->packet,
				       ia, IA_NA_OFFSET)) {
		log_error("reply_process_ia_na: error evaluating ia");
		status = ISC_R_FAILURE;
		goto cleanup;
	}

	/* Extract IA_NA header contents. */
	iaid = getULong(ia_data.data);
	reply->renew = getULong(ia_data.data + 4);
	reply->rebind = getULong(ia_data.data + 8);

	/* Create an IA_NA structure. */
	if (ia_allocate(&reply->ia, iaid, (char *)reply->client_id.data,
			reply->client_id.len, MDL) != ISC_R_SUCCESS) {
		log_error("reply_process_ia_na: no memory for ia.");
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}
	reply->ia->ia_type = D6O_IA_NA;

	/* Cache pre-existing IA, if any. */
	ia_hash_lookup(&reply->old_ia, ia_na_active,
		       (unsigned char *)reply->ia->iaid_duid.data,
		       reply->ia->iaid_duid.len, MDL);

	/*
	 * Create an option cache to carry the IA_NA option contents, and
	 * execute any user-supplied values into it.
	 */
	if (!option_state_allocate(&reply->reply_ia, MDL)) {
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}

	/* Check & cache the fixed host record. */
	if ((reply->host != NULL) && (reply->host->fixed_addr != NULL)) {
		struct iaddr tmp_addr;

		if (!evaluate_option_cache(&reply->fixed, NULL, NULL, NULL,
					   NULL, NULL, &global_scope,
					   reply->host->fixed_addr, MDL)) {
			log_error("reply_process_ia_na: unable to evaluate "
				  "fixed address.");
			status = ISC_R_FAILURE;
			goto cleanup;
		}

		if (reply->fixed.len < 16) {
			log_error("reply_process_ia_na: invalid fixed address.");
			status = DHCP_R_INVALIDARG;
			goto cleanup;
		}

		/* Find the static lease's subnet. */
		tmp_addr.len = 16;
		memcpy(tmp_addr.iabuf, reply->fixed.data, 16);

		if (find_grouped_subnet(&reply->subnet, reply->shared,
					tmp_addr, MDL) == 0)
			log_fatal("Impossible condition at %s:%d.", MDL);

		reply->static_lease = ISC_TRUE;
	} else
		reply->static_lease = ISC_FALSE;

	/*
	 * Save the cursor position at the start of the IA, so we can
	 * set length and adjust t1/t2 values later.  We write a temporary
	 * header out now just in case we decide to adjust the packet
	 * within sub-process functions.
	 */
	ia_cursor = reply->cursor;

	/* Initialize the IA_NA header.  First the code. */
	putUShort(reply->buf.data + reply->cursor, (unsigned)D6O_IA_NA);
	reply->cursor += 2;

	/* Then option length. */
	putUShort(reply->buf.data + reply->cursor, 0x0Cu);
	reply->cursor += 2;

	/* Then IA_NA header contents; IAID. */
	putULong(reply->buf.data + reply->cursor, iaid);
	reply->cursor += 4;

	/* We store the client's t1 for now, and may over-ride it later. */
	putULong(reply->buf.data + reply->cursor, reply->renew);
	reply->cursor += 4;

	/* We store the client's t2 for now, and may over-ride it later. */
	putULong(reply->buf.data + reply->cursor, reply->rebind);
	reply->cursor += 4;

	/*
	 * For each address in this IA_NA, decide what to do about it.
	 *
	 * Guidelines:
	 *
	 * The client leaves unchanged any information about addresses
	 * it has recorded but are not included ("cancel/break" below).
	 * A not included IA ("cleanup" below) could give a Renew/Rebind.
	 */
	oc = lookup_option(&dhcpv6_universe, packet_ia, D6O_IAADDR);
	reply->min_valid = reply->min_prefer = INFINITE_TIME;
	reply->client_valid = reply->client_prefer = 0;
	for (; oc != NULL ; oc = oc->next) {
		status = reply_process_addr(reply, oc);

		/*
		 * Canceled means we did not allocate addresses to the
		 * client, but we're "done" with this IA - we set a status
		 * code.  So transmit this reply, e.g., move on to the next
		 * IA.
		 */
		if (status == ISC_R_CANCELED)
			break;

		if ((status != ISC_R_SUCCESS) &&
		    (status != ISC_R_ADDRINUSE) &&
		    (status != ISC_R_ADDRNOTAVAIL))
			goto cleanup;
	}

	reply->ia_count++;

	/*
	 * If we fell through the above and never gave the client
	 * an address, give it one now.
	 */
	if ((status != ISC_R_CANCELED) && (reply->client_resources == 0)) {
		status = find_client_address(reply);

		if (status == ISC_R_NORESOURCES) {
			switch (reply->packet->dhcpv6_msg_type) {
			      case DHCPV6_SOLICIT:
				/*
				 * No address for any IA is handled
				 * by the caller.
				 */
				/* FALL THROUGH */

			      case DHCPV6_REQUEST:
				/* Section 18.2.1 (Request):
				 *
				 * If the server cannot assign any addresses to
				 * an IA in the message from the client, the
				 * server MUST include the IA in the Reply
				 * message with no addresses in the IA and a
				 * Status Code option in the IA containing
				 * status code NoAddrsAvail.
				 */
				option_state_dereference(&reply->reply_ia, MDL);
				if (!option_state_allocate(&reply->reply_ia,
							   MDL))
				{
					log_error("reply_process_ia_na: No "
						  "memory for option state "
						  "wipe.");
					status = ISC_R_NOMEMORY;
					goto cleanup;
				}

				if (!set_status_code(STATUS_NoAddrsAvail,
						     "No addresses available "
						     "for this interface.",
						      reply->reply_ia)) {
					log_error("reply_process_ia_na: Unable "
						  "to set NoAddrsAvail status "
						  "code.");
					status = ISC_R_FAILURE;
					goto cleanup;
				}

				status = ISC_R_SUCCESS;
				break;

			      default:
				/*
				 * RFC 3315 does not tell us to emit a status
				 * code in this condition, or anything else.
				 *
				 * If we included non-allocated addresses
				 * (zeroed lifetimes) in an IA, then the client
				 * will deconfigure them.
				 *
				 * So we want to include the IA even if we
				 * can't give it a new address if it includes
				 * zeroed lifetime addresses.
				 *
				 * We don't want to include the IA if we
				 * provide zero addresses including zeroed
				 * lifetimes.
				 */
				if (reply->resources_included)
					status = ISC_R_SUCCESS;
				else
					goto cleanup;
				break;
			}
		}

		if (status != ISC_R_SUCCESS)
			goto cleanup;
	}

	/*
	 * yes, goto's aren't the best but we also want to avoid extra
	 * indents
	 */
	if (status == ISC_R_CANCELED) {
		/* We're replying with a status code so we still need to
		 * write it out in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);
		goto cleanup;
	}

	/*
	 * Handle static leases, we always log stuff and if it's
	 * a hard binding we run any commit statements that we have
	 */
	if (reply->static_lease) {
		char tmp_addr[INET6_ADDRSTRLEN];
		log_info("%s NA: address %s to client with duid %s iaid = %d "
			 "static",
			 dhcpv6_type_names[reply->buf.reply.msg_type],
			 inet_ntop(AF_INET6, reply->fixed.data, tmp_addr,
				   sizeof(tmp_addr)),
			 print_hex_1(reply->client_id.len,
				     reply->client_id.data, 60),
			 iaid);

		/* Write the lease out in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);
#ifdef NSUPDATE
		/* Performs DDNS updates if we're configured to do them */
		ddns_update_static6(reply);
#endif
		if ((reply->buf.reply.msg_type == DHCPV6_REPLY) &&
		    (reply->on_star.on_commit != NULL)) {
			execute_statements(NULL, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state, NULL,
					   reply->on_star.on_commit, NULL);
			executable_statement_dereference
				(&reply->on_star.on_commit, MDL);
		}
		goto cleanup;
	}

	/*
	 * If we have any addresses log what we are doing.
	 */
	if (reply->ia->num_iasubopt != 0) {
		struct iasubopt *tmp;
		int i;
		char tmp_addr[INET6_ADDRSTRLEN];

		for (i = 0 ; i < reply->ia->num_iasubopt ; i++) {
			tmp = reply->ia->iasubopt[i];

			log_info("%s NA: address %s to client with duid %s "
				 "iaid = %d valid for %u seconds",
				 dhcpv6_type_names[reply->buf.reply.msg_type],
				 inet_ntop(AF_INET6, &tmp->addr,
					   tmp_addr, sizeof(tmp_addr)),
				 print_hex_1(reply->client_id.len,
					     reply->client_id.data, 60),
				 iaid, tmp->valid);
		}
	}

	/*
	 * If this is not a 'soft' binding, consume the new changes into
	 * the database (if any have been attached to the ia_na).
	 *
	 * Loop through the assigned dynamic addresses, referencing the
	 * leases onto this IA_NA rather than any old ones, and updating
	 * pool timers for each (if any).
	 *
	 * Note that we must do ddns_updates() before we test for lease
	 * reuse (so we'll know if DNS entries are different).  To ensure
	 * we don't break any configs, we run on_commit statements before
	 * we do ddns_updates() just in case the former affects the later.
	 * This is symetrical with v4 logic.  We always run on_commit and
	 * ddns_udpates() whether a lease is reused or renewed.
	 */
	if ((reply->ia->num_iasubopt != 0) &&
	    (reply->buf.reply.msg_type == DHCPV6_REPLY)) {
		int must_commit = 0;
		struct iasubopt *tmp;
		struct data_string *ia_id;
		int i;

		for (i = 0 ; i < reply->ia->num_iasubopt ; i++) {
			tmp = reply->ia->iasubopt[i];
			if (tmp->ia != NULL) {
				ia_dereference(&tmp->ia, MDL);
			}

			ia_reference(&tmp->ia, reply->ia, MDL);

			/* If we have anything to do on commit do it now */
			if (tmp->on_star.on_commit != NULL) {
				execute_statements(NULL, reply->packet,
						   NULL, NULL,
						   reply->packet->options,
						   reply->opt_state,
						   &tmp->scope,
						   tmp->on_star.on_commit,
						   &tmp->on_star);
				executable_statement_dereference
					(&tmp->on_star.on_commit, MDL);
			}

#if defined (NSUPDATE)

			/* Perform ddns updates */
			oc = lookup_option(&server_universe, reply->opt_state,
					   SV_DDNS_UPDATES);
			if ((oc == NULL) ||
			    evaluate_boolean_option_cache(NULL, reply->packet,
							  NULL, NULL,
							reply->packet->options,
							  reply->opt_state,
							  &tmp->scope,
							  oc, MDL)) {
				ddns_updates(reply->packet, NULL, NULL,
					     tmp, NULL, reply->opt_state);
			}
#endif
			if (!reuse_lease6(reply, tmp)) {
				/* Commit 'hard' bindings. */
				must_commit = 1;
				renew_lease6(tmp->ipv6_pool, tmp);
				schedule_lease_timeout(tmp->ipv6_pool);

				/* Do our threshold check. */
				check_pool6_threshold(reply, tmp);
			}
		}

		/* write the IA_NA in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);

		/* Remove any old ia from the hash. */
		if (reply->old_ia != NULL) {
			if (!release_on_roam(reply)) {
				ia_id = &reply->old_ia->iaid_duid;
				ia_hash_delete(ia_na_active,
					       (unsigned char *)ia_id->data,
					       ia_id->len, MDL);
			}

			ia_dereference(&reply->old_ia, MDL);
		}

		/* Put new ia into the hash. */
		reply->ia->cltt = cur_time;
		ia_id = &reply->ia->iaid_duid;
		ia_hash_add(ia_na_active, (unsigned char *)ia_id->data,
			    ia_id->len, reply->ia, MDL);

		/* If we couldn't reuse all of the iasubopts, we
		* must update udpate the lease db */
		if (must_commit) {
			write_ia(reply->ia);
		}
	} else {
		/* write the IA_NA in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);
		schedule_lease_timeout_reply(reply);
	}

      cleanup:
	if (packet_ia != NULL)
		option_state_dereference(&packet_ia, MDL);
	if (reply->reply_ia != NULL)
		option_state_dereference(&reply->reply_ia, MDL);
	if (ia_data.data != NULL)
		data_string_forget(&ia_data, MDL);
	if (data.data != NULL)
		data_string_forget(&data, MDL);
	if (reply->ia != NULL)
		ia_dereference(&reply->ia, MDL);
	if (reply->old_ia != NULL)
		ia_dereference(&reply->old_ia, MDL);
	if (reply->lease != NULL)
		iasubopt_dereference(&reply->lease, MDL);
	if (reply->fixed.data != NULL)
		data_string_forget(&reply->fixed, MDL);
	if (reply->subnet != NULL)
		subnet_dereference(&reply->subnet, MDL);
	if (reply->on_star.on_expiry != NULL)
		executable_statement_dereference
			(&reply->on_star.on_expiry, MDL);
	if (reply->on_star.on_release != NULL)
		executable_statement_dereference
			(&reply->on_star.on_release, MDL);

	/*
	 * ISC_R_CANCELED is a status code used by the addr processing to
	 * indicate we're replying with a status code.  This is still a
	 * success at higher layers.
	 */
	return((status == ISC_R_CANCELED) ? ISC_R_SUCCESS : status);
}

/*
 * Writes the populated IA_xx in wire format to the reply buffer
 */
void
write_to_packet(struct reply_state *reply, unsigned ia_cursor) {
	reply->cursor += store_options6((char *)reply->buf.data + reply->cursor,
					sizeof(reply->buf) - reply->cursor,
					reply->reply_ia, reply->packet,
					(reply->ia->ia_type != D6O_IA_PD ?
					required_opts_IA : required_opts_IA_PD),
					NULL);

	/* Reset the length of this IA to match what was just written. */
	putUShort(reply->buf.data + ia_cursor + 2,
		  reply->cursor - (ia_cursor + 4));

	if (reply->ia->ia_type != D6O_IA_TA) {
		/* Calculate T1/T2 and stuff them in the reply */
		set_reply_tee_times(reply, ia_cursor);
	}
}

/*
 * Process an IAADDR within a given IA_xA, storing any IAADDR reply contents
 * into the reply's current ia-scoped option cache.  Returns ISC_R_CANCELED
 * in the event we are replying with a status code and do not wish to process
 * more IAADDRs within this IA.
 */
static isc_result_t
reply_process_addr(struct reply_state *reply, struct option_cache *addr) {
	u_int32_t pref_life, valid_life;
	struct binding_scope **scope;
	struct group *group;
	struct subnet *subnet;
	struct iaddr tmp_addr;
	struct option_cache *oc;
	struct data_string iaaddr, data;
	isc_result_t status = ISC_R_SUCCESS;
#ifdef EUI_64
	int invalid_for_eui_64 = 0;
#endif

	/* Initializes values that will be cleaned up. */
	memset(&iaaddr, 0, sizeof(iaaddr));
	memset(&data, 0, sizeof(data));
	/* Note that reply->lease may be set by address_is_owned() */

	/*
	 * There is no point trying to process an incoming address if there
	 * is no room for an outgoing address.
	 */
	if ((reply->cursor + 28) > sizeof(reply->buf)) {
		log_error("reply_process_addr: Out of room for address.");
		return ISC_R_NOSPACE;
	}

	/* Extract this IAADDR option. */
	if (!evaluate_option_cache(&iaaddr, reply->packet, NULL, NULL,
				   reply->packet->options, NULL, &global_scope,
				   addr, MDL) ||
	    (iaaddr.len < IAADDR_OFFSET)) {
		log_error("reply_process_addr: error evaluating IAADDR.");
		status = ISC_R_FAILURE;
		goto cleanup;
	}

	/* The first 16 bytes are the IPv6 address. */
	pref_life = getULong(iaaddr.data + 16);
	valid_life = getULong(iaaddr.data + 20);

	if ((reply->client_valid == 0) ||
	    (reply->client_valid > valid_life))
		reply->client_valid = valid_life;

	if ((reply->client_prefer == 0) ||
	    (reply->client_prefer > pref_life))
		reply->client_prefer = pref_life;

	/*
	 * Clients may choose to send :: as an address, with the idea to give
	 * hints about preferred-lifetime or valid-lifetime.
	 */
	tmp_addr.len = 16;
	memset(tmp_addr.iabuf, 0, 16);
	if (!memcmp(iaaddr.data, tmp_addr.iabuf, 16)) {
		/* Status remains success; we just ignore this one. */
		goto cleanup;
	}

	/* tmp_addr len remains 16 */
	memcpy(tmp_addr.iabuf, iaaddr.data, 16);

	/*
	 * Verify that this address is on the client's network.
	 */
	for (subnet = reply->shared->subnets ; subnet != NULL ;
	     subnet = subnet->next_sibling) {
		if (addr_eq(subnet_number(tmp_addr, subnet->netmask),
			    subnet->net))
			break;
	}

#ifdef EUI_64
	if (subnet) {
		/* If the requested address falls into an EUI-64 pool, then
		 * we need to verify if it has EUI-64 duid AND the requested
		 * address is correct for that duid.  If not we treat it just
		 * like an not-on-link request. */
		struct ipv6_pool* pool = NULL;
		struct in6_addr* addr = (struct in6_addr*)(iaaddr.data);
		if ((find_ipv6_pool(&pool, D6O_IA_NA, addr) == ISC_R_SUCCESS)
		    && (pool->ipv6_pond->use_eui_64) &&
		   (!valid_for_eui_64_pool(pool, &reply->client_id, 0, addr))) {
			log_debug ("Requested address: %s,"
				   " not valid for EUI-64 pool",
				   pin6_addr(addr));
			invalid_for_eui_64 = 1;
		}
	}
#endif

	/* Address not found on shared network. */
#ifdef EUI_64
	if ((subnet == NULL) || invalid_for_eui_64) {
#else
	if (subnet == NULL) {
#endif
		/* Ignore this address on 'soft' bindings. */
		if (reply->packet->dhcpv6_msg_type == DHCPV6_SOLICIT) {
			/* disable rapid commit */
			reply->buf.reply.msg_type = DHCPV6_ADVERTISE;
			delete_option(&dhcpv6_universe,
				      reply->opt_state,
				      D6O_RAPID_COMMIT);
			/* status remains success */
			goto cleanup;
		}

		/*
		 * RFC3315 section 18.2.1:
		 *
		 * If the server finds that the prefix on one or more IP
		 * addresses in any IA in the message from the client is not
		 * appropriate for the link to which the client is connected,
		 * the server MUST return the IA to the client with a Status
		 * Code option with the value NotOnLink.
		 */
		if (reply->packet->dhcpv6_msg_type == DHCPV6_REQUEST) {
			/* Rewind the IA_NA to empty. */
			option_state_dereference(&reply->reply_ia, MDL);
			if (!option_state_allocate(&reply->reply_ia, MDL)) {
				log_error("reply_process_addr: No memory for "
					  "option state wipe.");
				status = ISC_R_NOMEMORY;
				goto cleanup;
			}

			/* Append a NotOnLink status code. */
			if (!set_status_code(STATUS_NotOnLink,
					     "Address not for use on this "
					     "link.", reply->reply_ia)) {
				log_error("reply_process_addr: Failure "
					  "setting status code.");
				status = ISC_R_FAILURE;
				goto cleanup;
			}

			/* Fin (no more IAADDRs). */
			status = ISC_R_CANCELED;
			goto cleanup;
		}

		/*
		 * RFC3315 sections 18.2.3 and 18.2.4 have identical language:
		 *
		 * If the server finds that any of the addresses are not
		 * appropriate for the link to which the client is attached,
		 * the server returns the address to the client with lifetimes
		 * of 0.
		 */
		if ((reply->packet->dhcpv6_msg_type != DHCPV6_RENEW) &&
		    (reply->packet->dhcpv6_msg_type != DHCPV6_REBIND)) {
			log_error("It is impossible to lease a client that is "
				  "not sending a solicit, request, renew, or "
				  "rebind.");
			status = ISC_R_FAILURE;
			goto cleanup;
		}

		reply->send_prefer = reply->send_valid = 0;
		goto send_addr;
	}


	/* Verify the address belongs to the client. */
	if (!address_is_owned(reply, &tmp_addr)) {
		/*
		 * For solicit and request, any addresses included are
		 * 'requested' addresses.  For rebind, we actually have
		 * no direction on what to do from 3315 section 18.2.4!
		 * So I think the best bet is to try and give it out, and if
		 * we can't, zero lifetimes.
		 */
		if ((reply->packet->dhcpv6_msg_type == DHCPV6_SOLICIT) ||
		    (reply->packet->dhcpv6_msg_type == DHCPV6_REQUEST) ||
		    (reply->packet->dhcpv6_msg_type == DHCPV6_REBIND)) {
			status = reply_process_try_addr(reply, &tmp_addr);

			/*
			 * If the address is in use, or isn't in any dynamic
			 * range, continue as normal.  If any other error was
			 * found, error out.
			 */
			if ((status != ISC_R_SUCCESS) &&
			    (status != ISC_R_ADDRINUSE) &&
			    (status != ISC_R_ADDRNOTAVAIL))
				goto cleanup;

			/*
			 * If we didn't honor this lease, for solicit and
			 * request we simply omit it from our answer.  For
			 * rebind, we send it with zeroed lifetimes.
			 */
			if (reply->lease == NULL) {
				if (reply->packet->dhcpv6_msg_type ==
							DHCPV6_REBIND) {
					reply->send_prefer = 0;
					reply->send_valid = 0;
					goto send_addr;
				}

				/* status remains success - ignore */
				goto cleanup;
			}
		/*
		 * RFC3315 section 18.2.3:
		 *
		 * If the server cannot find a client entry for the IA the
		 * server returns the IA containing no addresses with a Status
		 * Code option set to NoBinding in the Reply message.
		 *
		 * On mismatch we (ab)use this pretending we have not the IA
		 * as soon as we have not an address.
		 */
		} else if (reply->packet->dhcpv6_msg_type == DHCPV6_RENEW) {
			/* Rewind the IA_NA to empty. */
			option_state_dereference(&reply->reply_ia, MDL);
			if (!option_state_allocate(&reply->reply_ia, MDL)) {
				log_error("reply_process_addr: No memory for "
					  "option state wipe.");
				status = ISC_R_NOMEMORY;
				goto cleanup;
			}

			/* Append a NoBinding status code.  */
			if (!set_status_code(STATUS_NoBinding,
					     "Address not bound to this "
					     "interface.", reply->reply_ia)) {
				log_error("reply_process_addr: Unable to "
					  "attach status code.");
				status = ISC_R_FAILURE;
				goto cleanup;
			}

			/* Fin (no more IAADDRs). */
			status = ISC_R_CANCELED;
			goto cleanup;
		} else {
			log_error("It is impossible to lease a client that is "
				  "not sending a solicit, request, renew, or "
				  "rebind message.");
			status = ISC_R_FAILURE;
			goto cleanup;
		}
	}

	if (reply->static_lease) {
		if (reply->host == NULL)
			log_fatal("Impossible condition at %s:%d.", MDL);

		scope = &global_scope;
		group = reply->subnet->group;
	} else {
		if (reply->lease == NULL)
			log_fatal("Impossible condition at %s:%d.", MDL);

		scope = &reply->lease->scope;
		group = reply->lease->ipv6_pool->ipv6_pond->group;
	}

	/*
	 * If client_resources is nonzero, then the reply_process_is_addressed
	 * function has executed configuration state into the reply option
	 * cache.  We will use that valid cache to derive configuration for
	 * whether or not to engage in additional addresses, and similar.
	 */
	if (reply->client_resources != 0) {
		unsigned limit = 1;

		/*
		 * Does this client have "enough" addresses already?  Default
		 * to one.  Everybody gets one, and one should be enough for
		 * anybody.
		 */
		oc = lookup_option(&server_universe, reply->opt_state,
				   SV_LIMIT_ADDRS_PER_IA);
		if (oc != NULL) {
			if (!evaluate_option_cache(&data, reply->packet,
						   NULL, NULL,
						   reply->packet->options,
						   reply->opt_state,
						   scope, oc, MDL) ||
			    (data.len != 4)) {
				log_error("reply_process_addr: unable to "
					  "evaluate addrs-per-ia value.");
				status = ISC_R_FAILURE;
				goto cleanup;
			}

			limit = getULong(data.data);
			data_string_forget(&data, MDL);
		}

		/*
		 * If we wish to limit the client to a certain number of
		 * addresses, then omit the address from the reply.
		 */
		if (reply->client_resources >= limit)
			goto cleanup;
	}

	status = reply_process_is_addressed(reply, scope, group);
	if (status != ISC_R_SUCCESS)
		goto cleanup;

      send_addr:
	status = reply_process_send_addr(reply, &tmp_addr);

      cleanup:
	if (iaaddr.data != NULL)
		data_string_forget(&iaaddr, MDL);
	if (data.data != NULL)
		data_string_forget(&data, MDL);
	if (reply->lease != NULL)
		iasubopt_dereference(&reply->lease, MDL);

	return status;
}

/*
 * Verify the address belongs to the client.  If we've got a host
 * record with a fixed address, it has to be the assigned address
 * (fault out all else).  Otherwise it's a dynamic address, so lookup
 * that address and make sure it belongs to this DUID:IAID pair.
 */
static isc_boolean_t
address_is_owned(struct reply_state *reply, struct iaddr *addr) {
	int i;
	struct ipv6_pond *pond;

	/*
	 * This faults out addresses that don't match fixed addresses.
	 */
	if (reply->static_lease) {
		if (reply->fixed.data == NULL)
			log_fatal("Impossible condition at %s:%d.", MDL);

		if (memcmp(addr->iabuf, reply->fixed.data, 16) == 0)
			return (ISC_TRUE);

		return (ISC_FALSE);
	}

	if ((reply->old_ia == NULL) || (reply->old_ia->num_iasubopt == 0))
		return (ISC_FALSE);

	for (i = 0 ; i < reply->old_ia->num_iasubopt ; i++) {
		struct iasubopt *tmp;

		tmp = reply->old_ia->iasubopt[i];

		if (memcmp(addr->iabuf, &tmp->addr, 16) == 0) {
			if (lease6_usable(tmp) == ISC_FALSE) {
				return (ISC_FALSE);
			}

			pond = tmp->ipv6_pool->ipv6_pond;
			if (((pond->prohibit_list != NULL) &&
			     (permitted(reply->packet, pond->prohibit_list))) ||
			    ((pond->permit_list != NULL) &&
			     (!permitted(reply->packet, pond->permit_list))))
				return (ISC_FALSE);

			iasubopt_reference(&reply->lease, tmp, MDL);

			return (ISC_TRUE);
		}
	}

	return (ISC_FALSE);
}

/* Process a client-supplied IA_TA.  This may append options to the tail of
 * the reply packet being built in the reply_state structure.
 */
static isc_result_t
reply_process_ia_ta(struct reply_state *reply, struct option_cache *ia) {
	isc_result_t status = ISC_R_SUCCESS;
	u_int32_t iaid;
	unsigned ia_cursor;
	struct option_state *packet_ia;
	struct option_cache *oc;
	struct data_string ia_data, data;
	struct data_string iaaddr;
	u_int32_t pref_life, valid_life;
	struct iaddr tmp_addr;

	/* Initialize values that will get cleaned up on return. */
	packet_ia = NULL;
	memset(&ia_data, 0, sizeof(ia_data));
	memset(&data, 0, sizeof(data));
	memset(&iaaddr, 0, sizeof(iaaddr));

	/* Make sure there is at least room for the header. */
	if ((reply->cursor + IA_TA_OFFSET + 4) > sizeof(reply->buf)) {
		log_error("reply_process_ia_ta: Reply too long for IA.");
		return ISC_R_NOSPACE;
	}


	/* Fetch the IA_TA contents. */
	if (!get_encapsulated_IA_state(&packet_ia, &ia_data, reply->packet,
				       ia, IA_TA_OFFSET)) {
		log_error("reply_process_ia_ta: error evaluating ia");
		status = ISC_R_FAILURE;
		goto cleanup;
	}

	/* Extract IA_TA header contents. */
	iaid = getULong(ia_data.data);

	/* Create an IA_TA structure. */
	if (ia_allocate(&reply->ia, iaid, (char *)reply->client_id.data,
			reply->client_id.len, MDL) != ISC_R_SUCCESS) {
		log_error("reply_process_ia_ta: no memory for ia.");
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}
	reply->ia->ia_type = D6O_IA_TA;

	/* Cache pre-existing IA, if any. */
	ia_hash_lookup(&reply->old_ia, ia_ta_active,
		       (unsigned char *)reply->ia->iaid_duid.data,
		       reply->ia->iaid_duid.len, MDL);

	/*
	 * Create an option cache to carry the IA_TA option contents, and
	 * execute any user-supplied values into it.
	 */
	if (!option_state_allocate(&reply->reply_ia, MDL)) {
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}

	/*
	 * Temporary leases are dynamic by definition.
	 */
	reply->static_lease = ISC_FALSE;

	/*
	 * Save the cursor position at the start of the IA, so we can
	 * set length later.  We write a temporary
	 * header out now just in case we decide to adjust the packet
	 * within sub-process functions.
	 */
	ia_cursor = reply->cursor;

	/* Initialize the IA_TA header.  First the code. */
	putUShort(reply->buf.data + reply->cursor, (unsigned)D6O_IA_TA);
	reply->cursor += 2;

	/* Then option length. */
	putUShort(reply->buf.data + reply->cursor, 0x04u);
	reply->cursor += 2;

	/* Then IA_TA header contents; IAID. */
	putULong(reply->buf.data + reply->cursor, iaid);
	reply->cursor += 4;

	/*
	 * Deal with an IAADDR for lifetimes.
	 * For all or none, process IAADDRs as hints.
	 */
	reply->min_valid = reply->min_prefer = INFINITE_TIME;
	reply->client_valid = reply->client_prefer = 0;
	oc = lookup_option(&dhcpv6_universe, packet_ia, D6O_IAADDR);
	for (; oc != NULL; oc = oc->next) {
		memset(&iaaddr, 0, sizeof(iaaddr));
		if (!evaluate_option_cache(&iaaddr, reply->packet,
					   NULL, NULL,
					   reply->packet->options, NULL,
					   &global_scope, oc, MDL) ||
		    (iaaddr.len < IAADDR_OFFSET)) {
			log_error("reply_process_ia_ta: error "
				  "evaluating IAADDR.");
			status = ISC_R_FAILURE;
			goto cleanup;
		}
		/* The first 16 bytes are the IPv6 address. */
		pref_life = getULong(iaaddr.data + 16);
		valid_life = getULong(iaaddr.data + 20);

		if ((reply->client_valid == 0) ||
		    (reply->client_valid > valid_life))
			reply->client_valid = valid_life;

		if ((reply->client_prefer == 0) ||
		    (reply->client_prefer > pref_life))
			reply->client_prefer = pref_life;

		/* Nothing more if something has failed. */
		if (status == ISC_R_CANCELED)
			continue;

		tmp_addr.len = 16;
		memcpy(tmp_addr.iabuf, iaaddr.data, 16);
		if (!temporary_is_available(reply, &tmp_addr))
			goto bad_temp;
		status = reply_process_is_addressed(reply,
						    &reply->lease->scope,
						    reply->lease->ipv6_pool->ipv6_pond->group);
		if (status != ISC_R_SUCCESS)
			goto bad_temp;
		status = reply_process_send_addr(reply, &tmp_addr);
		if (status != ISC_R_SUCCESS)
			goto bad_temp;
		if (reply->lease != NULL)
			iasubopt_dereference(&reply->lease, MDL);
		continue;

	bad_temp:
		/* Rewind the IA_TA to empty. */
		option_state_dereference(&reply->reply_ia, MDL);
		if (!option_state_allocate(&reply->reply_ia, MDL)) {
			status = ISC_R_NOMEMORY;
			goto cleanup;
		}
		status = ISC_R_CANCELED;
		reply->client_resources = 0;
		reply->resources_included = ISC_FALSE;
		if (reply->lease != NULL)
			iasubopt_dereference(&reply->lease, MDL);
	}
	reply->ia_count++;

	/*
	 * Give the client temporary addresses.
	 */
	if (reply->client_resources != 0)
		goto store;
	status = find_client_temporaries(reply);
	if (status == ISC_R_NORESOURCES) {
		switch (reply->packet->dhcpv6_msg_type) {
		      case DHCPV6_SOLICIT:
			/*
			 * No address for any IA is handled
			 * by the caller.
			 */
			/* FALL THROUGH */

		      case DHCPV6_REQUEST:
			/* Section 18.2.1 (Request):
			 *
			 * If the server cannot assign any addresses to
			 * an IA in the message from the client, the
			 * server MUST include the IA in the Reply
			 * message with no addresses in the IA and a
			 * Status Code option in the IA containing
			 * status code NoAddrsAvail.
			 */
			option_state_dereference(&reply->reply_ia, MDL);
			if (!option_state_allocate(&reply->reply_ia,  MDL)) {
				log_error("reply_process_ia_ta: No "
					  "memory for option state wipe.");
				status = ISC_R_NOMEMORY;
				goto cleanup;
			}

			if (!set_status_code(STATUS_NoAddrsAvail,
					     "No addresses available "
					     "for this interface.",
					      reply->reply_ia)) {
				log_error("reply_process_ia_ta: Unable "
					  "to set NoAddrsAvail status code.");
				status = ISC_R_FAILURE;
				goto cleanup;
			}

			status = ISC_R_SUCCESS;
			break;

		      default:
			/*
			 * We don't want to include the IA if we
			 * provide zero addresses including zeroed
			 * lifetimes.
			 */
			if (reply->resources_included)
				status = ISC_R_SUCCESS;
			else
				goto cleanup;
			break;
		}
	} else if (status != ISC_R_SUCCESS)
		goto cleanup;

      store:

	/*
	 * yes, goto's aren't the best but we also want to avoid extra
	 * indents
	 */
	if (status == ISC_R_CANCELED) {
		/* We're replying with a status code so we still need to
		* write it out in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);
		goto cleanup;
	}

	/*
	 * If we have any addresses log what we are doing.
	 */
	if (reply->ia->num_iasubopt != 0) {
		struct iasubopt *tmp;
		int i;
		char tmp_addr[INET6_ADDRSTRLEN];

		for (i = 0 ; i < reply->ia->num_iasubopt ; i++) {
			tmp = reply->ia->iasubopt[i];

			log_info("%s TA: address %s to client with duid %s "
				 "iaid = %d valid for %u seconds",
				 dhcpv6_type_names[reply->buf.reply.msg_type],
				 inet_ntop(AF_INET6, &tmp->addr,
					   tmp_addr, sizeof(tmp_addr)),
				 print_hex_1(reply->client_id.len,
					     reply->client_id.data, 60),
				 iaid,
				 tmp->valid);
		}
	}

	/*
	 * For hard bindings we consume the new changes into
	 * the database (if any have been attached to the ia_ta).
	 *
	 * Loop through the assigned dynamic addresses, referencing the
	 * leases onto this IA_TA rather than any old ones, and updating
	 * pool timers for each (if any).
	 */
	if ((reply->ia->num_iasubopt != 0) &&
	    (reply->buf.reply.msg_type == DHCPV6_REPLY)) {
		int must_commit = 0;
		struct iasubopt *tmp;
		struct data_string *ia_id;
		int i;

		for (i = 0 ; i < reply->ia->num_iasubopt ; i++) {
			tmp = reply->ia->iasubopt[i];

			if (tmp->ia != NULL)
				ia_dereference(&tmp->ia, MDL);
			ia_reference(&tmp->ia, reply->ia, MDL);

			/* If we have anything to do on commit do it now */
			if (tmp->on_star.on_commit != NULL) {
				execute_statements(NULL, reply->packet,
						   NULL, NULL,
						   reply->packet->options,
						   reply->opt_state,
						   &tmp->scope,
						   tmp->on_star.on_commit,
						   &tmp->on_star);
				executable_statement_dereference
					(&tmp->on_star.on_commit, MDL);
			}

#if defined (NSUPDATE)
			/*
			 * Perform ddns updates.
			 */
			oc = lookup_option(&server_universe, reply->opt_state,
					   SV_DDNS_UPDATES);
			if ((oc == NULL) ||
			    evaluate_boolean_option_cache(NULL, reply->packet,
							  NULL, NULL,
							reply->packet->options,
							  reply->opt_state,
							  &tmp->scope,
							  oc, MDL)) {
				ddns_updates(reply->packet, NULL, NULL,
					     tmp, NULL, reply->opt_state);
			}
#endif

			if (!reuse_lease6(reply, tmp)) {
				/* Commit 'hard' bindings. */
				must_commit = 1;
				renew_lease6(tmp->ipv6_pool, tmp);
				schedule_lease_timeout(tmp->ipv6_pool);

				/* Do our threshold check. */
				check_pool6_threshold(reply, tmp);
			}
		}

		/* write the IA_TA in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);

		/* Remove any old ia from the hash. */
		if (reply->old_ia != NULL) {
			if (!release_on_roam(reply)) {
				ia_id = &reply->old_ia->iaid_duid;
				ia_hash_delete(ia_ta_active,
					       (unsigned char *)ia_id->data,
					       ia_id->len, MDL);
			}

			ia_dereference(&reply->old_ia, MDL);
		}

		/* Put new ia into the hash. */
		reply->ia->cltt = cur_time;
		ia_id = &reply->ia->iaid_duid;
		ia_hash_add(ia_ta_active, (unsigned char *)ia_id->data,
			    ia_id->len, reply->ia, MDL);

		/* If we couldn't reuse all of the iasubopts, we
		* must update udpate the lease db */
		if (must_commit) {
			write_ia(reply->ia);
		}
	} else {
		/* write the IA_TA in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);
		schedule_lease_timeout_reply(reply);
	}

      cleanup:
	if (packet_ia != NULL)
		option_state_dereference(&packet_ia, MDL);
	if (iaaddr.data != NULL)
		data_string_forget(&iaaddr, MDL);
	if (reply->reply_ia != NULL)
		option_state_dereference(&reply->reply_ia, MDL);
	if (ia_data.data != NULL)
		data_string_forget(&ia_data, MDL);
	if (data.data != NULL)
		data_string_forget(&data, MDL);
	if (reply->ia != NULL)
		ia_dereference(&reply->ia, MDL);
	if (reply->old_ia != NULL)
		ia_dereference(&reply->old_ia, MDL);
	if (reply->lease != NULL)
		iasubopt_dereference(&reply->lease, MDL);

	/*
	 * ISC_R_CANCELED is a status code used by the addr processing to
	 * indicate we're replying with other addresses.  This is still a
	 * success at higher layers.
	 */
	return((status == ISC_R_CANCELED) ? ISC_R_SUCCESS : status);
}
/*
 * Determines if a lease (iasubopt) can be reused without extending it.
 * If dhcp-cache-threshold is greater than zero (i.e enabled) then
 * a lease may be reused without going through a full renewal if
 * it meets all the requirements.  In short it must be active, younger
 * than the threshold, and not have DNS changes.
 *
 * If it is determined that it can be reused, that a call to
 * shorten_lifetimes() is made to reduce the valid and preferred lifetimes
 * sent to the client by the age of the lease.
 *
 * Returns 1 if lease can be reused, 0 otherwise
 */
int
reuse_lease6(struct reply_state *reply, struct iasubopt *lease) {
	int threshold = DEFAULT_CACHE_THRESHOLD;
	struct option_cache* oc = NULL;
	struct data_string d1;
	time_t age;
	time_t limit;
	int reuse_it = 0;

	/* In order to even qualify for reuse consideration:
	 * 1. Lease must be active
	 * 2. It must have been accepted at least once
	 * 3. DNS info must not have changed */
	if ((lease->state != FTS_ACTIVE) ||
	    (lease->hard_lifetime_end_time == 0) ||
	    (lease->ddns_cb != NULL)) {
		return (0);
	}

	/* Look up threshold value */
	memset(&d1, 0, sizeof(struct data_string));
	oc = lookup_option(&server_universe, reply->opt_state,
			   SV_CACHE_THRESHOLD);
	if (oc &&
	    evaluate_option_cache(&d1, reply->packet, NULL, NULL,
				  reply->packet->options, reply->opt_state,
				  &lease->scope, oc, MDL)) {
			if (d1.len == 1 && (d1.data[0] < 100)) {
                                threshold = d1.data[0];
			}

		data_string_forget(&d1, MDL);
	}

	if (threshold <= 0) {
		return (0);
	}

	if (lease->valid >= MAX_TIME) {
		/* Infinite leases are always reused.  We have to make
		* a choice because we cannot determine when they actually
		* began, so we either always reuse them or we never do. */
		log_debug ("reusing infinite lease for: %s%s",
			    pin6_addr(&lease->addr), iasubopt_plen_str(lease));
		return (1);
	}

	age = cur_tv.tv_sec - (lease->hard_lifetime_end_time - lease->valid);
	if (lease->valid <= (INT_MAX / threshold))
		limit = lease->valid * threshold / 100;
	else
		limit = lease->valid / 100 * threshold;

	if (age < limit) {
		/* Reduce valid/preferred going to the client by age */
		shorten_lifetimes(reply, lease, age, threshold);
		reuse_it = 1;
	}

	return (reuse_it);
}

/*
 * Reduces the valid and preferred lifetimes for a given lease (iasubopt)
 *
 * We cannot determine until after a iasubopt has been added to
 * the reply if the lease can be reused. Therefore, when we do reuse a
 * lease we need a way to alter the lifetimes that will be sent to the client.
 * That's where this function comes in handy:
 *
 * Locate the iasubopt by it's address within the reply the reduce both
 * the preferred and valid lifetimes by the given number of seconds.
 *
 * Note that this function, by necessity, works directly with the
 * option_cache data. Sort of a no-no but I don't have any better ideas.
 */
void shorten_lifetimes(struct reply_state *reply, struct iasubopt *lease,
		       time_t age, int threshold) {
	struct option_cache* oc = NULL;
	int subopt_type;
	int addr_offset;
	int pref_offset;
	int val_offset;
	int exp_length;

	if (reply->ia->ia_type != D6O_IA_PD) {
		subopt_type = D6O_IAADDR;
		addr_offset = IASUBOPT_NA_ADDR_OFFSET;
		pref_offset = IASUBOPT_NA_PREF_OFFSET;
		val_offset = IASUBOPT_NA_VALID_OFFSET;
		exp_length = IASUBOPT_NA_LEN;
	}
	else {
		subopt_type = D6O_IAPREFIX;
		addr_offset = IASUBOPT_PD_PREFIX_OFFSET;
		pref_offset = IASUBOPT_PD_PREF_OFFSET;
		val_offset = IASUBOPT_PD_VALID_OFFSET;
		exp_length = IASUBOPT_PD_LEN;
	}

	// loop through the iasubopts for the one that matches this lease
	oc = lookup_option(&dhcpv6_universe, reply->reply_ia, subopt_type);
        for (; oc != NULL ; oc = oc->next) {
		if (oc->data.data == NULL || oc->data.len != exp_length) {
			/* shouldn't happen */
			continue;
		}

		/* If address matches (and for PDs the prefix len matches)
		* we assume this is our subopt, so update the lifetimes */
		if (!memcmp(oc->data.data + addr_offset, &lease->addr, 16) &&
		    (subopt_type != D6O_IAPREFIX ||
		     (oc->data.data[IASUBOPT_PD_PREFLEN_OFFSET] ==
		      lease->plen))) {
			u_int32_t pref_life = getULong(oc->data.data +
						       pref_offset);
			u_int32_t valid_life = getULong(oc->data.data +
							val_offset);

			if (pref_life < MAX_TIME && pref_life > age) {
				pref_life -= age;
				putULong((unsigned char*)(oc->data.data) +
					  pref_offset, pref_life);

				if (reply->min_prefer > pref_life) {
					reply->min_prefer = pref_life;
				}
			}

			if (valid_life < MAX_TIME && valid_life > age) {
				valid_life -= age;
				putULong((unsigned char*)(oc->data.data) +
					 val_offset, valid_life);

				if (reply->min_valid > reply->send_valid) {
					reply->min_valid = valid_life;
				}
			}

			log_debug ("Reusing lease for: %s%s, "
				   "age %ld secs < %d%%,"
				   " sending shortened lifetimes -"
				   " preferred: %u, valid %u",
				   pin6_addr(&lease->addr),
				   iasubopt_plen_str(lease),
				   (long)age, threshold,
				   pref_life, valid_life);
			break;
		}
	}
}

/*
 * Verify the temporary address is available.
 */
static isc_boolean_t
temporary_is_available(struct reply_state *reply, struct iaddr *addr) {
	struct in6_addr tmp_addr;
	struct subnet *subnet;
	struct ipv6_pool *pool = NULL;
	struct ipv6_pond *pond = NULL;
	int i;

	memcpy(&tmp_addr, addr->iabuf, sizeof(tmp_addr));
	/*
	 * Clients may choose to send :: as an address, with the idea to give
	 * hints about preferred-lifetime or valid-lifetime.
	 * So this is not a request for this address.
	 */
	if (IN6_IS_ADDR_UNSPECIFIED(&tmp_addr))
		return ISC_FALSE;

	/*
	 * Verify that this address is on the client's network.
	 */
	for (subnet = reply->shared->subnets ; subnet != NULL ;
	     subnet = subnet->next_sibling) {
		if (addr_eq(subnet_number(*addr, subnet->netmask),
			    subnet->net))
			break;
	}

	/* Address not found on shared network. */
	if (subnet == NULL)
		return ISC_FALSE;

	/*
	 * Check if this address is owned (must be before next step).
	 */
	if (address_is_owned(reply, addr))
		return ISC_TRUE;

	/*
	 * Verify that this address is in a temporary pool and try to get it.
	 */
	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (((pond->prohibit_list != NULL) &&
		     (permitted(reply->packet, pond->prohibit_list))) ||
		    ((pond->permit_list != NULL) &&
		     (!permitted(reply->packet, pond->permit_list))))
			continue;

		for (i = 0 ; (pool = pond->ipv6_pools[i]) != NULL ; i++) {
			if (pool->pool_type != D6O_IA_TA)
				continue;

			if (ipv6_in_pool(&tmp_addr, pool))
				break;
		}

		if (pool != NULL)
			break;
	}

	if (pool == NULL)
		return ISC_FALSE;
	if (lease6_exists(pool, &tmp_addr))
		return ISC_FALSE;
	if (iasubopt_allocate(&reply->lease, MDL) != ISC_R_SUCCESS)
		return ISC_FALSE;
	reply->lease->addr = tmp_addr;
	reply->lease->plen = 0;
	/* Default is soft binding for 2 minutes. */
	if (add_lease6(pool, reply->lease, cur_time + 120) != ISC_R_SUCCESS)
		return ISC_FALSE;

	return ISC_TRUE;
}

/*
 * Get a temporary address per prefix.
 */
static isc_result_t
find_client_temporaries(struct reply_state *reply) {
	int i;
	struct ipv6_pool *p = NULL;
	struct ipv6_pond *pond;
	isc_result_t status = ISC_R_NORESOURCES;;
	unsigned int attempts;
	struct iaddr send_addr;

	/*
	 * Do a quick walk through of the ponds and pools
	 * to see if we have any prefix pools
	 */
	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (pond->ipv6_pools == NULL)
			continue;

		for (i = 0; (p = pond->ipv6_pools[i]) != NULL; i++) {
			if (p->pool_type == D6O_IA_TA)
				break;
		}
		if (p != NULL)
			break;
	}

	/* If we get here and p is NULL we have no useful pools */
	if (p == NULL) {
		log_debug("Unable to get client addresses: "
			  "no IPv6 pools on this shared network");
		return ISC_R_NORESOURCES;
	}

	/*
	 * We have at least one pool that could provide an address
	 * Now we walk through the ponds and pools again and check
	 * to see if the client is permitted and if an address is
	 * available
	 */

	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (((pond->prohibit_list != NULL) &&
		     (permitted(reply->packet, pond->prohibit_list))) ||
		    ((pond->permit_list != NULL) &&
		     (!permitted(reply->packet, pond->permit_list))))
			continue;

		for (i = 0; (p = pond->ipv6_pools[i]) != NULL; i++) {
			if (p->pool_type != D6O_IA_TA) {
				continue;
			}

			/*
			 * Get an address in this temporary pool.
			 */
			status = create_lease6(p, &reply->lease, &attempts,
					       &reply->client_id,
                                               cur_time + 120);

			if (status != ISC_R_SUCCESS) {
				log_debug("Unable to get a temporary address.");
				goto cleanup;
			}

			status = reply_process_is_addressed(reply,
							    &reply->lease->scope,
							    pond->group);
			if (status != ISC_R_SUCCESS) {
				goto cleanup;
			}
			send_addr.len = 16;
			memcpy(send_addr.iabuf, &reply->lease->addr, 16);
			status = reply_process_send_addr(reply, &send_addr);
			if (status != ISC_R_SUCCESS) {
				goto cleanup;
			}
			/*
			 * reply->lease can't be null as we use it above
			 * add check if that changes
			 */
			iasubopt_dereference(&reply->lease, MDL);
		}
	}

      cleanup:
	if (reply->lease != NULL) {
		iasubopt_dereference(&reply->lease, MDL);
	}
	return status;
}

/*
 * This function only returns failure on 'hard' failures.  If it succeeds,
 * it will leave a lease structure behind.
 */
static isc_result_t
reply_process_try_addr(struct reply_state *reply, struct iaddr *addr) {
	isc_result_t status = ISC_R_ADDRNOTAVAIL;
	struct ipv6_pool *pool = NULL;
	struct ipv6_pond *pond = NULL;
	int i;
	struct data_string data_addr;

	if ((reply == NULL) || (reply->shared == NULL) ||
	    (addr == NULL) || (reply->lease != NULL))
		return (DHCP_R_INVALIDARG);

	/*
	 * Do a quick walk through of the ponds and pools
	 * to see if we have any NA address pools
	 */
	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (pond->ipv6_pools == NULL)
			continue;

		for (i = 0; ; i++) {
			pool = pond->ipv6_pools[i];
			if ((pool == NULL) ||
			    (pool->pool_type == D6O_IA_NA))
				break;
		}
		if (pool != NULL)
			break;
	}

	/* If we get here and p is NULL we have no useful pools */
	if (pool == NULL) {
		return (ISC_R_ADDRNOTAVAIL);
	}

	memset(&data_addr, 0, sizeof(data_addr));
	data_addr.len = addr->len;
	data_addr.data = addr->iabuf;

	/*
	 * We have at least one pool that could provide an address
	 * Now we walk through the ponds and pools again and check
	 * to see if the client is permitted and if an address is
	 * available
	 *
	 * Within a given pond we start looking at the last pool we
	 * allocated from, unless it had a collision trying to allocate
	 * an address. This will tend to move us into less-filled pools.
	 */

	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (((pond->prohibit_list != NULL) &&
		     (permitted(reply->packet, pond->prohibit_list))) ||
		    ((pond->permit_list != NULL) &&
		     (!permitted(reply->packet, pond->permit_list))))
			continue;

		for (i = 0 ; (pool = pond->ipv6_pools[i]) != NULL ; i++) {
			if (pool->pool_type != D6O_IA_NA)
				continue;

			status = try_client_v6_address(&reply->lease, pool,
						       &data_addr);
			if (status == ISC_R_SUCCESS)
				break;
		}

		if (status == ISC_R_SUCCESS)
			break;
	}

	/* Note that this is just pedantry.  There is no allocation to free. */
	data_string_forget(&data_addr, MDL);
	/* Return just the most recent status... */
	return (status);
}

/* Look around for an address to give the client.  First, look through the
 * old IA for addresses we can extend.  Second, try to allocate a new address.
 * Finally, actually add that address into the current reply IA.
 */
static isc_result_t
find_client_address(struct reply_state *reply) {
	struct iaddr send_addr;
	isc_result_t status = ISC_R_NORESOURCES;
	struct iasubopt *lease, *best_lease = NULL;
	struct binding_scope **scope;
	struct group *group;
	int i;

	if (reply->static_lease) {
		if (reply->host == NULL)
			return DHCP_R_INVALIDARG;

		send_addr.len = 16;
		memcpy(send_addr.iabuf, reply->fixed.data, 16);

		scope = &global_scope;
		group = reply->subnet->group;
		goto send_addr;
	}

	if (reply->old_ia != NULL)  {
		for (i = 0 ; i < reply->old_ia->num_iasubopt ; i++) {
			struct shared_network *candidate_shared;
			struct ipv6_pond *pond;

			lease = reply->old_ia->iasubopt[i];
			candidate_shared = lease->ipv6_pool->shared_network;
			pond = lease->ipv6_pool->ipv6_pond;

			/*
			 * Look for the best lease on the client's shared
			 * network, that is still permitted
			 */

			if ((candidate_shared != reply->shared) ||
			    (lease6_usable(lease) != ISC_TRUE))
				continue;

			if (((pond->prohibit_list != NULL) &&
			     (permitted(reply->packet, pond->prohibit_list))) ||
			    ((pond->permit_list != NULL) &&
			     (!permitted(reply->packet, pond->permit_list))))
				continue;

			best_lease = lease_compare(lease, best_lease);
		}
	}

	/* Try to pick a new address if we didn't find one, or if we found an
	 * abandoned lease.
	 */
	if ((best_lease == NULL) || (best_lease->state == FTS_ABANDONED)) {
		status = pick_v6_address(reply);
	} else if (best_lease != NULL) {
		iasubopt_reference(&reply->lease, best_lease, MDL);
		status = ISC_R_SUCCESS;
	}

	/* Pick the abandoned lease as a last resort. */
	if ((status == ISC_R_NORESOURCES) && (best_lease != NULL)) {
		/* I don't see how this is supposed to be done right now. */
		log_error("Best match for DUID %s is an abandoned address,"
			  " This may be a result of multiple clients attempting"
			  " to use this DUID",
			 print_hex_1(reply->client_id.len,
				     reply->client_id.data, 60));
		/* iasubopt_reference(&reply->lease, best_lease, MDL); */
	}

	/* Give up now if we didn't find a lease. */
	if (status != ISC_R_SUCCESS)
		return status;

	if (reply->lease == NULL)
		log_fatal("Impossible condition at %s:%d.", MDL);

	/* Draw binding scopes from the lease's binding scope, and config
	 * from the lease's containing subnet and higher.  Note that it may
	 * be desirable to place the group attachment directly in the pool.
	 */
	scope = &reply->lease->scope;
	group = reply->lease->ipv6_pool->ipv6_pond->group;

	send_addr.len = 16;
	memcpy(send_addr.iabuf, &reply->lease->addr, 16);

      send_addr:
	status = reply_process_is_addressed(reply, scope, group);
	if (status != ISC_R_SUCCESS)
		return status;

	status = reply_process_send_addr(reply, &send_addr);
	return status;
}

/* Once an address is found for a client, perform several common functions;
 * Calculate and store valid and preferred lease times, draw client options
 * into the option state.
 */
static isc_result_t
reply_process_is_addressed(struct reply_state *reply,
			   struct binding_scope **scope, struct group *group)
{
	isc_result_t status = ISC_R_SUCCESS;
	struct data_string data;
	struct option_cache *oc;
	struct option_state *tmp_options = NULL;
	struct on_star *on_star;
	int i;

	/* Initialize values we will cleanup. */
	memset(&data, 0, sizeof(data));

	/*
	 * Find the proper on_star block to use.  We use the
	 * one in the lease if we have a lease or the one in
	 * the reply if we don't have a lease because this is
	 * a static instance
	 */
	if (reply->lease) {
		on_star = &reply->lease->on_star;
	} else {
		on_star = &reply->on_star;
	}

	/*
	 * Bring in the root configuration.  We only do this to bring
	 * in the on * statements, as we didn't have the lease available
	 * we did it the first time.
	 */
	option_state_allocate(&tmp_options, MDL);
	execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
				    reply->packet->options, tmp_options,
				    &global_scope, root_group, NULL,
				    on_star);
	if (tmp_options != NULL) {
		option_state_dereference(&tmp_options, MDL);
	}

	/*
	 * Bring configured options into the root packet level cache - start
	 * with the lease's closest enclosing group (passed in by the caller
	 * as 'group').
	 */
	execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
				    reply->packet->options, reply->opt_state,
				    scope, group, root_group, on_star);

	/* Execute statements from class scopes. */
	for (i = reply->packet->class_count; i > 0; i--) {
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->opt_state, scope,
					    reply->packet->classes[i - 1]->group,
					    group, on_star);
	}

	/*
	 * If there is a host record, over-ride with values configured there,
	 * without re-evaluating configuration from the previously executed
	 * group or its common enclosers.
	 */
	if (reply->host != NULL)
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->opt_state, scope,
					    reply->host->group, group,
					    on_star);

	/* Determine valid lifetime. */
	if (reply->client_valid == 0)
		reply->send_valid = DEFAULT_DEFAULT_LEASE_TIME;
	else
		reply->send_valid = reply->client_valid;

	oc = lookup_option(&server_universe, reply->opt_state,
			   SV_DEFAULT_LEASE_TIME);
	if (oc != NULL) {
		if (!evaluate_option_cache(&data, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state,
					   scope, oc, MDL) ||
		    (data.len != 4)) {
			log_error("reply_process_is_addressed: unable to "
				  "evaluate default lease time");
			status = ISC_R_FAILURE;
			goto cleanup;
		}

		reply->send_valid = getULong(data.data);
		data_string_forget(&data, MDL);
	}

	/* Check to see if the lease time would cause us to wrap
	 * in which case we make it infinite.
	 * The following doesn't work on at least some systems:
	 * (cur_time + reply->send_valid < cur_time)
	 */
	if (reply->send_valid != INFINITE_TIME) {
		time_t test_time = cur_time + reply->send_valid;
		if (test_time < cur_time)
			reply->send_valid = INFINITE_TIME;
        }

	if (reply->client_prefer == 0)
		reply->send_prefer = reply->send_valid;
	else
		reply->send_prefer = reply->client_prefer;

	if ((reply->send_prefer >= reply->send_valid) &&
	    (reply->send_valid != INFINITE_TIME))
		reply->send_prefer = (reply->send_valid / 2) +
				     (reply->send_valid / 8);

	oc = lookup_option(&server_universe, reply->opt_state,
			   SV_PREFER_LIFETIME);
	if (oc != NULL) {
		if (!evaluate_option_cache(&data, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state,
					   scope, oc, MDL) ||
		    (data.len != 4)) {
			log_error("reply_process_is_addressed: unable to "
				  "evaluate preferred lease time");
			status = ISC_R_FAILURE;
			goto cleanup;
		}

		reply->send_prefer = getULong(data.data);
		data_string_forget(&data, MDL);
	}

	/* Note lowest values for later calculation of renew/rebind times. */
	if (reply->min_prefer > reply->send_prefer)
		reply->min_prefer = reply->send_prefer;

	if (reply->min_valid > reply->send_valid)
		reply->min_valid = reply->send_valid;

#if 0
	/*
	 * XXX: Old 4.0.0 alpha code would change the host {} record
	 * XXX: uid upon lease assignment.  This was intended to cover the
	 * XXX: case where a client first identifies itself using vendor
	 * XXX: options in a solicit, or request, but later neglects to include
	 * XXX: these options in a Renew or Rebind.  It is not clear that this
	 * XXX: is required, and has some startling ramifications (such as
	 * XXX: how to recover this dynamic host {} state across restarts).
	 */
	if (reply->host != NULL)
		change_host_uid(host, reply->client_id->data,
				reply->client_id->len);
#endif /* 0 */

	/* Perform dynamic lease related update work. */
	if (reply->lease != NULL) {
		/* Cached lifetimes */
		reply->lease->prefer = reply->send_prefer;
		reply->lease->valid = reply->send_valid;

		/* Advance (or rewind) the valid lifetime.
		 * In the protocol 0xFFFFFFFF is infinite
		 * when connecting to the lease file MAX_TIME is
		 */
		if (reply->buf.reply.msg_type == DHCPV6_REPLY) {
			if (reply->send_valid == INFINITE_TIME) {
				reply->lease->soft_lifetime_end_time = MAX_TIME;
			} else {
				reply->lease->soft_lifetime_end_time =
				  cur_time + reply->send_valid;
			}
			/* Wait before renew! */
		}

		status = ia_add_iasubopt(reply->ia, reply->lease, MDL);
		if (status != ISC_R_SUCCESS) {
			log_fatal("reply_process_is_addressed: Unable to "
				  "attach lease to new IA: %s",
				  isc_result_totext(status));
		}

		/*
		 * If this is a new lease, make sure it is attached somewhere.
		 */
		if (reply->lease->ia == NULL) {
			ia_reference(&reply->lease->ia, reply->ia, MDL);
		}
	}

	/* Bring a copy of the relevant options into the IA scope. */
	execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
				    reply->packet->options, reply->reply_ia,
				    scope, group, root_group, NULL);

	/* Execute statements from class scopes. */
	for (i = reply->packet->class_count; i > 0; i--) {
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->reply_ia, scope,
					    reply->packet->classes[i - 1]->group,
					    group, NULL);
	}

	/*
	 * And bring in host record configuration, if any, but not to overlap
	 * the previous group or its common enclosers.
	 */
	if (reply->host != NULL)
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->reply_ia, scope,
					    reply->host->group, group, NULL);

      cleanup:
	if (data.data != NULL)
		data_string_forget(&data, MDL);

	if (status == ISC_R_SUCCESS)
		reply->client_resources++;

	return status;
}

/* Simply send an IAADDR within the IA scope as described. */
static isc_result_t
reply_process_send_addr(struct reply_state *reply, struct iaddr *addr) {
	isc_result_t status = ISC_R_SUCCESS;
	struct data_string data;

	memset(&data, 0, sizeof(data));

	/* Now append the lease. */
	data.len = IAADDR_OFFSET;
	if (!buffer_allocate(&data.buffer, data.len, MDL)) {
		log_error("reply_process_send_addr: out of memory"
			  "allocating new IAADDR buffer.");
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}
	data.data = data.buffer->data;

	memcpy(data.buffer->data, addr->iabuf, 16);
	putULong(data.buffer->data + 16, reply->send_prefer);
	putULong(data.buffer->data + 20, reply->send_valid);

	if (!append_option_buffer(&dhcpv6_universe, reply->reply_ia,
				  data.buffer, data.buffer->data,
				  data.len, D6O_IAADDR, 0)) {
		log_error("reply_process_send_addr: unable "
			  "to save IAADDR option");
		status = ISC_R_FAILURE;
		goto cleanup;
	}

	reply->resources_included = ISC_TRUE;

      cleanup:
	if (data.data != NULL)
		data_string_forget(&data, MDL);

	return status;
}

/* Choose the better of two leases. */
static struct iasubopt *
lease_compare(struct iasubopt *alpha, struct iasubopt *beta) {
	if (alpha == NULL)
		return beta;
	if (beta == NULL)
		return alpha;

	switch(alpha->state) {
	      case FTS_ACTIVE:
		switch(beta->state) {
		      case FTS_ACTIVE:
			/* Choose the lease with the longest lifetime (most
			 * likely the most recently allocated).
			 */
			if (alpha->hard_lifetime_end_time <
			    beta->hard_lifetime_end_time)
				return beta;
			else
				return alpha;

		      case FTS_EXPIRED:
		      case FTS_ABANDONED:
			return alpha;

		      default:
			log_fatal("Impossible condition at %s:%d.", MDL);
		}
		break;

	      case FTS_EXPIRED:
		switch (beta->state) {
		      case FTS_ACTIVE:
			return beta;

		      case FTS_EXPIRED:
			/* Choose the most recently expired lease. */
			if (alpha->hard_lifetime_end_time <
			    beta->hard_lifetime_end_time)
				return beta;
			else if ((alpha->hard_lifetime_end_time ==
				  beta->hard_lifetime_end_time) &&
				 (alpha->soft_lifetime_end_time <
				  beta->soft_lifetime_end_time))
				return beta;
			else
				return alpha;

		      case FTS_ABANDONED:
			return alpha;

		      default:
			log_fatal("Impossible condition at %s:%d.", MDL);
		}
		break;

	      case FTS_ABANDONED:
		switch (beta->state) {
		      case FTS_ACTIVE:
		      case FTS_EXPIRED:
			return alpha;

		      case FTS_ABANDONED:
			/* Choose the lease that was abandoned longest ago. */
			if (alpha->hard_lifetime_end_time <
			    beta->hard_lifetime_end_time)
				return alpha;
			else
				return beta;

		      default:
			log_fatal("Impossible condition at %s:%d.", MDL);
		}
		break;

	      default:
		log_fatal("Impossible condition at %s:%d.", MDL);
	}

	log_fatal("Triple impossible condition at %s:%d.", MDL);
	return NULL;
}

/* Process a client-supplied IA_PD.  This may append options to the tail of
 * the reply packet being built in the reply_state structure.
 */
static isc_result_t
reply_process_ia_pd(struct reply_state *reply, struct option_cache *ia) {
	isc_result_t status = ISC_R_SUCCESS;
	u_int32_t iaid;
	unsigned ia_cursor;
	struct option_state *packet_ia;
	struct option_cache *oc;
	struct data_string ia_data, data;

	/* Initialize values that will get cleaned up on return. */
	packet_ia = NULL;
	memset(&ia_data, 0, sizeof(ia_data));
	memset(&data, 0, sizeof(data));
	/*
	 * Note that find_client_prefix() may set reply->lease.
	 */

	/* Make sure there is at least room for the header. */
	if ((reply->cursor + IA_PD_OFFSET + 4) > sizeof(reply->buf)) {
		log_error("reply_process_ia_pd: Reply too long for IA.");
		return ISC_R_NOSPACE;
	}


	/* Fetch the IA_PD contents. */
	if (!get_encapsulated_IA_state(&packet_ia, &ia_data, reply->packet,
				       ia, IA_PD_OFFSET)) {
		log_error("reply_process_ia_pd: error evaluating ia");
		status = ISC_R_FAILURE;
		goto cleanup;
	}

	/* Extract IA_PD header contents. */
	iaid = getULong(ia_data.data);
	reply->renew = getULong(ia_data.data + 4);
	reply->rebind = getULong(ia_data.data + 8);

	/* Create an IA_PD structure. */
	if (ia_allocate(&reply->ia, iaid, (char *)reply->client_id.data,
			reply->client_id.len, MDL) != ISC_R_SUCCESS) {
		log_error("reply_process_ia_pd: no memory for ia.");
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}
	reply->ia->ia_type = D6O_IA_PD;

	/* Cache pre-existing IA_PD, if any. */
	ia_hash_lookup(&reply->old_ia, ia_pd_active,
		       (unsigned char *)reply->ia->iaid_duid.data,
		       reply->ia->iaid_duid.len, MDL);

	/*
	 * Create an option cache to carry the IA_PD option contents, and
	 * execute any user-supplied values into it.
	 */
	if (!option_state_allocate(&reply->reply_ia, MDL)) {
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}

	/* Check & count the fixed prefix host records. */
	reply->static_prefixes = 0;
	if ((reply->host != NULL) && (reply->host->fixed_prefix != NULL)) {
		struct iaddrcidrnetlist *fp;

		for (fp = reply->host->fixed_prefix; fp != NULL;
		     fp = fp->next) {
			reply->static_prefixes += 1;
		}
	}

	/*
	 * Save the cursor position at the start of the IA_PD, so we can
	 * set length and adjust t1/t2 values later.  We write a temporary
	 * header out now just in case we decide to adjust the packet
	 * within sub-process functions.
	 */
	ia_cursor = reply->cursor;

	/* Initialize the IA_PD header.  First the code. */
	putUShort(reply->buf.data + reply->cursor, (unsigned)D6O_IA_PD);
	reply->cursor += 2;

	/* Then option length. */
	putUShort(reply->buf.data + reply->cursor, 0x0Cu);
	reply->cursor += 2;

	/* Then IA_PD header contents; IAID. */
	putULong(reply->buf.data + reply->cursor, iaid);
	reply->cursor += 4;

	/* We store the client's t1 for now, and may over-ride it later. */
	putULong(reply->buf.data + reply->cursor, reply->renew);
	reply->cursor += 4;

	/* We store the client's t2 for now, and may over-ride it later. */
	putULong(reply->buf.data + reply->cursor, reply->rebind);
	reply->cursor += 4;

	/*
	 * For each prefix in this IA_PD, decide what to do about it.
	 */
	oc = lookup_option(&dhcpv6_universe, packet_ia, D6O_IAPREFIX);
	reply->min_valid = reply->min_prefer = INFINITE_TIME;
	reply->client_valid = reply->client_prefer = 0;
	reply->preflen = -1;
	for (; oc != NULL ; oc = oc->next) {
		status = reply_process_prefix(reply, oc);

		/*
		 * Canceled means we did not allocate prefixes to the
		 * client, but we're "done" with this IA - we set a status
		 * code.  So transmit this reply, e.g., move on to the next
		 * IA.
		 */
		if (status == ISC_R_CANCELED)
			break;

		if ((status != ISC_R_SUCCESS) &&
		    (status != ISC_R_ADDRINUSE) &&
		    (status != ISC_R_ADDRNOTAVAIL))
			goto cleanup;
	}

	reply->pd_count++;

	/*
	 * If we fell through the above and never gave the client
	 * a prefix, give it one now.
	 */
	if ((status != ISC_R_CANCELED) && (reply->client_resources == 0)) {
		status = find_client_prefix(reply);

		if (status == ISC_R_NORESOURCES) {
			switch (reply->packet->dhcpv6_msg_type) {
			      case DHCPV6_SOLICIT:
				/*
				 * No prefix for any IA is handled
				 * by the caller.
				 */
				/* FALL THROUGH */

			      case DHCPV6_REQUEST:
				/* Same than for addresses. */
				option_state_dereference(&reply->reply_ia, MDL);
				if (!option_state_allocate(&reply->reply_ia,
							   MDL))
				{
					log_error("reply_process_ia_pd: No "
						  "memory for option state "
						  "wipe.");
					status = ISC_R_NOMEMORY;
					goto cleanup;
				}

				if (!set_status_code(STATUS_NoPrefixAvail,
						     "No prefixes available "
						     "for this interface.",
						      reply->reply_ia)) {
					log_error("reply_process_ia_pd: "
						  "Unable to set "
						  "NoPrefixAvail status "
						  "code.");
					status = ISC_R_FAILURE;
					goto cleanup;
				}

				status = ISC_R_SUCCESS;
				break;

			      default:
				if (reply->resources_included)
					status = ISC_R_SUCCESS;
				else
					goto cleanup;
				break;
			}
		}

		if (status != ISC_R_SUCCESS)
			goto cleanup;
	}

	/*
	 * yes, goto's aren't the best but we also want to avoid extra
	 * indents
	 */
	if (status == ISC_R_CANCELED) {
		/* We're replying with a status code so we still need to
		 * write it out in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);
		goto cleanup;
	}

	/*
	 * Handle static prefixes, we always log stuff and if it's
	 * a hard binding we run any commit statements that we have
	 */
	if (reply->static_prefixes != 0) {
		char tmp_addr[INET6_ADDRSTRLEN];
		log_info("%s PD: address %s/%d to client with duid %s "
			 "iaid = %d static",
			 dhcpv6_type_names[reply->buf.reply.msg_type],
			 inet_ntop(AF_INET6, reply->fixed_pref.lo_addr.iabuf,
				   tmp_addr, sizeof(tmp_addr)),
			 reply->fixed_pref.bits,
			 print_hex_1(reply->client_id.len,
				     reply->client_id.data, 60),
			 iaid);

		/* Write the lease out in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);

		if ((reply->buf.reply.msg_type == DHCPV6_REPLY) &&
		    (reply->on_star.on_commit != NULL)) {
			execute_statements(NULL, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state,
					   NULL, reply->on_star.on_commit,
					   NULL);
			executable_statement_dereference
				(&reply->on_star.on_commit, MDL);
		}
		goto cleanup;
	}

	/*
	 * If we have any addresses log what we are doing.
	 */
	if (reply->ia->num_iasubopt != 0) {
		struct iasubopt *tmp;
		int i;
		char tmp_addr[INET6_ADDRSTRLEN];

		for (i = 0 ; i < reply->ia->num_iasubopt ; i++) {
			tmp = reply->ia->iasubopt[i];

			log_info("%s PD: address %s/%d to client with duid %s"
				 " iaid = %d valid for %u seconds",
				 dhcpv6_type_names[reply->buf.reply.msg_type],
				 inet_ntop(AF_INET6, &tmp->addr,
					   tmp_addr, sizeof(tmp_addr)),
				 (int)tmp->plen,
				 print_hex_1(reply->client_id.len,
					     reply->client_id.data, 60),
				 iaid, tmp->valid);
		}
	}

	/*
	 * If this is not a 'soft' binding, consume the new changes into
	 * the database (if any have been attached to the ia_pd).
	 *
	 * Loop through the assigned dynamic prefixes, referencing the
	 * prefixes onto this IA_PD rather than any old ones, and updating
	 * prefix pool timers for each (if any).
	 *
	 * If a lease can be reused we skip renewing it or checking the
	 * pool threshold. If it can't we flag that the IA must be commited
	 * to the db and do the renewal and pool check.
	 */
	if ((reply->buf.reply.msg_type == DHCPV6_REPLY) &&
	    (reply->ia->num_iasubopt != 0)) {
		int must_commit = 0;
		struct iasubopt *tmp;
		struct data_string *ia_id;
		int i;

		for (i = 0 ; i < reply->ia->num_iasubopt ; i++) {
			tmp = reply->ia->iasubopt[i];

			if (tmp->ia != NULL)
				ia_dereference(&tmp->ia, MDL);
			ia_reference(&tmp->ia, reply->ia, MDL);

			/* If we have anything to do on commit do it now */
			if (tmp->on_star.on_commit != NULL) {
				execute_statements(NULL, reply->packet,
						   NULL, NULL,
						   reply->packet->options,
						   reply->opt_state,
						   &tmp->scope,
						   tmp->on_star.on_commit,
						   &tmp->on_star);
				executable_statement_dereference
					(&tmp->on_star.on_commit, MDL);
			}

			if (!reuse_lease6(reply, tmp)) {
				/* Commit 'hard' bindings. */
				must_commit = 1;
				renew_lease6(tmp->ipv6_pool, tmp);
				schedule_lease_timeout(tmp->ipv6_pool);

				/* Do our threshold check. */
				check_pool6_threshold(reply, tmp);
			}
		}

		/* write the IA_PD in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);

		/* Remove any old ia from the hash. */
		if (reply->old_ia != NULL) {
			if (!release_on_roam(reply)) {
				ia_id = &reply->old_ia->iaid_duid;
				ia_hash_delete(ia_pd_active,
					       (unsigned char *)ia_id->data,
					       ia_id->len, MDL);
			}

			ia_dereference(&reply->old_ia, MDL);
		}

		/* Put new ia into the hash. */
		reply->ia->cltt = cur_time;
		ia_id = &reply->ia->iaid_duid;
		ia_hash_add(ia_pd_active, (unsigned char *)ia_id->data,
			    ia_id->len, reply->ia, MDL);

		/* If we couldn't reuse all of the iasubopts, we
		* must udpate the lease db */
		if (must_commit) {
			write_ia(reply->ia);
		}
	} else {
		/* write the IA_PD in wire-format to the outbound buffer */
		write_to_packet(reply, ia_cursor);
		schedule_lease_timeout_reply(reply);
	}

      cleanup:
	if (packet_ia != NULL)
		option_state_dereference(&packet_ia, MDL);
	if (reply->reply_ia != NULL)
		option_state_dereference(&reply->reply_ia, MDL);
	if (ia_data.data != NULL)
		data_string_forget(&ia_data, MDL);
	if (data.data != NULL)
		data_string_forget(&data, MDL);
	if (reply->ia != NULL)
		ia_dereference(&reply->ia, MDL);
	if (reply->old_ia != NULL)
		ia_dereference(&reply->old_ia, MDL);
	if (reply->lease != NULL)
		iasubopt_dereference(&reply->lease, MDL);
	if (reply->on_star.on_expiry != NULL)
		executable_statement_dereference
			(&reply->on_star.on_expiry, MDL);
	if (reply->on_star.on_release != NULL)
		executable_statement_dereference
			(&reply->on_star.on_release, MDL);

	/*
	 * ISC_R_CANCELED is a status code used by the prefix processing to
	 * indicate we're replying with a status code.  This is still a
	 * success at higher layers.
	 */
	return((status == ISC_R_CANCELED) ? ISC_R_SUCCESS : status);
}

/*!
 *
 * \brief Find the proper scoping group for use with a v6 static prefix.
 *
 * We start by trying to find a subnet based on the given prefix and
 * the shared network.  If we don't find one then the prefix has been
 * declared outside of any subnets.  If there is a static address
 * associated with the host we use it to try and find a subnet (this
 * should succeed).  If there isn't a static address we fall back
 * to the shared subnet itself.
 * Once we have a subnet we extract the group from it and return it.
 *
 * \param reply - the reply structure we use to collect information
 *                we will use the fields shared, fixed_pref and host
 *                from the structure
 *
 * \return a pointer to the group structure to use for scoping
 */

static struct group *
find_group_by_prefix(struct reply_state *reply) {
	/* default group if we don't find anything better */
	struct group *group = reply->shared->group;
	struct subnet *subnet = NULL;
	struct iaddr tmp_addr;
	struct data_string fixed_addr;

	/* Try with the prefix first */
	if (find_grouped_subnet(&subnet, reply->shared,
				reply->fixed_pref.lo_addr, MDL) != 0) {
		group = subnet->group;
		subnet_dereference(&subnet, MDL);
		return (group);
	}

	/* Didn't find a subnet via prefix, what about fixed address */
	/* The caller has already tested reply->host != NULL */

	memset(&fixed_addr, 0, sizeof(fixed_addr));

	if ((reply->host->fixed_addr != NULL) &&
	    (evaluate_option_cache(&fixed_addr, NULL, NULL, NULL,
				   NULL, NULL, &global_scope,
				   reply->host->fixed_addr, MDL))) {
		if (fixed_addr.len >= 16) {
			tmp_addr.len = 16;
			memcpy(tmp_addr.iabuf, fixed_addr.data, 16);
			if (find_grouped_subnet(&subnet, reply->shared,
						tmp_addr, MDL) != 0) {
				group = subnet->group;
				subnet_dereference(&subnet, MDL);
			}
		}
		data_string_forget(&fixed_addr, MDL);
	}

	/* return whatever we got */
	return (group);
}

/*
 * Process an IAPREFIX within a given IA_PD, storing any IAPREFIX reply
 * contents into the reply's current ia_pd-scoped option cache.  Returns
 * ISC_R_CANCELED in the event we are replying with a status code and do
 * not wish to process more IAPREFIXes within this IA_PD.
 */
static isc_result_t
reply_process_prefix(struct reply_state *reply, struct option_cache *pref) {
	u_int32_t pref_life, valid_life;
	struct binding_scope **scope;
	struct iaddrcidrnet tmp_pref;
	struct option_cache *oc;
	struct data_string iapref, data;
	isc_result_t status = ISC_R_SUCCESS;
	struct group *group;

	/* Initializes values that will be cleaned up. */
	memset(&iapref, 0, sizeof(iapref));
	memset(&data, 0, sizeof(data));
	/* Note that reply->lease may be set by prefix_is_owned() */

	/*
	 * There is no point trying to process an incoming prefix if there
	 * is no room for an outgoing prefix.
	 */
	if ((reply->cursor + 29) > sizeof(reply->buf)) {
		log_error("reply_process_prefix: Out of room for prefix.");
		return ISC_R_NOSPACE;
	}

	/* Extract this IAPREFIX option. */
	if (!evaluate_option_cache(&iapref, reply->packet, NULL, NULL,
				   reply->packet->options, NULL, &global_scope,
				   pref, MDL) ||
	    (iapref.len < IAPREFIX_OFFSET)) {
		log_error("reply_process_prefix: error evaluating IAPREFIX.");
		status = ISC_R_FAILURE;
		goto cleanup;
	}

	/*
	 * Layout: preferred and valid lifetimes followed by the prefix
	 * length and the IPv6 address.
	 */
	pref_life = getULong(iapref.data);
	valid_life = getULong(iapref.data + 4);

	if ((reply->client_valid == 0) ||
	    (reply->client_valid > valid_life))
		reply->client_valid = valid_life;

	if ((reply->client_prefer == 0) ||
	    (reply->client_prefer > pref_life))
		reply->client_prefer = pref_life;

	/*
	 * Clients may choose to send ::/0 as a prefix, with the idea to give
	 * hints about preferred-lifetime or valid-lifetime.
	 */
	tmp_pref.lo_addr.len = 16;
	memset(tmp_pref.lo_addr.iabuf, 0, 16);
	if ((iapref.data[8] == 0) &&
	    (memcmp(iapref.data + 9, tmp_pref.lo_addr.iabuf, 16) == 0)) {
		/* Status remains success; we just ignore this one. */
		goto cleanup;
	}

	/*
	 * Clients may choose to send ::/X as a prefix to specify a
	 * preferred/requested prefix length. Note X is never zero here.
	 */
	tmp_pref.bits = (int) iapref.data[8];
	if (reply->preflen < 0) {
		/* Cache the first preferred prefix length. */
		reply->preflen = tmp_pref.bits;
	}
	if (memcmp(iapref.data + 9, tmp_pref.lo_addr.iabuf, 16) == 0) {
		goto cleanup;
	}

	memcpy(tmp_pref.lo_addr.iabuf, iapref.data + 9, 16);

	/* Verify the prefix belongs to the client. */
	if (!prefix_is_owned(reply, &tmp_pref)) {
		/* Same than for addresses. */
		if ((reply->packet->dhcpv6_msg_type == DHCPV6_SOLICIT) ||
		    (reply->packet->dhcpv6_msg_type == DHCPV6_REQUEST) ||
		    (reply->packet->dhcpv6_msg_type == DHCPV6_REBIND)) {
			status = reply_process_try_prefix(reply, &tmp_pref);

			/* Either error out or skip this prefix. */
			if ((status != ISC_R_SUCCESS) &&
			    (status != ISC_R_ADDRINUSE) &&
			    (status != ISC_R_ADDRNOTAVAIL))
				goto cleanup;

			if (reply->lease == NULL) {
				if (reply->packet->dhcpv6_msg_type ==
							DHCPV6_REBIND) {
					reply->send_prefer = 0;
					reply->send_valid = 0;
					goto send_pref;
				}

				/* status remains success - ignore */
				goto cleanup;
			}
		/*
		 * RFC3633 section 18.2.3:
		 *
		 * If the delegating router cannot find a binding
		 * for the requesting router's IA_PD the delegating
		 * router returns the IA_PD containing no prefixes
		 * with a Status Code option set to NoBinding in the
		 * Reply message.
		 *
		 * On mismatch we (ab)use this pretending we have not the IA
		 * as soon as we have not a prefix.
		 */
		} else if (reply->packet->dhcpv6_msg_type == DHCPV6_RENEW) {
			/* Rewind the IA_PD to empty. */
			option_state_dereference(&reply->reply_ia, MDL);
			if (!option_state_allocate(&reply->reply_ia, MDL)) {
				log_error("reply_process_prefix: No memory "
					  "for option state wipe.");
				status = ISC_R_NOMEMORY;
				goto cleanup;
			}

			/* Append a NoBinding status code.  */
			if (!set_status_code(STATUS_NoBinding,
					     "Prefix not bound to this "
					     "interface.", reply->reply_ia)) {
				log_error("reply_process_prefix: Unable to "
					  "attach status code.");
				status = ISC_R_FAILURE;
				goto cleanup;
			}

			/* Fin (no more IAPREFIXes). */
			status = ISC_R_CANCELED;
			goto cleanup;
		} else {
			log_error("It is impossible to lease a client that is "
				  "not sending a solicit, request, renew, or "
				  "rebind message.");
			status = ISC_R_FAILURE;
			goto cleanup;
		}
	}

	if (reply->static_prefixes > 0) {
		if (reply->host == NULL)
			log_fatal("Impossible condition at %s:%d.", MDL);

		scope = &global_scope;

		/* Copy the static prefix for logging and finding the group */
		memcpy(&reply->fixed_pref, &tmp_pref, sizeof(tmp_pref));

		/* Try to find a group for the static prefix */
		group = find_group_by_prefix(reply);
	} else {
		if (reply->lease == NULL)
			log_fatal("Impossible condition at %s:%d.", MDL);

		scope = &reply->lease->scope;
		group = reply->lease->ipv6_pool->ipv6_pond->group;
	}

	/*
	 * If client_resources is nonzero, then the reply_process_is_prefixed
	 * function has executed configuration state into the reply option
	 * cache.  We will use that valid cache to derive configuration for
	 * whether or not to engage in additional prefixes, and similar.
	 */
	if (reply->client_resources != 0) {
		unsigned limit = 1;

		/*
		 * Does this client have "enough" prefixes already?  Default
		 * to one.  Everybody gets one, and one should be enough for
		 * anybody.
		 */
		oc = lookup_option(&server_universe, reply->opt_state,
				   SV_LIMIT_PREFS_PER_IA);
		if (oc != NULL) {
			if (!evaluate_option_cache(&data, reply->packet,
						   NULL, NULL,
						   reply->packet->options,
						   reply->opt_state,
						   scope, oc, MDL) ||
			    (data.len != 4)) {
				log_error("reply_process_prefix: unable to "
					  "evaluate prefs-per-ia value.");
				status = ISC_R_FAILURE;
				goto cleanup;
			}

			limit = getULong(data.data);
			data_string_forget(&data, MDL);
		}

		/*
		 * If we wish to limit the client to a certain number of
		 * prefixes, then omit the prefix from the reply.
		 */
		if (reply->client_resources >= limit)
			goto cleanup;
	}

	status = reply_process_is_prefixed(reply, scope, group);
	if (status != ISC_R_SUCCESS)
		goto cleanup;

      send_pref:
	status = reply_process_send_prefix(reply, &tmp_pref);

      cleanup:
	if (iapref.data != NULL)
		data_string_forget(&iapref, MDL);
	if (data.data != NULL)
		data_string_forget(&data, MDL);
	if (reply->lease != NULL)
		iasubopt_dereference(&reply->lease, MDL);

	return status;
}

/*
 * Verify the prefix belongs to the client.  If we've got a host
 * record with fixed prefixes, it has to be an assigned prefix
 * (fault out all else).  Otherwise it's a dynamic prefix, so lookup
 * that prefix and make sure it belongs to this DUID:IAID pair.
 */
static isc_boolean_t
prefix_is_owned(struct reply_state *reply, struct iaddrcidrnet *pref) {
	struct iaddrcidrnetlist *l;
	int i;
	struct ipv6_pond *pond;

	/*
	 * This faults out prefixes that don't match fixed prefixes.
	 */
	if (reply->static_prefixes > 0) {
		for (l = reply->host->fixed_prefix; l != NULL; l = l->next) {
			if ((pref->bits == l->cidrnet.bits) &&
			    (memcmp(pref->lo_addr.iabuf,
				    l->cidrnet.lo_addr.iabuf, 16) == 0))
				return (ISC_TRUE);
		}
		return (ISC_FALSE);
	}

	if ((reply->old_ia == NULL) ||
	    (reply->old_ia->num_iasubopt == 0))
		return (ISC_FALSE);

	for (i = 0 ; i < reply->old_ia->num_iasubopt ; i++) {
		struct iasubopt *tmp;

		tmp = reply->old_ia->iasubopt[i];

		if ((pref->bits == (int) tmp->plen) &&
		    (memcmp(pref->lo_addr.iabuf, &tmp->addr, 16) == 0)) {
			if (lease6_usable(tmp) == ISC_FALSE) {
				return (ISC_FALSE);
			}

			pond = tmp->ipv6_pool->ipv6_pond;
			if (((pond->prohibit_list != NULL) &&
			     (permitted(reply->packet, pond->prohibit_list))) ||
			    ((pond->permit_list != NULL) &&
			     (!permitted(reply->packet, pond->permit_list))))
				return (ISC_FALSE);

			iasubopt_reference(&reply->lease, tmp, MDL);
			return (ISC_TRUE);
		}
	}

	return (ISC_FALSE);
}

/*
 * This function only returns failure on 'hard' failures.  If it succeeds,
 * it will leave a prefix structure behind.
 */
static isc_result_t
reply_process_try_prefix(struct reply_state *reply,
			 struct iaddrcidrnet *pref) {
	isc_result_t status = ISC_R_ADDRNOTAVAIL;
	struct ipv6_pool *pool = NULL;
	struct ipv6_pond *pond = NULL;
	int i;
	struct data_string data_pref;

	if ((reply == NULL) || (reply->shared == NULL) ||
	    (pref == NULL) || (reply->lease != NULL))
		return (DHCP_R_INVALIDARG);

	/*
	 * Do a quick walk through of the ponds and pools
	 * to see if we have any prefix pools
	 */
	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (pond->ipv6_pools == NULL)
			continue;

		for (i = 0; (pool = pond->ipv6_pools[i]) != NULL; i++) {
			if (pool->pool_type == D6O_IA_PD)
				break;
		}
		if (pool != NULL)
			break;
	}

	/* If we get here and p is NULL we have no useful pools */
	if (pool == NULL) {
		return (ISC_R_ADDRNOTAVAIL);
	}

	memset(&data_pref, 0, sizeof(data_pref));
	data_pref.len = 17;
	if (!buffer_allocate(&data_pref.buffer, data_pref.len, MDL)) {
		log_error("reply_process_try_prefix: out of memory.");
		return (ISC_R_NOMEMORY);
	}
	data_pref.data = data_pref.buffer->data;
	data_pref.buffer->data[0] = (u_int8_t) pref->bits;
	memcpy(data_pref.buffer->data + 1, pref->lo_addr.iabuf, 16);

	/*
	 * We have at least one pool that could provide a prefix
	 * Now we walk through the ponds and pools again and check
	 * to see if the client is permitted and if an prefix is
	 * available
	 *
	 */

	for (pond = reply->shared->ipv6_pond; pond != NULL; pond = pond->next) {
		if (((pond->prohibit_list != NULL) &&
		     (permitted(reply->packet, pond->prohibit_list))) ||
		    ((pond->permit_list != NULL) &&
		     (!permitted(reply->packet, pond->permit_list))))
			continue;

		for (i = 0; (pool = pond->ipv6_pools[i]) != NULL; i++) {
			if (pool->pool_type != D6O_IA_PD) {
				continue;
			}

			status = try_client_v6_prefix(&reply->lease, pool,
						      &data_pref);
			/* If we found it in this pool (either in use or available),
			   there is no need to look further. */
			if ( (status == ISC_R_SUCCESS) || (status == ISC_R_ADDRINUSE) )
				break;
			}
		if ( (status == ISC_R_SUCCESS) || (status == ISC_R_ADDRINUSE) )
			break;
	}

	data_string_forget(&data_pref, MDL);
	/* Return just the most recent status... */
	return (status);
}

/* Look around for a prefix to give the client.  First, look through the old
 * IA_PD for prefixes we can extend.  Second, try to allocate a new prefix.
 * Finally, actually add that prefix into the current reply IA_PD.
 */
static isc_result_t
find_client_prefix(struct reply_state *reply) {
	struct iaddrcidrnet send_pref;
	isc_result_t status = ISC_R_NORESOURCES;
	struct iasubopt *prefix, *best_prefix = NULL;
	struct binding_scope **scope;
	int i;
	struct group *group;

	if (reply->static_prefixes > 0) {
		struct iaddrcidrnetlist *l;

		if (reply->host == NULL)
			return DHCP_R_INVALIDARG;

		for (l = reply->host->fixed_prefix; l != NULL; l = l->next) {
			if (l->cidrnet.bits == reply->preflen)
				break;
		}
		if (l == NULL) {
			/*
			 * If no fixed prefix has the preferred length,
			 * get the first one.
			 */
			l = reply->host->fixed_prefix;
		}
		memcpy(&send_pref, &l->cidrnet, sizeof(send_pref));

		scope = &global_scope;

		/* Copy the prefix for logging purposes */
		memcpy(&reply->fixed_pref, &l->cidrnet, sizeof(send_pref));

		/* Try to find a group for the static prefix */
		group = find_group_by_prefix(reply);

		goto send_pref;
	}

	if (reply->old_ia != NULL)  {
		for (i = 0 ; i < reply->old_ia->num_iasubopt ; i++) {
			struct shared_network *candidate_shared;
			struct ipv6_pond *pond;

			prefix = reply->old_ia->iasubopt[i];
			candidate_shared = prefix->ipv6_pool->shared_network;
			pond = prefix->ipv6_pool->ipv6_pond;

			/*
			 * Consider this prefix if it is in a global pool or
			 * if it is scoped in a pool under the client's shared
			 * network.
			 */
			if (((candidate_shared != NULL) &&
			     (candidate_shared != reply->shared)) ||
			    (lease6_usable(prefix) != ISC_TRUE))
				continue;

			/*
			 * And check if the prefix is still permitted
			 */

			if (((pond->prohibit_list != NULL) &&
			     (permitted(reply->packet, pond->prohibit_list))) ||
			    ((pond->permit_list != NULL) &&
			     (!permitted(reply->packet, pond->permit_list))))
				continue;

			best_prefix = prefix_compare(reply, prefix,
						     best_prefix);
		}

		/*
		 * If we have prefix length hint and we're not igoring them,
		 * then toss the best match if it doesn't match the hint,
		 * unless this is in response to a rebind.  In the latter
		 * case we're supposed to return it with zero lifetimes.
		 * (See rt45780) */
		if (best_prefix && (reply->preflen > 0)
		    && (prefix_length_mode != PLM_IGNORE)
		    && (reply->preflen != best_prefix->plen)
		    && (reply->packet->dhcpv6_msg_type != DHCPV6_REBIND)) {
			best_prefix = NULL;
		}
	}

	/* Try to pick a new prefix if we didn't find one, or if we found an
	 * abandoned prefix.
	 */
	if ((best_prefix == NULL) || (best_prefix->state == FTS_ABANDONED)) {
		status = pick_v6_prefix(reply);
	} else if (best_prefix != NULL) {
		iasubopt_reference(&reply->lease, best_prefix, MDL);
		status = ISC_R_SUCCESS;
	}

	/* Pick the abandoned prefix as a last resort. */
	if ((status == ISC_R_NORESOURCES) && (best_prefix != NULL)) {
		/* I don't see how this is supposed to be done right now. */
		log_error("Reclaiming abandoned prefixes is not yet "
			  "supported.  Treating this as an out of space "
			  "condition.");
		/* iasubopt_reference(&reply->lease, best_prefix, MDL); */
	}

	/* Give up now if we didn't find a prefix. */
	if (status != ISC_R_SUCCESS)
		return status;

	if (reply->lease == NULL)
		log_fatal("Impossible condition at %s:%d.", MDL);

	scope = &reply->lease->scope;
	group = reply->lease->ipv6_pool->ipv6_pond->group;

	send_pref.lo_addr.len = 16;
	memcpy(send_pref.lo_addr.iabuf, &reply->lease->addr, 16);
	send_pref.bits = (int) reply->lease->plen;

      send_pref:
	status = reply_process_is_prefixed(reply, scope, group);
	if (status != ISC_R_SUCCESS)
		return status;

	status = reply_process_send_prefix(reply, &send_pref);
	return status;
}

/* Once a prefix is found for a client, perform several common functions;
 * Calculate and store valid and preferred prefix times, draw client options
 * into the option state.
 */
static isc_result_t
reply_process_is_prefixed(struct reply_state *reply,
			  struct binding_scope **scope, struct group *group)
{
	isc_result_t status = ISC_R_SUCCESS;
	struct data_string data;
	struct option_cache *oc;
	struct option_state *tmp_options = NULL;
	struct on_star *on_star;
	int i;

	/* Initialize values we will cleanup. */
	memset(&data, 0, sizeof(data));

	/*
	 * Find the proper on_star block to use.  We use the
	 * one in the lease if we have a lease or the one in
	 * the reply if we don't have a lease because this is
	 * a static instance
	 */
	if (reply->lease) {
		on_star = &reply->lease->on_star;
	} else {
		on_star = &reply->on_star;
	}

	/*
	 * Bring in the root configuration.  We only do this to bring
	 * in the on * statements, as we didn't have the lease available
	 * we we did it the first time.
	 */
	option_state_allocate(&tmp_options, MDL);
	execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
				    reply->packet->options, tmp_options,
				    &global_scope, root_group, NULL,
				    on_star);
	if (tmp_options != NULL) {
		option_state_dereference(&tmp_options, MDL);
	}

	/*
	 * Bring configured options into the root packet level cache - start
	 * with the lease's closest enclosing group (passed in by the caller
	 * as 'group').
	 */
	execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
				    reply->packet->options, reply->opt_state,
				    scope, group, root_group, on_star);

	/* Execute statements from class scopes. */
	for (i = reply->packet->class_count; i > 0; i--) {
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->opt_state, scope,
					    reply->packet->classes[i - 1]->group,
					    group, on_star);
	}

	/*
	 * If there is a host record, over-ride with values configured there,
	 * without re-evaluating configuration from the previously executed
	 * group or its common enclosers.
	 */
	if (reply->host != NULL)
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->opt_state, scope,
					    reply->host->group, group,
					    on_star);

	/* Determine valid lifetime. */
	if (reply->client_valid == 0)
		reply->send_valid = DEFAULT_DEFAULT_LEASE_TIME;
	else
		reply->send_valid = reply->client_valid;

	oc = lookup_option(&server_universe, reply->opt_state,
			   SV_DEFAULT_LEASE_TIME);
	if (oc != NULL) {
		if (!evaluate_option_cache(&data, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state,
					   scope, oc, MDL) ||
		    (data.len != 4)) {
			log_error("reply_process_is_prefixed: unable to "
				  "evaluate default prefix time");
			status = ISC_R_FAILURE;
			goto cleanup;
		}

		reply->send_valid = getULong(data.data);
		data_string_forget(&data, MDL);
	}

	/* Check to see if the lease time would cause us to wrap
	 * in which case we make it infinite.
	 * The following doesn't work on at least some systems:
	 * (cur_time + reply->send_valid < cur_time)
	 */
	if (reply->send_valid != INFINITE_TIME) {
		time_t test_time = cur_time + reply->send_valid;
		if (test_time < cur_time)
			reply->send_valid = INFINITE_TIME;
        }

	if (reply->client_prefer == 0)
		reply->send_prefer = reply->send_valid;
	else
		reply->send_prefer = reply->client_prefer;

	if ((reply->send_prefer >= reply->send_valid) &&
	    (reply->send_valid != INFINITE_TIME))
		reply->send_prefer = (reply->send_valid / 2) +
				     (reply->send_valid / 8);

	oc = lookup_option(&server_universe, reply->opt_state,
			   SV_PREFER_LIFETIME);
	if (oc != NULL) {
		if (!evaluate_option_cache(&data, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state,
					   scope, oc, MDL) ||
		    (data.len != 4)) {
			log_error("reply_process_is_prefixed: unable to "
				  "evaluate preferred prefix time");
			status = ISC_R_FAILURE;
			goto cleanup;
		}

		reply->send_prefer = getULong(data.data);
		data_string_forget(&data, MDL);
	}

	/* Note lowest values for later calculation of renew/rebind times. */
	if (reply->min_prefer > reply->send_prefer)
		reply->min_prefer = reply->send_prefer;

	if (reply->min_valid > reply->send_valid)
		reply->min_valid = reply->send_valid;

	/* Perform dynamic prefix related update work. */
	if (reply->lease != NULL) {
		/* Cached lifetimes */
		reply->lease->prefer = reply->send_prefer;
		reply->lease->valid = reply->send_valid;

		/* Advance (or rewind) the valid lifetime.
		 * In the protocol 0xFFFFFFFF is infinite
		 * when connecting to the lease file MAX_TIME is
		 */
		if (reply->buf.reply.msg_type == DHCPV6_REPLY) {
			if (reply->send_valid == INFINITE_TIME) {
				reply->lease->soft_lifetime_end_time = MAX_TIME;
			} else {
				reply->lease->soft_lifetime_end_time =
				  cur_time + reply->send_valid;
			}
			/* Wait before renew! */
		}

		status = ia_add_iasubopt(reply->ia, reply->lease, MDL);
		if (status != ISC_R_SUCCESS) {
			log_fatal("reply_process_is_prefixed: Unable to "
				  "attach prefix to new IA_PD: %s",
				  isc_result_totext(status));
		}

		/*
		 * If this is a new prefix, make sure it is attached somewhere.
		 */
		if (reply->lease->ia == NULL) {
			ia_reference(&reply->lease->ia, reply->ia, MDL);
		}
	}

	/* Bring a copy of the relevant options into the IA_PD scope. */
	execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
				    reply->packet->options, reply->reply_ia,
				    scope, group, root_group, NULL);

	/* Execute statements from class scopes. */
	for (i = reply->packet->class_count; i > 0; i--) {
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->reply_ia, scope,
					    reply->packet->classes[i - 1]->group,
					    group, NULL);
	}

	/*
	 * And bring in host record configuration, if any, but not to overlap
	 * the previous group or its common enclosers.
	 */
	if (reply->host != NULL)
		execute_statements_in_scope(NULL, reply->packet, NULL, NULL,
					    reply->packet->options,
					    reply->reply_ia, scope,
					    reply->host->group, group, NULL);

      cleanup:
	if (data.data != NULL)
		data_string_forget(&data, MDL);

	if (status == ISC_R_SUCCESS)
		reply->client_resources++;

	return status;
}

/* Simply send an IAPREFIX within the IA_PD scope as described. */
static isc_result_t
reply_process_send_prefix(struct reply_state *reply,
			  struct iaddrcidrnet *pref) {
	isc_result_t status = ISC_R_SUCCESS;
	struct data_string data;

	memset(&data, 0, sizeof(data));

	/* Now append the prefix. */
	data.len = IAPREFIX_OFFSET;
	if (!buffer_allocate(&data.buffer, data.len, MDL)) {
		log_error("reply_process_send_prefix: out of memory"
			  "allocating new IAPREFIX buffer.");
		status = ISC_R_NOMEMORY;
		goto cleanup;
	}
	data.data = data.buffer->data;

	putULong(data.buffer->data, reply->send_prefer);
	putULong(data.buffer->data + 4, reply->send_valid);
	data.buffer->data[8] = pref->bits;
	memcpy(data.buffer->data + 9, pref->lo_addr.iabuf, 16);

	if (!append_option_buffer(&dhcpv6_universe, reply->reply_ia,
				  data.buffer, data.buffer->data,
				  data.len, D6O_IAPREFIX, 0)) {
		log_error("reply_process_send_prefix: unable "
			  "to save IAPREFIX option");
		status = ISC_R_FAILURE;
		goto cleanup;
	}

	reply->resources_included = ISC_TRUE;

      cleanup:
	if (data.data != NULL)
		data_string_forget(&data, MDL);

	return status;
}

/* Choose the better of two prefixes. */
static struct iasubopt *
prefix_compare(struct reply_state *reply,
	       struct iasubopt *alpha, struct iasubopt *beta) {
	if (alpha == NULL)
		return beta;
	if (beta == NULL)
		return alpha;

	if (reply->preflen >= 0) {
		if ((alpha->plen == reply->preflen) &&
		    (beta->plen != reply->preflen))
			return alpha;
		if ((beta->plen == reply->preflen) &&
		    (alpha->plen != reply->preflen))
			return beta;
	}

	switch(alpha->state) {
	      case FTS_ACTIVE:
		switch(beta->state) {
		      case FTS_ACTIVE:
			/* Choose the prefix with the longest lifetime (most
			 * likely the most recently allocated).
			 */
			if (alpha->hard_lifetime_end_time <
			    beta->hard_lifetime_end_time)
				return beta;
			else
				return alpha;

		      case FTS_EXPIRED:
		      case FTS_ABANDONED:
			return alpha;

		      default:
			log_fatal("Impossible condition at %s:%d.", MDL);
		}
		break;

	      case FTS_EXPIRED:
		switch (beta->state) {
		      case FTS_ACTIVE:
			return beta;

		      case FTS_EXPIRED:
			/* Choose the most recently expired prefix. */
			if (alpha->hard_lifetime_end_time <
			    beta->hard_lifetime_end_time)
				return beta;
			else if ((alpha->hard_lifetime_end_time ==
				  beta->hard_lifetime_end_time) &&
				 (alpha->soft_lifetime_end_time <
				  beta->soft_lifetime_end_time))
				return beta;
			else
				return alpha;

		      case FTS_ABANDONED:
			return alpha;

		      default:
			log_fatal("Impossible condition at %s:%d.", MDL);
		}
		break;

	      case FTS_ABANDONED:
		switch (beta->state) {
		      case FTS_ACTIVE:
		      case FTS_EXPIRED:
			return alpha;

		      case FTS_ABANDONED:
			/* Choose the prefix that was abandoned longest ago. */
			if (alpha->hard_lifetime_end_time <
			    beta->hard_lifetime_end_time)
				return alpha;
			else
				return beta;

		      default:
			log_fatal("Impossible condition at %s:%d.", MDL);
		}
		break;

	      default:
		log_fatal("Impossible condition at %s:%d.", MDL);
	}

	log_fatal("Triple impossible condition at %s:%d.", MDL);
	return NULL;
}

/*
 * Solicit is how a client starts requesting addresses.
 *
 * If the client asks for rapid commit, and we support it, we will
 * allocate the addresses and reply.
 *
 * Otherwise we will send an advertise message.
 */

static void
dhcpv6_solicit(struct data_string *reply_ret, struct packet *packet) {
	struct data_string client_id;

	/*
	 * Validate our input.
	 */
	if (!valid_client_msg(packet, &client_id)) {
		return;
	}

	lease_to_client(reply_ret, packet, &client_id, NULL);

	/*
	 * Clean up.
	 */
	data_string_forget(&client_id, MDL);
}

/*
 * Request is how a client actually requests addresses.
 *
 * Very similar to Solicit handling, except the server DUID is required.
 */

static void
dhcpv6_request(struct data_string *reply_ret, struct packet *packet) {
	struct data_string client_id;
	struct data_string server_id;

	/*
	 * Validate our input.
	 */
	if (!valid_client_resp(packet, &client_id, &server_id)) {
		return;
	}

	/* If the REQUEST arrived via unicast and unicast option isn't set,
 	 * reject it per RFC 3315, Sec 18.2.1 */
	if (packet->unicast == ISC_TRUE &&
	    is_unicast_option_defined(packet) == ISC_FALSE) {
		unicast_reject(reply_ret, packet, &client_id, &server_id);
	} else {
		/*
		 * Issue our lease.
		 */
		lease_to_client(reply_ret, packet, &client_id, &server_id);
	}

	/*
	 * Cleanup.
	 */
	data_string_forget(&client_id, MDL);
	data_string_forget(&server_id, MDL);
}

/* Find a DHCPv6 packet's shared network from hints in the packet.
 */
static isc_result_t
shared_network_from_packet6(struct shared_network **shared,
			    struct packet *packet)
{
	const struct packet *chk_packet;
	const struct in6_addr *link_addr, *first_link_addr;
	struct iaddr tmp_addr;
	struct subnet *subnet;
	isc_result_t status;

	if ((shared == NULL) || (*shared != NULL) || (packet == NULL))
		return DHCP_R_INVALIDARG;

	/*
	 * First, find the link address where the packet from the client
	 * first appeared (if this packet was relayed).
	 */
	first_link_addr = NULL;
	chk_packet = packet->dhcpv6_container_packet;
	while (chk_packet != NULL) {
		link_addr = &chk_packet->dhcpv6_link_address;
		if (!IN6_IS_ADDR_UNSPECIFIED(link_addr) &&
		    !IN6_IS_ADDR_LINKLOCAL(link_addr)) {
			first_link_addr = link_addr;
			break;
		}
		chk_packet = chk_packet->dhcpv6_container_packet;
	}

	/*
	 * If there is a relayed link address, find the subnet associated
	 * with that, and use that to get the appropriate
	 * shared_network.
	 */
	if (first_link_addr != NULL) {
		tmp_addr.len = sizeof(*first_link_addr);
		memcpy(tmp_addr.iabuf,
		       first_link_addr, sizeof(*first_link_addr));
		subnet = NULL;
		if (!find_subnet(&subnet, tmp_addr, MDL)) {
			log_debug("No subnet found for link-address %s.",
				  piaddr(tmp_addr));
			return ISC_R_NOTFOUND;
		}
		status = shared_network_reference(shared,
						  subnet->shared_network, MDL);
		subnet_dereference(&subnet, MDL);

	/*
	 * If there is no link address, we will use the interface
	 * that this packet came in on to pick the shared_network.
	 */
	} else if (packet->interface != NULL) {
		status = shared_network_reference(shared,
					 packet->interface->shared_network,
					 MDL);
                if (packet->dhcpv6_container_packet != NULL) {
			log_info("[L2 Relay] No link address in relay packet "
				 "assuming L2 relay and using receiving "
				 "interface");
                }

	} else {
		/*
		 * We shouldn't be able to get here but if there is no link
		 * address and no interface we don't know where to get the
		 * pool from log an error and return an error.
		 */
		log_error("No interface and no link address "
			  "can't determine pool");
		status = DHCP_R_INVALIDARG;
	}

	return status;
}

/*
 * When a client thinks it might be on a new link, it sends a
 * Confirm message.
 *
 * From RFC3315 section 18.2.2:
 *
 *   When the server receives a Confirm message, the server determines
 *   whether the addresses in the Confirm message are appropriate for the
 *   link to which the client is attached.  If all of the addresses in the
 *   Confirm message pass this test, the server returns a status of
 *   Success.  If any of the addresses do not pass this test, the server
 *   returns a status of NotOnLink.  If the server is unable to perform
 *   this test (for example, the server does not have information about
 *   prefixes on the link to which the client is connected), or there were
 *   no addresses in any of the IAs sent by the client, the server MUST
 *   NOT send a reply to the client.
 */

static void
dhcpv6_confirm(struct data_string *reply_ret, struct packet *packet) {
	struct shared_network *shared;
	struct subnet *subnet;
	struct option_cache *ia, *ta, *oc;
	struct data_string cli_enc_opt_data, iaaddr, client_id, packet_oro;
	struct option_state *cli_enc_opt_state, *opt_state;
	struct iaddr cli_addr;
	int pass;
	isc_boolean_t inappropriate, has_addrs;
	char reply_data[65536];
	struct dhcpv6_packet *reply = (struct dhcpv6_packet *)reply_data;
	int reply_ofs = (int)(offsetof(struct dhcpv6_packet, options));

	/*
	 * Basic client message validation.
	 */
	memset(&client_id, 0, sizeof(client_id));
	if (!valid_client_msg(packet, &client_id)) {
		return;
	}

	/*
	 * Do not process Confirms that do not have IA's we do not recognize.
	 */
	ia = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_NA);
	ta = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_TA);
	if ((ia == NULL) && (ta == NULL))
		return;

	/*
	 * IA_PD's are simply ignored.
	 */
	delete_option(&dhcpv6_universe, packet->options, D6O_IA_PD);

	/*
	 * Bit of variable initialization.
	 */
	opt_state = cli_enc_opt_state = NULL;
	memset(&cli_enc_opt_data, 0, sizeof(cli_enc_opt_data));
	memset(&iaaddr, 0, sizeof(iaaddr));
	memset(&packet_oro, 0, sizeof(packet_oro));

	/* Determine what shared network the client is connected to.  We
	 * must not respond if we don't have any information about the
	 * network the client is on.
	 */
	shared = NULL;
	if ((shared_network_from_packet6(&shared, packet) != ISC_R_SUCCESS) ||
	    (shared == NULL))
		goto exit;

	/* If there are no recorded subnets, then we have no
	 * information about this subnet - ignore Confirms.
	 */
	subnet = shared->subnets;
	if (subnet == NULL)
		goto exit;

	/* Are the addresses in all the IA's appropriate for that link? */
	has_addrs = inappropriate = ISC_FALSE;
	pass = D6O_IA_NA;
	while(!inappropriate) {
		/* If we've reached the end of the IA_NA pass, move to the
		 * IA_TA pass.
		 */
		if ((pass == D6O_IA_NA) && (ia == NULL)) {
			pass = D6O_IA_TA;
			ia = ta;
		}

		/* If we've reached the end of all passes, we're done. */
		if (ia == NULL)
			break;

		if (((pass == D6O_IA_NA) &&
		     !get_encapsulated_IA_state(&cli_enc_opt_state,
						&cli_enc_opt_data,
						packet, ia, IA_NA_OFFSET)) ||
		    ((pass == D6O_IA_TA) &&
		     !get_encapsulated_IA_state(&cli_enc_opt_state,
						&cli_enc_opt_data,
						packet, ia, IA_TA_OFFSET))) {
			goto exit;
		}

		oc = lookup_option(&dhcpv6_universe, cli_enc_opt_state,
				   D6O_IAADDR);

		for ( ; oc != NULL ; oc = oc->next) {
			if (!evaluate_option_cache(&iaaddr, packet, NULL, NULL,
						   packet->options, NULL,
						   &global_scope, oc, MDL) ||
			    (iaaddr.len < IAADDR_OFFSET)) {
				log_error("dhcpv6_confirm: "
					  "error evaluating IAADDR.");
				goto exit;
			}

			/* Copy out the IPv6 address for processing. */
			cli_addr.len = 16;
			memcpy(cli_addr.iabuf, iaaddr.data, 16);

			data_string_forget(&iaaddr, MDL);

			/* Record that we've processed at least one address. */
			has_addrs = ISC_TRUE;

			/* Find out if any subnets cover this address. */
			for (subnet = shared->subnets ; subnet != NULL ;
			     subnet = subnet->next_sibling) {
				if (addr_eq(subnet_number(cli_addr,
							  subnet->netmask),
					    subnet->net))
					break;
			}

			/* If we reach the end of the subnet list, and no
			 * subnet matches the client address, then it must
			 * be inappropriate to the link (so far as our
			 * configuration says).  Once we've found one
			 * inappropriate address, there is no reason to
			 * continue searching.
			 */
			if (subnet == NULL) {
				inappropriate = ISC_TRUE;
				break;
			}
		}

		option_state_dereference(&cli_enc_opt_state, MDL);
		data_string_forget(&cli_enc_opt_data, MDL);

		/* Advance to the next IA_*. */
		ia = ia->next;
	}

	/* If the client supplied no addresses, do not reply. */
	if (!has_addrs)
		goto exit;

	/*
	 * Set up reply.
	 */
	if (!start_reply(packet, &client_id, NULL, &opt_state, reply)) {
		goto exit;
	}

	/*
	 * Set our status.
	 */
	if (inappropriate) {
		if (!set_status_code(STATUS_NotOnLink,
				     "Some of the addresses are not on link.",
				     opt_state)) {
			goto exit;
		}
	} else {
		if (!set_status_code(STATUS_Success,
				     "All addresses still on link.",
				     opt_state)) {
			goto exit;
		}
	}

	/*
	 * Only one option: add it.
	 */
	reply_ofs += store_options6(reply_data+reply_ofs,
				    sizeof(reply_data)-reply_ofs,
				    opt_state, packet,
				    required_opts, &packet_oro);

	/*
	 * Return our reply to the caller.
	 */
	reply_ret->len = reply_ofs;
	reply_ret->buffer = NULL;
	if (!buffer_allocate(&reply_ret->buffer, reply_ofs, MDL)) {
		log_fatal("No memory to store reply.");
	}
	reply_ret->data = reply_ret->buffer->data;
	memcpy(reply_ret->buffer->data, reply, reply_ofs);

exit:
	/* Cleanup any stale data strings. */
	if (cli_enc_opt_data.buffer != NULL)
		data_string_forget(&cli_enc_opt_data, MDL);
	if (iaaddr.buffer != NULL)
		data_string_forget(&iaaddr, MDL);
	if (client_id.buffer != NULL)
		data_string_forget(&client_id, MDL);
	if (packet_oro.buffer != NULL)
		data_string_forget(&packet_oro, MDL);

	/* Release any stale option states. */
	if (cli_enc_opt_state != NULL)
		option_state_dereference(&cli_enc_opt_state, MDL);
	if (opt_state != NULL)
		option_state_dereference(&opt_state, MDL);
}

/*
 * Renew is when a client wants to extend its lease/prefix, at time T1.
 *
 * We handle this the same as if the client wants a new lease/prefix,
 * except for the error code of when addresses don't match.
 */

static void
dhcpv6_renew(struct data_string *reply, struct packet *packet) {
	struct data_string client_id;
	struct data_string server_id;

	/*
	 * Validate the request.
	 */
	if (!valid_client_resp(packet, &client_id, &server_id)) {
		return;
	}

	/* If the RENEW arrived via unicast and unicast option isn't set,
	 * reject it per RFC 3315, Sec 18.2.3 */
	if (packet->unicast == ISC_TRUE &&
	    is_unicast_option_defined(packet) == ISC_FALSE) {
		unicast_reject(reply, packet, &client_id, &server_id);
	} else {
		/*
		 * Renew our lease.
		 */
		lease_to_client(reply, packet, &client_id, &server_id);
	}

	/*
	 * Cleanup.
	 */
	data_string_forget(&server_id, MDL);
	data_string_forget(&client_id, MDL);
}

/*
 * Rebind is when a client wants to extend its lease, at time T2.
 *
 * We handle this the same as if the client wants a new lease, except
 * for the error code of when addresses don't match.
 */

static void
dhcpv6_rebind(struct data_string *reply, struct packet *packet) {
	struct data_string client_id;

	if (!valid_client_msg(packet, &client_id)) {
		return;
	}

	lease_to_client(reply, packet, &client_id, NULL);

	data_string_forget(&client_id, MDL);
}

static void
ia_na_match_decline(const struct data_string *client_id,
		    const struct data_string *iaaddr,
		    struct iasubopt *lease)
{
	char tmp_addr[INET6_ADDRSTRLEN];

	log_error("Client %s reports address %s is "
		  "already in use by another host!",
		  print_hex_1(client_id->len, client_id->data, 60),
		  inet_ntop(AF_INET6, iaaddr->data,
		  	    tmp_addr, sizeof(tmp_addr)));
	if (lease != NULL) {
		decline_lease6(lease->ipv6_pool, lease);
		lease->ia->cltt = cur_time;
		write_ia(lease->ia);
	}
}

static void
ia_na_nomatch_decline(const struct data_string *client_id,
		      const struct data_string *iaaddr,
		      u_int32_t *ia_na_id,
		      struct packet *packet,
		      char *reply_data,
		      int *reply_ofs,
		      int reply_len)
{
	char tmp_addr[INET6_ADDRSTRLEN];
	struct option_state *host_opt_state;
	int len;

	log_info("Client %s declines address %s, which is not offered to it.",
		 print_hex_1(client_id->len, client_id->data, 60),
		 inet_ntop(AF_INET6, iaaddr->data, tmp_addr, sizeof(tmp_addr)));

	/*
	 * Create state for this IA_NA.
	 */
	host_opt_state = NULL;
	if (!option_state_allocate(&host_opt_state, MDL)) {
		log_error("ia_na_nomatch_decline: out of memory "
			  "allocating option_state.");
		goto exit;
	}

	if (!set_status_code(STATUS_NoBinding, "Decline for unknown address.",
			     host_opt_state)) {
		goto exit;
	}

	/*
	 * Insure we have enough space
	 */
	if (reply_len < (*reply_ofs + 16)) {
		log_error("ia_na_nomatch_decline: "
			  "out of space for reply packet.");
		goto exit;
	}

	/*
	 * Put our status code into the reply packet.
	 */
	len = store_options6(reply_data+(*reply_ofs)+16,
			     reply_len-(*reply_ofs)-16,
			     host_opt_state, packet,
			     required_opts_STATUS_CODE, NULL);

	/*
	 * Store the non-encapsulated option data for this
	 * IA_NA into our reply packet. Defined in RFC 3315,
	 * section 22.4.
	 */
	/* option number */
	putUShort((unsigned char *)reply_data+(*reply_ofs), D6O_IA_NA);
	/* option length */
	putUShort((unsigned char *)reply_data+(*reply_ofs)+2, len + 12);
	/* IA_NA, copied from the client */
	memcpy(reply_data+(*reply_ofs)+4, ia_na_id, 4);
	/* t1 and t2, odd that we need them, but here it is */
	putULong((unsigned char *)reply_data+(*reply_ofs)+8, 0);
	putULong((unsigned char *)reply_data+(*reply_ofs)+12, 0);

	/*
	 * Get ready for next IA_NA.
	 */
	*reply_ofs += (len + 16);

exit:
	option_state_dereference(&host_opt_state, MDL);
}

static void
iterate_over_ia_na(struct data_string *reply_ret,
		   struct packet *packet,
		   const struct data_string *client_id,
		   const struct data_string *server_id,
		   const char *packet_type,
		   void (*ia_na_match)(const struct data_string *,
                                       const struct data_string *,
                                       struct iasubopt *),
		   void (*ia_na_nomatch)(const struct data_string *,
                                         const struct data_string *,
                                         u_int32_t *, struct packet *, char *,
                                         int *, int))
{
	struct option_state *opt_state;
	struct host_decl *packet_host;
	struct option_cache *ia;
	struct option_cache *oc;
	/* cli_enc_... variables come from the IA_NA/IA_TA options */
	struct data_string cli_enc_opt_data;
	struct option_state *cli_enc_opt_state;
	struct host_decl *host;
	struct data_string iaaddr;
	struct data_string fixed_addr;
	char reply_data[65536];
	struct dhcpv6_packet *reply = (struct dhcpv6_packet *)reply_data;
	int reply_ofs = (int)(offsetof(struct dhcpv6_packet, options));
	char status_msg[32];
	struct iasubopt *lease;
	struct ia_xx *existing_ia_na;
	int i;
	struct data_string key;
	u_int32_t iaid;

	/*
	 * Initialize to empty values, in case we have to exit early.
	 */
	opt_state = NULL;
	memset(&cli_enc_opt_data, 0, sizeof(cli_enc_opt_data));
	cli_enc_opt_state = NULL;
	memset(&iaaddr, 0, sizeof(iaaddr));
	memset(&fixed_addr, 0, sizeof(fixed_addr));
	lease = NULL;

	/*
	 * Find the host record that matches from the packet, if any.
	 */
	packet_host = NULL;
	find_hosts6(&packet_host, packet, client_id, MDL);

	/*
	 * Set our reply information.
	 */
	reply->msg_type = DHCPV6_REPLY;
	memcpy(reply->transaction_id, packet->dhcpv6_transaction_id,
	       sizeof(reply->transaction_id));

	/*
	 * Build our option state for reply.
	 */
	opt_state = NULL;
	if (!option_state_allocate(&opt_state, MDL)) {
		log_error("iterate_over_ia_na: no memory for option_state.");
		goto exit;
	}
	execute_statements_in_scope(NULL, packet, NULL, NULL,
				    packet->options, opt_state,
				    &global_scope, root_group, NULL, NULL);

	/*
	 * RFC 3315, section 18.2.7 tells us which options to include.
	 */
	oc = lookup_option(&dhcpv6_universe, opt_state, D6O_SERVERID);
	if (oc == NULL) {
		if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
					(unsigned char *)server_duid.data,
					server_duid.len, D6O_SERVERID, 0)) {
			log_error("iterate_over_ia_na: "
				  "error saving server identifier.");
			goto exit;
		}
	}

	if (!save_option_buffer(&dhcpv6_universe, opt_state,
				client_id->buffer,
				(unsigned char *)client_id->data,
				client_id->len,
				D6O_CLIENTID, 0)) {
		log_error("iterate_over_ia_na: "
			  "error saving client identifier.");
		goto exit;
	}

	snprintf(status_msg, sizeof(status_msg), "%s received.", packet_type);
	if (!set_status_code(STATUS_Success, status_msg, opt_state)) {
		goto exit;
	}

	/*
	 * Add our options that are not associated with any IA_NA or IA_TA.
	 */
	reply_ofs += store_options6(reply_data+reply_ofs,
				    sizeof(reply_data)-reply_ofs,
				    opt_state, packet,
				    required_opts, NULL);

	/*
	 * Loop through the IA_NA reported by the client, and deal with
	 * addresses reported as already in use.
	 */
	for (ia = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_NA);
	     ia != NULL; ia = ia->next) {

		if (!get_encapsulated_IA_state(&cli_enc_opt_state,
					       &cli_enc_opt_data,
					       packet, ia, IA_NA_OFFSET)) {
			goto exit;
		}

		iaid = getULong(cli_enc_opt_data.data);

		/*
		 * XXX: It is possible that we can get multiple addresses
		 *      sent by the client. We don't send multiple
		 *      addresses, so this indicates a client error.
		 *      We should check for multiple IAADDR options, log
		 *      if found, and set as an error.
		 */
		oc = lookup_option(&dhcpv6_universe, cli_enc_opt_state,
				   D6O_IAADDR);
		if (oc == NULL) {
			/* no address given for this IA, ignore */
			option_state_dereference(&cli_enc_opt_state, MDL);
			data_string_forget(&cli_enc_opt_data, MDL);
			continue;
		}

		memset(&iaaddr, 0, sizeof(iaaddr));
		if (!evaluate_option_cache(&iaaddr, packet, NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("iterate_over_ia_na: "
				  "error evaluating IAADDR.");
			goto exit;
		}

		/*
		 * Now we need to figure out which host record matches
		 * this IA_NA and IAADDR (encapsulated option contents
		 * matching a host record by option).
		 *
		 * XXX: We don't currently track IA_NA separately, but
		 *      we will need to do this!
		 */
		host = NULL;
		if (!find_hosts_by_option(&host, packet,
					  cli_enc_opt_state, MDL)) {
			if (packet_host != NULL) {
				host = packet_host;
			} else {
				host = NULL;
			}
		}
		while (host != NULL) {
			if (host->fixed_addr != NULL) {
				if (!evaluate_option_cache(&fixed_addr, NULL,
							   NULL, NULL, NULL,
							   NULL, &global_scope,
							   host->fixed_addr,
							   MDL)) {
					log_error("iterate_over_ia_na: error "
						  "evaluating host address.");
					goto exit;
				}
				if ((iaaddr.len >= 16) &&
				    !memcmp(fixed_addr.data, iaaddr.data, 16)) {
					data_string_forget(&fixed_addr, MDL);
					break;
				}
				data_string_forget(&fixed_addr, MDL);
			}
			host = host->n_ipaddr;
		}

		if ((host == NULL) && (iaaddr.len >= IAADDR_OFFSET)) {
			/*
			 * Find existing IA_NA.
			 */
			if (ia_make_key(&key, iaid,
					(char *)client_id->data,
					client_id->len,
					MDL) != ISC_R_SUCCESS) {
				log_fatal("iterate_over_ia_na: no memory for "
					  "key.");
			}

			existing_ia_na = NULL;
			if (ia_hash_lookup(&existing_ia_na, ia_na_active,
					   (unsigned char *)key.data,
					   key.len, MDL)) {
				/*
				 * Make sure this address is in the IA_NA.
				 */
				for (i=0; i<existing_ia_na->num_iasubopt; i++) {
					struct iasubopt *tmp;
					struct in6_addr *in6_addr;

					tmp = existing_ia_na->iasubopt[i];
					in6_addr = &tmp->addr;
					if (memcmp(in6_addr,
						   iaaddr.data, 16) == 0) {
						iasubopt_reference(&lease,
								   tmp, MDL);
						break;
					}
				}
			}

			data_string_forget(&key, MDL);
		}

		if ((host != NULL) || (lease != NULL)) {
			ia_na_match(client_id, &iaaddr, lease);
		} else {
			ia_na_nomatch(client_id, &iaaddr,
				      (u_int32_t *)cli_enc_opt_data.data,
				      packet, reply_data, &reply_ofs,
				      sizeof(reply_data));
		}

		if (lease != NULL) {
			iasubopt_dereference(&lease, MDL);
		}

		data_string_forget(&iaaddr, MDL);
		option_state_dereference(&cli_enc_opt_state, MDL);
		data_string_forget(&cli_enc_opt_data, MDL);
	}

	/*
	 * Return our reply to the caller.
	 */
	reply_ret->len = reply_ofs;
	reply_ret->buffer = NULL;
	if (!buffer_allocate(&reply_ret->buffer, reply_ofs, MDL)) {
		log_fatal("No memory to store reply.");
	}
	reply_ret->data = reply_ret->buffer->data;
	memcpy(reply_ret->buffer->data, reply, reply_ofs);

exit:
	if (lease != NULL) {
		iasubopt_dereference(&lease, MDL);
	}
	if (fixed_addr.buffer != NULL) {
		data_string_forget(&fixed_addr, MDL);
	}
	if (iaaddr.buffer != NULL) {
		data_string_forget(&iaaddr, MDL);
	}
	if (cli_enc_opt_state != NULL) {
		option_state_dereference(&cli_enc_opt_state, MDL);
	}
	if (cli_enc_opt_data.buffer != NULL) {
		data_string_forget(&cli_enc_opt_data, MDL);
	}
	if (opt_state != NULL) {
		option_state_dereference(&opt_state, MDL);
	}
}

/*
 * Decline means a client has detected that something else is using an
 * address we gave it.
 *
 * Since we're only dealing with fixed leases for now, there's not
 * much we can do, other that log the occurrence.
 *
 * When we start issuing addresses from pools, then we will have to
 * record our declined addresses and issue another. In general with
 * IPv6 there is no worry about DoS by clients exhausting space, but
 * we still need to be aware of this possibility.
 */

/* TODO: IA_TA */
static void
dhcpv6_decline(struct data_string *reply, struct packet *packet) {
	struct data_string client_id;
	struct data_string server_id;

	/*
	 * Validate our input.
	 */
	if (!valid_client_resp(packet, &client_id, &server_id)) {
		return;
	}

	/* If the DECLINE arrived via unicast and unicast option isn't set,
	 * reject it per RFC 3315, Sec 18.2.7 */
	if (packet->unicast == ISC_TRUE &&
	    is_unicast_option_defined(packet) == ISC_FALSE) {
		unicast_reject(reply, packet, &client_id, &server_id);
	} else {
		/*
		 * Undefined for IA_PD.
		 */
		delete_option(&dhcpv6_universe, packet->options, D6O_IA_PD);

		/*
		 * And operate on each IA_NA in this packet.
		 */
		iterate_over_ia_na(reply, packet, &client_id, &server_id,
				   "Decline", ia_na_match_decline,
				   ia_na_nomatch_decline);

	}

	data_string_forget(&server_id, MDL);
	data_string_forget(&client_id, MDL);
}

static void
ia_na_match_release(const struct data_string *client_id,
		    const struct data_string *iaaddr,
		    struct iasubopt *lease)
{
	char tmp_addr[INET6_ADDRSTRLEN];

	log_info("Client %s releases address %s",
		 print_hex_1(client_id->len, client_id->data, 60),
		 inet_ntop(AF_INET6, iaaddr->data, tmp_addr, sizeof(tmp_addr)));
	if (lease != NULL) {
		release_lease6(lease->ipv6_pool, lease);
		lease->ia->cltt = cur_time;
		write_ia(lease->ia);
	}
}

static void
ia_na_nomatch_release(const struct data_string *client_id,
		      const struct data_string *iaaddr,
		      u_int32_t *ia_na_id,
		      struct packet *packet,
		      char *reply_data,
		      int *reply_ofs,
		      int reply_len)
{
	char tmp_addr[INET6_ADDRSTRLEN];
	struct option_state *host_opt_state;
	int len;

	log_info("Client %s releases address %s, which is not leased to it.",
		 print_hex_1(client_id->len, client_id->data, 60),
		 inet_ntop(AF_INET6, iaaddr->data, tmp_addr, sizeof(tmp_addr)));

	/*
	 * Create state for this IA_NA.
	 */
	host_opt_state = NULL;
	if (!option_state_allocate(&host_opt_state, MDL)) {
		log_error("ia_na_nomatch_release: out of memory "
			  "allocating option_state.");
		goto exit;
	}

	if (!set_status_code(STATUS_NoBinding,
			     "Release for non-leased address.",
			     host_opt_state)) {
		goto exit;
	}

	/*
	 * Insure we have enough space
	 */
	if (reply_len < (*reply_ofs + 16)) {
		log_error("ia_na_nomatch_release: "
			  "out of space for reply packet.");
		goto exit;
	}

	/*
	 * Put our status code into the reply packet.
	 */
	len = store_options6(reply_data+(*reply_ofs)+16,
			     reply_len-(*reply_ofs)-16,
			     host_opt_state, packet,
			     required_opts_STATUS_CODE, NULL);

	/*
	 * Store the non-encapsulated option data for this
	 * IA_NA into our reply packet. Defined in RFC 3315,
	 * section 22.4.
	 */
	/* option number */
	putUShort((unsigned char *)reply_data+(*reply_ofs), D6O_IA_NA);
	/* option length */
	putUShort((unsigned char *)reply_data+(*reply_ofs)+2, len + 12);
	/* IA_NA, copied from the client */
	memcpy(reply_data+(*reply_ofs)+4, ia_na_id, 4);
	/* t1 and t2, odd that we need them, but here it is */
	putULong((unsigned char *)reply_data+(*reply_ofs)+8, 0);
	putULong((unsigned char *)reply_data+(*reply_ofs)+12, 0);

	/*
	 * Get ready for next IA_NA.
	 */
	*reply_ofs += (len + 16);

exit:
	option_state_dereference(&host_opt_state, MDL);
}

static void
ia_pd_match_release(const struct data_string *client_id,
		    const struct data_string *iapref,
		    struct iasubopt *prefix)
{
	char tmp_addr[INET6_ADDRSTRLEN];

	log_info("Client %s releases prefix %s/%u",
		 print_hex_1(client_id->len, client_id->data, 60),
		 inet_ntop(AF_INET6, iapref->data + 9,
			   tmp_addr, sizeof(tmp_addr)),
		 (unsigned) getUChar(iapref->data + 8));
	if (prefix != NULL) {
		release_lease6(prefix->ipv6_pool, prefix);
		prefix->ia->cltt = cur_time;
		write_ia(prefix->ia);
	}
}

static void
ia_pd_nomatch_release(const struct data_string *client_id,
		      const struct data_string *iapref,
		      u_int32_t *ia_pd_id,
		      struct packet *packet,
		      char *reply_data,
		      int *reply_ofs,
		      int reply_len)
{
	char tmp_addr[INET6_ADDRSTRLEN];
	struct option_state *host_opt_state;
	int len;

	log_info("Client %s releases prefix %s/%u, which is not leased to it.",
		 print_hex_1(client_id->len, client_id->data, 60),
		 inet_ntop(AF_INET6, iapref->data + 9,
			   tmp_addr, sizeof(tmp_addr)),
		 (unsigned) getUChar(iapref->data + 8));

	/*
	 * Create state for this IA_PD.
	 */
	host_opt_state = NULL;
	if (!option_state_allocate(&host_opt_state, MDL)) {
		log_error("ia_pd_nomatch_release: out of memory "
			  "allocating option_state.");
		goto exit;
	}

	if (!set_status_code(STATUS_NoBinding,
			     "Release for non-leased prefix.",
			     host_opt_state)) {
		goto exit;
	}

	/*
	 * Insure we have enough space
	 */
	if (reply_len < (*reply_ofs + 16)) {
		log_error("ia_pd_nomatch_release: "
			  "out of space for reply packet.");
		goto exit;
	}

	/*
	 * Put our status code into the reply packet.
	 */
	len = store_options6(reply_data+(*reply_ofs)+16,
			     reply_len-(*reply_ofs)-16,
			     host_opt_state, packet,
			     required_opts_STATUS_CODE, NULL);

	/*
	 * Store the non-encapsulated option data for this
	 * IA_PD into our reply packet. Defined in RFC 3315,
	 * section 22.4.
	 */
	/* option number */
	putUShort((unsigned char *)reply_data+(*reply_ofs), D6O_IA_PD);
	/* option length */
	putUShort((unsigned char *)reply_data+(*reply_ofs)+2, len + 12);
	/* IA_PD, copied from the client */
	memcpy(reply_data+(*reply_ofs)+4, ia_pd_id, 4);
	/* t1 and t2, odd that we need them, but here it is */
	putULong((unsigned char *)reply_data+(*reply_ofs)+8, 0);
	putULong((unsigned char *)reply_data+(*reply_ofs)+12, 0);

	/*
	 * Get ready for next IA_PD.
	 */
	*reply_ofs += (len + 16);

exit:
	option_state_dereference(&host_opt_state, MDL);
}

static void
iterate_over_ia_pd(struct data_string *reply_ret,
		   struct packet *packet,
		   const struct data_string *client_id,
		   const struct data_string *server_id,
		   const char *packet_type,
                   void (*ia_pd_match)(const struct data_string *,
                                       const struct data_string *,
                                       struct iasubopt *),
                   void (*ia_pd_nomatch)(const struct data_string *,
                                         const struct data_string *,
                                         u_int32_t *, struct packet *, char *,
                                         int *, int))
{
	struct data_string reply_new;
	int reply_len;
	struct option_state *opt_state;
	struct host_decl *packet_host;
	struct option_cache *ia;
	struct option_cache *oc;
	/* cli_enc_... variables come from the IA_PD options */
	struct data_string cli_enc_opt_data;
	struct option_state *cli_enc_opt_state;
	struct host_decl *host;
	struct data_string iaprefix;
	char reply_data[65536];
	int reply_ofs;
	struct iasubopt *prefix;
	struct ia_xx *existing_ia_pd;
	int i;
	struct data_string key;
	u_int32_t iaid;

	/*
	 * Initialize to empty values, in case we have to exit early.
	 */
	memset(&reply_new, 0, sizeof(reply_new));
	opt_state = NULL;
	memset(&cli_enc_opt_data, 0, sizeof(cli_enc_opt_data));
	cli_enc_opt_state = NULL;
	memset(&iaprefix, 0, sizeof(iaprefix));
	prefix = NULL;

	/*
	 * Compute the available length for the reply.
	 */
	reply_len = sizeof(reply_data) - reply_ret->len;
	reply_ofs = 0;

	/*
	 * Find the host record that matches from the packet, if any.
	 */
	packet_host = NULL;
	find_hosts6(&packet_host, packet, client_id, MDL);

	/*
	 * Build our option state for reply.
	 */
	opt_state = NULL;
	if (!option_state_allocate(&opt_state, MDL)) {
		log_error("iterate_over_ia_pd: no memory for option_state.");
		goto exit;
	}
	execute_statements_in_scope(NULL, packet, NULL, NULL,
				    packet->options, opt_state,
				    &global_scope, root_group, NULL, NULL);

	/*
	 * Loop through the IA_PD reported by the client, and deal with
	 * prefixes reported as already in use.
	 */
	for (ia = lookup_option(&dhcpv6_universe, packet->options, D6O_IA_PD);
	     ia != NULL; ia = ia->next) {

	    if (!get_encapsulated_IA_state(&cli_enc_opt_state,
					   &cli_enc_opt_data,
					   packet, ia, IA_PD_OFFSET)) {
		goto exit;
	    }

	    iaid = getULong(cli_enc_opt_data.data);

	    oc = lookup_option(&dhcpv6_universe, cli_enc_opt_state,
			       D6O_IAPREFIX);
	    if (oc == NULL) {
		/* no prefix given for this IA_PD, ignore */
		option_state_dereference(&cli_enc_opt_state, MDL);
		data_string_forget(&cli_enc_opt_data, MDL);
		continue;
	    }

	    for (; oc != NULL; oc = oc->next) {
		memset(&iaprefix, 0, sizeof(iaprefix));
		if (!evaluate_option_cache(&iaprefix, packet, NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("iterate_over_ia_pd: "
				  "error evaluating IAPREFIX.");
			goto exit;
		}

		/*
		 * Now we need to figure out which host record matches
		 * this IA_PD and IAPREFIX (encapsulated option contents
		 * matching a host record by option).
		 *
		 * XXX: We don't currently track IA_PD separately, but
		 *      we will need to do this!
		 */
		host = NULL;
		if (!find_hosts_by_option(&host, packet,
					  cli_enc_opt_state, MDL)) {
			if (packet_host != NULL) {
				host = packet_host;
			} else {
				host = NULL;
			}
		}
		while (host != NULL) {
			if (host->fixed_prefix != NULL) {
				struct iaddrcidrnetlist *l;
				int plen = (int) getUChar(iaprefix.data + 8);

				for (l = host->fixed_prefix; l != NULL;
				     l = l->next) {
					if (plen != l->cidrnet.bits)
						continue;
					if (memcmp(iaprefix.data + 9,
						   l->cidrnet.lo_addr.iabuf,
						   16) == 0)
						break;
				}
				if ((l != NULL) && (iaprefix.len >= 17))
					break;
			}
			host = host->n_ipaddr;
		}

		if ((host == NULL) && (iaprefix.len >= IAPREFIX_OFFSET)) {
			/*
			 * Find existing IA_PD.
			 */
			if (ia_make_key(&key, iaid,
					(char *)client_id->data,
					client_id->len,
					MDL) != ISC_R_SUCCESS) {
				log_fatal("iterate_over_ia_pd: no memory for "
					  "key.");
			}

			existing_ia_pd = NULL;
			if (ia_hash_lookup(&existing_ia_pd, ia_pd_active,
					   (unsigned char *)key.data,
					   key.len, MDL)) {
				/*
				 * Make sure this prefix is in the IA_PD.
				 */
				for (i = 0;
				     i < existing_ia_pd->num_iasubopt;
				     i++) {
					struct iasubopt *tmp;
					u_int8_t plen;

					plen = getUChar(iaprefix.data + 8);
					tmp = existing_ia_pd->iasubopt[i];
					if ((tmp->plen == plen) &&
					    (memcmp(&tmp->addr,
						    iaprefix.data + 9,
						    16) == 0)) {
						iasubopt_reference(&prefix,
								   tmp, MDL);
						break;
					}
				}
			}

			data_string_forget(&key, MDL);
		}

		if ((host != NULL) || (prefix != NULL)) {
			ia_pd_match(client_id, &iaprefix, prefix);
		} else {
			ia_pd_nomatch(client_id, &iaprefix,
				      (u_int32_t *)cli_enc_opt_data.data,
				      packet, reply_data, &reply_ofs,
				      reply_len - reply_ofs);
		}

		if (prefix != NULL) {
			iasubopt_dereference(&prefix, MDL);
		}

		data_string_forget(&iaprefix, MDL);
	    }

	    option_state_dereference(&cli_enc_opt_state, MDL);
	    data_string_forget(&cli_enc_opt_data, MDL);
	}

	/*
	 * Return our reply to the caller.
	 * The IA_NA routine has already filled at least the header.
	 */
	reply_new.len = reply_ret->len + reply_ofs;
	if (!buffer_allocate(&reply_new.buffer, reply_new.len, MDL)) {
		log_fatal("No memory to store reply.");
	}
	reply_new.data = reply_new.buffer->data;
	memcpy(reply_new.buffer->data,
	       reply_ret->buffer->data, reply_ret->len);
	memcpy(reply_new.buffer->data + reply_ret->len,
	       reply_data, reply_ofs);
	data_string_forget(reply_ret, MDL);
	data_string_copy(reply_ret, &reply_new, MDL);
	data_string_forget(&reply_new, MDL);

exit:
	if (prefix != NULL) {
		iasubopt_dereference(&prefix, MDL);
	}
	if (iaprefix.buffer != NULL) {
		data_string_forget(&iaprefix, MDL);
	}
	if (cli_enc_opt_state != NULL) {
		option_state_dereference(&cli_enc_opt_state, MDL);
	}
	if (cli_enc_opt_data.buffer != NULL) {
		data_string_forget(&cli_enc_opt_data, MDL);
	}
	if (opt_state != NULL) {
		option_state_dereference(&opt_state, MDL);
	}
}

/*
 * Release means a client is done with the leases.
 */

static void
dhcpv6_release(struct data_string *reply, struct packet *packet) {
	struct data_string client_id;
	struct data_string server_id;

	/*
	 * Validate our input.
	 */
	if (!valid_client_resp(packet, &client_id, &server_id)) {
		return;
	}

	/* If the RELEASE arrived via unicast and unicast option isn't set,
 	 * reject it per RFC 3315, Sec 18.2.6 */
	if (packet->unicast == ISC_TRUE &&
	    is_unicast_option_defined(packet) == ISC_FALSE) {
		unicast_reject(reply, packet, &client_id, &server_id);
	} else {
		/*
		 * And operate on each IA_NA in this packet.
		 */
		iterate_over_ia_na(reply, packet, &client_id, &server_id,
				   "Release", ia_na_match_release,
				   ia_na_nomatch_release);

		/*
		 * And operate on each IA_PD in this packet.
		 */
		iterate_over_ia_pd(reply, packet, &client_id, &server_id,
				   "Release", ia_pd_match_release,
				   ia_pd_nomatch_release);
	}

	data_string_forget(&server_id, MDL);
	data_string_forget(&client_id, MDL);
}

/*
 * Information-Request is used by clients who have obtained an address
 * from other means, but want configuration information from the server.
 */

static void
dhcpv6_information_request(struct data_string *reply, struct packet *packet) {
	struct data_string client_id;
	struct data_string server_id;

	/*
	 * Validate our input.
	 */
	if (!valid_client_info_req(packet, &server_id)) {
		return;
	}

	/*
	 * Get our client ID, if there is one.
	 */
	memset(&client_id, 0, sizeof(client_id));
	if (get_client_id(packet, &client_id) != ISC_R_SUCCESS) {
		data_string_forget(&client_id, MDL);
	}

	/*
	 * Use the lease_to_client() function. This will work fine,
	 * because the valid_client_info_req() insures that we
	 * don't have any IA that would cause us to allocate
	 * resources to the client.
	 */
	lease_to_client(reply, packet, &client_id,
			server_id.data != NULL ? &server_id : NULL);

	/*
	 * Cleanup.
	 */
	if (client_id.data != NULL) {
		data_string_forget(&client_id, MDL);
	}
	data_string_forget(&server_id, MDL);
}

/*
 * The Relay-forw message is sent by relays. It typically contains a
 * single option, which encapsulates an entire packet.
 *
 * We need to build an encapsulated reply.
 */

/* XXX: this is very, very similar to do_packet6(), and should probably
	be combined in a clever way */
/* DHCPv6 server side */
static void
dhcpv6_relay_forw(struct data_string *reply_ret, struct packet *packet) {
	struct option_cache *oc;
	struct data_string enc_opt_data;
	struct packet *enc_packet;
	unsigned char msg_type;
	const struct dhcpv6_packet *msg;
	const struct dhcpv6_relay_packet *relay;
	struct data_string enc_reply;
	char link_addr[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	char peer_addr[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	struct data_string a_opt, packet_ero;
	struct option_state *opt_state;
	static char reply_data[65536];
	struct dhcpv6_relay_packet *reply;
	int reply_ofs;

	/*
	 * Initialize variables for early exit.
	 */
	opt_state = NULL;
	memset(&a_opt, 0, sizeof(a_opt));
	memset(&packet_ero, 0, sizeof(packet_ero));
	memset(&enc_reply, 0, sizeof(enc_reply));
	memset(&enc_opt_data, 0, sizeof(enc_opt_data));
	enc_packet = NULL;

	/*
	 * Get our encapsulated relay message.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_RELAY_MSG);
	if (oc == NULL) {
		inet_ntop(AF_INET6, &packet->dhcpv6_link_address,
			  link_addr, sizeof(link_addr));
		inet_ntop(AF_INET6, &packet->dhcpv6_peer_address,
			  peer_addr, sizeof(peer_addr));
		log_info("Relay-forward from %s with link address=%s and "
			 "peer address=%s missing Relay Message option.",
			  piaddr(packet->client_addr), link_addr, peer_addr);
		goto exit;
	}

	if (!evaluate_option_cache(&enc_opt_data, NULL, NULL, NULL,
				   NULL, NULL, &global_scope, oc, MDL)) {
		/* should be dhcpv6_relay_forw */
		log_error("dhcpv6_forw_relay: error evaluating "
			  "relayed message.");
		goto exit;
	}

	if (!packet6_len_okay((char *)enc_opt_data.data, enc_opt_data.len)) {
		/* should be dhcpv6_relay_forw */
		log_error("dhcpv6_forw_relay: encapsulated packet too short.");
		goto exit;
	}

	/*
	 * Build a packet structure from this encapsulated packet.
	 */
	enc_packet = NULL;
	if (!packet_allocate(&enc_packet, MDL)) {
		/* should be dhcpv6_relay_forw */
		log_error("dhcpv6_forw_relay: "
			  "no memory for encapsulated packet.");
		goto exit;
	}

	if (!option_state_allocate(&enc_packet->options, MDL)) {
		/* should be dhcpv6_relay_forw */
		log_error("dhcpv6_forw_relay: "
			  "no memory for encapsulated packet's options.");
		goto exit;
	}

	enc_packet->client_port = packet->client_port;
	enc_packet->client_addr = packet->client_addr;
	interface_reference(&enc_packet->interface, packet->interface, MDL);
	enc_packet->dhcpv6_container_packet = packet;

	msg_type = enc_opt_data.data[0];
	if ((msg_type == DHCPV6_RELAY_FORW) ||
	    (msg_type == DHCPV6_RELAY_REPL)) {
		int relaylen = (int)(offsetof(struct dhcpv6_relay_packet, options));
		relay = (struct dhcpv6_relay_packet *)enc_opt_data.data;
		enc_packet->dhcpv6_msg_type = relay->msg_type;

		/* relay-specific data */
		enc_packet->dhcpv6_hop_count = relay->hop_count;
		memcpy(&enc_packet->dhcpv6_link_address,
		       relay->link_address, sizeof(relay->link_address));
		memcpy(&enc_packet->dhcpv6_peer_address,
		       relay->peer_address, sizeof(relay->peer_address));

		if (!parse_option_buffer(enc_packet->options,
					 relay->options,
					 enc_opt_data.len - relaylen,
					 &dhcpv6_universe)) {
			/* no logging here, as parse_option_buffer() logs all
			   cases where it fails */
			goto exit;
		}
	} else if ((msg_type == DHCPV6_DHCPV4_QUERY) ||
		   (msg_type == DHCPV6_DHCPV4_RESPONSE)) {
#ifdef DHCP4o6
		if (!dhcpv4_over_dhcpv6 ||
		    (msg_type == DHCPV6_DHCPV4_RESPONSE)) {
			log_error("dhcpv6_relay_forw: "
				  "unsupported %s message type.",
				  dhcpv6_type_names[msg_type]);
			goto exit;
		}
		forw_dhcpv4_query(packet);
		goto exit;
#else /* DHCP4o6 */
		log_error("dhcpv6_relay_forw: unsupported %s message type.",
			  dhcpv6_type_names[msg_type]);
		goto exit;
#endif /* DHCP4o6 */
	} else {
		int msglen = (int)(offsetof(struct dhcpv6_packet, options));
		msg = (struct dhcpv6_packet *)enc_opt_data.data;
		enc_packet->dhcpv6_msg_type = msg->msg_type;

		/* message-specific data */
		memcpy(enc_packet->dhcpv6_transaction_id,
		       msg->transaction_id,
		       sizeof(enc_packet->dhcpv6_transaction_id));

		if (!parse_option_buffer(enc_packet->options,
					 msg->options,
					 enc_opt_data.len - msglen,
					 &dhcpv6_universe)) {
			/* no logging here, as parse_option_buffer() logs all
			   cases where it fails */
			goto exit;
		}
	}

	/*
	 * This is recursive. It is possible to exceed maximum packet size.
	 * XXX: This will cause the packet send to fail.
	 */
	build_dhcpv6_reply(&enc_reply, enc_packet);

	/*
	 * If we got no encapsulated data, then it is discarded, and
	 * our reply-forw is also discarded.
	 */
	if (enc_reply.data == NULL) {
		goto exit;
	}

	/*
	 * Now we can use the reply_data buffer.
	 * Packet header stuff all comes from the forward message.
	 */
	reply = (struct dhcpv6_relay_packet *)reply_data;
	reply->msg_type = DHCPV6_RELAY_REPL;
	reply->hop_count = packet->dhcpv6_hop_count;
	memcpy(reply->link_address, &packet->dhcpv6_link_address,
	       sizeof(reply->link_address));
	memcpy(reply->peer_address, &packet->dhcpv6_peer_address,
	       sizeof(reply->peer_address));
	reply_ofs = (int)(offsetof(struct dhcpv6_relay_packet, options));

	/*
	 * Get the reply option state.
	 */
	opt_state = NULL;
	if (!option_state_allocate(&opt_state, MDL)) {
		log_error("dhcpv6_relay_forw: no memory for option state.");
		goto exit;
	}

	/*
	 * Append the interface-id if present.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options,
			   D6O_INTERFACE_ID);
	if (oc != NULL) {
		if (!evaluate_option_cache(&a_opt, packet,
					   NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("dhcpv6_relay_forw: error evaluating "
				  "Interface ID.");
			goto exit;
		}
		if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
					(unsigned char *)a_opt.data,
					a_opt.len,
					D6O_INTERFACE_ID, 0)) {
			log_error("dhcpv6_relay_forw: error saving "
				  "Interface ID.");
			goto exit;
		}
		data_string_forget(&a_opt, MDL);
	}

#if defined(RELAY_PORT)
	/*
	 * Append the relay_source_port option if present.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options,
			   D6O_RELAY_SOURCE_PORT);
	if (oc != NULL) {
		if (!evaluate_option_cache(&a_opt, packet,
					   NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("dhcpv6_relay_forw: error evaluating "
				  "Relay Source Port.");
			goto exit;
		}
		if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
					(unsigned char *)a_opt.data,
					a_opt.len,
					D6O_RELAY_SOURCE_PORT, 0)) {
			log_error("dhcpv6_relay_forw: error saving "
				  "Relay Source Port.");
			goto exit;
		}
		data_string_forget(&a_opt, MDL);

		packet->relay_source_port = ISC_TRUE;
	}
#endif

	/*
	 * Append our encapsulated stuff for caller.
	 */
	if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
				(unsigned char *)enc_reply.data,
				enc_reply.len,
				D6O_RELAY_MSG, 0)) {
		log_error("dhcpv6_relay_forw: error saving Relay MSG.");
		goto exit;
	}

	/*
	 * Get the ERO if any.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_ERO);
	if (oc != NULL) {
		unsigned req;
		int i;

		if (!evaluate_option_cache(&packet_ero, packet,
					   NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL) ||
			(packet_ero.len & 1)) {
			log_error("dhcpv6_relay_forw: error evaluating ERO.");
			goto exit;
		}

		/* Decode and apply the ERO. */
		for (i = 0; i < packet_ero.len; i += 2) {
			req = getUShort(packet_ero.data + i);
			/* Already in the reply? */
			oc = lookup_option(&dhcpv6_universe, opt_state, req);
			if (oc != NULL)
				continue;
			/* Get it from the packet if present. */
			oc = lookup_option(&dhcpv6_universe,
					   packet->options,
					   req);
			if (oc == NULL)
				continue;
			if (!evaluate_option_cache(&a_opt, packet,
						   NULL, NULL,
						   packet->options, NULL,
						   &global_scope, oc, MDL)) {
				log_error("dhcpv6_relay_forw: error "
					  "evaluating option %u.", req);
				goto exit;
			}
			if (!save_option_buffer(&dhcpv6_universe,
						opt_state,
						NULL,
						(unsigned char *)a_opt.data,
						a_opt.len,
						req,
						0)) {
				log_error("dhcpv6_relay_forw: error saving "
					  "option %u.", req);
				goto exit;
			}
			data_string_forget(&a_opt, MDL);
		}
	}

	reply_ofs += store_options6(reply_data + reply_ofs,
				    sizeof(reply_data) - reply_ofs,
				    opt_state, packet,
				    required_opts_agent, &packet_ero);

	/*
	 * Return our reply to the caller.
	 */
	reply_ret->len = reply_ofs;
	reply_ret->buffer = NULL;
	if (!buffer_allocate(&reply_ret->buffer, reply_ret->len, MDL)) {
		log_fatal("No memory to store reply.");
	}
	reply_ret->data = reply_ret->buffer->data;
	memcpy(reply_ret->buffer->data, reply_data, reply_ofs);

exit:
	if (opt_state != NULL)
		option_state_dereference(&opt_state, MDL);
	if (a_opt.data != NULL) {
		data_string_forget(&a_opt, MDL);
	}
	if (packet_ero.data != NULL) {
		data_string_forget(&packet_ero, MDL);
	}
	if (enc_reply.data != NULL) {
		data_string_forget(&enc_reply, MDL);
	}
	if (enc_opt_data.data != NULL) {
		data_string_forget(&enc_opt_data, MDL);
	}
	if (enc_packet != NULL) {
		packet_dereference(&enc_packet, MDL);
	}
}

#ifdef DHCP4o6
/* \brief Internal processing of a relayed DHCPv4-query
 *  (DHCPv4 server side)
 *
 * Code copied from \ref dhcpv6_relay_forw() which itself is
 * from \ref do_packet6().
 *
 * \param reply_ret pointer to the response
 * \param packet the query
 */
static void
dhcp4o6_relay_forw(struct data_string *reply_ret, struct packet *packet) {
	struct option_cache *oc;
	struct data_string enc_opt_data;
	struct packet *enc_packet;
	unsigned char msg_type;
	const struct dhcpv6_relay_packet *relay;
	const struct dhcpv4_over_dhcpv6_packet *msg;
	struct data_string enc_reply;
	char link_addr[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	char peer_addr[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	struct data_string a_opt, packet_ero;
	struct option_state *opt_state;
	static char reply_data[65536];
	struct dhcpv6_relay_packet *reply;
	int reply_ofs;

	/*
	 * Initialize variables for early exit.
	 */
	opt_state = NULL;
	memset(&a_opt, 0, sizeof(a_opt));
	memset(&packet_ero, 0, sizeof(packet_ero));
	memset(&enc_reply, 0, sizeof(enc_reply));
	memset(&enc_opt_data, 0, sizeof(enc_opt_data));
	enc_packet = NULL;

	/*
	 * Get our encapsulated relay message.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_RELAY_MSG);
	if (oc == NULL) {
		inet_ntop(AF_INET6, &packet->dhcpv6_link_address,
			  link_addr, sizeof(link_addr));
		inet_ntop(AF_INET6, &packet->dhcpv6_peer_address,
			  peer_addr, sizeof(peer_addr));
		log_info("Relay-forward from %s with link address=%s and "
			 "peer address=%s missing Relay Message option.",
			  piaddr(packet->client_addr), link_addr, peer_addr);
		goto exit;
	}

	if (!evaluate_option_cache(&enc_opt_data, NULL, NULL, NULL,
				   NULL, NULL, &global_scope, oc, MDL)) {
		log_error("dhcp4o6_relay_forw: error evaluating "
			  "relayed message.");
		goto exit;
	}

	if (!packet6_len_okay((char *)enc_opt_data.data, enc_opt_data.len)) {
		log_error("dhcp4o6_relay_forw: "
			  "encapsulated packet too short.");
		goto exit;
	}

	/*
	 * Build a packet structure from this encapsulated packet.
	 */
	if (!packet_allocate(&enc_packet, MDL)) {
		log_error("dhcp4o6_relay_forw: "
			  "no memory for encapsulated packet.");
		goto exit;
	}

	if (!option_state_allocate(&enc_packet->options, MDL)) {
		log_error("dhcp4o6_relay_forw: "
			  "no memory for encapsulated packet's options.");
		goto exit;
	}

	enc_packet->client_port = packet->client_port;
	enc_packet->client_addr = packet->client_addr;
	interface_reference(&enc_packet->interface, packet->interface, MDL);
	enc_packet->dhcpv6_container_packet = packet;

	msg_type = enc_opt_data.data[0];
	if ((msg_type == DHCPV6_RELAY_FORW) ||
	    (msg_type == DHCPV6_RELAY_REPL)) {
		int relaylen = (int)(offsetof(struct dhcpv6_relay_packet, options));
		relay = (struct dhcpv6_relay_packet *)enc_opt_data.data;
		enc_packet->dhcpv6_msg_type = relay->msg_type;

		/* relay-specific data */
		enc_packet->dhcpv6_hop_count = relay->hop_count;
		memcpy(&enc_packet->dhcpv6_link_address,
		       relay->link_address, sizeof(relay->link_address));
		memcpy(&enc_packet->dhcpv6_peer_address,
		       relay->peer_address, sizeof(relay->peer_address));

		if (!parse_option_buffer(enc_packet->options,
					 relay->options,
					 enc_opt_data.len - relaylen,
					 &dhcpv6_universe)) {
			/* no logging here, as parse_option_buffer() logs all
			   cases where it fails */
			goto exit;
		}
	} else if ((msg_type == DHCPV6_DHCPV4_QUERY) ||
		   (msg_type == DHCPV6_DHCPV4_RESPONSE)) {
		int msglen =
		    (int)(offsetof(struct dhcpv4_over_dhcpv6_packet, options));
		msg = (struct dhcpv4_over_dhcpv6_packet *)enc_opt_data.data;
		enc_packet->dhcpv6_msg_type = msg->msg_type;

		/* message-specific data */
		memcpy(enc_packet->dhcp4o6_flags,
		       msg->flags,
		       sizeof(enc_packet->dhcp4o6_flags));

		if (!parse_option_buffer(enc_packet->options,
					 msg->options,
					 enc_opt_data.len - msglen,
					 &dhcpv6_universe)) {
			/* no logging here, as parse_option_buffer() logs all
			   cases where it fails */
			goto exit;
		}
	} else {
		log_error("dhcp4o6_relay_forw: unexpected message of type %d.",
			  (int)msg_type);
		goto exit;
	}

	/*
	 * This is recursive. It is possible to exceed maximum packet size.
	 * XXX: This will cause the packet send to fail.
	 */
	build_dhcpv6_reply(&enc_reply, enc_packet);

	/*
	 * If we got no encapsulated data, then it is discarded, and
	 * our reply-forw is also discarded.
	 */
	if (enc_reply.data == NULL) {
		goto exit;
	}

	/*
	 * Now we can use the reply_data buffer.
	 * Packet header stuff all comes from the forward message.
	 */
	reply = (struct dhcpv6_relay_packet *)reply_data;
	reply->msg_type = DHCPV6_RELAY_REPL;
	reply->hop_count = packet->dhcpv6_hop_count;
	memcpy(reply->link_address, &packet->dhcpv6_link_address,
	       sizeof(reply->link_address));
	memcpy(reply->peer_address, &packet->dhcpv6_peer_address,
	       sizeof(reply->peer_address));
	reply_ofs = (int)(offsetof(struct dhcpv6_relay_packet, options));

	/*
	 * Get the reply option state.
	 */
	if (!option_state_allocate(&opt_state, MDL)) {
		log_error("dhcp4o6_relay_forw: no memory for option state.");
		goto exit;
	}

	/*
	 * Append the interface-id if present.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options,
			   D6O_INTERFACE_ID);
	if (oc != NULL) {
		if (!evaluate_option_cache(&a_opt, packet,
					   NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("dhcp4o6_relay_forw: error evaluating "
				  "Interface ID.");
			goto exit;
		}
		if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
					(unsigned char *)a_opt.data,
					a_opt.len,
					D6O_INTERFACE_ID, 0)) {
			log_error("dhcp4o6_relay_forw: error saving "
				  "Interface ID.");
			goto exit;
		}
		data_string_forget(&a_opt, MDL);
	}

#if defined(RELAY_PORT)
	/*
	 * Append the relay_source_port option if present.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options,
			   D6O_RELAY_SOURCE_PORT);
	if (oc != NULL) {
		if (!evaluate_option_cache(&a_opt, packet,
					   NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL)) {
			log_error("dhcpv4o6_relay_forw: error evaluating "
				  "Relay Source Port.");
			goto exit;
		}
		if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
					(unsigned char *)a_opt.data,
					a_opt.len,
					D6O_RELAY_SOURCE_PORT, 0)) {
			log_error("dhcpv4o6_relay_forw: error saving "
				  "Relay Source Port.");
			goto exit;
		}
		data_string_forget(&a_opt, MDL);

		packet->relay_source_port = ISC_TRUE;
	}
#endif

	/*
	 * Append our encapsulated stuff for caller.
	 */
	if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
				(unsigned char *)enc_reply.data,
				enc_reply.len,
				D6O_RELAY_MSG, 0)) {
		log_error("dhcp4o6_relay_forw: error saving Relay MSG.");
		goto exit;
	}

	/*
	 * Get the ERO if any.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_ERO);
	if (oc != NULL) {
		unsigned req;
		int i;

		if (!evaluate_option_cache(&packet_ero, packet,
					   NULL, NULL,
					   packet->options, NULL,
					   &global_scope, oc, MDL) ||
			(packet_ero.len & 1)) {
			log_error("dhcp4o6_relay_forw: error evaluating ERO.");
			goto exit;
		}

		/* Decode and apply the ERO. */
		for (i = 0; i < packet_ero.len; i += 2) {
			req = getUShort(packet_ero.data + i);
			/* Already in the reply? */
			oc = lookup_option(&dhcpv6_universe, opt_state, req);
			if (oc != NULL)
				continue;
			/* Get it from the packet if present. */
			oc = lookup_option(&dhcpv6_universe,
					   packet->options,
					   req);
			if (oc == NULL)
				continue;
			if (!evaluate_option_cache(&a_opt, packet,
						   NULL, NULL,
						   packet->options, NULL,
						   &global_scope, oc, MDL)) {
				log_error("dhcp4o6_relay_forw: error "
					  "evaluating option %u.", req);
				goto exit;
			}
			if (!save_option_buffer(&dhcpv6_universe,
						opt_state,
						NULL,
						(unsigned char *)a_opt.data,
						a_opt.len,
						req,
						0)) {
				log_error("dhcp4o6_relay_forw: error saving "
					  "option %u.", req);
				goto exit;
			}
			data_string_forget(&a_opt, MDL);
		}
	}

	reply_ofs += store_options6(reply_data + reply_ofs,
				    sizeof(reply_data) - reply_ofs,
				    opt_state, packet,
				    required_opts_agent, &packet_ero);

	/*
	 * Return our reply to the caller.
	 */
	reply_ret->len = reply_ofs;
	reply_ret->buffer = NULL;
	if (!buffer_allocate(&reply_ret->buffer, reply_ret->len, MDL)) {
		log_fatal("No memory to store reply.");
	}
	reply_ret->data = reply_ret->buffer->data;
	memcpy(reply_ret->buffer->data, reply_data, reply_ofs);

exit:
	if (opt_state != NULL)
		option_state_dereference(&opt_state, MDL);
	if (a_opt.data != NULL) {
		data_string_forget(&a_opt, MDL);
	}
	if (packet_ero.data != NULL) {
		data_string_forget(&packet_ero, MDL);
	}
	if (enc_reply.data != NULL) {
		data_string_forget(&enc_reply, MDL);
	}
	if (enc_opt_data.data != NULL) {
		data_string_forget(&enc_opt_data, MDL);
	}
	if (enc_packet != NULL) {
		packet_dereference(&enc_packet, MDL);
	}
}

/*
 * \brief Internal processing of a DHCPv4-query
 *  (DHCPv4 server function)
 *
 * Code copied from \ref do_packet().
 *
 * \param reply_ret pointer to the response
 * \param packet the query
 */
static void
dhcp4o6_dhcpv4_query(struct data_string *reply_ret, struct packet *packet) {
	struct option_cache *oc;
	struct data_string enc_opt_data;
	struct packet *enc_packet;
	struct data_string enc_response;
	struct option_state *opt_state;
	static char response_data[65536];
	struct dhcpv4_over_dhcpv6_packet *response;
	int response_ofs;

	/*
	 * Initialize variables for early exit.
	 */
	opt_state = NULL;
	memset(&enc_response, 0, sizeof(enc_response));
	memset(&enc_opt_data, 0, sizeof(enc_opt_data));
	enc_packet = NULL;

	/*
	 * Get our encapsulated relay message.
	 */
	oc = lookup_option(&dhcpv6_universe, packet->options, D6O_DHCPV4_MSG);
	if (oc == NULL) {
		log_info("DHCPv4-query from %s missing DHCPv4 Message option.",
			 piaddr(packet->client_addr));
		goto exit;
	}

	if (!evaluate_option_cache(&enc_opt_data, NULL, NULL, NULL,
				   NULL, NULL, &global_scope, oc, MDL)) {
		log_error("dhcp4o6_dhcpv4_query: error evaluating "
			  "DHCPv4 message.");
		goto exit;
	}

	if (enc_opt_data.len < DHCP_FIXED_NON_UDP) {
		log_error("dhcp4o6_dhcpv4_query: DHCPv4 packet too short.");
		goto exit;
	}

	/*
	 * Build a packet structure from this encapsulated packet.
         */
	if (!packet_allocate(&enc_packet, MDL)) {
		log_error("dhcp4o6_dhcpv4_query: "
			  "no memory for encapsulated packet.");
		goto exit;
	}

	enc_packet->raw = (struct dhcp_packet *)enc_opt_data.data;
	enc_packet->packet_length = enc_opt_data.len;
	enc_packet->dhcp4o6_response = &enc_response;
	enc_packet->client_port = packet->client_port;
	enc_packet->client_addr = packet->client_addr;
	interface_reference(&enc_packet->interface, packet->interface, MDL);
	enc_packet->dhcpv6_container_packet = packet;
	if (packet->dhcp4o6_flags[0] & DHCP4O6_QUERY_UNICAST)
		enc_packet->unicast = 1;

	if (enc_packet->raw->hlen > sizeof(enc_packet->raw->chaddr)) {
		log_info("dhcp4o6_dhcpv4_query: "
			 "discarding packet with bogus hlen.");
		goto exit;
	}

	/* Allocate packet->options now so it is non-null for all packets */
	if (!option_state_allocate (&enc_packet->options, MDL)) {
		log_error("dhcp4o6_dhcpv4_query: no memory for options.");
		goto exit;
	}

	/* If there's an option buffer, try to parse it. */
	if (enc_packet->packet_length >= DHCP_FIXED_NON_UDP + 4) {
		struct option_cache *op;
		if (!parse_options(enc_packet)) {
			if (enc_packet->options)
				option_state_dereference
					(&enc_packet->options, MDL);
			packet_dereference (&enc_packet, MDL);
			goto exit;
		}

		if (enc_packet->options_valid &&
		    (op = lookup_option(&dhcp_universe,
					enc_packet->options,
					DHO_DHCP_MESSAGE_TYPE))) {
			struct data_string dp;
			memset(&dp, 0, sizeof dp);
			evaluate_option_cache(&dp, enc_packet, NULL, NULL,
					      enc_packet->options, NULL,
					      NULL, op, MDL);
			if (dp.len > 0)
				enc_packet->packet_type = dp.data[0];
			else
				enc_packet->packet_type = 0;
			data_string_forget(&dp, MDL);
		}
	}

	if (validate_packet(enc_packet) != 0) {
		if (enc_packet->packet_type)
			dhcp(enc_packet);
		else
			bootp(enc_packet);
	}

	/* If the caller kept the packet, they'll have upped the refcnt. */
	packet_dereference(&enc_packet, MDL);

	/*
	 * If we got no response data, then it is discarded, and
	 * our DHCPv4-response is also discarded.
	 */
	if (enc_response.data == NULL) {
		goto exit;
	}

	/*
	 * Now we can use the response_data buffer.
	 */
	response = (struct dhcpv4_over_dhcpv6_packet *)response_data;
	response->msg_type = DHCPV6_DHCPV4_RESPONSE;
	response->flags[0] = response->flags[1] = response->flags[2] = 0;
	response_ofs =
		(int)(offsetof(struct dhcpv4_over_dhcpv6_packet, options));

	/*
	 * Get the response option state.
	 */
	if (!option_state_allocate(&opt_state, MDL)) {
		log_error("dhcp4o6_dhcpv4_query: no memory for option state.");
		goto exit;
	}

	/*
	 * Append our encapsulated stuff for caller.
	 */
	if (!save_option_buffer(&dhcpv6_universe, opt_state, NULL,
				(unsigned char *)enc_response.data,
				enc_response.len,
				D6O_DHCPV4_MSG, 0)) {
		log_error("dhcp4o6_dhcpv4_query: error saving DHCPv4 MSG.");
		goto exit;
	}

	response_ofs += store_options6(response_data + response_ofs,
				       sizeof(response_data) - response_ofs,
				       opt_state, packet,
				       required_opts_4o6, NULL);

	/*
         * Return our response to the caller.
	 */
	reply_ret->len = response_ofs;
	reply_ret->buffer = NULL;
	if (!buffer_allocate(&reply_ret->buffer, reply_ret->len, MDL)) {
		log_fatal("dhcp4o6_dhcpv4_query: no memory to store reply.");
	}
	reply_ret->data = reply_ret->buffer->data;
	memcpy(reply_ret->buffer->data, response_data, response_ofs);

exit:
	if (opt_state != NULL)
		option_state_dereference(&opt_state, MDL);
	if (enc_response.data != NULL) {
		data_string_forget(&enc_response, MDL);
	}
	if (enc_opt_data.data != NULL) {
		data_string_forget(&enc_opt_data, MDL);
	}
	if (enc_packet != NULL) {
		packet_dereference(&enc_packet, MDL);
	}
}

/*
 * \brief Forward a DHCPv4-query message to the DHCPv4 side
 *  (DHCPv6 server function)
 *
 * Format: interface:16 + address:16 + udp:4 + DHCPv6 DHCPv4-query message
 *
 * \brief packet the DHCPv6 DHCPv4-query message
 */
static void forw_dhcpv4_query(struct packet *packet) {
	struct data_string ds;
	struct udp_data4o6 udp_data;
	unsigned len;
	int cc;

	/* Get the initial message. */
	while (packet->dhcpv6_container_packet != NULL)
		packet = packet->dhcpv6_container_packet;

	/* Check the initial message. */
	if ((packet->raw == NULL) ||
	    (packet->client_addr.len != 16) ||
	    (packet->interface == NULL)) {
		log_error("forw_dhcpv4_query: can't find initial message.");
		return;
	}

	/* Get a buffer. */
	len = packet->packet_length + 36;
	memset(&ds, 0, sizeof(ds));
	if (!buffer_allocate(&ds.buffer, len, MDL)) {
		log_error("forw_dhcpv4_query: "
			  "no memory for encapsulating packet.");
		return;
	}
	ds.data = ds.buffer->data;
	ds.len = len;

	/* Fill the buffer. */
	strncpy((char *)ds.buffer->data, packet->interface->name, 16);
	memcpy(ds.buffer->data + 16,
	       packet->client_addr.iabuf, 16);
	memset(&udp_data, 0, sizeof(udp_data));
	udp_data.src_port = packet->client_port;
	memcpy(ds.buffer->data + 32, &udp_data, 4);
	memcpy(ds.buffer->data + 36,
	       (unsigned char *)packet->raw,
	       packet->packet_length);

	/* Forward to the DHCPv4 server. */
	cc = send(dhcp4o6_fd, ds.data, ds.len, 0);
	if (cc < 0)
		log_error("forw_dhcpv4_query: send(): %m");
	data_string_forget(&ds, MDL);
}
#endif

static void
dhcpv6_discard(struct packet *packet) {
	/* INSIST(packet->msg_type > 0); */
	/* INSIST(packet->msg_type < dhcpv6_type_name_max); */

	log_debug("Discarding %s from %s; message type not handled by server",
		  dhcpv6_type_names[packet->dhcpv6_msg_type],
		  piaddr(packet->client_addr));
}

static void
build_dhcpv6_reply(struct data_string *reply, struct packet *packet) {
	memset(reply, 0, sizeof(*reply));

	/* I would like to classify the client once here, but
	 * as I don't want to classify all of the incoming packets
	 * I need to do it before handling specific types.
	 * We don't need to classify if we are tossing the packet
	 * or if it is a relay - the classification step will get
	 * done when we process the inner client packet.
	 */

	switch (packet->dhcpv6_msg_type) {
		case DHCPV6_SOLICIT:
			classify_client(packet);
			dhcpv6_solicit(reply, packet);
			break;
		case DHCPV6_ADVERTISE:
			dhcpv6_discard(packet);
			break;
		case DHCPV6_REQUEST:
			classify_client(packet);
			dhcpv6_request(reply, packet);
			break;
		case DHCPV6_CONFIRM:
			classify_client(packet);
			dhcpv6_confirm(reply, packet);
			break;
		case DHCPV6_RENEW:
			classify_client(packet);
			dhcpv6_renew(reply, packet);
			break;
		case DHCPV6_REBIND:
			classify_client(packet);
			dhcpv6_rebind(reply, packet);
			break;
		case DHCPV6_REPLY:
			dhcpv6_discard(packet);
			break;
		case DHCPV6_RELEASE:
			classify_client(packet);
			dhcpv6_release(reply, packet);
			break;
		case DHCPV6_DECLINE:
			classify_client(packet);
			dhcpv6_decline(reply, packet);
			break;
		case DHCPV6_RECONFIGURE:
			dhcpv6_discard(packet);
			break;
		case DHCPV6_INFORMATION_REQUEST:
			classify_client(packet);
			dhcpv6_information_request(reply, packet);
			break;
		case DHCPV6_RELAY_FORW:
#ifdef DHCP4o6
			if (dhcpv4_over_dhcpv6 && (local_family == AF_INET))
				dhcp4o6_relay_forw(reply, packet);
			else
#endif /* DHCP4o6 */
			dhcpv6_relay_forw(reply, packet);
			break;
		case DHCPV6_RELAY_REPL:
			dhcpv6_discard(packet);
			break;
		case DHCPV6_LEASEQUERY:
			classify_client(packet);
			dhcpv6_leasequery(reply, packet);
			break;
		case DHCPV6_LEASEQUERY_REPLY:
			dhcpv6_discard(packet);
			break;
		case DHCPV6_DHCPV4_QUERY:
#ifdef DHCP4o6
			if (dhcpv4_over_dhcpv6) {
				if (local_family == AF_INET6) {
					forw_dhcpv4_query(packet);
				} else {
					dhcp4o6_dhcpv4_query(reply, packet);
				}
			} else
#endif /* DHCP4o6 */
			dhcpv6_discard(packet);
			break;
		case DHCPV6_DHCPV4_RESPONSE:
			dhcpv6_discard(packet);
			break;
		default:
			/* XXX: would be nice if we had "notice" level,
				as syslog, for this */
			log_info("Discarding unknown DHCPv6 message type %d "
				 "from %s", packet->dhcpv6_msg_type,
				 piaddr(packet->client_addr));
	}
}

static void
log_packet_in(const struct packet *packet) {
	struct data_string s;
	u_int32_t tid;
	char tmp_addr[INET6_ADDRSTRLEN];
	const void *addr;

	memset(&s, 0, sizeof(s));

	if (packet->dhcpv6_msg_type < dhcpv6_type_name_max) {
		data_string_sprintfa(&s, "%s message from %s port %d",
				     dhcpv6_type_names[packet->dhcpv6_msg_type],
				     piaddr(packet->client_addr),
				     ntohs(packet->client_port));
	} else {
		data_string_sprintfa(&s,
				     "Unknown message type %d from %s port %d",
				     packet->dhcpv6_msg_type,
				     piaddr(packet->client_addr),
				     ntohs(packet->client_port));
	}
	if ((packet->dhcpv6_msg_type == DHCPV6_RELAY_FORW) ||
	    (packet->dhcpv6_msg_type == DHCPV6_RELAY_REPL)) {
	    	addr = &packet->dhcpv6_link_address;
	    	data_string_sprintfa(&s, ", link address %s",
				     inet_ntop(AF_INET6, addr,
					       tmp_addr, sizeof(tmp_addr)));
	    	addr = &packet->dhcpv6_peer_address;
	    	data_string_sprintfa(&s, ", peer address %s",
				     inet_ntop(AF_INET6, addr,
					       tmp_addr, sizeof(tmp_addr)));
	} else if ((packet->dhcpv6_msg_type != DHCPV6_DHCPV4_QUERY) &&
		   (packet->dhcpv6_msg_type != DHCPV6_DHCPV4_RESPONSE)) {
		tid = 0;
		memcpy(((char *)&tid)+1, packet->dhcpv6_transaction_id, 3);
		data_string_sprintfa(&s, ", transaction ID 0x%06X", tid);

/*
		oc = lookup_option(&dhcpv6_universe, packet->options,
				   D6O_CLIENTID);
		if (oc != NULL) {
			memset(&tmp_ds, 0, sizeof(tmp_ds_));
			if (!evaluate_option_cache(&tmp_ds, packet, NULL, NULL,
						   packet->options, NULL,
						   &global_scope, oc, MDL)) {
				log_error("Error evaluating Client Identifier");
			} else {
				data_strint_sprintf(&s, ", client ID %s",

				data_string_forget(&tmp_ds, MDL);
			}
		}
*/

	}
	log_info("%s", s.data);

	data_string_forget(&s, MDL);
}

void
dhcpv6(struct packet *packet) {
	struct data_string reply;
	struct sockaddr_in6 to_addr;
	int send_ret;

	/*
	 * Log a message that we received this packet.
	 */
	log_packet_in(packet);

	/*
	 * Build our reply packet.
	 */
	build_dhcpv6_reply(&reply, packet);

	if (reply.data != NULL) {
		/*
		 * Send our reply, if we have one.
		 */
		memset(&to_addr, 0, sizeof(to_addr));
		to_addr.sin6_family = AF_INET6;
		if ((packet->dhcpv6_msg_type == DHCPV6_RELAY_FORW) ||
		    (packet->dhcpv6_msg_type == DHCPV6_RELAY_REPL)) {
			to_addr.sin6_port = local_port;
		} else {
			to_addr.sin6_port = remote_port;
		}

#if defined (REPLY_TO_SOURCE_PORT)
		/*
		 * This appears to have been included for testing so we would
		 * not need a root client, but was accidently left in the
		 * final code.  We continue to include it in case
		 * some users have come to rely upon it, but leave
		 * it off by default as it's a bad idea.
		 */
		to_addr.sin6_port = packet->client_port;
#endif

#if defined(RELAY_PORT)
		/*
		 * Check relay source port.
		 */
		if (packet->relay_source_port) {
			to_addr.sin6_port = packet->client_port;
		}
#endif

		memcpy(&to_addr.sin6_addr, packet->client_addr.iabuf,
		       sizeof(to_addr.sin6_addr));

		log_info("Sending %s to %s port %d",
			 dhcpv6_type_names[reply.data[0]],
			 piaddr(packet->client_addr),
			 ntohs(to_addr.sin6_port));

		send_ret = send_packet6(packet->interface,
					reply.data, reply.len, &to_addr);
		if (send_ret != reply.len) {
			log_error("dhcpv6: send_packet6() sent %d of %d bytes",
				  send_ret, reply.len);
		}
		data_string_forget(&reply, MDL);
	}
}

#ifdef DHCP4o6
/*
 * \brief Receive a DHCPv4-query message from the DHCPv6 side
 *  (DHCPv4 server function)
 *
 * Receive a message with a DHCPv4-query inside from the DHCPv6 server.
 * (code copied from \ref do_packet6() \ref and dhcpv6())
 *
 * Format: interface:16 + address:16 + udp:4 + DHCPv6 DHCPv4-query message
 *
 * \param raw the DHCPv6 DHCPv4-query message raw content
 */
static void recv_dhcpv4_query(struct data_string *raw) {
	struct interface_info *ip;
	char name[16 + 1];
	struct iaddr iaddr;
	struct packet *packet;
	unsigned char msg_type;
	const struct dhcpv6_relay_packet *relay;
	const struct dhcpv4_over_dhcpv6_packet *msg;
	struct data_string reply;
	struct data_string ds;
	struct udp_data4o6 udp_data;
	unsigned len;
	int cc;

	memset(name, 0, sizeof(name));
	memcpy(name, raw->data, 16);
	for (ip = interfaces; ip != NULL; ip = ip->next) {
		if (!strcmp(name, ip->name))
			break;
	}
	if (ip == NULL) {
		log_error("recv_dhcpv4_query: can't find interface %s.",
			  name);
		return;
	}

	iaddr.len = 16;
	memcpy(iaddr.iabuf, raw->data + 16, 16);

	memset(&udp_data, 0, sizeof(udp_data));
	memcpy(&udp_data, raw->data + 32, 4);

	/*
	 * From do_packet6().
	 */

	if (!packet6_len_okay((char *)raw->data + 36, raw->len - 36)) {
		log_error("recv_dhcpv4_query: "
			 "short packet from %s, len %d, dropped",
			 piaddr(iaddr), raw->len - 36);
		return;
	}

	/*
	 * Build a packet structure.
	 */
	packet = NULL;
	if (!packet_allocate(&packet, MDL)) {
		log_error("recv_dhcpv4_query: no memory for packet.");
		return;
	}

	if (!option_state_allocate(&packet->options, MDL)) {
		log_error("recv_dhcpv4_query: no memory for options.");
		packet_dereference(&packet, MDL);
		return;
	}

	packet->raw = (struct dhcp_packet *)(raw->data + 36);
	packet->packet_length = raw->len - 36;
	packet->client_port = udp_data.src_port;
	packet->client_addr = iaddr;
	interface_reference(&packet->interface, ip, MDL);

	msg_type = raw->data[36];
	if ((msg_type == DHCPV6_RELAY_FORW) ||
	    (msg_type == DHCPV6_RELAY_REPL)) {
		int relaylen =
		    (int)(offsetof(struct dhcpv6_relay_packet, options));
		relay = (const struct dhcpv6_relay_packet *)(raw->data + 36);
		packet->dhcpv6_msg_type = relay->msg_type;

		/* relay-specific data */
		packet->dhcpv6_hop_count = relay->hop_count;
		memcpy(&packet->dhcpv6_link_address,
		       relay->link_address, sizeof(relay->link_address));
		memcpy(&packet->dhcpv6_peer_address,
		       relay->peer_address, sizeof(relay->peer_address));

		if (!parse_option_buffer(packet->options,
					 relay->options,
					 raw->len - 36 - relaylen,
					 &dhcpv6_universe)) {
			/* no logging here, as parse_option_buffer() logs all
			   cases where it fails */
			packet_dereference(&packet, MDL);
			return;
		}
	} else if ((msg_type == DHCPV6_DHCPV4_QUERY) ||
		   (msg_type == DHCPV6_DHCPV4_RESPONSE)) {
		int msglen =
		    (int)(offsetof(struct dhcpv4_over_dhcpv6_packet, options));
		msg = (struct dhcpv4_over_dhcpv6_packet *)(raw->data + 36);
		packet->dhcpv6_msg_type = msg->msg_type;

		/* message-specific data */
		memcpy(packet->dhcp4o6_flags, msg->flags,
		       sizeof(packet->dhcp4o6_flags));

		if (!parse_option_buffer(packet->options,
					 msg->options,
					 raw->len - 36 - msglen,
					 &dhcpv6_universe)) {
			/* no logging here, as parse_option_buffer() logs all
			   cases where it fails */
			packet_dereference(&packet, MDL);
			return;
		}
	} else {
		log_error("recv_dhcpv4_query: unexpected message of type %d.",
			  (int)msg_type);
		packet_dereference(&packet, MDL);
		return;
	}

	/*
	 * From dhcpv6().
	 */

	/*
	 * Log a message that we received this packet.
	 */
	/* log_packet_in(packet); */
	memset(&ds, 0, sizeof(ds));
	if (packet->dhcpv6_msg_type < dhcpv6_type_name_max) {
		data_string_sprintfa(&ds, "%s message from %s",
				     dhcpv6_type_names[packet->dhcpv6_msg_type],
				     piaddr(packet->client_addr));
	} else {
		data_string_sprintfa(&ds,
				     "Unknown message type %d from %s",
				     packet->dhcpv6_msg_type,
				     piaddr(packet->client_addr));
	}
	if ((packet->dhcpv6_msg_type == DHCPV6_RELAY_FORW) ||
	    (packet->dhcpv6_msg_type == DHCPV6_RELAY_REPL)) {
		char tmp_addr[INET6_ADDRSTRLEN];
		const void *addr;

		addr = &packet->dhcpv6_link_address;
		data_string_sprintfa(&ds, ", link address %s",
				     inet_ntop(AF_INET6, addr,
					       tmp_addr, sizeof(tmp_addr)));
	    	addr = &packet->dhcpv6_peer_address;
	    	data_string_sprintfa(&ds, ", peer address %s",
				     inet_ntop(AF_INET6, addr,
					       tmp_addr, sizeof(tmp_addr)));
	} else if ((packet->dhcpv6_msg_type != DHCPV6_DHCPV4_QUERY) &&
		   (packet->dhcpv6_msg_type != DHCPV6_DHCPV4_RESPONSE)) {
		u_int32_t tid = 0;

		memcpy(((char *)&tid)+1, packet->dhcpv6_transaction_id, 3);
		data_string_sprintfa(&ds, ", transaction ID 0x%06X", tid);
	}
	log_info("%s", ds.data);
	data_string_forget(&ds, MDL);

	/*
	 * Build our reply packet.
         */
	build_dhcpv6_reply(&reply, packet);

	if (reply.data == NULL) {
		packet_dereference(&packet, MDL);
		return;
	}

	/*
	 * Forward the response.
	 */
	len = reply.len + 36;
	memset(&ds, 0, sizeof(ds));
	if (!buffer_allocate(&ds.buffer, len, MDL)) {
		log_error("recv_dhcpv4_query: no memory.");
		packet_dereference(&packet, MDL);
		return;
	}
	ds.data = ds.buffer->data;
	ds.len = len;

	memcpy(ds.buffer->data, name, 16);
	memcpy(ds.buffer->data + 16, iaddr.iabuf, 16);
	udp_data.rsp_opt_exist = packet->relay_source_port ? 1 : 0;
	memcpy(ds.buffer->data + 32, &udp_data, 4);
	memcpy(ds.buffer->data + 36, reply.data, reply.len);

	/*
	 * Now we can release the packet.
	 */
	packet_dereference(&packet, MDL);

	cc = send(dhcp4o6_fd, ds.data, ds.len, 0);
	if (cc < 0)
		log_error("recv_dhcpv4_query: send(): %m");
	data_string_forget(&ds, MDL);
}
#endif /* DHCP4o6 */

static void
seek_shared_host(struct host_decl **hp, struct shared_network *shared) {
	struct host_decl *nofixed = NULL;
	struct host_decl *seek, *hold = NULL;

	/*
	 * Seek forward through fixed addresses for the right link.
	 *
	 * Note: how to do this for fixed prefixes???
	 */
	host_reference(&hold, *hp, MDL);
	host_dereference(hp, MDL);
	seek = hold;
	while (seek != NULL) {
		if (seek->fixed_addr == NULL)
			nofixed = seek;
		else if (fixed_matches_shared(seek, shared))
			break;

		seek = seek->n_ipaddr;
	}

	if ((seek == NULL) && (nofixed != NULL))
		seek = nofixed;

	if (seek != NULL)
		host_reference(hp, seek, MDL);
}

static isc_boolean_t
fixed_matches_shared(struct host_decl *host, struct shared_network *shared) {
	struct subnet *subnet;
	struct data_string addr;
	isc_boolean_t matched;
	struct iaddr fixed;

	if (host->fixed_addr == NULL)
		return ISC_FALSE;

	memset(&addr, 0, sizeof(addr));
	if (!evaluate_option_cache(&addr, NULL, NULL, NULL, NULL, NULL,
				   &global_scope, host->fixed_addr, MDL))
		return ISC_FALSE;

	if (addr.len < 16) {
		data_string_forget(&addr, MDL);
		return ISC_FALSE;
	}

	fixed.len = 16;
	memcpy(fixed.iabuf, addr.data, 16);

	matched = ISC_FALSE;
	for (subnet = shared->subnets ; subnet != NULL ;
	     subnet = subnet->next_sibling) {
		if (addr_eq(subnet_number(fixed, subnet->netmask),
			    subnet->net)) {
			matched = ISC_TRUE;
			break;
		}
	}

	data_string_forget(&addr, MDL);
	return matched;
}

/*!
 *
 * \brief Constructs a REPLY with status of UseMulticast to a given packet
 *
 * Per RFC 3315 Secs 18.2.1,3,6 & 7, when a server rejects a client's
 * unicast-sent packet, the response must only contain the client id,
 * server id, and a status code option of 5 (UseMulticast).  This function
 * constructs such a packet and returns it as a data_string.
 *
 * \param reply_ret = data_string which will receive the newly constructed
 * reply
 * \param packet = client request which is being rejected
 * \param client_id = data_string which contains the client id
 * \param server_id = data_string which which contains the server id
 *
 */
void
unicast_reject(struct data_string *reply_ret,
	     struct packet *packet,
	     const struct data_string *client_id,
	     const struct data_string *server_id)
{
	struct reply_state reply;
	memset(&reply, 0x0, sizeof(struct reply_state));

	/* Locate the client. */
	if (shared_network_from_packet6(&reply.shared, packet)
		!= ISC_R_SUCCESS) {
		log_error("unicast_reject: could not locate client.");
		return;
	}

	/* Initialize the reply. */
	packet_reference(&reply.packet, packet, MDL);
	data_string_copy(&reply.client_id, client_id, MDL);

	if (start_reply(packet, client_id, server_id, &reply.opt_state,
			 &reply.buf.reply)) {
		/* Set the UseMulticast status code. */
		if (!set_status_code(STATUS_UseMulticast,
				     "Unicast not allowed by server.",
				     reply.opt_state)) {
			log_error("unicast_reject: Unable to set status code.");
		} else {
			/* Set write cursor to just past the reply header. */
			reply.cursor = REPLY_OPTIONS_INDEX;
			reply.cursor += store_options6(((char *)reply.buf.data
							+ reply.cursor),
						       (sizeof(reply.buf)
						        - reply.cursor),
						       reply.opt_state,
						       reply.packet,
						       unicast_reject_opts,
						       NULL);

			/* Return our reply to the caller. */
			reply_ret->len = reply.cursor;
			reply_ret->buffer = NULL;
			if (!buffer_allocate(&reply_ret->buffer,
					     reply.cursor, MDL)) {
				log_fatal("unicast_reject:"
					  "No memory to store Reply.");
			}

			memcpy(reply_ret->buffer->data, reply.buf.data,
			       reply.cursor);
			reply_ret->data = reply_ret->buffer->data;
		}

	}

	/* Cleanup. */
	if (reply.shared != NULL)
		shared_network_dereference(&reply.shared, MDL);
	if (reply.opt_state != NULL)
		option_state_dereference(&reply.opt_state, MDL);
	if (reply.packet != NULL)
		packet_dereference(&reply.packet, MDL);
	if (reply.client_id.data != NULL)
		data_string_forget(&reply.client_id, MDL);
}

/*!
 *
 * \brief Checks if the dhcp6.unicast option has been defined
 *
 * Scans the option space for the presence of the dhcp6.unicast option. The
 * function attempts to map the inbound packet to a shared network first
 * by an ip address specified via an D6O_IA_XX option and if that fails then
 * by the packet's source information (e.g. relay link, link, or interace).
 * Once the packet is mapped to a shared network, the function executes all
 * statements from the network's group outward into a local option cache.
 * The option cache is then scanned for the presence of unicast option.  If
 * the packet cannot be mapped to a shared network, the function returns
 * ISC_FALSE.
 * \param packet inbound packet from the client
 *
 * \return ISC_TRUE if the dhcp6.unicast option is defined, false otherwise.
 *
 */
isc_boolean_t
is_unicast_option_defined(struct packet *packet) {
        isc_boolean_t is_defined = ISC_FALSE;
	struct option_state *opt_state = NULL;
	struct option_cache *oc = NULL;
	struct shared_network *shared = NULL;

	if (!option_state_allocate(&opt_state, MDL)) {
		log_fatal("is_unicast_option_defined:"
			  "No memory for option state.");
	}

	/* We try to map the packet to a network first by an IA_XX value.
 	 * If that fails, we try by packet source. */
	if (((shared_network_from_requested_addr(&shared, packet)
	     != ISC_R_SUCCESS) &&
	    (shared_network_from_packet6(&shared, packet) != ISC_R_SUCCESS))
	    || (shared == NULL)) {
		/* @todo what would this really mean? I think wrong network
		 * logic will catch it */
		log_error("is_unicast_option_defined:"
			  "cannot attribute packet to a network.");
		return (ISC_FALSE);
	}

	/* Now that we've mapped it to a network, execute statments to that
	 * scope, looking for the unicast option. We don't care about the
	 * value of the option, only whether or not it is defined. */
	execute_statements_in_scope(NULL, NULL, NULL, NULL, NULL, opt_state,
				    &global_scope, shared->group, NULL, NULL);

	oc = lookup_option(&dhcpv6_universe, opt_state, D6O_UNICAST);
	is_defined = (oc != NULL ? ISC_TRUE : ISC_FALSE);
	log_debug("is_unicast_option_defined: option found : %d", is_defined);

	if (shared != NULL) {
		shared_network_dereference(&shared, MDL);
	}

	if (opt_state != NULL) {
		option_state_dereference(&opt_state, MDL);
	}

	return (is_defined);
}

/*!
 *
 * \brief Maps a packet to a shared network based on the requested IP address
 *
 * The function attempts to find a subnet that matches the first requested IP
 * address contained within the given packet.  Note that it looks first for
 * D6O_IA_NAs, then D6O_IA_PDs and lastly D6O_IA_TAs.  If a matching network is
 * found, a reference to it is returned in the parameter, shared.
 *
 * \param shared shared_network pointer which will receive the matching network
 * \param packet inbound packet from the client
 *
 * \return ISC_R_SUCCESS if the packet can be mapped to a shared_network.
 *
 */
static isc_result_t
shared_network_from_requested_addr (struct shared_network **shared,
				    struct packet* packet) {
	struct iaddr iaddr;
	struct subnet* subnet = NULL;
	isc_result_t status = ISC_R_FAILURE;

	/* Try to match first IA_ address or prefix we find to a subnet. In
 	 * theory all  IA_ values in a given request are supposed to be in the
 	 * same subnet so we only need to try one right? */
	if ((get_first_ia_addr_val(packet, D6O_IA_NA, &iaddr) != ISC_R_SUCCESS)
	     && (get_first_ia_addr_val(packet, D6O_IA_PD, &iaddr)
		 != ISC_R_SUCCESS)
	     && (get_first_ia_addr_val(packet, D6O_IA_TA, &iaddr)
		 != ISC_R_SUCCESS))  {
		/* we found nothing to match against */
		log_debug("share_network_from_request_addr: nothing to match");
		return (ISC_R_FAILURE);
	}

	if (!find_subnet(&subnet, iaddr, MDL)) {
		log_debug("shared_network_from_requested_addr:"
			  "No subnet found for addr %s.", piaddr(iaddr));
	} else {
		status = shared_network_reference(shared,
                                                  subnet->shared_network, MDL);
		subnet_dereference(&subnet, MDL);
		log_debug("shared_network_from_requested_addr:"
			  " found shared network %s for address %s.",
			  ((*shared)->name ? (*shared)->name : "unnamed"),
			  piaddr(iaddr));
		return (status);
	}

	return (ISC_R_FAILURE);
}

/*!
 *
 * \brief Retrieves the first IP address from a given packet of a given type
 *
 * Search a packet for options of a given type (D6O_IA_AN, D6O_IA_PD, or
 * D6O_IA_TA) for the first non-blank IA_XX value and return its IP address
 * component.
 *
 * \param packet packet received from the client
 * \param addr_type the address option type (D6O_IA_NA , D6O_IA_PD, or
 * D6O_IP_TA) to look for within the packet.
 * \param iaddr pointer to the iaddr structure which will receive the extracted
 * address.
 *
 * \return ISC_R_SUCCESS if an address was succesfully extracted, ISC_R_FALURE
 * otherwise.
 *
 */
static isc_result_t
get_first_ia_addr_val (struct packet* packet, int addr_type,
		       struct iaddr* iaddr)  {
        struct option_cache *ia;
        struct option_cache *oc = NULL;
        struct data_string cli_enc_opt_data;
        struct option_state *cli_enc_opt_state;
	int addr_opt_offset;
	int addr_opt;
	int addr_opt_data_len;
	int ip_addr_offset;

	isc_result_t status = ISC_R_FAILURE;
	memset(iaddr, 0, sizeof(struct iaddr));

	/* Set up address type specifics */
	switch (addr_type) {
	case D6O_IA_NA:
		addr_opt_offset = IA_NA_OFFSET;
		addr_opt = D6O_IAADDR;
		addr_opt_data_len = 24;
		ip_addr_offset = 0;
		break;
	case D6O_IA_TA:
		addr_opt_offset = IA_TA_OFFSET;
		addr_opt = D6O_IAADDR;
		addr_opt_data_len = 24;
		ip_addr_offset = 0;
		break;
	case D6O_IA_PD:
		addr_opt_offset = IA_PD_OFFSET;
		addr_opt = D6O_IAPREFIX;
		addr_opt_data_len = 25;
		ip_addr_offset = 9;
		break;
	default:
		/* shouldn't be here */
		log_error ("get_first_ia_addr_val: invalid opt type %d",
			   addr_type);
		return (ISC_R_FAILURE);
	}

	/* Find the first, non-blank IA_XX value within an D6O_IA_XX option. */
	for (ia = lookup_option(&dhcpv6_universe, packet->options, addr_type);
             ia != NULL && oc == NULL; ia = ia->next) {
                if (!get_encapsulated_IA_state(&cli_enc_opt_state,
                                               &cli_enc_opt_data,
                                               packet, ia, addr_opt_offset)) {
			log_debug ("get_first_ia_addr_val:"
				   " couldn't unroll enclosing option");
                        return (ISC_R_FAILURE);
                }

                oc = lookup_option(&dhcpv6_universe, cli_enc_opt_state,
                                   addr_opt);
                if (oc == NULL) {
			/* no address given for this IA, ignore */
                	option_state_dereference(&cli_enc_opt_state, MDL);
                	data_string_forget(&cli_enc_opt_data, MDL);
		}
	}

	/* If we found a non-blank IA_XX then extract its ip address. */
	if (oc != NULL) {
		struct data_string iaddr_str;

		memset(&iaddr_str, 0, sizeof(iaddr_str));
		if (!evaluate_option_cache(&iaddr_str, packet, NULL, NULL,
				   	  packet->options, NULL, &global_scope,
				    	  oc, MDL)) {
			log_error("get_first_ia_addr_val: "
			  	  "error evaluating IA_XX option.");
		} else {
			if (iaddr_str.len != addr_opt_data_len) {
				log_error("shared_network_from_requested_addr:"
                                  	  " invalid length %d, expected %d",
				  	  iaddr_str.len, addr_opt_data_len);
			} else {
				iaddr->len = 16;
				memcpy (iaddr->iabuf,
					iaddr_str.data + ip_addr_offset, 16);
				status = ISC_R_SUCCESS;
			}
			data_string_forget(&iaddr_str, MDL);
		}

		option_state_dereference(&cli_enc_opt_state, MDL);
		data_string_forget(&cli_enc_opt_data, MDL);
	}

	return (status);
}

/*
* \brief Calculates the reply T1/T2 times and stuffs them in outbound buffer
*
* T1/T2 time selection is kind of weird.  We actually use DHCP * (v4) scoped
* options, dhcp-renewal-time and dhcp-rebinding-time, as handy existing places
* where these can be configured by an administrator.  A value of zero tells the
* client it may choose its own value.
*
* When those options are not defined, the values will be set to zero unless
* the global option, dhcpv6-set-tee-times is enabled. When this option is
* enabled the values are calculated as recommended by RFC 3315, Section 22.4:
*
* 	T1 will be set to 0.5 times the shortest preferred lifetime
* 	in the IA_XX option.  If the "shortest" preferred lifetime is
* 	0xFFFFFFFF,  T1 will set to 0xFFFFFFFF.
*
* 	T2 will be set to 0.8 times the shortest preferred lifetime
* 	in the IA_XX option.  If the "shortest" preferred lifetime is
* 	0xFFFFFFFF,  T2 will set to 0xFFFFFFFF.
*
* Note that dhcpv6-set-tee-times is intended to be transitional and will
* likely be removed in 4.4.0, leaving the behavior as getting the values
* either from the configured parameters (if you want zeros, define them as
* zeros) or by calculating them per the RFC.
*
* \param reply - pointer to the reply_state structure
* \param ia_cursor - offset of the beginning of the IA_XX option within the
* reply's outbound data buffer
*/
static void
set_reply_tee_times(struct reply_state* reply, unsigned ia_cursor)
{
	struct option_cache *oc;
	int set_tee_times;

	/* Found out if calculated values are enabled. */
	oc = lookup_option(&server_universe, reply->opt_state,
			   SV_DHCPV6_SET_TEE_TIMES);
	set_tee_times = (oc &&
			 evaluate_boolean_option_cache(NULL, reply->packet,
						       NULL, NULL,
						       reply->packet->options,
						       reply->opt_state,
						       &global_scope, oc, MDL));

	oc = lookup_option(&dhcp_universe, reply->opt_state,
			   DHO_DHCP_RENEWAL_TIME);
	if (oc != NULL) {
		/* dhcp-renewal-time is defined, use it */
		struct data_string data;
		memset(&data, 0x00, sizeof(data));

		if (!evaluate_option_cache(&data, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state, &global_scope,
					   oc, MDL) ||
		    (data.len != 4)) {
			log_error("Invalid renewal time.");
			reply->renew = 0;
		} else {
			reply->renew = getULong(data.data);
		}

		if (data.data != NULL)
			data_string_forget(&data, MDL);
	} else if (set_tee_times) {
		/* Setting them is enabled so T1 is either infinite or
		 * 0.5 * the shortest preferred lifetime in the IA_XX  */
		if (reply->min_prefer == INFINITE_TIME)
			reply->renew = INFINITE_TIME;
		else
			reply->renew = reply->min_prefer / 2;
	} else {
		/* Default is to let the client choose */
		reply->renew = 0;
	}

	putULong(reply->buf.data + ia_cursor + 8, reply->renew);

	/* Now T2. */
	oc = lookup_option(&dhcp_universe, reply->opt_state,
			   DHO_DHCP_REBINDING_TIME);
	if (oc != NULL) {
		/* dhcp-rebinding-time is defined, use it */
		struct data_string data;
		memset(&data, 0x00, sizeof(data));

		if (!evaluate_option_cache(&data, reply->packet, NULL, NULL,
					   reply->packet->options,
					   reply->opt_state, &global_scope,
					   oc, MDL) ||
		    (data.len != 4)) {
			log_error("Invalid rebinding time.");
			reply->rebind = 0;
		} else {
			reply->rebind = getULong(data.data);
		}

		if (data.data != NULL)
			data_string_forget(&data, MDL);
	} else if (set_tee_times) {
		/* Setting them is enabled so T2 is either infinite or
		 * 0.8 * the shortest preferred lifetime in the reply */
		if (reply->min_prefer == INFINITE_TIME)
			reply->rebind = INFINITE_TIME;
		else
			reply->rebind = (reply->min_prefer / 5) * 4;
	} else {
		/* Default is to let the client choose */
		reply->rebind = 0;
	}

	putULong(reply->buf.data + ia_cursor + 12, reply->rebind);
}

/*
 * Releases the iasubopts in the pre-existing IA, if they are not in
 * the same shared-network as the new IA.
 *
 * returns 1 if the release was done, 0 otherwise
 */
int
release_on_roam(struct reply_state* reply) {
	struct ia_xx* old_ia = reply->old_ia;
	struct iasubopt *lease = NULL;
	int i;

	if ((!do_release_on_roam) || old_ia == NULL
	    || old_ia->num_iasubopt <= 0) {
		return(0);
	}

	/* If the old shared-network and new are the same, client hasn't
	* roamed, nothing to do. We only check the first one because you
	* cannot have iasubopts on different shared-networks within a
	* single ia. */
	lease = old_ia->iasubopt[0];
	if (lease->ipv6_pool->shared_network == reply->shared) {
		return (0);
	}

	/* Old and new are on different shared networks so the client must
	* roamed. Release the old leases. */
	for (i = 0;  i < old_ia->num_iasubopt; i++) {
		lease = old_ia->iasubopt[i];

		log_info("Client: %s roamed to new network,"
			 " releasing lease: %s%s",
			 print_hex_1(reply->client_id.len,
				     reply->client_id.data, 60),
			 pin6_addr(&lease->addr), iasubopt_plen_str(lease));

                release_lease6(lease->ipv6_pool, lease);
                lease->ia->cltt = cur_time;
                write_ia(lease->ia);
        }

	return (1);
}

/*
 * Convenience function which returns a string (static buffer)
 * containing either a "/" followed by the prefix length or an
 * empty string depending on the lease type
 */
const char *iasubopt_plen_str(struct iasubopt *lease) {
	static char prefix_buf[16];
	*prefix_buf = 0;
	if ((lease->ia) && (lease->ia->ia_type == D6O_IA_PD)) {
		sprintf(prefix_buf, "/%-d", lease->plen);
	}

	return (prefix_buf);
}

#ifdef NSUPDATE
/*
 * Initiates DDNS updates for static v6 leases if configured to do so.
 *
 * The function, which must be called after the IA has been written to the
 * packet, adds an iasubopt to the IA for static lease.  This is done so we
 * have an iasubopt to pass into ddns_updates().  A reference to the IA is
 * added to the DDNS control block to ensure it and it's iasubopt remain in
 * scope until the update is complete.
 *
 */
void ddns_update_static6(struct reply_state* reply) {
	struct iasubopt *iasub = NULL;
	struct binding_scope *scope = NULL;
	struct option_cache *oc = NULL;

	oc = lookup_option(&server_universe, reply->opt_state, SV_DDNS_UPDATES);
	if ((oc != NULL) &&
		(evaluate_boolean_option_cache(NULL, reply->packet, NULL, NULL,
					       reply->packet->options,
                                               reply->opt_state, NULL,
                                               oc, MDL) == 0)) {
		return;
	}

	oc = lookup_option(&server_universe, reply->opt_state,
			   SV_UPDATE_STATIC_LEASES);
	if ((oc == NULL) ||
		(evaluate_boolean_option_cache(NULL, reply->packet,
						     NULL, NULL,
						     reply->packet->options,
						     reply->opt_state, NULL,
						     oc, MDL) == 0)) {
		return;
	}

	if (iasubopt_allocate(&iasub, MDL) != ISC_R_SUCCESS) {
		log_fatal("No memory for iasubopt.");
	}

	if (ia_add_iasubopt(reply->ia, iasub, MDL) != ISC_R_SUCCESS) {
		log_fatal("Could not add iasubopt.");
	}

	ia_reference(&iasub->ia, reply->ia, MDL);

	memcpy(iasub->addr.s6_addr, reply->fixed.data, 16);
	iasub->plen = 0;
	iasub->prefer =  MAX_TIME;
	iasub->valid =  MAX_TIME;
	iasub->static_lease = 1;

	if (!binding_scope_allocate(&scope, MDL)) {
		log_fatal("Out of memory for binding scope.");
	}

	binding_scope_reference(&iasub->scope, scope, MDL);

	ddns_updates(reply->packet, NULL, NULL, iasub, NULL, reply->opt_state);
}
#endif /* NSUPDATE */

#endif /* DHCPv6 */
