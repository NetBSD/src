/*	$NetBSD: confparse.c,v 1.3 2022/04/03 01:10:59 christos Exp $	*/

/*
 * Copyright (C) 2017-2022 Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   PO Box 360
 *   Newmarket, NH 03857 USA
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: confparse.c,v 1.3 2022/04/03 01:10:59 christos Exp $");

/* From server/confpars.c */

#include "keama.h"

#include <sys/errno.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* Print failover stuff once */
isc_boolean_t failover_once = ISC_TRUE;

/* To manage host-reservation-identifiers */
isc_boolean_t use_client_id = ISC_FALSE;
isc_boolean_t use_flex_id = ISC_FALSE;
isc_boolean_t use_hw_address = ISC_FALSE;

/* option and relays used for flexible host identifier */
const struct option *host_id_option = NULL;
int host_id_relays = 0;

/* Simple or complex config */
unsigned subnet_counter = 0;

/* For subclass name generation */
unsigned subclass_counter = 0;

/* To map reservations to declared subnets */
struct subnet {
	struct element *subnet;
	struct element *share;
	struct string *addr;
	struct string *mask;
	TAILQ_ENTRY(subnet) next;
};

TAILQ_HEAD(subnets, subnet) known_subnets;

/* To map pools to subnets inside a shared-network */
struct range {
	struct element *pool;
	struct element *share;
	struct string *low;
	TAILQ_ENTRY(range) next;
};

TAILQ_HEAD(ranges, range) known_ranges;

static void post_process_lifetimes(struct parse *);
static size_t post_process_reservations(struct parse *);
static void post_process_classes(struct parse *);
static void post_process_generated_classes(struct parse *);
static void check_depend(struct element *, struct element *);
static void post_process_option_definitions(struct parse *);
static void add_host_reservation_identifiers(struct parse *, const char *);
static void add_host_id_option(struct parse *, const struct option *, int);
static void subclass_inherit(struct parse *, struct element *,
			     struct element *);
static void add_match_class(struct parse *, struct element *,
			    struct element *);
static void option_data_derive(struct parse *, struct handle *,
			       struct handle *);
static void derive_classes(struct parse *, struct handle *, struct handle *);
static isc_boolean_t is_hexa_only(const char *, unsigned len);
static void new_network_interface(struct parse *, struct element *);
static struct string *addrmask(const struct string *, const struct string *);
static struct element *find_match(struct parse *, struct element *,
				  isc_boolean_t *);
static struct element *find_location(struct element *, struct range *);
static int get_prefix_length(const char *, const char *);
static struct element *get_class(struct parse *, struct element *);
static void concat_classes(struct parse *, struct element *, struct element *);
static void generate_class(struct parse *, struct element *, struct element *,
			   struct element *);

static struct string *CLASS_ALL;
static struct string *CLASS_KNOWN;

/* Add head config file comments to the DHCP server map */

size_t
conf_file_parse(struct parse *cfile)
{
	struct element *top;
	struct element *dhcp;
	size_t issues;

	TAILQ_INIT(&known_subnets);
	TAILQ_INIT(&known_ranges);
	CLASS_ALL = makeString(-1, "ALL");
	CLASS_KNOWN = makeString(-1, "KNOWN");

	top = createMap();
	top->kind = TOPLEVEL;
	TAILQ_CONCAT(&top->comments, &cfile->comments);

	dhcp = createMap();
	dhcp->kind = ROOT_GROUP;
	(void) peek_token(NULL, NULL, cfile);
	TAILQ_CONCAT(&dhcp->comments, &cfile->comments);
	stackPush(cfile, dhcp);
	assert(cfile->stack_top == 1);
	cfile->stack[0] = top;

	if (local_family == AF_INET)
		mapSet(top, dhcp, "Dhcp4");
	else if (local_family == AF_INET6)
		mapSet(top, dhcp, "Dhcp6");
	else
		parse_error(cfile, "address family is not set");

	issues = conf_file_subparse(cfile, ROOT_GROUP);

	/* Add a warning when interfaces-config is not present */
	if (subnet_counter > 0) {
		struct element *ifconf;

		ifconf = mapGet(cfile->stack[1], "interfaces-config");
		if (ifconf == NULL) {
			struct comment *comment;

			comment = createComment("/// This configuration "
						"declares some subnets but "
						"has no interfaces-config");
			TAILQ_INSERT_TAIL(&cfile->stack[1]->comments, comment);
			comment = createComment("/// Reference Kea #245");
			TAILQ_INSERT_TAIL(&cfile->stack[1]->comments, comment);
		}
	}

	post_process_lifetimes(cfile);
	if (!global_hr)
		issues += post_process_reservations(cfile);
	post_process_classes(cfile);
	post_process_generated_classes(cfile);
	post_process_option_definitions(cfile);

	return issues;
}

/* Lifetime post-processing */
static void
post_process_lifetimes(struct parse *cfile)
{
	struct element *entry;

	entry = mapGet(cfile->stack[1], "valid-lifetime");
	if ((entry == NULL) && use_isc_lifetimes) {
		struct comment *comment;

		/* DEFAULT_DEFAULT_LEASE_TIME is 43200 */
		entry = createInt(43200);
		comment = createComment("/// Use ISC DHCP default lifetime");
		TAILQ_INSERT_TAIL(&entry->comments, comment);
		mapSet(cfile->stack[1], entry, "valid-lifetime");
	}

	entry = mapGet(cfile->stack[1], "min-valid-lifetime");
	if ((entry == NULL) && use_isc_lifetimes) {
		struct comment *comment;

		/* DEFAULT_MIN_LEASE_TIME is 300 */
		entry = createInt(300);
		comment = createComment("/// Use ISC DHCP min lifetime");
		TAILQ_INSERT_TAIL(&entry->comments, comment);
		mapSet(cfile->stack[1], entry, "min-valid-lifetime");
	}

	entry = mapGet(cfile->stack[1], "max-valid-lifetime");
	if ((entry == NULL) && use_isc_lifetimes) {
		struct comment *comment;

		/* DEFAULT_MAX_LEASE_TIME is 86400 */
		entry = createInt(86400);
		comment = createComment("/// Use ISC DHCP max lifetime");
		TAILQ_INSERT_TAIL(&entry->comments, comment);
		mapSet(cfile->stack[1], entry, "max-valid-lifetime");
	}

	/* Done for DHCPv4 */
	if (local_family == AF_INET)
		return;

	/* There is no builtin default for preferred-lifetime,
	   nor min/max values in ISC DHCP. */
}

/* Reservation post-processing */

static size_t
post_process_reservations(struct parse *cfile)
{
	struct element *hosts;
	struct element *orphans;
	struct element *host;
	struct element *where;
	struct element *dest;
	isc_boolean_t used_heuristic;
	size_t issues;

	issues = 0;
	hosts = mapGet(cfile->stack[1], "reservations");
	if ((hosts == NULL) || global_hr)
		return issues;
	mapRemove(cfile->stack[1], "reservations");
	orphans = createList();
	orphans->kind = HOST_DECL;
	while (listSize(hosts) > 0) {
		host = listGet(hosts, 0);
		listRemove(hosts, 0);
		used_heuristic = ISC_FALSE;
		where = find_match(cfile, host, &used_heuristic);
		if (where == cfile->stack[1])
			dest = orphans;
		else
			dest = mapGet(where, "reservations");
		if (dest == NULL) {
			dest = createList();
			dest->kind = HOST_DECL;
			mapSet(where, dest, "reservations");
		}
		listPush(dest, host);
	}
	if (listSize(orphans) > 0) {
		struct comment *comment;

		comment = createComment("/// Orphan reservations");
		TAILQ_INSERT_TAIL(&orphans->comments, comment);
		comment = createComment("/// Kea reservations are per subnet");
		TAILQ_INSERT_TAIL(&orphans->comments, comment);
		comment = createComment("/// Reference Kea #231");
		TAILQ_INSERT_TAIL(&orphans->comments, comment);
		orphans->skip = ISC_TRUE;
		issues++;
		mapSet(cfile->stack[1], orphans, "reservations");
	}
	return issues;
}

/* Cleanup classes */

static void
post_process_classes(struct parse *cfile)
{
	struct element *classes;
	struct element *class;
	struct element *name;
	struct element *entry;
	struct element *reduced;
	struct string *msg;
	struct comment *comment;
	isc_boolean_t lose;
	size_t i;

	classes = mapGet(cfile->stack[1], "client-classes");
	if ((classes == NULL) || (listSize(classes) == 0))
		return;
	for (i = 0; i < listSize(classes); i++) {
		class = listGet(classes, i);
		if ((class == NULL) || (class->type != ELEMENT_MAP))
			parse_error(cfile, "null global class at %i",
				    (unsigned)i);
		name = mapGet(class, "name");
		if ((name == NULL) || (name->type != ELEMENT_STRING))
			parse_error(cfile, "global class at %u "
				    "without a name", (unsigned)i);
		if (!mapContains(class, "super"))
			goto cleanup_superclass;

		/* cleanup subclass */
		mapRemove(class,"super");
		entry = mapGet(class, "string");
		if (entry != NULL) {
			if (entry->type != ELEMENT_STRING)
				parse_error(cfile, "subclass %s has "
					    "a bad string selector",
					    stringValue(name)->content);
			msg = makeString(-1, "/// subclass selector ");
			appendString(msg, "'");
			concatString(msg, stringValue(entry));
			appendString(msg, "'");
			comment = createComment(msg->content);
			TAILQ_INSERT_TAIL(&class->comments, comment);
			mapRemove(class, "string");
			continue;
		}
		entry = mapGet(class, "binary");
		if (entry == NULL)
			parse_error(cfile, "subclass %s has no selector",
				    stringValue(name)->content);
		msg = makeString(-1, "/// subclass selector 0x");
		concatString(msg, stringValue(entry));
		comment = createComment(msg->content);
		TAILQ_INSERT_TAIL(&class->comments, comment);
		mapRemove(class, "binary");

	cleanup_superclass:
		/* cleanup superclass */
		entry = mapGet(class, "spawning");
		if (entry == NULL)
			goto cleanup_class;
		if (entry->type != ELEMENT_BOOLEAN)
			parse_error(cfile, "superclass %s has bad "
				    "spawning flag",
				    stringValue(name)->content);
		if (boolValue(entry)) {
			msg = makeString(-1, "/// Spawning classes "
					 "are not supported by Kea");
			comment = createComment(msg->content);
			TAILQ_INSERT_TAIL(&class->comments, comment);
			msg = makeString(-1, "/// Reference Kea #248");
			comment = createComment(msg->content);
			TAILQ_INSERT_TAIL(&class->comments, comment);
			msg = makeString(-1, "/// spawn with: ");
		} else
			msg = makeString(-1, "/// match: ");
		entry = mapGet(class, "submatch");

		if (entry == NULL)
			parse_error(cfile, "superclass %s has no submatch",
				    stringValue(name)->content);
		lose = ISC_FALSE;
		appendString(msg, print_data_expression(entry, &lose));
		if (!lose) {
			comment = createComment(msg->content);
			TAILQ_INSERT_TAIL(&class->comments, comment);
			mapRemove(class, "spawning");
			mapRemove(class, "submatch");
		}

	cleanup_class:
		/* cleanup class */
		entry = mapGet(class, "match-if");
		if (entry == NULL)
			continue;
		reduced = mapGet(class, "test");
		lose = ISC_FALSE;
		if (reduced != NULL)
			msg = makeString(-1, "/// from: match if ");
		else
			msg = makeString(-1, "/// match if ");
		appendString(msg, print_boolean_expression(entry, &lose));
		if (!lose) {
			comment = createComment(msg->content);
			if (reduced != NULL) {
				TAILQ_INSERT_TAIL(&reduced->comments, comment);
				mapRemove(class, "match-if");
				continue;
			}
			TAILQ_INSERT_TAIL(&entry->comments, comment);
		}
	}
}

/* Move generated client classes to the end of client class list */

static void
post_process_generated_classes(struct parse *cfile)
{
	struct element *generated;
	struct element *classes;
	struct element *class;

	generated = mapGet(cfile->stack[1], "generated-classes");
	if (generated == NULL)
		return;
	mapRemove(cfile->stack[1], "generated-classes");
	if (listSize(generated) == 0)
		return;
	classes = mapGet(cfile->stack[1], "client-classes");
	if (classes == NULL) {
		classes = createList();
		mapSet(cfile->stack[1], classes, "client-classes");
	}

	while (listSize(generated) > 0) {
		class = listGet(generated, 0);
		listRemove(generated, 0);
		check_depend(class, classes);
		listPush(classes, class);
	}
}

static void
check_depend(struct element *class, struct element *classes)
{
	struct element *list;

	if (!mapContains(class, "depend"))
		return;
	list = mapGet(class, "depend");
	mapRemove(class, "depend");
	while (listSize(list) > 0) {
		struct element *depend;
		struct string *dname;
		struct string *msg;
		struct comment *comment;
		isc_boolean_t found;
		size_t i;

		depend = listGet(list, 0);
		listRemove(list, 0);
		assert(depend != NULL);
		assert(depend->type == ELEMENT_STRING);
		dname = stringValue(depend);
		if (eqString(dname, CLASS_ALL) ||
		    eqString(dname, CLASS_KNOWN))
			continue;
		found = ISC_FALSE;
		for (i = 0; i < listSize(classes); i++) {
			struct element *item;
			struct element *name;

			item = listGet(classes, i);
			assert(item != NULL);
			assert(item->type == ELEMENT_MAP);
			name = mapGet(item, "name");
			if (name == NULL)
				continue;
			assert(name->type == ELEMENT_STRING);
			if (eqString(stringValue(name), dname)) {
				found = ISC_TRUE;
				break;
			}
		}
		if (found)
			continue;
		msg = makeString(-1, "/// Depend on missing '");
		concatString(msg, dname);
		appendString(msg, "' class");
		comment = createComment(msg->content);
		TAILQ_INSERT_TAIL(&class->comments, comment);
		class->skip = ISC_TRUE;
	}
}

static void
post_process_option_definitions(struct parse *cfile)
{
	struct element *opt_def;
	struct element *def, *ndef;

	opt_def = mapGet(cfile->stack[1], "option-def");
	if (opt_def == NULL)
		return;
	TAILQ_FOREACH_SAFE(def, &opt_def->value.list_value, ndef) {
		if (mapContains(def, "no-export"))
			TAILQ_REMOVE(&opt_def->value.list_value, def);
	}
}

void
read_conf_file(struct parse *parent, const char *filename, int group_type)
{
	int file;
	struct parse *cfile;
	struct string *msg;
	struct comment *comment;
	size_t amount = parent->stack_size * sizeof(struct element *);

	if ((file = open (filename, O_RDONLY)) < 0)

		parse_error(parent, "Can't open %s: %s",
			    filename, strerror(errno));

	cfile = new_parse(file, NULL, 0, filename, 0);
	if (cfile == NULL)
		parse_error(parent, "Can't create new parse structure");

	cfile->stack = (struct element **)malloc(amount);
	if (cfile->stack == NULL)
		parse_error(parent, "Can't create new element stack");
	memcpy(cfile->stack, parent->stack, amount);
	cfile->stack_size = parent->stack_size;
	cfile->stack_top = parent->stack_top;
	cfile->issue_counter = parent->issue_counter;

	msg = makeString(-1, "/// Begin file ");
	concatString(msg, makeString(-1, filename));
	comment = createComment(msg->content);
	TAILQ_INSERT_TAIL(&cfile->comments, comment);

	conf_file_subparse(cfile, group_type);

	amount = cfile->stack_size * sizeof(struct element *);
	if (cfile->stack_size > parent->stack_size) {
		parent->stack =
			(struct element **)realloc(parent->stack, amount);
		if (parent->stack == NULL)
			parse_error(cfile, "can't resize element stack");
	}
	memcpy(parent->stack, cfile->stack, amount);
	parent->stack_size = cfile->stack_size;
	parent->stack_top = cfile->stack_top;
	parent->issue_counter = cfile->issue_counter;
	msg = makeString(-1, "/// End file ");
	concatString(msg, makeString(-1, filename));
	comment= createComment(msg->content);
	TAILQ_INSERT_TAIL(&parent->comments, comment);
	end_parse(cfile);
}

/* conf-file :== parameters declarations END_OF_FILE
   parameters :== <nil> | parameter | parameters parameter
   declarations :== <nil> | declaration | declarations declaration */

size_t
conf_file_subparse(struct parse *cfile, int type)
{
	const char *val;
	enum dhcp_token token;
	isc_boolean_t declaration = ISC_FALSE;

	for (;;) {
		token = peek_token(&val, NULL, cfile);
		if (token == END_OF_FILE)
			break;
		declaration = parse_statement(cfile, type, declaration);
	}
	skip_token(&val, NULL, cfile);

	return cfile->issue_counter;
}

/* statement :== parameter | declaration | PERCENT directive

   parameter :== DEFAULT_LEASE_TIME lease_time
	       | MAX_LEASE_TIME lease_time
	       | DYNAMIC_BOOTP_LEASE_CUTOFF date
	       | DYNAMIC_BOOTP_LEASE_LENGTH lease_time
	       | BOOT_UNKNOWN_CLIENTS boolean
	       | ONE_LEASE_PER_CLIENT boolean
	       | GET_LEASE_HOSTNAMES boolean
	       | USE_HOST_DECL_NAME boolean
	       | NEXT_SERVER ip-addr-or-hostname SEMI
	       | option_parameter
	       | SERVER-IDENTIFIER ip-addr-or-hostname SEMI
	       | FILENAME string-parameter
	       | SERVER_NAME string-parameter
	       | hardware-parameter
	       | fixed-address-parameter
	       | ALLOW allow-deny-keyword
	       | DENY allow-deny-keyword
	       | USE_LEASE_ADDR_FOR_DEFAULT_ROUTE boolean
	       | AUTHORITATIVE
	       | NOT AUTHORITATIVE

   declaration :== host-declaration
		 | group-declaration
		 | shared-network-declaration
		 | subnet-declaration
		 | VENDOR_CLASS class-declaration
		 | USER_CLASS class-declaration
		 | RANGE address-range-declaration */

isc_boolean_t
parse_statement(struct parse *cfile, int type, isc_boolean_t declaration)
{
	enum dhcp_token token;
	const char *val;
	struct element *hardware;
	struct element *cache;
	struct element *et;
	isc_boolean_t lose;
	isc_boolean_t known;
	isc_boolean_t authoritative;
	struct option *option;
	size_t host_decl = 0;
	size_t subnet = 0;
	size_t i;

	token = peek_token(&val, NULL, cfile);

	switch (token) {
	case INCLUDE:
		skip_token(&val, NULL, cfile);
		token = next_token(&val, NULL, cfile);
		if (token != STRING)
			parse_error(cfile, "filename string expected.");
		read_conf_file(cfile, val, type);
		parse_semi(cfile);
		return 1;

	case HOST:
		skip_token(&val, NULL, cfile);
		if (type != HOST_DECL && type != CLASS_DECL)
			parse_host_declaration(cfile);
		else
			parse_error(cfile,
				    "host declarations not allowed here.");
		return 1;

	case GROUP:
		skip_token(&val, NULL, cfile);
		if (type != HOST_DECL && type != CLASS_DECL)
			parse_group_declaration(cfile);
		else
			parse_error(cfile,
				    "group declarations not allowed here.");
		return 1;

	case SHARED_NETWORK:
		skip_token(&val, NULL, cfile);
		if (type == SHARED_NET_DECL ||
		    type == HOST_DECL ||
		    type == SUBNET_DECL ||
		    type == CLASS_DECL)
			parse_error(cfile, "shared-network parameters not %s.",
				    "allowed here");
		parse_shared_net_declaration(cfile);
		return 1;

	case SUBNET:
	case SUBNET6:
		skip_token(&val, NULL, cfile);
		if (type == HOST_DECL || type == SUBNET_DECL ||
		    type == CLASS_DECL)
			parse_error(cfile,
				    "subnet declarations not allowed here.");

		if (token == SUBNET)
			parse_subnet_declaration(cfile);
		else
			parse_subnet6_declaration(cfile);
		return 1;

	case VENDOR_CLASS:
	case USER_CLASS:
	case CLASS:
	case SUBCLASS:
		skip_token(&val, NULL, cfile);
		if (token == VENDOR_CLASS)
			parse_error(cfile, "obsolete 'vendor-class' "
				    "declaration");
		if (token == USER_CLASS)
			parse_error(cfile, "obsolete 'user-class' "
				    "declaration");
		if (type == CLASS_DECL)
			parse_error(cfile,
				    "class declarations not allowed here.");
		parse_class_declaration(cfile, token == CLASS
					       ? CLASS_TYPE_CLASS
					       : CLASS_TYPE_SUBCLASS);
		return 1;

	case HARDWARE:
		if (!use_hw_address) {
			add_host_reservation_identifiers(cfile,
							 "hw-address");
			use_hw_address = ISC_TRUE;
		}

		skip_token(&val, NULL, cfile);
		if (!host_decl) {
			for (i = cfile->stack_top; i > 0; --i) {
				if (cfile->stack[i]->kind == HOST_DECL) {
					host_decl = i;
					break;
				}
			}
		}
		if (!host_decl)
			parse_error(cfile, "hardware address parameter %s",
				    "not allowed here.");
		if (mapContains(cfile->stack[host_decl], "hw-address"))
			parse_error(cfile, "Host hardware address already "
				    "configured.");
		hardware = parse_hardware_param(cfile);
		mapSet(cfile->stack[host_decl], hardware, "hw-address");
		if (hardware->skip)
			cfile->stack[host_decl]->skip = ISC_TRUE;
		break;

	case FIXED_ADDR:
	case FIXED_ADDR6:
		skip_token(&val, NULL, cfile);
		if (!host_decl) {
			for (i = cfile->stack_top; i > 0; --i) {
				if (cfile->stack[i]->kind == HOST_DECL) {
					host_decl = i;
					break;
				}
			}
		}
		if (!host_decl)
			parse_error(cfile,
				   "fixed-address parameter not "
				   "allowed here.");
		cache = parse_fixed_addr_param(cfile, token);
		if (token == FIXED_ADDR) {
			struct element *addr;

			if (mapContains(cfile->stack[host_decl], "ip-address"))
				parse_error(cfile, "Only one fixed address "
					    "declaration per host.");
			addr = listGet(cache, 0);
			listRemove(cache, 0);
			mapSet(cfile->stack[host_decl], addr, "ip-address");
			if (listSize(cache) > 0) {
				cache->skip = ISC_TRUE;
				cfile->issue_counter++;
				mapSet(cfile->stack[host_decl],
				       cache, "extra-ip-addresses");
			}
		} else {
			if (mapContains(cfile->stack[host_decl],
					"ip-addresses"))
				parse_error(cfile, "Only one fixed address "
					    "declaration per host.");
			mapSet(cfile->stack[host_decl], cache, "ip-addresses");
		}
		break;

	case POOL:
		skip_token(&val, NULL, cfile);
		if (type == POOL_DECL)
			parse_error(cfile, "pool declared within pool.");
		if (type != SUBNET_DECL && type != SHARED_NET_DECL)
			parse_error(cfile, "pool declared outside of network");
		parse_pool_statement(cfile, type);

		return declaration;

	case RANGE:
		skip_token(&val, NULL, cfile);
		if (!subnet) {
			for (i = cfile->stack_top; i > 0; --i) {
				if (cfile->stack[i]->kind == SUBNET_DECL) {
					subnet = i;
					break;
				}
			}
		}
		if (type != SUBNET_DECL || !subnet)
			parse_error(cfile,
				    "range declaration not allowed here.");
		parse_address_range(cfile, type, subnet);
		return declaration;

	case RANGE6:
		if (local_family != AF_INET6)
			goto unknown;
		skip_token(NULL, NULL, cfile);
		if (!subnet) {
			for (i = cfile->stack_top; i > 0; --i) {
				if (cfile->stack[i]->kind == SUBNET_DECL) {
					subnet = i;
					break;
				}
			}
		}
	        if ((type != SUBNET_DECL) || !subnet)
			parse_error(cfile,
				    "range6 declaration not allowed here.");
	      	parse_address_range6(cfile, type, subnet);
		return declaration;

	case PREFIX6:
		if (local_family != AF_INET6)
			goto unknown;
		skip_token(NULL, NULL, cfile);
		if (!subnet) {
			for (i = cfile->stack_top; i > 0; --i) {
				if (cfile->stack[i]->kind == SUBNET_DECL) {
					subnet = i;
					break;
				}
			}
		}
		if ((type != SUBNET_DECL) || !subnet)
			parse_error(cfile,
				    "prefix6 declaration not allowed here.");
	      	parse_prefix6(cfile, type, subnet);
		return declaration;

	case FIXED_PREFIX6:
		if (local_family != AF_INET6)
			goto unknown;
		skip_token(&val, NULL, cfile);
		if (!host_decl) {
			for (i = cfile->stack_top; i > 0; --i) {
				if (cfile->stack[i]->kind == HOST_DECL) {
					host_decl = i;
					break;
				}
			}
		}
		if (!host_decl)
			parse_error(cfile,
				    "fixed-prefix6 declaration not "
				    "allowed here.");
		parse_fixed_prefix6(cfile, host_decl);
		break;

	case POOL6:
		if (local_family != AF_INET6)
			goto unknown;
		skip_token(&val, NULL, cfile);
		if (type == POOL_DECL)
			parse_error(cfile, "pool6 declared within pool.");
		if (type != SUBNET_DECL)
			parse_error(cfile,
				    "pool6 declared outside of network");
		parse_pool6_statement(cfile, type);

		return declaration;

	case TOKEN_NOT:
		skip_token(&val, NULL, cfile);
		token = next_token(&val, NULL, cfile);
		switch (token) {
		case AUTHORITATIVE:
			authoritative = ISC_FALSE;
			goto authoritative;
		default:
			parse_error(cfile, "expecting assertion");
		}
		break;

	case AUTHORITATIVE:
		skip_token(&val, NULL, cfile);
		authoritative = ISC_TRUE;
	authoritative:
		if (type == HOST_DECL)
			parse_error(cfile, "authority makes no sense here.");
		if (!subnet) {
			for (i = cfile->stack_top; i > 0; --i) {
				int kind;

				kind = cfile->stack[i]->kind;
				if ((kind == SUBNET_DECL) ||
				    (kind == SHARED_NET_DECL) ||
				    (kind == ROOT_GROUP)) {
					subnet = i;
					break;
				}
			}
		}
		if (!subnet)
			parse_error(cfile, "can't find root group");
		if (local_family == AF_INET) {
			cache = createBool(authoritative);
			TAILQ_CONCAT(&cache->comments, &cfile->comments);
			mapSet(cfile->stack[subnet], cache, "authoritative");
		}
		parse_semi(cfile);
		break;

		/* "server-identifier" is a special hack, equivalent to
		   "option dhcp-server-identifier". */
	case SERVER_IDENTIFIER:
		option = option_lookup_code("dhcp",
					    DHO_DHCP_SERVER_IDENTIFIER);
		assert(option);
		skip_token(&val, NULL, cfile);
		goto finish_option;

	case OPTION:
		skip_token(&val, NULL, cfile);
		token = peek_token(&val, NULL, cfile);
		if (token == SPACE) {
			if (type != ROOT_GROUP)
				parse_error(cfile,
					    "option space definitions %s",
					    "may not be scoped.");
			parse_option_space_decl(cfile);
			return declaration;
		}

		known = ISC_FALSE;
		option = parse_option_name(cfile, ISC_TRUE, &known);
		token = peek_token(&val, NULL, cfile);
		if (token == CODE) {
			if (type != ROOT_GROUP)
				parse_error(cfile,
					    "option definitions%s",
					    " may not be scoped.");
			skip_token(&val, NULL, cfile);

			/* next function must deal with redefinitions */
			parse_option_code_definition(cfile, option);
			return declaration;
		}
		/* If this wasn't an option code definition, don't
		   allow an unknown option. */
		if (!known)
			parse_error(cfile, "unknown option %s.%s",
				    option->space->old, option->old);
	finish_option:
		parse_option_statement(NULL, cfile, option,
				       supersede_option_statement);
		return declaration;

	case FAILOVER:
		if (failover_once)
			fprintf(stderr, "ignoring failover\n");
		failover_once = ISC_FALSE;
		skip_to_semi(cfile);
		break;

	case SERVER_DUID:
		if (local_family != AF_INET6)
			goto unknown;
		parse_server_duid_conf(cfile);
		break;

	case LEASE_ID_FORMAT:
		token = next_token(&val, NULL, cfile);
		/* ignore: ISC DHCP specific */
		break;

	case PERCENT:
		skip_token(&val, NULL, cfile);
		if (type != ROOT_GROUP)
			parse_error(cfile, "directives are only supported "
				    "at toplevel");
		parse_directive(cfile);
		return declaration;

	unknown:
		skip_token(&val, NULL, cfile);

	default:
		et = createMap();
		TAILQ_CONCAT(&et->comments, &cfile->comments);
		lose = ISC_FALSE;
		if (!parse_executable_statement(et, cfile, &lose,
						context_any, ISC_TRUE)) {
			if (!lose) {
				if (declaration)
					parse_error(cfile,
						    "expecting a declaration");
				else
					parse_error(cfile,
						    "expecting a parameter %s",
						    "or declaration");
			}
			return declaration;
		}
		if (mapSize(et) == 0)
			return declaration;

		et->skip = ISC_TRUE;
		cfile->issue_counter++;
		mapSet(cfile->stack[cfile->stack_top], et, "statement");
	}

	return 0;
}

/*!
 *
 * \brief Parse allow and deny statements
 *
 * This function handles the common processing code for permit and deny
 * statements in the parse_pool_statement and parse_pool6_statement functions.
 *
 * The allow or deny token should already be consumed, this function expects
 * one of the following:
 *   known-clients;
 *   unknown-clients;
 *   known clients;
 *   unknown clients;
 *   authenticated clients;
 *   unauthenticated clients;
 *   all clients;
 *   dynamic bootp clients;
 *   members of <class name>;
 *   after <date>;
 *
 * \param[in] cfile       = the configuration file being parsed
 * \param[in] permit_head = the head of the permit list (permit or prohibit)
 *			    to which to attach the newly created  permit structure
 */

void
get_permit(struct parse *cfile, struct element *permit_head)
{
	enum dhcp_token token;
	const char *val;
	struct string *permit;
	struct string *alias = NULL;
	struct comment *comment = NULL;
	struct element *member;
	isc_boolean_t need_clients = ISC_TRUE;
	isc_boolean_t negative = ISC_FALSE;

	token = next_token(&val, NULL, cfile);
	switch (token) {
	case UNKNOWN:
		permit = CLASS_KNOWN;
		negative = ISC_TRUE;
		alias = makeString(-1, "unknown clients");
		break;

	case KNOWN_CLIENTS:
		need_clients = ISC_FALSE;
		permit = CLASS_KNOWN;
		alias = makeString(-1, "known-clients");
		break;

	case UNKNOWN_CLIENTS:
		need_clients = ISC_FALSE;
		permit = CLASS_KNOWN;
		negative = ISC_TRUE;
		alias = makeString(-1, "unknown-clients");
		break;

	case KNOWN:
		permit = CLASS_KNOWN;
		alias = makeString(-1, "known clients");
		break;

	case AUTHENTICATED:
		permit = CLASS_ALL;
		alias = makeString(-1, "authenticated clients");
		negative = ISC_TRUE;
	authenticated_clients:
		comment = createComment("/// [un]authenticated-clients is "
					"not supported by ISC DHCP and Kea");
		break;

	case UNAUTHENTICATED:
		permit = CLASS_ALL;
		alias = makeString(-1, "unauthenticated clients");
		goto authenticated_clients;
		break;

	case ALL:
		permit = CLASS_ALL;
		alias = makeString(-1, "all clients");
		break;

	case DYNAMIC:
		/* bootp is not supported by Kea so the dynamic bootp
		 * client set is the empty set. */
		if (next_token(&val, NULL, cfile) != TOKEN_BOOTP)
			parse_error(cfile, "expecting \"bootp\"");
		permit = CLASS_ALL;
		negative = ISC_TRUE;
		alias = makeString(-1, "dynamic bootp clients");
		cfile->issue_counter++;
		comment = createComment("/// dynamic-bootp-client is not "
					"supported by Kea");
		break;

	case MEMBERS:
		/* we don't check the class... */
		need_clients = ISC_FALSE;
		if (next_token(&val, NULL, cfile) != OF)
			parse_error(cfile, "expecting \"of\"");
		if (next_token(&val, NULL, cfile) != STRING)
			parse_error(cfile, "expecting class name.");
		permit = makeString(-1, val);
		break;

	case AFTER:
		/* don't use parse_date_code() */
		need_clients = ISC_FALSE;
		permit = makeString(-1, "AFTER_");
		alias = makeString(-1, "after ");
		while (peek_raw_token(NULL, NULL, cfile) != SEMI) {
			next_raw_token(&val, NULL, cfile);
			appendString(permit, val);
			appendString(alias, val);
		}
		permit_head->skip = ISC_TRUE;
		cfile->issue_counter++;
		comment = createComment("/// after <date> is not yet "
					"supported by Kea");
		break;

	default:
		parse_error(cfile, "expecting permit type.");
	}

	/*
	 * The need_clients flag is set if we are expecting the
	 * CLIENTS token
	 */
	if (need_clients && (next_token(&val, NULL, cfile) != CLIENTS))
		parse_error(cfile, "expecting \"clients\"");
	member = createMap();
	mapSet(member, createString(permit), "class");
	mapSet(member, createBool(!negative), "way");
	if (alias != NULL)
		mapSet(member, createString(alias), "real");
	if (comment != NULL)
		TAILQ_INSERT_TAIL(&permit_head->comments, comment);
	listPush(permit_head, member);
	parse_semi(cfile);

	return;
}

/*!
 *
 * \brief Parse a pool statement
 *
 * Pool statements are used to group declarations and permit & deny information
 * with a specific address range.  They must be declared within a shared network
 * or subnet and there may be multiple pools withing a shared network or subnet.
 * Each pool may have a different set of permit or deny options.
 *
 * \param[in] cfile = the configuration file being parsed
 * \param[in] type  = the type of the enclosing statement.  This must be
 *		      SHARED_NET_DECL or SUBNET_DECL for this function.
 *
 * \return
 * void - This function either parses the statement and updates the structures
 *        or it generates an error message and possible halts the program if
 *        it encounters a problem.
 */
void
parse_pool_statement(struct parse *cfile, int type)
{
	enum dhcp_token token;
	const char *val;
	isc_boolean_t done = ISC_FALSE;
	struct element *pool;
	struct element *pools;
	struct element *permit;
	struct element *prohibit;
	int declaration = 0;
	unsigned range_counter = 0;

	pool = createMap();
	pool->kind = POOL_DECL;
	TAILQ_CONCAT(&pool->comments, &cfile->comments);

	if (type != SUBNET_DECL && type != SHARED_NET_DECL)
		parse_error(cfile, "Dynamic pools are only valid inside "
			    "subnet or shared-network statements.");
	parse_lbrace(cfile);

	stackPush(cfile, pool);
	type = POOL_DECL;

	permit = createList();
	prohibit = createList();

	do {
		token = peek_token(&val, NULL, cfile);
		switch (token) {
		case TOKEN_NO:
		case FAILOVER:
			if (failover_once)
				fprintf(stderr, "ignoring failover\n");
			failover_once = ISC_FALSE;
			skip_to_semi(cfile);
			break;

		case RANGE:
			skip_token(&val, NULL, cfile);
			parse_address_range(cfile, type, cfile->stack_top);
			range_counter++;
			break;

		case ALLOW:
			skip_token(&val, NULL, cfile);
			get_permit(cfile, permit);
			break;

		case DENY:
			skip_token(&val, NULL, cfile);
			get_permit(cfile, prohibit);
			break;

		case RBRACE:
			skip_token(&val, NULL, cfile);
			done = ISC_TRUE;
			break;

		case END_OF_FILE:
			/*
			 * We can get to END_OF_FILE if, for instance,
			 * the parse_statement() reads all available tokens
			 * and leaves us at the end.
			 */
			parse_error(cfile, "unexpected end of file");

		default:
			declaration = parse_statement(cfile, type,
						      declaration);
			break;
		}
	} while (!done);

	cfile->stack_top--;

	generate_class(cfile, pool, permit, prohibit);

	pools = mapGet(cfile->stack[cfile->stack_top], "pools");
	if (pools == NULL) {
		pools = createList();
		pools->kind = POOL_DECL;
		mapSet(cfile->stack[cfile->stack_top], pools, "pools");
	}
	if (range_counter == 0) {
		struct comment *comment;

		/* no range */
		comment = createComment("empty pool");
		TAILQ_INSERT_TAIL(&pool->comments, comment);
		pool->skip = ISC_TRUE;
		cfile->issue_counter++;
		listPush(pools, pool);
		return;
	}
	/* spread extra ranges into pool copies */
	while (--range_counter != 0) {
		struct handle *handle;
		struct element *first;
		struct element *saved;
		isc_boolean_t seen = ISC_FALSE;

		first = createMap();
		saved = copy(pool);
		TAILQ_CONCAT(&first->comments, &pool->comments);
		while (mapSize(pool) > 0) {
			handle = mapPop(pool);
			if ((handle == NULL) || (handle->key == NULL) ||
			    (handle->value == NULL))
				parse_error(cfile, "bad pool entry");
			if (strcmp(handle->key, "pool") != 0)
				mapSet(first, handle->value, handle->key);
			else if (!seen) {
				mapSet(first, handle->value, handle->key);
				mapRemove(saved, "pool");
				seen = ISC_TRUE;
			}
		}
		listPush(pools, first);
		pool = saved;
	}
	listPush(pools, pool);
}

/* Expect a left brace */

void
parse_lbrace(struct parse *cfile)
{
	enum dhcp_token token;
	const char *val;

	token = next_token(&val, NULL, cfile);
	if (token != LBRACE)
		parse_error(cfile, "expecting left brace.");
}

/* host-declaration :== hostname RBRACE parameters declarations LBRACE */

void
parse_host_declaration(struct parse *cfile)
{
	const char *val;
	enum dhcp_token token;
	struct element *host;
	struct string *name;
	struct element *where;
	struct element *hosts = NULL;
	int declaration = 0;
	isc_boolean_t used_heuristic = ISC_FALSE;

	host = createMap();
	host->kind = HOST_DECL;
	TAILQ_CONCAT(&host->comments, &cfile->comments);

	name = parse_host_name(cfile);
	if (!name)
		parse_error(cfile, "expecting a name for host declaration.");

	mapSet(host, createString(name), "hostname");

	parse_lbrace(cfile);

	stackPush(cfile, host);

	for (;;) {
		token = peek_token(&val, NULL, cfile);
		if (token == RBRACE) {
			skip_token(&val, NULL, cfile);
			break;
		}
		if (token == END_OF_FILE)
			parse_error(cfile, "unexpected end of file");
		/* If the host declaration was created by the server,
		   remember to save it. */
		if (token == DYNAMIC) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "dynamic hosts don't exist "
				    "in the config file");
		}
		/* If the host declaration was created by the server,
		   remember to save it. */
		if (token == TOKEN_DELETED) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "deleted hosts don't exist "
				    "in the config file");
		}

		if (token == GROUP) {
			struct element *group;
			struct comment *comment;

			skip_token(&val, NULL, cfile);
			token = next_token(&val, NULL, cfile);
			if (token != STRING && !is_identifier(token))
				parse_error(cfile,
					    "expecting string or identifier.");
			group = createString(makeString(-1, val));
			group->skip = ISC_TRUE;
			cfile->issue_counter++;
			comment = createComment("/// Unsupported group in "
						"host reservations");
			TAILQ_INSERT_TAIL(&group->comments, comment);
			comment = createComment("/// Reference Kea #233");
			TAILQ_INSERT_TAIL(&group->comments, comment);
			mapSet(host, group, "group");
			parse_semi(cfile);
			continue;
		}

		if (token == UID) {
			struct string *client_id;

			if (!use_client_id) {
				add_host_reservation_identifiers(cfile,
								 "client-id");
				use_client_id = ISC_TRUE;
			}

			skip_token(&val, NULL, cfile);

			if (mapContains(host, "client-id"))
				parse_error(cfile, "Host %s already has a "
						   "client identifier.",
					    name->content);

			/* See if it's a string or a cshl. */
			token = peek_token(&val, NULL, cfile);
			if (token == STRING) {
				skip_token(&val, NULL, cfile);
				client_id = makeString(-1, val);
			} else {
				struct string *bin;
				unsigned len = 0;

				bin = parse_numeric_aggregate
					(cfile, NULL, &len, ':', 16, 8);
				if (!bin)
					parse_error(cfile,
						    "expecting hex list.");
				client_id = makeStringExt(bin->length,
							  bin->content, 'H');
			}
			mapSet(host, createString(client_id), "client-id");

			parse_semi(cfile);
			continue;
		}

		if (token == HOST_IDENTIFIER) {
			struct string *host_id;
			isc_boolean_t known;
			struct option *option;
			struct element *expr;
			struct string *data;
			int relays = 0;

			if (!use_flex_id) {
				add_host_reservation_identifiers(cfile,
								 "flex-id");
				use_flex_id = ISC_TRUE;
			}

			if (mapContains(host, "host-identifier") ||
			    mapContains(host, "flex-id"))
				parse_error(cfile,
					    "only one host-identifier allowed "
					    "per host");
	      		skip_token(&val, NULL, cfile);
			token = next_token(&val, NULL, cfile);
			host_id = makeString(-1, val);
			appendString(host_id, " ");
			if (token == V6RELOPT) {
				token = next_token(&val, NULL, cfile);

				if (token != NUMBER)
					parse_error(cfile,
						    "host-identifier v6relopt "
						    "must have a number");
				appendString(host_id, val);
				appendString(host_id, " ");
				relays = atoi(val);
				if (relays < 0)
					parse_error(cfile,
						    "host-identifier v6relopt "
						    "must have a number >= 0");
				if (relays > MAX_V6RELAY_HOPS)
					relays = MAX_V6RELAY_HOPS + 1;
			} else if (token != OPTION)
				parse_error(cfile,
					    "host-identifier must be an option"
					    " or v6relopt");
			known = ISC_FALSE;
			option = parse_option_name(cfile, ISC_TRUE, &known);
			if (!known)
				parse_error(cfile, "unknown option %s.%s",
					    option->space->old, option->old);
			appendString(host_id, option->space->name);
			appendString(host_id, ".");
			appendString(host_id, option->name);
			appendString(host_id, " ");

			data = parse_option_textbin(cfile, option);
			parse_semi(cfile);

			if (data == NULL)
				parse_error(cfile, "can't get option data");
			concatString(host_id, data);
			expr = createString(host_id);
			expr->skip = ISC_TRUE;
			cfile->issue_counter++;
			mapSet(host, expr, "host-identifier");

			if (host_id_option == NULL)
				add_host_id_option(cfile, option, relays);
			else if ((host_id_option != option) ||
				 (host_id_relays != relays)) {
				struct string *msg;
				struct comment *comment;

				msg = allocString();
				appendString(msg, "/// Another option (");
				appendString(msg, host_id_option->name);
				appendString(msg, ") is already used as ");
				appendString(msg, "host-identifier");
				comment = createComment(msg->content);
				TAILQ_INSERT_TAIL(&expr->comments, comment);
				continue;
			}

			/*
			 * Everything good: set a flex-id and remove
			 * the host-identifier entry.
			 */
			mapSet(host, createString(data), "flex-id");
			mapRemove(host, "host-identifier");
			continue;
		}

		declaration = parse_statement(cfile, HOST_DECL, declaration);
	}

	cfile->stack_top--;

	where = find_match(cfile, host, &used_heuristic);
	hosts = mapGet(where, "reservations");
	if (hosts == NULL) {
		hosts = createList();
		hosts->kind = HOST_DECL;
		mapSet(where, hosts, "reservations");
		if (used_heuristic) {
			struct comment *comment;

			comment = createComment("/// Host reservations "
						"without fixed addresses "
						"were put in the last "
						"declared subnet");
			TAILQ_INSERT_TAIL(&hosts->comments, comment);
			comment = createComment("/// Reference Kea #231");
			TAILQ_INSERT_TAIL(&hosts->comments, comment);
		}
	}
	listPush(hosts, host);
}

/* Simple tool to declare used (and only used) reservation identifiers */
static void
add_host_reservation_identifiers(struct parse *cfile, const char *id)
{
	struct element *ids;

	ids = mapGet(cfile->stack[1], "host-reservation-identifiers");
	if (ids == NULL) {
		ids = createList();
		mapSet(cfile->stack[1], ids, "host-reservation-identifiers");
	}
	listPush(ids, createString(makeString(-1, id)));
}

/* Add the flexible host identifier glue */
static void
add_host_id_option(struct parse *cfile,
		   const struct option *option, int relays)
{
	struct string *path;
	struct string *expr;
	struct element *params;
	struct element *entry;
	struct element *hooks;
	struct comment *comment;
	char buf[40];

	host_id_option = option;
	host_id_relays = relays;

	/*
	 * Using the example from the Kea Administrator Reference Manual
	 * as recommended by Tomek
	 */
	hooks = createList();
	mapSet(cfile->stack[1], hooks, "hooks-libraries");
	comment = createComment("/// The flexible host identifier "
				"is a premium feature");
	TAILQ_INSERT_TAIL(&hooks->comments, comment);
	entry = createMap();
	listPush(hooks, entry);
	if (hook_library_path != NULL)
		path = makeString(-1, hook_library_path);
	else
		path = makeString(-1, "/path/");
	appendString(path, "libdhcp_flex_id.so");
	params = createString(path);
	if (hook_library_path == NULL) {
		comment = createComment("/// Please update the path here");
		TAILQ_INSERT_TAIL(&params->comments, comment);
	}
	mapSet(entry, params, "library");
	params = createMap();
	mapSet(entry, params, "parameters");

	snprintf(buf, sizeof(buf), "%soption[%u].hex",
		 relays > 0 ? "relay[0]." : "", option->code);
	expr = makeString(-1, buf);
	mapSet(params, createString(expr), "identifier-expression");
}

static void add_host_reservation_identifiers(struct parse *, const char *);
/* class-declaration :== STRING LBRACE parameters declarations RBRACE
 *
 * in fact:
 * (CLASS) NAME(STRING) LBRACE ... RBRACE
 * (SUBCLASS) SUPER(STRING) DATA/HASH(STRING | <hexa>) [BRACE ... RBRACE]
 *
 * class "name" { MATCH IF <boolean-expr> }: direct: belong when true
 * class "name" { MATCH <data-expr> }: indirect: use subclasses
 * class "name" { MATCH <data-expr> SPAWN WITH <data-expr> }: indirect:
 *  create dynamically a subclass
 * subclass "super" <data-expr = string or binary aka hash>: belongs when
 *  super <data-expr> == <hash>
 */

void
parse_class_declaration(struct parse *cfile, int type)
{
	const char *val = NULL;
	enum dhcp_token token;
	size_t group = 0;
	size_t i = 0;
	struct element *group_classes = NULL;
	struct element *classes = NULL;
	struct element *class = NULL;
	struct element *pc = NULL; /* p(arent)c(lass) */
	struct element *tmp = NULL;
	struct element *expr = NULL;
	struct element *data = NULL;
	isc_boolean_t binary = ISC_FALSE;
	int declaration = 0;
	struct string *name = NULL;
	isc_boolean_t lose = ISC_FALSE;
	isc_boolean_t matchedonce = ISC_FALSE;
	isc_boolean_t submatchedonce = ISC_FALSE;

	token = next_token(&val, NULL, cfile);
	if (token != STRING)
		parse_error(cfile, "Expecting class name");

	/* Find group and root classes */
	classes = mapGet(cfile->stack[1], "client-classes");
	if (classes == NULL) {
		classes = createList();
		classes->kind = CLASS_DECL;
		mapSet(cfile->stack[1], classes, "client-classes");
	}
	for (group = cfile->stack_top; group > 0; --group) {
		int kind;

		kind = cfile->stack[group]->kind;
		if (kind == CLASS_DECL)
			parse_error(cfile, "class in class");
		if ((kind == GROUP_DECL) || (kind == ROOT_GROUP))
			break;
	}
	if (!group)
		parse_error(cfile, "can't find root group");
	if (cfile->stack[group]->kind == GROUP_DECL) {
		group_classes = mapGet(cfile->stack[group], "client-classes");
		if (group_classes == NULL) {
			group_classes = createList();
			group_classes->kind = CLASS_DECL;
			mapSet(cfile->stack[group], group_classes,
			       "client-classes");
		}
	} else
		group_classes = classes;

	/* See if there's already a class with the specified name. */
	for (i = 0; i < listSize(classes); i++) {
		struct element *name;

		tmp = listGet(classes, i);
		name = mapGet(tmp, "name");
		if (name == NULL)
			continue;
		if (strcmp(stringValue(name)->content, val) == 0) {
			pc = tmp;
			break;
		}
	}

	/* If it is a class, we're updating it.  If it's any of the other
	 * types (subclass, vendor or user class), the named class is a
	 * reference to the parent class so its mandatory.
	 */
	if ((pc != NULL) && (type == CLASS_TYPE_CLASS)) {
		class = pc;
		pc = NULL;
	} else if (type != CLASS_TYPE_CLASS) {
		if (pc == NULL)
			parse_error(cfile, "no class named %s", val);
		if (!mapContains(pc, "spawning") ||
		    !mapContains(pc, "submatch"))
			parse_error(cfile, "found class name %s but it is "
				    "not a suitable superclass", val);
	}

	name = makeString(-1, val);
	/* If this is a straight subclass, parse the hash string. */
	if (type == CLASS_TYPE_SUBCLASS) {
		token = peek_token(&val, NULL, cfile);
		if (token == STRING) {
			unsigned len;

			skip_token(&val, &len, cfile);
			data = createString(makeString(len, val));
		} else if (token == NUMBER_OR_NAME || token == NUMBER) {
			data = createHexa(parse_hexa(cfile));
			binary = ISC_TRUE;
		} else {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "Expecting string or hex list.");
		}
	}

	/* See if there's already a class in the hash table matching the
	   hash data. */
	if (type != CLASS_TYPE_CLASS) {
		for (i = 0; i < listSize(classes); i++) {
			struct element *super;
			struct element *selector;

			tmp = listGet(classes, i);
			super = mapGet(tmp, "super");
			if (super == NULL)
				continue;
			if (!eqString(stringValue(super), name))
				continue;
			if (binary)
				selector = mapGet(tmp, "binary");
			else
				selector = mapGet(tmp, "string");
			if (selector == NULL)
				continue;
			if (eqString(stringValue(selector),
				     stringValue(data))) {
				class = tmp;
				break;
			}
		}
	}

	/* Note the class declaration in the enclosing group */
	if (group_classes != classes) {
		struct element *gc;

		gc = createMap();
		gc->kind = CLASS_DECL;
		tmp = createString(name);
		if (type == CLASS_TYPE_CLASS)
			mapSet(gc, tmp, "name");
		else {
			tmp->skip = ISC_TRUE;
			mapSet(gc, tmp, "super");
			data->skip = ISC_TRUE;
			if (binary)
				mapSet(gc, data, "binary");
			else
				mapSet(gc, data, "string");
		}
		listPush(group_classes, gc);
	}

	/* If we didn't find an existing class, allocate a new one. */
	if (!class) {
		/* Allocate the class structure... */
		class = createMap();
		class->kind = CLASS_DECL;
		TAILQ_CONCAT(&class->comments, &cfile->comments);
		if (type == CLASS_TYPE_SUBCLASS) {
			struct string *subname;
			char buf[40];

			cfile->issue_counter++;
			tmp = createString(name);
			tmp->skip = ISC_TRUE;
			mapSet(class, tmp, "super");
			data->skip = ISC_TRUE;
			if (binary)
				mapSet(class, data, "binary");
			else
				mapSet(class, data, "string");
			subname = makeString(-1, "sub#");
			concatString(subname, name);
			snprintf(buf, sizeof(buf),
				 "#%u", subclass_counter++);
			appendString(subname, buf);
			mapSet(class, createString(subname), "name");
		} else
			/* Save the name, if there is one. */
			mapSet(class, createString(name), "name");
		listPush(classes, class);
	}

	/* Spawned classes don't have to have their own settings. */
	if (type == CLASS_TYPE_SUBCLASS) {
		token = peek_token(&val, NULL, cfile);
		if (token == SEMI) {
			skip_token(&val, NULL, cfile);
			subclass_inherit(cfile, class, copy(pc));
			return;
		}
	}

	parse_lbrace(cfile);

	stackPush(cfile, class);

	for (;;) {
		token = peek_token(&val, NULL, cfile);
		if (token == RBRACE) {
			skip_token(&val, NULL, cfile);
			break;
		} else if (token == END_OF_FILE) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "unexpected end of file");
		} else if (token == DYNAMIC) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "dynamic classes don't exist "
				    "in the config file");
		} else if (token == TOKEN_DELETED) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "deleted hosts don't exist "
				    "in the config file");
		} else if (token == MATCH) {
			skip_token(&val, NULL, cfile);
			if (pc)
				parse_error(cfile,
					    "invalid match in subclass.");
			token = peek_token(&val, NULL, cfile);
			if (token != IF) {
				expr = createBool(ISC_FALSE);
				expr->skip = 1;
				mapSet(class, expr, "spawning");
				goto submatch;
			}

			skip_token(&val, NULL, cfile);
			if (matchedonce)
				parse_error(cfile,
					    "A class may only have "
					    "one 'match if' clause.");
			matchedonce = ISC_TRUE;
			expr = createMap();
			if (!parse_boolean_expression(expr, cfile, &lose)) {
				if (!lose)
					parse_error(cfile,
						    "expecting boolean expr.");
			} else {
				expr->skip = ISC_TRUE;
				mapSet(class, expr, "match-if");
				add_match_class(cfile, class, copy(expr));
				parse_semi(cfile);
			}
		} else if (token == SPAWN) {
			skip_token(&val, NULL, cfile);
			if (pc)
				parse_error(cfile,
					    "invalid spawn in subclass.");
			expr = createBool(ISC_TRUE);
			expr->skip = ISC_TRUE;
			cfile->issue_counter++;
			mapSet(class, expr, "spawning");
			token = next_token(&val, NULL, cfile);
			if (token != WITH)
				parse_error(cfile,
					    "expecting with after spawn");
		submatch:
			if (submatchedonce)
				parse_error(cfile,
					    "can't override existing "
					    "submatch/spawn");
			submatchedonce = ISC_TRUE;
			expr = createMap();
			if (!parse_data_expression(expr, cfile, &lose)) {
				if (!lose)
					parse_error(cfile,
						    "expecting data expr.");
			} else {
				expr->skip = ISC_TRUE;
				cfile->issue_counter++;
				mapSet(class, expr, "submatch");
				parse_semi(cfile);
			}
		} else if (token == LEASE) {
			struct comment *comment;

			skip_token(&val, NULL, cfile);
			token = next_token(&val, NULL, cfile);
			if (token != LIMIT)
				parse_error(cfile, "expecting \"limit\"");
			token = next_token(&val, NULL, cfile);
			if (token != NUMBER)
				parse_error(cfile, "expecting a number");
			tmp = createInt(atoll(val));
			tmp->skip = ISC_TRUE;
			cfile->issue_counter++;
			comment = createComment("/// Per-class limit is not "
						"supported by Kea");
			TAILQ_INSERT_TAIL(&tmp->comments, comment);
			comment = createComment("/// Reference Kea #237");
			TAILQ_INSERT_TAIL(&tmp->comments, comment);
			mapSet(class, tmp, "lease-limit");
			parse_semi(cfile);
		} else
			declaration = parse_statement(cfile, CLASS_DECL,
						      declaration);
	}

	cfile->stack_top--;

	if (type == CLASS_TYPE_SUBCLASS)
		subclass_inherit(cfile, class, copy(pc));
}

/*
 * Inherit entries:
 *  - first copy entries from the current superclass to the subclass
 *  - second try to reduce the subclass matching condition
 */

static void
subclass_inherit(struct parse *cfile,
		 struct element *class,
		 struct element *superclass)
{
	struct string *name;
	struct element *guard;
	struct element *submatch;
	struct handle *handle;
	struct string *gmsg;
	struct string *mmsg;
	struct string *dmsg;
	struct element *expr;
	struct element *data;
	struct element *match;
	struct element *reduced;
	unsigned order = 0;
	struct comment *comment;
	isc_boolean_t marked = ISC_FALSE;
	isc_boolean_t lose = ISC_FALSE;
	isc_boolean_t modified = ISC_FALSE;

	expr = mapGet(superclass, "name");
	if (expr == NULL)
		parse_error(cfile, "can't get superclass name");
	name = stringValue(expr);
	guard = mapGet(superclass, "match-if");
	submatch = mapGet(superclass, "submatch");
	if (submatch == NULL)
		parse_error(cfile, "can't get superclass submatch");

	/* Iterates on (copy of) superclass entries */
	while (mapSize(superclass) > 0) {
		handle = mapPop(superclass);
		if ((handle == NULL) || (handle->key == NULL) ||
		    (handle->value == NULL))
			parse_error(cfile, "can't get superclass %s item at "
				    "%u", name->content, order);
		handle->order = order++;
		/* Superclass specific entries */
		if ((strcmp(handle->key, "name") == 0) ||
		    (strcmp(handle->key, "spawning") == 0) ||
		    (strcmp(handle->key, "match-if") == 0) ||
		    (strcmp(handle->key, "test") == 0) ||
		    (strcmp(handle->key, "submatch") == 0))
			continue;
		/* Subclass specific so impossible entries */
		if ((strcmp(handle->key, "super") == 0) ||
		    (strcmp(handle->key, "binary") == 0) ||
		    (strcmp(handle->key, "string") == 0))
			parse_error(cfile, "superclass %s has unexpected %s "
				    "at %u",
				    name->content, handle->key, order);
		/* Special entries */
		if (strcmp(handle->key, "option-data") == 0) {
			struct element *opt_list;

			opt_list = mapGet(class, handle->key);
			if (opt_list != NULL)
				merge_option_data(handle->value, opt_list);
			else
				mapSet(class, handle->value, handle->key);
			continue;
		}
		/* Just copy */
		if ((strcmp(handle->key, "lease-limit") == 0) ||
		    (strcmp(handle->key, "boot-file-name") == 0) ||
		    (strcmp(handle->key, "serverhostname") == 0) ||
		    (strcmp(handle->key, "next-server") == 0)) {
			mapSet(class, handle->value, handle->key);
			continue;
		}
		/* Unknown */
		if (!marked) {
			marked = ISC_TRUE;
			comment = createComment("/// copied from superclass");
			TAILQ_INSERT_TAIL(&handle->value->comments, comment);
		}
		comment = createComment("/// unhandled entry");
		TAILQ_INSERT_TAIL(&handle->value->comments, comment);
		if (!handle->value->skip) {
			handle->value->skip = ISC_TRUE;
			cfile->issue_counter++;
		}
		mapSet(class, handle->value, handle->key);
	}

	/* build [guard and] submatch = data */
	expr = mapGet(class, "binary");
	if (expr != NULL) {
		data = createMap();
		mapSet(data, copy(expr), "const-data");
	} else
		data = mapGet(class, "string");
	if (data == NULL)
		parse_error(cfile, "can't get subclass %s data",
			    name->content);
	match = createMap();
	mapSet(match, copy(submatch), "left");
	mapSet(match, copy(data), "right");
	expr = createMap();
	mapSet(expr, match, "equal");

	if (guard != NULL) {
		match = createMap();
		mapSet(match, copy(guard), "left");
		mapSet(match, expr, "right");
		expr = createMap();
		mapSet(expr, match, "and");

		gmsg = makeString(-1, "/// from: match-if ");
		appendString(gmsg, print_boolean_expression(guard, &lose));
		mmsg = makeString(-1, "/// match: ");
	} else {
		gmsg = NULL;
		mmsg = makeString(-1, "/// from: match ");
	}

	appendString(mmsg, print_data_expression(submatch, &lose));
	dmsg = makeString(-1, "/// data: ");
	appendString(dmsg, print_data_expression(data, &lose));

	/* evaluate the expression and try to reduce it */
	reduced = eval_boolean_expression(expr, &modified);
	reduced = reduce_boolean_expression(reduced);
	if ((reduced != NULL) && (reduced->type == ELEMENT_BOOLEAN))
		parse_error(cfile, "class matching rule evaluated to a "
			    "constant boolean expression: %s = %s",
			    print_data_expression(submatch, &lose),
			    print_data_expression(data, &lose));
	if ((reduced == NULL) || (reduced->type != ELEMENT_STRING))
		return;
	if (!lose) {
		if (gmsg != NULL) {
			comment = createComment(gmsg->content);
			TAILQ_INSERT_TAIL(&reduced->comments, comment);
		}
		comment = createComment(mmsg->content);
		TAILQ_INSERT_TAIL(&reduced->comments, comment);
		comment = createComment(dmsg->content);
		TAILQ_INSERT_TAIL(&reduced->comments, comment);
	}
	mapSet(class, reduced, "test");
}

/*
 * Try to reduce a match-if condition into a Kea evaluate bool "test"
 */

static void
add_match_class(struct parse *cfile,
		struct element *class,
		struct element *expr)
{
	struct element *reduced;
	isc_boolean_t modified = ISC_FALSE;
	isc_boolean_t lose = ISC_FALSE;

	/* evaluate the expression and try to reduce it */
	reduced = eval_boolean_expression(expr, &modified);
	reduced = reduce_boolean_expression(reduced);
	if ((reduced != NULL) && (reduced->type == ELEMENT_BOOLEAN))
		parse_error(cfile, "'match if' with a constant boolean "
			    "expression %s",
			    print_boolean_expression(expr, &lose));
	if ((reduced != NULL) && (reduced->type == ELEMENT_STRING))
		mapSet(class, reduced, "test");
	else
		cfile->issue_counter++;
}

/* Move pools to subnets */

static void
relocate_pools(struct element *share)
{
	struct element *srcs;
	struct element *dsts;
	struct element *subnet;
	struct range *range;
	size_t i;

	srcs = mapGet(share, "pools");
	if (srcs == NULL)
		return;
	if (listSize(srcs) == 0)
		return;
	TAILQ_FOREACH(range, &known_ranges) {
		if (range->share != share)
			continue;
		subnet = find_location(share, range);
		if (subnet == NULL)
			continue;
		for (i = 0; i < listSize(srcs); i++) {
			struct element *pool;

			pool = listGet(srcs, i);
			if (range->pool != pool)
				continue;
			listRemove(srcs, i);
			dsts = mapGet(subnet, "pools");
			if (dsts == NULL) {
				dsts = createList();
				mapSet(subnet, dsts, "pools");
			}
			listPush(dsts, pool);
		}
	}
}

/* shared-network-declaration :==
			hostname LBRACE declarations parameters RBRACE */

void
parse_shared_net_declaration(struct parse *cfile)
{
	const char *val;
	enum dhcp_token token;
	struct element *share;
	struct element *subnets;
	struct element *interface;
	struct element *subnet;
	struct string *name;
	int declaration = 0;

	share = createMap();
	share->kind = SHARED_NET_DECL;
	TAILQ_CONCAT(&share->comments, &cfile->comments);

	/* Get the name of the shared network... */
	token = peek_token(&val, NULL, cfile);
	if (token == STRING) {
		skip_token(&val, NULL, cfile);

		if (val[0] == 0)
			parse_error(cfile, "zero-length shared network name");
		name = makeString(-1, val);
	} else {
		name = parse_host_name(cfile);
		if (!name)
			parse_error(cfile,
				    "expecting a name for shared-network");
	}
	mapSet(share, createString(name), "name");

	subnets = createList();
	mapSet(share, subnets,
	       local_family == AF_INET ? "subnet4" : "subnet6");

	parse_lbrace(cfile);

	stackPush(cfile, share);

	for (;;) {
		token = peek_token(&val, NULL, cfile);
		if (token == RBRACE) {
			skip_token(&val, NULL, cfile);
			break;
		} else if (token == END_OF_FILE) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "unexpected end of file");
		} else if (token == INTERFACE) {
			skip_token(&val, NULL, cfile);
			token = next_token(&val, NULL, cfile);
			if (mapContains(share, "interface"))
				parse_error(cfile,
					    "A shared network can't be "
					    "connected to two interfaces.");
			interface = createString(makeString(-1, val));
			mapSet(share, interface, "interface");
			new_network_interface(cfile, interface);
			parse_semi(cfile);
			continue;
		}

		declaration = parse_statement(cfile, SHARED_NET_DECL,
					      declaration);
	}

	cfile->stack_top--;

	if (listSize(subnets) == 0)
		parse_error(cfile, "empty shared-network decl");
	if (listSize(subnets) > 1) {
		struct element *shares;
		struct element *pools;

		shares = mapGet(cfile->stack[cfile->stack_top],
				"shared-networks");
		if (shares == NULL) {
			struct comment *comment;

			shares = createList();
			shares->kind = SHARED_NET_DECL;
			mapSet(cfile->stack[cfile->stack_top],
			       shares, "shared-networks");
			comment = createComment("/// Kea shared-networks "
						"are different, cf Kea #236");
			TAILQ_INSERT_TAIL(&shares->comments, comment);
		}
		listPush(shares, share);

		/* Pools are forbidden at shared-network level in Kea */
		relocate_pools(share);
		pools = mapGet(share, "pools");
		if ((pools != NULL) && (listSize(pools) == 0)) {
			mapRemove(share, "pools");
			pools = NULL;
		}
		if (pools != NULL) {
			struct comment *comment;

			pools->skip = ISC_TRUE;
			cfile->issue_counter++;
			comment = createComment("/// Kea pools must be "
						"in a subnet");
			TAILQ_INSERT_TAIL(&pools->comments, comment);
			comment = createComment("/// Reference Kea #249");
			TAILQ_INSERT_TAIL(&pools->comments, comment);
		}
		pools = mapGet(share, "pd-pools");
		if ((pools != NULL) && (listSize(pools) == 0)) {
			mapRemove(share, "pd-pools");
			pools = NULL;
		}
		if (pools != NULL) {
			struct comment *comment;

			pools->skip = ISC_TRUE;
			cfile->issue_counter++;
			comment = createComment("/// Kea pools must be "
						"in a subnet");
			TAILQ_INSERT_TAIL(&pools->comments, comment);
			comment = createComment("/// Reference Kea #249");
			TAILQ_INSERT_TAIL(&pools->comments, comment);
		}
		return;
	}

	/* There is one subnet so the shared network is useless */
	subnet = listGet(subnets, 0);
	listRemove(subnets, 0);
	mapRemove(share, "name");
	mapRemove(share, local_family == AF_INET ? "subnet4" : "subnet6");
	/* specific case before calling generic merge */
	if (mapContains(share, "pools") &&
	    mapContains(subnet, "pools")) {
		struct element *pools;
		struct element *sub;

		pools = mapGet(share, "pools");
		mapRemove(share, "pools");
		sub = mapGet(subnet, "pools");
		concat(sub, pools);
	}
	if (mapContains(share, "pd-pools") &&
	    mapContains(subnet, "pd-pools")) {
		struct element *pools;
		struct element *sub;

		pools = mapGet(share, "pd-pools");
		mapRemove(share, "pd-pools");
		sub = mapGet(subnet, "pd-pools");
		concat(sub, pools);
	}
	if (mapContains(share, "option-data") &&
	    mapContains(subnet, "option-data")) {
		struct element *opt_list;
		struct element *sub;

		opt_list = mapGet(share, "option-data");
		mapRemove(share, "option-data");
		sub = mapGet(subnet, "option-data");
		merge_option_data(opt_list, sub);
	}
	merge(subnet, share);

	if (local_family == AF_INET) {
		subnets = mapGet(cfile->stack[1], "subnet4");
		if (subnets == NULL) {
			subnets = createList();
			subnets->kind = SUBNET_DECL;
			mapSet(cfile->stack[1], subnets, "subnet4");
		}
	} else {
		subnets = mapGet(cfile->stack[1], "subnet6");
		if (subnets == NULL) {
			subnets = createList();
			subnets->kind = SUBNET_DECL;
			mapSet(cfile->stack[1], subnets, "subnet6");
		}
	}
	listPush(subnets, subnet);
}

static void
common_subnet_parsing(struct parse *cfile,
		      struct element *subnets,
		      struct element *subnet)
{
	enum dhcp_token token;
	const char *val;
	struct element *interface;
	int declaration = 0;

	parse_lbrace(cfile);

	stackPush(cfile, subnet);

	for (;;) {
		token = peek_token(&val, NULL, cfile);
		if (token == RBRACE) {
			skip_token(&val, NULL, cfile);
			break;
		} else if (token == END_OF_FILE) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "unexpected end of file");
			break;
		} else if (token == INTERFACE) {
			skip_token(&val, NULL, cfile);
			token = next_token(&val, NULL, cfile);
			if (mapContains(subnet, "interface"))
				parse_error(cfile,
					    "A subnet can't be connected "
					    "to two interfaces.");
			interface = createString(makeString(-1, val));
			mapSet(subnet, interface, "interface");
			new_network_interface(cfile, interface);
			parse_semi(cfile);
			continue;
		}
		declaration = parse_statement(cfile, SUBNET_DECL, declaration);
	}

	cfile->stack_top--;

	/* Add the subnet to the list of subnets in this shared net. */
	listPush(subnets, subnet);

	return;
}

/* subnet-declaration :==
	net NETMASK netmask RBRACE parameters declarations LBRACE */

void
parse_subnet_declaration(struct parse *cfile)
{
	const char *val;
	enum dhcp_token token;
	struct element *subnet;
	struct subnet *chain;
	struct element *subnets;
	struct string *address;
	struct string *netmask;
	struct string *prefix;
	unsigned char addr[4];
	unsigned len = sizeof(addr);
	size_t parent = 0;
	size_t i;
	int kind = 0;

	subnet = createMap();
	subnet->kind = SUBNET_DECL;
	TAILQ_CONCAT(&subnet->comments, &cfile->comments);

	subnet_counter++;
	mapSet(subnet, createInt(subnet_counter), "id");

	chain = (struct subnet *)malloc(sizeof(*chain));
	if (chain == NULL)
		parse_error(cfile, "can't allocate subnet");
	memset(chain, 0, sizeof(*chain));
	chain->subnet = subnet;
	TAILQ_INSERT_TAIL(&known_subnets, chain);

	/* Find parent */
	for (i = cfile->stack_top; i > 0; --i) {
		kind = cfile->stack[i]->kind;
		if ((kind == SHARED_NET_DECL) ||
		    (kind == GROUP_DECL) ||
		    (kind == ROOT_GROUP)) {
			parent = i;
			break;
		}
	}
	if (kind == 0)
		parse_error(cfile, "can't find a place to put subnet");
	if (kind == SHARED_NET_DECL)
		chain->share = cfile->stack[parent];
	subnets = mapGet(cfile->stack[parent], "subnet4");
	if (subnets == NULL) {
		if (kind == SHARED_NET_DECL)
			parse_error(cfile, "shared network without subnets");
		subnets = createList();
		subnets->kind = SUBNET_DECL;
		mapSet(cfile->stack[parent], subnets, "subnet4");
	}

	/* Get the network number... */
	address = parse_numeric_aggregate(cfile, addr, &len, DOT, 10, 8);
	if (address == NULL)
		parse_error(cfile, "can't decode network number");
	if (address->length != 4)
		parse_error(cfile, "bad IPv4 address length");
	chain->addr = address;

	token = next_token(&val, NULL, cfile);
	if (token != NETMASK)
		parse_error(cfile, "Expecting netmask");

	/* Get the netmask... */
	netmask = parse_numeric_aggregate(cfile, addr, &len, DOT, 10, 8);
	if (netmask == NULL)
		parse_error(cfile, "can't decode network mask");
	if (netmask->length != address->length)
		parse_error(cfile, "bad IPv4 mask length");
	chain->mask = netmask;

	prefix = addrmask(address, netmask);
	if (prefix == NULL) {
		char bufa[INET_ADDRSTRLEN];
		char bufm[INET_ADDRSTRLEN];

		inet_ntop(AF_INET, address->content, bufa, INET_ADDRSTRLEN);
		inet_ntop(AF_INET, netmask->content, bufm, INET_ADDRSTRLEN);
		parse_error(cfile, "can't get a prefix from %s mask %s",
			    bufa, bufm);
	}
	mapSet(subnet, createString(prefix), "subnet");

	common_subnet_parsing(cfile, subnets, subnet);
}

/* subnet6-declaration :==
	net / bits RBRACE parameters declarations LBRACE */

void
parse_subnet6_declaration(struct parse *cfile)
{
	enum dhcp_token token;
	const char *val;
	struct element *subnet;
	struct subnet *chain;
	struct element *subnets;
	struct string *address;
	struct string *prefix;
	struct string *netmask;
	size_t parent = 0;
        size_t i;
        int kind = 0;
	char *p;

        if (local_family != AF_INET6)
                parse_error(cfile, "subnet6 statement is only supported "
				   "in DHCPv6 mode.");

	subnet = createMap();
	subnet->kind = SUBNET_DECL;
	TAILQ_CONCAT(&subnet->comments, &cfile->comments);

	subnet_counter++;
	mapSet(subnet, createInt(subnet_counter), "id");

	chain = (struct subnet *)malloc(sizeof(*chain));
	if (chain == NULL)
		parse_error(cfile, "can't allocate subnet");
	memset(chain, 0, sizeof(*chain));
	chain->subnet = subnet;
	TAILQ_INSERT_TAIL(&known_subnets, chain);

	/* Find parent */
	for (i = cfile->stack_top; i > 0; --i) {
		kind = cfile->stack[i]->kind;
		if ((kind == SHARED_NET_DECL) ||
		    (kind == GROUP_DECL) ||
		    (kind == ROOT_GROUP)) {
			parent = i;
			break;
		}
	}
	if (kind == 0)
		parse_error(cfile, "can't find a place to put subnet");
	if (kind == SHARED_NET_DECL)
		chain->share = cfile->stack[parent];
	subnets = mapGet(cfile->stack[parent], "subnet6");
	if (subnets == NULL) {
		if (kind == SHARED_NET_DECL)
			parse_error(cfile, "shared network without subnets");
		subnets = createList();
		subnets->kind = SUBNET_DECL;
		mapSet(cfile->stack[parent], subnets, "subnet6");
	}

	address = parse_ip6_addr(cfile);
	if (address == NULL)
		parse_error(cfile, "can't decode network number");
	if (address->length != 16)
		parse_error(cfile, "bad IPv6 address length");
	chain->addr = address;

	prefix = makeStringExt(address->length, address->content, '6');

	token = next_token(&val, NULL, cfile);
	if (token != SLASH)
		parse_error(cfile, "Expecting a '/'.");
	appendString(prefix, val);

	token = next_token(&val, NULL, cfile);
	if (token != NUMBER)
		parse_error(cfile, "Expecting a number.");
	appendString(prefix, val);

	netmask = makeString(16, "0123456789abcdef");
	memset(netmask->content, 0, 16);
	p = netmask->content;
	for (i = atoi(val); i >= 8; i -= 8)
		*p++ = 0xff;
	*p = 0xff << (8 - i);
	chain->mask = netmask;

	mapSet(subnet, createString(prefix), "subnet");

	common_subnet_parsing(cfile, subnets, subnet);
}

/* group-declaration :== RBRACE parameters declarations LBRACE */

void
parse_group_declaration(struct parse *cfile)
{
	const char *val;
	enum dhcp_token token;
	struct element *group;
	int declaration = 0;
	struct string *name = NULL;

	if (mapContains(cfile->stack[cfile->stack_top], "group"))
		parse_error(cfile, "another group is already open");
	group = createMap();
	group->skip = ISC_TRUE;
	group->kind = GROUP_DECL;
	TAILQ_CONCAT(&group->comments, &cfile->comments);
	mapSet(cfile->stack[cfile->stack_top], group, "group");

	token = peek_token(&val, NULL, cfile);
	if (is_identifier(token) || token == STRING) {
		skip_token(&val, NULL, cfile);

		name = makeString(-1, val);
		if (!name)
			parse_error(cfile, "no memory for group decl name %s",
				    val);
	}

	parse_lbrace(cfile);

	stackPush(cfile, group);

	for (;;) {
		token = peek_token(&val, NULL, cfile);
		if (token == RBRACE) {
			skip_token(&val, NULL, cfile);
			break;
		} else if (token == END_OF_FILE) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "unexpected end of file");
			break;
		} else if (token == TOKEN_DELETED) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "deleted groups don't exist "
				    "in the config file");
		} else if (token == DYNAMIC) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "dynamic groups don't exist "
				    "in the config file");
		} else if (token == STATIC) {
			skip_token(&val, NULL, cfile);
			parse_error(cfile, "static groups don't exist "
				    "in the config file");
		}
		declaration = parse_statement(cfile, GROUP_DECL, declaration);
	}

	cfile->stack_top--;

	if (name != NULL)
		mapSet(group, createString(name), "name");
	close_group(cfile, group);
}

/*
 * Close a group. Called when a group is closed.
 *  - spread parameters to children
 *  - attach declarations at an upper level
 */

void
close_group(struct parse *cfile, struct element *group)
{
	struct handle *handle;
	struct handle *nh;
	struct element *parent;
	struct element *item;
	struct element *param;
	struct handle *hosts = NULL;
	struct handle *shares = NULL;
	struct handle *subnets = NULL;
	struct handle *classes = NULL;
	struct handle *pdpools = NULL;
	struct handle *pools = NULL;
	struct handles downs;
	struct comment *comment;
	const char *key = NULL;
	const char *name = NULL;
	unsigned order = 0;
	isc_boolean_t marked = ISC_FALSE;

	TAILQ_INIT(&downs);

	/* check that group is in its parent */
	parent = cfile->stack[cfile->stack_top];
	if (parent->kind == PARAMETER)
		parse_error(cfile, "unexpected kind for group parent %d",
			    parent->kind);
	item = mapGet(parent, "group");
	if (item == NULL)
		parse_error(cfile, "no group in parent");
	if (item != group)
		parse_error(cfile, "got a different group from parent");
	mapRemove(parent, "group");

	/* classify content */
	while (mapSize(group) > 0) {
		handle = mapPop(group);
		if ((handle == NULL) || (handle->key == NULL) ||
		    (handle->value == NULL))
		    parse_error(cfile, "can't get group item at %u",
				order);
		handle->order = order++;
		switch (handle->value->kind) {
		case TOPLEVEL:
		case ROOT_GROUP:
		case GROUP_DECL:
		badkind:
			parse_error(cfile, "impossible group item (kind %d) "
				    "for %s at order %u",
				    handle->value->kind, handle->key, order);

		case HOST_DECL:
			if (strcmp(handle->key, "reservations") != 0)
				parse_error(cfile, "expected reservations "
					    "got %s at %u",
					    handle->key, order);
			if (hosts != NULL)
				parse_error(cfile, "got reservations twice "
					    "at %u and %u",
					    hosts->order, order);
			if ((parent->kind == HOST_DECL) ||
			    (parent->kind == CLASS_DECL))
				parse_error(cfile, "host declarations not "
					    "allowed here.");
			hosts = handle;
			handle = NULL;
			break;

		case SHARED_NET_DECL:
			if (strcmp(handle->key, "shared-networks") != 0)
				parse_error(cfile, "expected shared-networks "
					    "got %s at %u",
					    handle->key, order);
			if ((parent->kind == SHARED_NET_DECL) ||
			    (parent->kind == HOST_DECL) ||
			    (parent->kind == SUBNET_DECL) ||
			    (parent->kind == CLASS_DECL))
				parse_error(cfile, "shared-network parameters "
					    "not allowed here.");
			shares = handle;
			handle = NULL;
			break;

		case SUBNET_DECL:
			key = local_family == AF_INET ? "subnet4" : "subnet6";
			if (strcmp(handle->key, key) != 0)
				parse_error(cfile, "expected %s got %s at %u",
					    key, handle->key, order);
			if (subnets != NULL)
				parse_error(cfile, "got %s twice at %u and %u",
					    key, subnets->order, order);
			if ((parent->kind == HOST_DECL) ||
			    (parent->kind == SUBNET_DECL) ||
			    (parent->kind == CLASS_DECL))
				parse_error(cfile, "subnet declarations not "
					    "allowed here.");
			subnets = handle;
			handle = NULL;
			break;

		case CLASS_DECL:
			if (strcmp(handle->key, "client-classes") != 0)
				parse_error(cfile, "expected client-classes "
					    "got %s at %u",
					    handle->key, order);
			if (classes != NULL)
				parse_error(cfile, "got %s twice at %u and %u",
					    key, classes->order, order);
			if (parent->kind == CLASS_DECL)
				parse_error(cfile, "class declarations not "
					    "allowed here.");
			classes = handle;
			handle = NULL;
                        break;

		case POOL_DECL:
			if (strcmp(handle->key, "pd-pools") == 0) {
                                if (pdpools != NULL)
                                        parse_error(cfile, "got pd-pools "
						    "twice at %u and %u",
                                                    pdpools->order, order);
                                pdpools = handle;
			} else if (strcmp(handle->key, "pools") == 0) {
				if (pools != NULL)
					parse_error(cfile, "got pools twice "
						    "at %u and %u",
						    pools->order, order);
				pools = handle;
			} else
				parse_error(cfile, "expecyed [pd-]pools got "
					    "%s at %u",
					    handle->key, order);
			if (parent->kind == POOL_DECL)
				parse_error(cfile, "pool declared within "
					    "pool.");
			if ((parent->kind == HOST_DECL) ||
			    (parent->kind == CLASS_DECL))
				parse_error(cfile, "pool declared outside "
					    "of network");
			handle = NULL;
			break;
		default:
			if (handle->value->kind != PARAMETER)
				goto badkind;
		}
		if (handle == NULL)
			continue;

		/* we have a parameter */
		param = handle->value;
		/* group name */
		if (strcmp(handle->key, "name") == 0) {
			name = stringValue(param)->content;
			continue;
		}
		/* unexpected values */
		if ((strcmp(handle->key, "reservations") == 0) ||
		    (strcmp(handle->key, "group") == 0) ||
		    (strcmp(handle->key, "shared-networks") == 0) ||
		    (strcmp(handle->key, "subnet4") == 0) ||
		    (strcmp(handle->key, "subnet6") == 0) ||
		    (strcmp(handle->key, "subnet") == 0) ||
		    (strcmp(handle->key, "client-classes") == 0) ||
		    (strcmp(handle->key, "hw-address") == 0) ||
		    (strcmp(handle->key, "ip-address") == 0) ||
		    (strcmp(handle->key, "extra-ip-addresses") == 0) ||
		    (strcmp(handle->key, "ip-addresses") == 0) ||
		    (strcmp(handle->key, "prefixes") == 0) ||
		    (strcmp(handle->key, "pool") == 0) ||
		    (strcmp(handle->key, "prefix") == 0) ||
		    (strcmp(handle->key, "delegated-len") == 0) ||
		    (strcmp(handle->key, "prefix-len") == 0) ||
		    (strcmp(handle->key, "prefix-highest") == 0) ||
		    (strcmp(handle->key, "option-def") == 0) ||
		    (strcmp(handle->key, "hostname") == 0) ||
		    (strcmp(handle->key, "client-id") == 0) ||
		    (strcmp(handle->key, "host-identifier") == 0) ||
		    (strcmp(handle->key, "flex-id") == 0) ||
		    (strcmp(handle->key, "test") == 0) ||
		    (strcmp(handle->key, "authoritative") == 0) ||
		    (strcmp(handle->key, "dhcp-ddns") == 0) ||
		    (strcmp(handle->key, "host-reservation-identifiers") == 0))
			parse_error(cfile, "unexpected parameter %s "
				    "in group at %u",
				    handle->key, order);

		/* to parent at group position */
		if ((strcmp(handle->key, "option-space") == 0) ||
		    (strcmp(handle->key, "server-duid") == 0) ||
		    (strcmp(handle->key, "statement") == 0) ||
		    (strcmp(handle->key, "config") == 0) ||
		    (strcmp(handle->key, "ddns-update-style") == 0) ||
		    (strcmp(handle->key, "echo-client-id") == 0)) {
			if (!marked) {
				struct string *msg;

				marked = ISC_TRUE;
				msg = makeString(-1, "/// moved from group");
				if (name != NULL)
					appendString(msg, " ");
				appendString(msg, name);
				comment = createComment(msg->content);
				TAILQ_INSERT_TAIL(&param->comments, comment);
			}
			mapSet(parent, param, handle->key);
			free(handle);
			continue;
		}
		/* To reconsider: qualifying-suffix, enable-updates */
		if ((strcmp(handle->key, "option-data") == 0) ||
		    (strcmp(handle->key, "allow") == 0) ||
		    (strcmp(handle->key, "deny") == 0) ||
		    (strcmp(handle->key, "interface") == 0) ||
		    (strcmp(handle->key, "valid-lifetime") == 0) ||
		    (strcmp(handle->key, "preferred-lifetime") == 0) ||
		    (strcmp(handle->key, "renew-timer") == 0) ||
		    (strcmp(handle->key, "rebind-timer") == 0) ||
		    (strcmp(handle->key, "boot-file-name") == 0) ||
		    (strcmp(handle->key, "server-hostname") == 0) ||
		    (strcmp(handle->key, "next-server") == 0) ||
		    (strcmp(handle->key, "match-client-id") == 0)) {
			TAILQ_INSERT_TAIL(&downs, handle);
			continue;
		}
		/* unknown */
		if (!marked) {
			struct string *msg;

			marked = ISC_TRUE;
			msg = makeString(-1, "/// moved from group");
			if (name != NULL)
				appendString(msg, " ");
			appendString(msg, name);
			comment = createComment(msg->content);
			TAILQ_INSERT_TAIL(&param->comments, comment);
		}
		comment = createComment("/// unhandled parameter");
		TAILQ_INSERT_TAIL(&param->comments, comment);
		param->skip = ISC_TRUE;
		cfile->issue_counter++;
		mapSet(parent, param, handle->key);
		free(handle);
	}
	TAILQ_FOREACH_SAFE(handle, &downs, nh) {
		if (strcmp(handle->key, "option-data") == 0) {
			option_data_derive(cfile, handle, hosts);
			option_data_derive(cfile, handle, shares);
			option_data_derive(cfile, handle, subnets);
			derive_classes(cfile, handle, classes);
			option_data_derive(cfile, handle, pdpools);
			option_data_derive(cfile, handle, pools);
		} else if ((strcmp(handle->key, "allow") == 0) ||
			   (strcmp(handle->key, "deny") == 0)) {
			derive(handle, pdpools);
			derive(handle, pools);
		} else if ((strcmp(handle->key, "interface") == 0) ||
			   (strcmp(handle->key, "valid-lifetime") == 0) ||
			   (strcmp(handle->key, "preferred-lifetime") == 0) ||
			   (strcmp(handle->key, "renew-timer") == 0) ||
			   (strcmp(handle->key, "rebind-timer") == 0) ||
			   (strcmp(handle->key, "match-client-id") == 0)) {
			derive(handle, shares);
			derive(handle, subnets);
		} else if ((strcmp(handle->key, "boot-file-name") == 0) ||
			   (strcmp(handle->key, "server-hostname") == 0)) {
			derive(handle, hosts);
			derive_classes(cfile, handle, classes);
		} else if (strcmp(handle->key, "next-server") == 0) {
			derive(handle, hosts);
			derive(handle, subnets);
			derive_classes(cfile, handle, classes);
		} else
			parse_error(cfile, "unexpected parameter %s to derive",
				    handle->key);
	}
	if (hosts != NULL) {
		struct element *root;

		root = mapGet(cfile->stack[1], "reservations");
		if (root == NULL)
			mapSet(cfile->stack[1], hosts->value, "reservations");
		else
			concat(root, hosts->value);
	}
	if (shares != NULL) {
		struct element *upper;

		upper = mapGet(parent, "shared-networks");
		if (upper == NULL)
			mapSet(parent, shares->value, "shared-networks");
		else
			concat(upper, shares->value);
	}
	key = local_family == AF_INET ? "subnet4" : "subnet6";
	if (subnets != NULL) {
		struct element *upper;

		upper = mapGet(parent, key);
		if (upper == NULL)
			mapSet(parent, subnets->value, key);
		else
			concat(upper, subnets->value);
	}
	if (classes != NULL) {
		struct element *upper;
		size_t where;
		int kind = 0;

		for (where = cfile->stack_top; where > 0; --where) {
			kind = cfile->stack[where]->kind;
			if ((kind == GROUP_DECL) || (kind == ROOT_GROUP))
				break;
		}
		if (kind == GROUP_DECL) {
			upper = mapGet(cfile->stack[where], "client-classes");
			if (upper == NULL)
				mapSet(cfile->stack[where],
				       classes->value,
				       "client-classes");
			else
				concat_classes(cfile, upper, classes->value);
		}
	}
	if (pdpools != NULL) {
		struct element *upper;

		upper = mapGet(parent, "pd-pools");
		if (upper == NULL)
                        mapSet(parent, pdpools->value, "pools");
                else
                        concat(upper, pdpools->value);
	}
	if (pools != NULL) {
		struct element *upper;

		upper = mapGet(parent, "pools");
		if (upper == NULL)
                        mapSet(parent, pools->value, "pools");
                else
                        concat(upper, pools->value);
	}
}

/*
 * Specialized derivation routine for option-data
 * (options are identified by space + name and/or code
 */

static void
option_data_derive(struct parse *cfile, struct handle *src, struct handle *dst)
{
	struct element *list;
	struct element *item;
	struct element *opt_list;
	size_t i;

	if (dst == NULL)
		return;
	list = dst->value;
	assert(list != NULL);
	assert(list->type == ELEMENT_LIST);
	for (i = 0; i < listSize(list); i++) {
		item = listGet(list, i);
		assert(item != NULL);
		assert(item->type == ELEMENT_MAP);
		opt_list = mapGet(item, src->key);
		if (opt_list != NULL) {
			merge_option_data(src->value, opt_list);
			continue;
		}
		opt_list = copy(src->value);
		mapSet(item, opt_list, src->key);
	}
}

/*
 * Specialized derivation routine for classes
 * (which are by reference so a resolution step is needed)
 */
static void
derive_classes(struct parse *cfile, struct handle *src, struct handle *dst)
{
	struct element *list;
	struct element *item;
	size_t i;

	if (dst == NULL)
		return;
	list = dst->value;
	assert(list != NULL);
	assert(list->type == ELEMENT_LIST);
	for (i = 0; i < listSize(list); i++) {
		item = listGet(list, i);
		assert(item != NULL);
		assert(item->type == ELEMENT_MAP);
		item = get_class(cfile, item);
		if (item == NULL)
			parse_error(cfile, "dangling class reference");
		if (strcmp(src->key, "option-data") == 0) {
			struct element *opt_list;

			opt_list = mapGet(item, "option-data");
			if (opt_list != NULL)
				merge_option_data(src->value, opt_list);
			else
				mapSet(item, copy(src->value), "option-data");
			continue;
		}
		if (mapContains(item, src->key))
			continue;
		mapSet(item, copy(src->value), src->key);
	}
}

/* fixed-addr-parameter :== ip-addrs-or-hostnames SEMI
   ip-addrs-or-hostnames :== ip-addr-or-hostname
			   | ip-addrs-or-hostnames ip-addr-or-hostname */

struct element *
parse_fixed_addr_param(struct parse *cfile, enum dhcp_token type) {
	const char *val;
	enum dhcp_token token;
	struct element *addr;
	struct element *addresses;
	struct string *address;

	addresses = createList();
	TAILQ_CONCAT(&addresses->comments, &cfile->comments);

	do {
		address = NULL;
		if (type == FIXED_ADDR)
			address = parse_ip_addr_or_hostname(cfile, ISC_TRUE);
		else if (type == FIXED_ADDR6)
			address = parse_ip6_addr_txt(cfile);
		else
			parse_error(cfile, "requires FIXED_ADDR[6]");
		if (address == NULL)
			parse_error(cfile, "can't parse fixed address");
		addr = createString(address);
		/* Take the comment for resolution into multiple addresses */
		TAILQ_CONCAT(&addr->comments, &cfile->comments);
		listPush(addresses, addr);
		token = peek_token(&val, NULL, cfile);
		if (token == COMMA)
			token = next_token(&val, NULL, cfile);
	} while (token == COMMA);

	parse_semi(cfile);

	/* Sanity */
	if (listSize(addresses) == 0)
		parse_error(cfile, "can't get fixed address");

	return addresses;

}

#ifdef notyet
/* Parse the right side of a 'binding value'.
 *
 * set foo = "bar"; is a string
 * set foo = false; is a boolean
 * set foo = %31; is a numeric value.
 */
static struct element *
parse_binding_value(struct parse *cfile)
{
	struct element *value = NULL;
	struct string *data;
	const char *val;
	unsigned buflen;
	int token;

	token = peek_token(&val, NULL, cfile);
	if (token == STRING) {
		skip_token(&val, &buflen, cfile);
		data = makeString(buflen, val);
		value = createString(data);
	} else if (token == NUMBER_OR_NAME) {
		value = createMap();
		data = parse_hexa(cfile);
		mapSet(value, createHexa(data), "const-data");
	} else if (token == PERCENT) {
		skip_token(&val, NULL, cfile);
		token = next_token(&val, NULL, cfile);
		if (token != NUMBER)
			parse_error(cfile, "expecting decimal number.");
		value = createInt(atol(val));
	} else if (token == NAME) {
		token = next_token(&val, NULL, cfile);
		if (!strcasecmp(val, "true"))
			value = createBool(ISC_TRUE);
		else if (!strcasecmp(val, "false"))
			value = createBool(ISC_FALSE);
		else
			parse_error(cfile, "expecting true or false");
	} else
		parse_error(cfile, "expecting a constant value.");

	return value;
}
#endif

/* address-range-declaration :== ip-address ip-address SEMI
			       | DYNAMIC_BOOTP ip-address ip-address SEMI */

void
parse_address_range(struct parse *cfile, int type, size_t where)
{
	struct string *low, *high, *range;
	unsigned char addr[4];
	unsigned len = sizeof(addr);
	enum dhcp_token token;
	const char *val;
	struct element *pool;
	struct element *r;
	struct range *chain;
	size_t i;
	int kind;

	if ((token = peek_token(&val, NULL, cfile)) == DYNAMIC_BOOTP) {
		skip_token(&val, NULL, cfile);
	}

	/* Get the bottom address in the range... */
	low = parse_numeric_aggregate(cfile, addr, &len, DOT, 10, 8);
	if (low == NULL)
		parse_error(cfile, "can't parse range (low)");

	/* Only one address? */
	token = peek_token(&val, NULL, cfile);
	if (token == SEMI)
		high = low;
	else {
		/* Get the top address in the range... */
		high = parse_numeric_aggregate(cfile, addr, &len, DOT, 10, 8);
		if (high ==  NULL)
			parse_error(cfile, "can't parse range (high)");
	}

	token = next_token(&val, NULL, cfile);
	if (token != SEMI)
		parse_error(cfile, "semicolon expected.");

	if (type != POOL_DECL) {
		struct element *group;
		struct element *pools;
#ifdef want_bootp
		struct element *permit;
#endif

		group = cfile->stack[where];
		pool = createMap();
#ifdef want_bootp
		permit = createList();
		permit->skip = ISC_TRUE;

		/* Dynamic pools permit all clients.   Otherwise
		   we prohibit BOOTP clients. */
		if (dynamic) {
			struct string *all;

			all = makeString(-1, "all clients");
			listPush(permit, createString(all));
			mapSet(pool, permit, "allow");
		} else {
			struct string *dyn_bootp;

			dyn_bootp = makeString(-1, "dynamic bootp clients");
			listPush(permit, createString(dyn_bootp));
			mapSet(pool, permit, "deny");
		}
#endif

		pools = mapGet(group, "pools");
		if (pools == NULL) {
			pools = createList();
			pools->kind = POOL_DECL;
			mapSet(group, pools, "pools");
		}
		listPush(pools, pool);
	} else
		pool = cfile->stack[where];

	/* Create the new address range... */
	if (memcmp(high->content, low->content, high->length) < 0) {
		struct string *swap;

		swap = low;
		low = high;
		high = swap;
	}
	range = makeStringExt(low->length, low->content, 'I');
	appendString(range, " - ");
	concatString(range, makeStringExt(high->length, high->content, 'I'));

	r = createString(range);
	TAILQ_CONCAT(&r->comments, &cfile->comments);

	mapSet(pool, r, "pool");

	chain = (struct range *)malloc(sizeof(*chain));
	if (chain == NULL)
		parse_error(cfile, "can't allocate range");
	memset(chain, 0, sizeof(*chain));
	chain->pool = pool;
	for (i = where; i > 0; --i) {
		kind = cfile->stack[i]->kind;
		if (kind == SHARED_NET_DECL) {
			chain->share = cfile->stack[i];
			break;
		}
	}
	chain->low = low;
	TAILQ_INSERT_TAIL(&known_ranges, chain);
}

/* address-range6-declaration :== ip-address6 ip-address6 SEMI
			       | ip-address6 SLASH number SEMI
			       | ip-address6 [SLASH number] TEMPORARY SEMI */

void
parse_address_range6(struct parse *cfile, int type, size_t where)
{
	struct string *low, *high, *range;
	enum dhcp_token token;
	const char *val;
	isc_boolean_t is_temporary = ISC_FALSE;
	struct element *pool;
	struct element *r;
	struct range *chain;
	size_t i;
	int kind;

        if (local_family != AF_INET6)
                parse_error(cfile, "range6 statement is only supported "
			    "in DHCPv6 mode.");

	/*
	 * Read starting address as text.
	 */
	low = parse_ip6_addr_txt(cfile);
	if (low == NULL)
		parse_error(cfile, "can't parse range6 address (low)");
	range = allocString();
	concatString(range, low);

	/*
	 * See if we we're using range or CIDR notation or TEMPORARY
	 */
	token = peek_token(&val, NULL, cfile);
	if (token == SLASH) {
		appendString(range, val);
		/*
		 * '/' means CIDR notation, so read the bits we want.
		 */
		skip_token(NULL, NULL, cfile);
		token = next_token(&val, NULL, cfile);
		if (token != NUMBER)
			parse_error(cfile, "expecting number");
		/*
		 * no sanity checks
		 */
		appendString(range, val);
		/*
		 * can be temporary (RFC 4941 like)
		 */
		token = peek_token(&val, NULL, cfile);
		if (token == TEMPORARY) {
			is_temporary = ISC_TRUE;
			appendString(range, " ");
			appendString(range, val);
			skip_token(NULL, NULL, cfile);
		}
	} else if (token == TEMPORARY) {
		/*
		 * temporary (RFC 4941)
		 */
		is_temporary = ISC_TRUE;
		appendString(range, "/64 ");
		appendString(range, val);
		skip_token(NULL, NULL, cfile);
	} else {
		/*
		 * No '/', so we are looking for the end address of
		 * the IPv6 pool.
		 */
		high = parse_ip6_addr_txt(cfile);
		if (high == NULL)
			parse_error(cfile,
				    "can't parse range6 address (high)");
		/* No sanity checks */
		appendString(range, " - ");
		appendString(range, high->content);
	}

	token = next_token(NULL, NULL, cfile);
	if (token != SEMI)
		parse_error(cfile, "semicolon expected.");

	if (type != POOL_DECL) {
		struct element *group;
		struct element *pools;

		group = cfile->stack[where];
		pool = createMap();
		pools = mapGet(group, "pools");
		if (pools == NULL) {
			pools = createList();
			pools->kind = POOL_DECL;
			mapSet(group, pools, "pools");
		}
		listPush(pools, pool);
	} else
		pool = cfile->stack[where];

	r = createString(range);
	TAILQ_CONCAT(&r->comments, &cfile->comments);
	if (is_temporary) {
		pool->skip = ISC_TRUE;
		cfile->issue_counter++;
	}
	mapSet(pool, r, "pool");

	chain = (struct range *)malloc(sizeof(*chain));
	if (chain == NULL)
		parse_error(cfile, "can't allocate range");
	memset(chain, 0, sizeof(*chain));
	chain->pool = pool;
	for (i = where; i > 0; --i) {
		kind = cfile->stack[i]->kind;
		if (kind == SHARED_NET_DECL) {
			chain->share = cfile->stack[i];
			break;
		}
	}
	chain->low = low;
	TAILQ_INSERT_TAIL(&known_ranges, chain);
}

/* prefix6-declaration :== ip-address6 ip-address6 SLASH number SEMI */

void
parse_prefix6(struct parse *cfile, int type, size_t where)
{
	struct string *lo, *hi;
	int plen;
	int bits;
	enum dhcp_token token;
	const char *val;
	struct element *pool;
	struct element *prefix;

	if (local_family != AF_INET6)
		parse_error(cfile, "prefix6 statement is only supported "
			    "in DHCPv6 mode.");

	/*
	 * Read starting and ending address as text.
	 */
	lo = parse_ip6_addr_txt(cfile);
	if (lo == NULL)
		parse_error(cfile, "can't parse prefix6 address (low)");

	hi = parse_ip6_addr_txt(cfile);
	if (hi == NULL)
		parse_error(cfile, "can't parse prefix6 address (high)");

	/*
	 * Next is '/' number ';'.
	 */
	token = next_token(NULL, NULL, cfile);
	if (token != SLASH)
		parse_error(cfile, "expecting '/'");
	token = next_token(&val, NULL, cfile);
	if (token != NUMBER)
		parse_error(cfile, "expecting number");
	bits = atoi(val);
	if ((bits <= 0) || (bits >= 128))
		parse_error(cfile, "networks have 0 to 128 bits (exclusive)");

	token = next_token(NULL, NULL, cfile);
	if (token != SEMI)
		parse_error(cfile, "semicolon expected.");

	if (type != POOL_DECL) {
		struct element *group;
		struct element *pools;

		group = cfile->stack[where];
		pool = createMap();
		pools = mapGet(group, "pd-pools");
		if (pools == NULL) {
			pools = createList();
			pools->kind = POOL_DECL;
			mapSet(group, pools, "pd-pools");
		}
		listPush(pools, pool);
	} else
		pool = cfile->stack[where];

	prefix = createString(lo);
	TAILQ_CONCAT(&prefix->comments, &cfile->comments);
	mapSet(pool, prefix, "prefix");
	mapSet(pool, createInt(bits), "delegated-len");
	plen = get_prefix_length(lo->content, hi->content);
	if (plen >= 0)
		mapSet(pool, createInt(plen), "prefix-len");
	else {
		if (!pool->skip)
			cfile->issue_counter++;
		pool->skip = ISC_TRUE;
		mapSet(pool, createString(hi), "prefix-highest");
	}
}

/* fixed-prefix6 :== ip6-address SLASH number SEMI */

void
parse_fixed_prefix6(struct parse *cfile, size_t host_decl)
{
	struct string *ia;
	enum dhcp_token token;
	const char *val;
	struct element *host;
	struct element *prefixes;
	struct element *prefix;

	if (local_family != AF_INET6)
		parse_error(cfile, "fixed-prefix6 statement is only "
			    "supported in DHCPv6 mode.");

	/*
	 * Get the fixed-prefix list.
	 */
	host = cfile->stack[host_decl];
	prefixes = mapGet(host, "prefixes");
	if (prefixes == NULL) {
		prefixes = createList();
		mapSet(host, prefixes, "prefixes");
	}

	ia = parse_ip6_addr_txt(cfile);
	if (ia == NULL)
		parse_error(cfile, "can't parse fixed-prefix6 address");
	token = next_token(&val, NULL, cfile);
	if (token != SLASH)
		parse_error(cfile, "expecting '/'");
	appendString(ia, val);
	token = next_token(&val, NULL, cfile);
	if (token != NUMBER)
		parse_error(cfile, "expecting number");
	appendString(ia, val);
	token = next_token(NULL, NULL, cfile);
	if (token != SEMI)
		parse_error(cfile, "semicolon expected.");

	prefix = createString(ia);
	TAILQ_CONCAT(&prefix->comments, &cfile->comments);
	listPush(prefixes, prefix);
}

/*!
 *
 * \brief Parse a pool6 statement
 *
 * Pool statements are used to group declarations and permit & deny information
 * with a specific address range.  They must be declared within a shared network
 * or subnet and there may be multiple pools withing a shared network or subnet.
 * Each pool may have a different set of permit or deny options.
 *
 * \param[in] cfile = the configuration file being parsed
 * \param[in] type  = the type of the enclosing statement.  This must be
 *		      SUBNET_DECL for this function.
 *
 * \return
 * void - This function either parses the statement and updates the structures
 *        or it generates an error message and possible halts the program if
 *        it encounters a problem.
 */

void
parse_pool6_statement(struct parse *cfile, int type)
{
	enum dhcp_token token;
	const char *val;
	isc_boolean_t done = ISC_FALSE;
	struct element *pool;
	struct element *pools;
	struct element *pdpool;
	struct element *pdpools;
	struct element *permit;
	struct element *prohibit;
	int declaration = 0;
	unsigned range_counter = 0;
	unsigned prefix_counter = 0;

	if (local_family != AF_INET6)
		parse_error(cfile, "pool6 statement is only supported "
			    "in DHCPv6 mode.");

	pool = createMap();
	pool->kind = POOL_DECL;
	TAILQ_CONCAT(&pool->comments, &cfile->comments);

	if (type != SUBNET_DECL)
		parse_error(cfile, "pool6s are only valid inside "
			    "subnet statements.");
	parse_lbrace(cfile);

	stackPush(cfile, pool);
	type = POOL_DECL;

	permit = createList();
	prohibit = createList();

	do {
		token = peek_token(&val, NULL, cfile);
		switch (token) {
		case RANGE6:
			skip_token(NULL, NULL, cfile);
			parse_address_range6(cfile, type, cfile->stack_top);
			range_counter++;
			break;

		case PREFIX6:
			skip_token(NULL, NULL, cfile);
			parse_prefix6(cfile, type, cfile->stack_top);
			mapSet(pool, createNull(), "***mark***");
			prefix_counter++;
			break;

		case ALLOW:
			skip_token(NULL, NULL, cfile);
			get_permit(cfile, permit);
			break;

		case DENY:
			skip_token(NULL, NULL, cfile);
			get_permit(cfile, prohibit);
			break;

		case RBRACE:
			skip_token(&val, NULL, cfile);
			done = ISC_TRUE;
			break;

		case END_OF_FILE:
			/*
			 * We can get to END_OF_FILE if, for instance,
			 * the parse_statement() reads all available tokens
			 * and leaves us at the end.
			 */
			parse_error(cfile, "unexpected end of file");

		default:
			declaration = parse_statement(cfile, POOL_DECL,
						      declaration);
			break;
		}
	} while (!done);

	cfile->stack_top--;

	generate_class(cfile, pool, permit, prohibit);

	/*
	 * Spread and eventually split between pools and pd-pools
	 */
	if (prefix_counter == 0) {
		/* we need pools list */
		pools = mapGet(cfile->stack[cfile->stack_top], "pools");
		if (pools == NULL) {
			pools = createList();
			pools->kind = POOL_DECL;
			mapSet(cfile->stack[cfile->stack_top], pools, "pools");
		}

		/* no address or prefix range */
		if (range_counter == 0) {
			struct comment *comment;

			comment = createComment("empty pool6");
			TAILQ_INSERT_TAIL(&pool->comments, comment);
			pool->skip = ISC_TRUE;
			cfile->issue_counter++;
			listPush(pools, pool);
			return;
		}
	} else {
		/* we need pd-pools list */
		pdpools = mapGet(cfile->stack[cfile->stack_top], "pd-pools");
		if (pdpools == NULL) {
			pdpools = createList();
			pdpools->kind = POOL_DECL;
			mapSet(cfile->stack[cfile->stack_top],
			       pdpools, "pd-pools");
		}

		/* split and purge copies */
		pdpool = copy(pool);
		while (mapContains(pdpool, "pool"))
			mapRemove(pdpool, "pool");
		while (mapContains(pool, "prefix"))
			mapRemove(pool, "prefix");
		while (mapContains(pool, "prefix-len"))
			mapRemove(pool, "prefix-len");
		while (mapContains(pool, "delegated-len"))
			mapRemove(pool, "delegated-len");
		while (mapContains(pool, "excluded-prefix"))
			mapRemove(pool, "excluded-prefix");
		while (mapContains(pool, "excluded-prefix-len"))
			mapRemove(pool, "excluded-prefix-len");
		while (mapContains(pool, "***mark***"))
			mapRemove(pool, "***mark***");

		/* spread extra prefixes into pdpool copies */
		while (--prefix_counter != 0) {
			struct handle *handle;
			struct element *first;
			struct element *saved;
			isc_boolean_t seen = ISC_FALSE;

			first = createMap();
			saved = copy(pdpool);
			while (mapSize(pdpool) > 0) {
				handle = mapPop(pdpool);
				if ((handle == NULL) ||
				    (handle->key == NULL) ||
				    (handle->value == NULL))
					parse_error(cfile, "bad pdpool entry");
				if (strcmp(handle->key, "***mark***") == 0) {
					if (!seen) {
						mapRemove(saved, handle->key);
						seen = ISC_TRUE;
					}
					continue;
				}
				if ((strcmp(handle->key, "prefix") != 0) &&
				    (strcmp(handle->key, "prefix-len") != 0) &&
				    (strcmp(handle->key,
					    "delegated-len") != 0) &&
				    (strcmp(handle->key,
					    "excluded-prefix") != 0) &&
				    (strcmp(handle->key,
					    "excluded-prefix-len") != 0))
					mapSet(first, handle->value,
					       handle->key);
				else if (!seen) {
					mapSet(first, handle->value,
					       handle->key);
					mapRemove(saved, handle->key);
				}
			}
			listPush(pdpools, first);
			pdpool = saved;
		}
		if (!mapContains(pdpool, "***mark***"))
			parse_error(cfile, "can't find prefix marker");
		mapRemove(pdpool, "***mark***");
		if (mapContains(pdpool, "***mark***"))
			parse_error(cfile, "unexpected prefix marker");
		listPush(pdpools, pdpool);
	}

	/* Do pools now */
	if (range_counter != 0) {
		/* we need pools list */
		pools = mapGet(cfile->stack[cfile->stack_top], "pools");
		if (pools == NULL) {
			pools = createList();
			pools->kind = POOL_DECL;
			mapSet(cfile->stack[cfile->stack_top], pools, "pools");
		}

		/* spread extra prefixes into pool copies */
		while (--range_counter != 0) {
			struct handle *handle;
			struct element *first;
			struct element *saved;
			isc_boolean_t seen = ISC_FALSE;

			first = createMap();
			saved = copy(pool);
			while (mapSize(pool) > 0) {
				handle = mapPop(pool);
				if ((handle == NULL) ||
				    (handle->key == NULL) ||
				    (handle->value == NULL))
					parse_error(cfile, "bad pool entry");
				if (strcmp(handle->key, "pool") != 0)
					mapSet(first, handle->value,
					       handle->key);
				else if (!seen) {
					mapSet(first, handle->value,
					       handle->key);
					mapRemove(saved, "pool");
					seen = ISC_TRUE;
				}
			}
			listPush(pools, first);
			pool = saved;
		}
		listPush(pools, pool);
	}
}

/* allow-deny-keyword :== BOOTP
   			| BOOTING
			| DYNAMIC_BOOTP
			| UNKNOWN_CLIENTS */

struct element *
parse_allow_deny(struct parse *cfile, int flag)
{
	enum dhcp_token token;
	const char *val;
	const char *value;
	const char *name;
	struct element *config;
	struct option *option;

	switch (flag) {
	case 0:
		value = "deny";
		break;
	case 1:
		value = "allow";
		break;
	case 2:
		value = "ignore";
		break;
	default:
		value = "unknown?";
		break;
	}

	token = next_token(&val, NULL, cfile);
	switch (token) {
	case TOKEN_BOOTP:
		name = "allow-bootp";
		break;

	case BOOTING:
		name = "allow-booting";
		break;

	case DYNAMIC_BOOTP:
		name = "dynamic-bootp";
		break;

	case UNKNOWN_CLIENTS:
		name = "boot-unknown-clients";
		break;

	case DUPLICATES:
		name = "duplicates";
		break;

	case DECLINES:
		name = "declines";
		break;

	case CLIENT_UPDATES:
		name = "client-updates";
		break;

	case LEASEQUERY:
		name = "leasequery";
		break;

	default:
		parse_error(cfile, "expecting allow/deny key");
	}
	parse_semi(cfile);

	config = createMap();
	mapSet(config, createString(makeString(-1, value)), "value");
	mapSet(config, createString(makeString(-1, name)), "name");
	option = option_lookup_name("server", name);
	if (option == NULL)
		parse_error(cfile, "unknown allow/deny keyword (%s)", name);
	mapSet(config, createInt(option->code), "code");
	config->skip = ISC_TRUE;
	cfile->issue_counter++;
	return config;
}

/*
 * When we parse a server-duid statement in a config file, we will
 * have the type of the server DUID to generate, and possibly the
 * actual value defined.
 *
 * server-duid llt;
 * server-duid llt ethernet|ieee802|fddi 213982198 00:16:6F:49:7D:9B;
 * server-duid ll;
 * server-duid ll ethernet|ieee802|fddi 00:16:6F:49:7D:9B;
 * server-duid en 2495 "enterprise-specific-identifier-1234";
 */
void
parse_server_duid_conf(struct parse *cfile) {
	enum dhcp_token token;
	const char *val;
	unsigned int len;
	struct string *ll_addr;
	struct element *duid;
	struct element *item;
	int ll_type;

	duid = createMap();
	TAILQ_CONCAT(&duid->comments, &cfile->comments);

	/*
	 * Consume the SERVER_DUID token.
	 */
	next_token(&val, NULL, cfile);

	/*
	 * Obtain the DUID type.
	 */
	token = next_token(&val, NULL, cfile);

	/*
	 * Enterprise is the easiest - enterprise number and raw data
	 * are required.
	 */
	if (token == EN) {
		item = createString(makeString(-1, "EN"));
		mapSet(duid, item, "type");

		/*
		 * Get enterprise number and identifier.
		 */
		token = next_token(&val, NULL, cfile);
		if (token != NUMBER)
			parse_error(cfile, "enterprise number expected");
		item = createInt(atoi(val));
		mapSet(duid, item, "enterprise-id");

		token = next_token(&val, &len, cfile);
		if (token != STRING)
			parse_error(cfile, "identifier expected");
		/* Kea requires a hexadecimal identifier */
		if (is_hexa_only(val, len))
			item = createString(makeString(len, val));
		else
			item = createString(makeStringExt(len, val, 'X'));
		mapSet(duid, item, "identifier");
	}

	/*
	 * Next easiest is the link-layer DUID. It consists only of
	 * the LL directive, or optionally the specific value to use.
	 *
	 * If we have LL only, then we set the type. If we have the
	 * value, then we set the actual DUID.
	 */
	else if (token == LL) {
		item = createString(makeString(-1, "LL"));
		mapSet(duid, item, "type");

		if (peek_token(NULL, NULL, cfile) != SEMI) {
			/*
			 * Get our hardware type and address.
			 */
			token = next_token(NULL, NULL, cfile);
			switch (token) {
			case ETHERNET:
				ll_type = HTYPE_ETHER;
				break;
			case TOKEN_RING:
				ll_type = HTYPE_IEEE802;
				break;
			case TOKEN_FDDI:
				ll_type = HTYPE_FDDI;
				break;
			default:
				parse_error(cfile, "hardware type expected");
			}
			item = createInt(ll_type);
			mapSet(duid, item, "htype");

			ll_addr = parse_hexa(cfile);
			if (ll_addr == NULL)
				parse_error(cfile,
					    "can't get hardware address");
			item = createString(ll_addr);
			mapSet(duid, item, "identifier");
		}
	}

	/*
	 * Finally the link-layer DUID plus time. It consists only of
	 * the LLT directive, or optionally the specific value to use.
	 *
	 * If we have LLT only, then we set the type. If we have the
	 * value, then we set the actual DUID.
	 */
	else if (token == LLT) {
		item = createString(makeString(-1, "LLT"));
		mapSet(duid, item, "type");

		if (peek_token(NULL, NULL, cfile) != SEMI) {
			/*
			 * Get our hardware type, timestamp, and address.
			 */
			token = next_token(NULL, NULL, cfile);
			switch (token) {
			case ETHERNET:
				ll_type = HTYPE_ETHER;
				break;
			case TOKEN_RING:
				ll_type = HTYPE_IEEE802;
				break;
			case TOKEN_FDDI:
				ll_type = HTYPE_FDDI;
				break;
			default:
				parse_error(cfile, "hardware type expected");
			}
			item = createInt(ll_type);
			mapSet(duid, item, "htype");

			token = next_token(&val, NULL, cfile);
			if (token != NUMBER)
				parse_error(cfile, "timestamp expected");
			item = createInt(atoi(val));
			mapSet(duid, item, "time");

			ll_addr = parse_hexa(cfile);
			if (ll_addr == NULL)
				parse_error(cfile,
					    "can't get hardware address");
			item = createString(ll_addr);
			mapSet(duid, item, "identifier");
		}
	}

	/*
	 * If users want they can use a number for DUID types.
	 * This is useful for supporting future, not-yet-defined
	 * DUID types.
	 *
	 * In this case, they have to put in the complete value.
	 *
	 * This also works for existing DUID types of course.
	 */
	else if (token == NUMBER) {
		item = createString(makeString(-1, val));
		item->skip = ISC_TRUE;
		/* Kea wants EN, LL or LLT so skip the whole thing */
		duid->skip = ISC_TRUE;
		cfile->issue_counter++;
		mapSet(duid, item, "type");

		token = next_token(&val, &len, cfile);
		if (token != STRING)
			parse_error(cfile, "identifier expected");
		item = createString(makeString(len, val));
		mapSet(duid, item, "identifier");
	}

	/*
	 * Anything else is an error.
	 */
	else
		parse_error(cfile, "DUID type of LLT, EN, or LL expected");

	/*
	 * Finally consume our trailing semicolon.
	 */
	token = next_token(NULL, NULL, cfile);
	if (token != SEMI)
		parse_error(cfile, "semicolon expected");

	/* server-id is a global parameter */
	if (mapContains(cfile->stack[1], "server-id"))
		parse_error(cfile, "there is already a server-id");
	/* DHCPv6 only but not fatal */
	if ((local_family != AF_INET6) && !duid->skip) {
		duid->skip = ISC_TRUE;
		cfile->issue_counter++;
	}
	mapSet(cfile->stack[1], duid, "server-id");
}

/* Check whether the argument is encoded in hexadecimal or not */
static isc_boolean_t
is_hexa_only(const char *s, unsigned l)
{
	unsigned i;

	for (i = 0; i < l; i++)
		if (!isxdigit((int)s[i]))
			return ISC_FALSE;
	return ISC_TRUE;
}

/*!
 *
 * \brief Parse (and execute) a directive (extension)
 *
 * OPTION SPACE <name> [ALIAS <kea-name>] [KNOWN*2|UNKNOWN*2|DYNAMIC]
 * OPTION <universe>.<name> [CHECK]
 *                          [ALIAS <name>]
 *                          [CODE <code> = "<format>"]
 *                          [KNOWN*2|UNKNOWN*2|DYNAMIC]
 *                          [LOCAL|DEFINE]
 */

void
parse_directive(struct parse *cfile)
{
	enum dhcp_token token;
	const char *val;
	isc_boolean_t known;
	struct option *option;

	token = peek_token(&val, NULL, cfile);

	switch (token) {
	case OPTION:
		skip_token(&val, NULL, cfile);
		token = peek_token(&val, NULL, cfile);
		if (token == SPACE) {
			parse_option_space_dir(cfile);
			return;
		}

		known = ISC_FALSE;
		option = parse_option_name(cfile, ISC_TRUE, &known);
		token = next_token(&val, NULL, cfile);
		if (token == CHECK) {
			struct string *datatype;
			isc_boolean_t is_array = ISC_FALSE;
			isc_boolean_t encapsulate = ISC_FALSE;

			datatype = convert_format(option->format,
						  &is_array,
						  &encapsulate);
			printf("option ISC DHCP (Kea)\n"
			       " %s.%s (%s.%s)\n"
			       " format \"%s\" (type \"%s\" "
			       "array %s encap %s)\n"
			       " status %s\n",
			       option->space->old, option->old,
			       option->space->name, option->name,
			       option->format, datatype->content,
			       is_array ? "true" : "false",
			       encapsulate ? "true" : "false",
			       display_status(option->status));
			parse_semi(cfile);
			return;
		}
		if (option->space->status == special)
			parse_error(cfile, "attempt to modify config %s.%s",
				    option->space->old, option->name);
		if (token == ALIAS) {
			token = next_token(&val, NULL, cfile);
			if (!is_identifier(token))
				parse_error(cfile,
					    "expecting identifier after "
					    "alias keyword.");
			if (option->status != dynamic)
				parse_error(cfile,
					    "attempt to rename %s.%s to %s",
					    option->space->name,
					    option->name, val);
			option->name = strdup(val);
			parse_semi(cfile);
			return;
		}
		if (token == CODE) {
			parse_option_code_dir(cfile, option);
			return;
		}
		if ((token == KNOWN) || (token == UNKNOWN) ||
		    (token == DYNAMIC)) {
			parse_option_status_dir(cfile, option, token);
			return;
		}
		if (token == LOCAL) {
			parse_option_local_dir(cfile, option);
			parse_semi(cfile);
			return;
		}
		if (token == DEFINE) {
			parse_option_define_dir(cfile, option);
			parse_semi(cfile);
			return;
		}
		parse_error(cfile, "unknown option directive %s", val);

	default:
		parse_error(cfile, "unknown directive %s", val);
	}
}

/* Set alias and status for option spaces */

void
parse_option_space_dir(struct parse *cfile)
{
	enum dhcp_token token;
	const char *val;
	struct space *space;

	skip_token(NULL, NULL, cfile);	/* Discard SPACE */
	token = next_token(&val, NULL, cfile);
	if (!is_identifier(token))
		parse_error(cfile, "expecting identifier.");
	space = space_lookup(val);
	if (space == NULL)
		parse_error(cfile, "can't find space '%s", val);

	token = next_token(&val, NULL, cfile);
	if (token == CHECK) {
		printf("space ISC DHCP (kea)\n"
		       " %s (%s)\n status %s\n%s",
		       space->old, space->name,
		       display_status(space->status),
		       space->vendor != NULL ? " vendor\n" : "");
		parse_semi(cfile);
		return;
	}
	if (token == ALIAS) {
		token = next_token(&val, NULL, cfile);
		if (!is_identifier(token))
			parse_error(cfile,
				    "expecting identifier after "
				    "alias keyword.");
		if (space->status != dynamic)
			parse_error(cfile,
				    "attempt to rename %s to %s",
				    space->name, val);
		space->name = strdup(val);
		parse_semi(cfile);
		return;
	}
	if (token == DYNAMIC)
		space->status = dynamic;
	else if (token == UNKNOWN) {
		token = next_token(NULL, NULL, cfile);
		if (token == KNOWN)
			space->status = known;
		else if (token == UNKNOWN)
			space->status = kea_unknown;
		else
			parse_error(cfile, "expected KNOW or UNKNOWN");
	} else if (token != UNKNOWN)
		parse_error(cfile, "expected KNOW or UNKNOWN or DYNAMIC");
	else {
                if (token == KNOWN)
			space->status = isc_dhcp_unknown;
                else if (token == UNKNOWN)
                        parse_error(cfile, "illicit combination: space "
                                    "%s is known by nobody", space->name);
		else
			parse_error(cfile, "expected KNOW or UNKNOWN");
	}
	parse_semi(cfile);
}

/* Alternative to parse_option_code_decl using the raw ISC DHCP format */

void
parse_option_code_dir(struct parse *cfile, struct option *option)
{
	const char *val;
	enum dhcp_token token;
	unsigned code;
	struct element *def;
	struct element *optdef;
	struct string *datatype;
	isc_boolean_t is_array = ISC_FALSE;
	isc_boolean_t encapsulate = ISC_FALSE;

	def = createMap();
	mapSet(def,
	       createString(makeString(-1, option->space->name)),
	       "space");
	mapSet(def, createString(makeString(-1, option->name)), "name");

	/* Parse the option code. */
	token = next_token(&val, NULL, cfile);
	if (token != NUMBER)
		parse_error(cfile, "expecting option code number.");
	code = atoi(val);
	mapSet(def, createInt(code), "code");

	/* We have the code so we can get the real option now */
	if (option->code == 0) {
		struct option *from_code;

		option->code = code;
		from_code = option_lookup_code(option->space->old, code);
		if (from_code != NULL)
			option = from_code;
	}

	/* Redefinitions are not allowed */
	if ((option->status != dynamic) ||
	    (strcmp(option->format, "u") != 0))
		parse_error(cfile, "attempt to redefine %s.%s code %u",
			    option->space->name, option->name, code);

	token = next_token(&val, NULL, cfile);
	if (token != EQUAL)
		parse_error(cfile, "expecting \"=\"");
	token = next_token(&val, NULL, cfile);
	if (token != STRING)
		parse_error(cfile, "expecting format string");
	option->format = strdup(val);
	parse_semi(cfile);

	datatype = convert_format(val, &is_array, &encapsulate);

	if ((datatype == NULL) && (strchr(datatype->content, '?') != NULL))
		parse_error(cfile, "failed to convert format \"%s\" for "
			    "option %s.%s code %u",
			    val, option->space->name, option->name, code);
	/* todo */
	if (encapsulate)
		parse_error(cfile, "option %s.%s code %u encapsulate?",
			    option->space->name, option->name, code);

	if (strchr(datatype->content, ',') == NULL)
		mapSet(def, createString(datatype), "type");
	else {
		mapSet(def, createString(datatype), "record-types");
		mapSet(def, createString(makeString(-1, "record")), "type");
	}
	if (is_array)
		mapSet(def, createBool(ISC_TRUE), "array");

	optdef = mapGet(cfile->stack[1], "option-def");
	if (optdef == NULL) {
		optdef = createList();
		mapSet(cfile->stack[1], optdef, "option-def");
	}
	listPush(optdef, def);
}

/* Update the option status for instance to add standard options */

void
parse_option_status_dir(struct parse *cfile, struct option *option,
			enum dhcp_token token)
{
	if (token == DYNAMIC)
		option->status = dynamic;
	else if (token == KNOWN) {
		token = next_token(NULL, NULL, cfile);
		if (token == KNOWN)
			option->status = known;
		else if (token == UNKNOWN)
			option->status = kea_unknown;
		else
			parse_error(cfile, "expected KNOW or UNKNOWN");
	} else if (token != UNKNOWN)
		parse_error(cfile, "expected KNOW or UNKNOWN or DYNAMIC");
	else {
		if (token == KNOWN)
			option->status = isc_dhcp_unknown;
		else if (token == UNKNOWN)
			parse_error(cfile, "illicit combination: option "
				    "%s.%s code %u is known by nobody",
				    option->space->name, option->name,
				    option->code);
		else
			parse_error(cfile, "expected KNOW or UNKNOWN");
	}
	parse_semi(cfile);
}

/* Make the option definition not exported to Kea */

void
parse_option_local_dir(struct parse *cfile, struct option *option)
{
	struct element *optdef;
	struct element *def;
	struct element *elem;
	size_t i;

	def = NULL;
	if (option->code == 0)
		parse_error(cfile, "unknown code for option %s.%s",
			    option->space->name, option->name);

	optdef = mapGet(cfile->stack[1], "option-def");
	if (optdef == NULL) {
		optdef = createList();
		mapSet(cfile->stack[1], optdef, "option-def");
		goto not_found;
	}
	for (i = 0; i < listSize(optdef); i++) {
		def = listGet(optdef, i);
		elem = mapGet(def, "space");
		if ((elem == NULL) || (elem->type != ELEMENT_STRING))
			parse_error(cfile, "got an option definition "
				    "without space at %u", (unsigned)i);
		if (strcmp(option->space->name,
			   stringValue(elem)->content) != 0)
			continue;
		elem = mapGet(def, "code");
		if ((elem == NULL) || (elem->type != ELEMENT_INTEGER))
			parse_error(cfile, "got an option definition "
				    "without code at %u", (unsigned)i);
		if (intValue(elem) == option->code)
			break;
	}
	if (def == NULL)
		goto not_found;
	def->skip = ISC_TRUE;
	mapSet(def, createNull(), "no-export");
	return;

not_found:
	parse_error(cfile, "can't find option %s.%s code %u in definitions",
		    option->space->name, option->name, option->code);
}

/* Make the opposite: force the definition */

void
parse_option_define_dir(struct parse *cfile, struct option *option)
{
	struct element *optdef;
	struct element *def;
	struct element *elem;
	struct string *datatype;
	isc_boolean_t is_array = ISC_FALSE;
	isc_boolean_t encapsulate = ISC_FALSE;
	size_t i;

	def = NULL;
	if (option->code == 0)
		parse_error(cfile, "unknown code for option %s.%s",
			    option->space->name, option->name);

	optdef = mapGet(cfile->stack[1], "option-def");
	if (optdef == NULL) {
		optdef = createList();
		mapSet(cfile->stack[1], optdef, "option-def");
		goto no_search;
	}
	for (i = 0; i < listSize(optdef); i++) {
		def = listGet(optdef, i);
		elem = mapGet(def, "space");
		if ((elem == NULL) || (elem->type != ELEMENT_STRING))
			parse_error(cfile, "got an option definition "
				    "without space at %u", (unsigned)i);
		if (strcmp(option->space->name,
			   stringValue(elem)->content) != 0)
			continue;
		elem = mapGet(def, "code");
		if ((elem == NULL) || (elem->type != ELEMENT_INTEGER))
			parse_error(cfile, "got an option definition "
				    "without code at %u", (unsigned)i);
		if (intValue(elem) == option->code)
			parse_error(cfile, "unexpected definition for "
				    "option %s.%s code %u",
				    option->space->name, option->name,
				    option->code);
	}
no_search:
	def = createMap();
	mapSet(def,
	       createString(makeString(-1, option->space->name)),
	       "space");
	mapSet(def, createString(makeString(-1, option->name)), "name");
	mapSet(def, createInt(option->code), "code");

	datatype = convert_format(option->format, &is_array, &encapsulate);

	if ((datatype == NULL) && (strchr(datatype->content, '?') != NULL))
		parse_error(cfile, "failed to convert format \"%s\" for "
			    "option %s.%s code %u",
			    option->format, option->space->name,
			    option->name, option->code);
	/* todo */
	if (encapsulate)
		parse_error(cfile, "option %s.%s code %u encapsulate?",
			    option->space->name, option->name, option->code);

	if (strchr(datatype->content, ',') == NULL)
		mapSet(def, createString(datatype), "type");
	else {
		mapSet(def, createString(datatype), "record-types");
		mapSet(def, createString(makeString(-1, "record")), "type");
	}
	if (is_array)
		mapSet(def, createBool(ISC_TRUE), "array");

	listPush(optdef, def);

	return;
}

/*
 * Push new interface on the interface list when it is not already.
 */

static void
new_network_interface(struct parse *cfile, struct element *iface)
{
	struct element *ifconf;
	struct element *iflist;
	struct string *name = stringValue(iface);
	int i;

	ifconf = mapGet(cfile->stack[1], "interfaces-config");
	if (ifconf == NULL) {
		ifconf = createMap();
		mapSet(cfile->stack[1], ifconf, "interfaces-config");
	}

	iflist = mapGet(ifconf, "interfaces");
	if (iflist == NULL) {
		iflist = createList();
		mapSet(ifconf, iflist, "interfaces");
	}

	for (i = 0; i < listSize(iflist); i++) {
		struct element *item;

		item = listGet(iflist, i);
		if ((item != NULL) &&
		    (item->type == ELEMENT_STRING) &&
		    eqString(stringValue(item), name))
			return;
	}

	listPush(iflist, createString(name));
}

/* Convert address and mask in binary into address/len text */

static const uint32_t bitmasks[32 + 1] = {
	0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
	0x0fffffff, 0x07ffffff, 0x03ffffff, 0x01ffffff,
	0x00ffffff, 0x007fffff, 0x003fffff, 0x001fffff,
	0x000fffff, 0x0007ffff, 0x0003ffff, 0x0001ffff,
	0x0000ffff, 0x00007fff, 0x00003fff, 0x00001fff,
	0x00000fff, 0x000007ff, 0x000003ff, 0x000001ff,
	0x000000ff, 0x0000007f, 0x0000003f, 0x0000001f,
	0x0000000f, 0x00000007, 0x00000003, 0x00000001,
	0x00000000 };

static struct string *
addrmask(const struct string *address, const struct string *netmask)
{
	struct string *result;
	uint8_t plen;
	uint32_t mask;

	result = makeStringExt(address->length, address->content, 'I');

	memcpy(&mask, netmask->content, 4);
	mask = ntohl(mask);
	for (plen = 0; plen <= 32; ++plen)
		if (~mask == bitmasks[plen])
			break;
	if (plen > 32)
		return NULL;

	appendString(result, "/");
	concatString(result, makeStringExt(1, (char *)&plen, 'B'));
	return result;
}

/*
 * find a place where to put a reservation
 * (reservations aka hosts must be in a subnet in Kea < 1.5)
 * (defaulting to the last defined subnet (e.g. for reservations
 *  without any address).
 * (first step is to find an enclosing group).
 */

static struct element *
find_match(struct parse *cfile, struct element *host,
	   isc_boolean_t *used_heuristicp)
{
	struct element *address;
	struct subnet *subnet;
	char addr[16];
	size_t group;
	size_t i, len;
	int kind;

	if (global_hr) {
		struct element *hosts;

		hosts = mapGet(cfile->stack[1], "reservations");
		if (!hosts) {
			mapSet(cfile->stack[1],
			       createString(makeString(-1, "global")),
			       "reservation-mode");
			hosts = createList();
			mapSet(cfile->stack[1], hosts, "reservations");
		}
		*used_heuristicp = ISC_FALSE;
		return cfile->stack[1];
	}

	for (group = cfile->stack_top; group > 0; --group) {
		kind = cfile->stack[group]->kind;
		if ((kind == GROUP_DECL) || (kind == ROOT_GROUP))
			break;
	}
	if (!group)
		parse_error(cfile, "can't find root group");
	if (kind == GROUP_DECL)
		return cfile->stack[group];

	if (local_family == AF_INET) {
		address = mapGet(host, "ip-address");
		if (address == NULL) {
			if (TAILQ_EMPTY(&known_subnets))
				return cfile->stack[1];
			if (used_heuristicp)
				*used_heuristicp = ISC_TRUE;
			return TAILQ_LAST(&known_subnets, subnets)->subnet;
		}
		len = 4;
	} else {
		address = mapGet(host, "ip-addresses");
		if (address == NULL) {
			if (TAILQ_EMPTY(&known_subnets))
				return cfile->stack[1];
			if (used_heuristicp)
				*used_heuristicp = ISC_TRUE;
			return TAILQ_LAST(&known_subnets, subnets)->subnet;
		}
		address = listGet(address, 0);
		if (address == NULL)
			return TAILQ_LAST(&known_subnets, subnets)->subnet;
		len = 16;
	}

	if (inet_pton(local_family, stringValue(address)->content, addr) != 1)
		parse_error(cfile, "bad address %s",
			    stringValue(address)->content);
	TAILQ_FOREACH(subnet, &known_subnets) {
		isc_boolean_t matching = ISC_TRUE;

		if (subnet->mask->length != len)
			continue;
		for (i = 0; i < len; i++)
			if ((addr[i] & subnet->mask->content[i]) !=
					subnet->addr->content[i]) {
				matching = ISC_FALSE;
				break;
			}
		if (matching)
			return subnet->subnet;
	}
	return cfile->stack[1];
}

/*
 * find a subnet where to put a pool
 * (pools are not allowed at shared-network level in Kea)
 */

static struct element *
find_location(struct element *share, struct range *range)
{
	struct subnet *subnet;
	size_t i;

	TAILQ_FOREACH(subnet, &known_subnets) {
		isc_boolean_t matching = ISC_TRUE;

		if (subnet->share != share)
			continue;
		if (subnet->mask->length != range->low->length)
			continue;
		for (i = 0; i < range->low->length; i++)
			if ((range->low->content[i] &
			     subnet->mask->content[i]) !=
			    subnet->addr->content[i]) {
				matching = ISC_FALSE;
				break;
			}
		if (matching)
			return subnet->subnet;
	}
	return NULL;
}

/*
 * Compute a prefix length from lower - higher IPv6 addresses.
 */

static const uint8_t bytemasks[8] = {
	0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

static int
get_prefix_length(const char *low, const char *high)
{
	uint8_t lo[16];
	uint8_t hi[16];
	uint8_t xor[16];
	int i, plen;

	if ((inet_pton(AF_INET6, low, lo) != 1) ||
	    (inet_pton(AF_INET6, high, hi) != 1))
		return -100;

	for (i = 0; i < 16; i++)
		xor[i] = lo[i] ^ hi[i];
	for (plen = 0; plen < 128; plen += 8)
		if (xor[plen / 8] != 0)
			break;
	if (plen == 128)
		return plen;
	for (i = (plen / 8) + 1; i < 16; i++)
		if (xor[i] != 0)
			return -2;
	for (i = 0; i < 8; i++) {
		uint8_t msk =  ~xor[plen / 8];

		if (msk == bytemasks[i])
			return plen + i + 1;
	}
	return -1;
}

/*
 * Get a (global) class from its reference, i.e.:
 * - name for a (super)class
 * - super, and binary or string for a subclass
 */
static struct element *
get_class(struct parse *cfile, struct element *ref)
{
	struct element *classes;
	struct element *class;
	struct element *name;
	struct element *selector;
	struct element *param;
	size_t i;

	classes = mapGet(cfile->stack[1], "client-classes");
	if ((classes == NULL) || (listSize(classes) == 0))
		return NULL;

	name = mapGet(ref, "super");
	if (name == NULL) {
		name = mapGet(ref, "name");
		if (name == NULL)
			return NULL;
		for (i = 0; i < listSize(classes); i++) {
			class = listGet(classes, i);
			if (mapContains(ref, "super"))
				continue;
			param = mapGet(class, "name");
			if (param == NULL)
				continue;
			if (eqString(stringValue(name), stringValue(param)))
				return class;
		}
		return NULL;
	}
	selector = mapGet(ref, "string");
	if (selector == NULL) {
		selector = mapGet(ref, "binary");
		if (selector == NULL)
			return NULL;
		for (i = 0; i <listSize(classes); i++) {
			class = listGet(classes, i);
			param = mapGet(class, "super");
			if (param == NULL)
				continue;
			if (!eqString(stringValue(name), stringValue(param)))
				continue;
			param = mapGet(class, "binary");
			if (param == NULL)
				continue;
			if (eqString(stringValue(selector),
				     stringValue(param)))
				return class;
		}
		return NULL;
	}
	for (i = 0; i <listSize(classes); i++) {
		class = listGet(classes, i);
		param = mapGet(class, "super");
		if (param == NULL)
			continue;
		if (!eqString(stringValue(name), stringValue(param)))
			continue;
		param = mapGet(class, "string");
		if (param == NULL)
			continue;
		if (eqString(stringValue(selector), stringValue(param)))
			return class;
	}
	return NULL;
}

/*
 * Concatenate two class reference lists eliminating duplicates
 * (complexity is bad: if this becomes a performance pig, use a hash table)
 */

static void
concat_classes(struct parse *cfile, struct element *dst, struct element *src)
{
	struct element *class;
	struct element *sitem;
	struct element *ditem;
	size_t i;
	isc_boolean_t dup;

	while (listSize(src) > 0) {
		sitem = listGet(src, 0);
		listRemove(src, 0);
		class = get_class(cfile, sitem);
		if (class == NULL)
			/* just ignore */
			continue;
		dup = ISC_FALSE;
		for (i = 0; i < listSize(dst); i++) {
			ditem = listGet(dst, i);
			if (class == get_class(cfile, ditem)) {
				dup = ISC_TRUE;
				break;
			}
		}
		if (dup)
			continue;
		listPush(dst, sitem);
	}
}

/* Generate a class from allow/deny member lists */

static void
generate_class(struct parse *cfile, struct element *pool,
	       struct element *allow, struct element *deny)
{
	struct element *classes;
	struct element *class;
	struct element *elem;
	struct element *prop;
	struct element *depend;
	struct element *result = NULL;
	struct string *name;
	struct string *expr;
	struct string *msg;
	struct comments comments;
	struct comment *comment;
	isc_boolean_t rescan;
	size_t i;

	if ((listSize(allow) == 0) && (listSize(deny) == 0))
		return;

	classes = mapGet(cfile->stack[1], "generated-classes");
	if (classes == NULL) {
		classes = createList();
		mapSet(cfile->stack[1], classes, "generated-classes");
	}

	/* Create comments */
	TAILQ_INIT(&comments);
	comment = createComment("/// From:");
	TAILQ_INSERT_TAIL(&comments, comment);
	for (i = 0; i < listSize(allow); i++) {
		struct element *alias;

		elem = listGet(allow, i);
		assert(elem != NULL);
		prop = mapGet(elem, "class");
		assert(prop != NULL);
		assert(prop->type == ELEMENT_STRING);
		alias = mapGet(elem, "real");
		msg = makeString(-1, "///   allow ");
		concatString(msg, stringValue(alias ? alias : prop));
		comment = createComment(msg->content);
		TAILQ_INSERT_TAIL(&comments, comment);
	}
	for (i = 0; i < listSize(deny); i++) {
		struct element *alias;

		elem = listGet(deny, i);
		assert(elem != NULL);
		prop = mapGet(elem, "class");
		assert(prop != NULL);
		assert(prop->type == ELEMENT_STRING);
		alias = mapGet(elem, "real");
		msg = makeString(-1, "///   deny ");
		concatString(msg, stringValue(alias ? alias : prop));
		comment = createComment(msg->content);
		TAILQ_INSERT_TAIL(&comments, comment);
	}
	TAILQ_CONCAT(&comments, &allow->comments);
	TAILQ_CONCAT(&comments, &deny->comments);

	/* Deal with special cases */
	for (;;) {
		rescan = ISC_FALSE;
		for (i = 0; i < listSize(allow); i++) {
			elem = listGet(allow, i);
			assert(elem != NULL);
			prop = mapGet(elem, "way");
			assert(prop != NULL);
			assert(prop->type == ELEMENT_BOOLEAN);
			class = mapGet(elem, "class");
			assert(class != NULL);
			assert(class->type == ELEMENT_STRING);
			/* allow !ALL and other */
			if (eqString(stringValue(class), CLASS_ALL) &&
			    !boolValue(prop) && (listSize(allow) > 1)) {
				listRemove(allow, i);
				rescan = ISC_TRUE;
				break;
			}
			/* allow ALL alone */
			if (eqString(stringValue(class), CLASS_ALL) &&
			    boolValue(prop) && (listSize(allow) == 1)) {
				resetList(allow);
				rescan = ISC_TRUE;
				break;
			}
		}
		if (!rescan)
			break;
	}

	for (;;) {
		rescan = ISC_FALSE;
		for (i = 0; i < listSize(deny); i++) {
			elem = listGet(deny, i);
			assert(elem != NULL);
			prop = mapGet(elem, "way");
			assert(prop != NULL);
			assert(prop->type == ELEMENT_BOOLEAN);
			class = mapGet(elem, "class");
			assert(class != NULL);
			assert(class->type == ELEMENT_STRING);
			/* DENY !ALL */
			if (eqString(stringValue(class), CLASS_ALL) &&
			    !boolValue(prop)) {
				listRemove(deny, i);
				rescan = ISC_TRUE;
				break;
			}
			/* DENY ALL */
			if (eqString(stringValue(class), CLASS_ALL) &&
			    boolValue(prop)) {
				resetList(allow);
				if (listSize(deny) > 1) {
					listRemove(deny, i);
					resetList(deny);
					listPush(deny, elem);
				}
				break;
			}
		}
		if (!rescan)
			break;
	}

	/* Fully cleaned? */
	if ((listSize(allow) == 0) && (listSize(deny) == 0)) {
		if (result != NULL)
			TAILQ_CONCAT(&result->comments, &comments);
		else
			TAILQ_CONCAT(&pool->comments, &comments);
		return;
	}

	/* Unique allow member short cut */
	if ((listSize(allow) == 1) && (listSize(deny) == 0) &&
	    !allow->skip && !deny->skip) {
		elem = listGet(allow, 0);
		assert(elem != NULL);
		prop = mapGet(elem, "way");
		assert(prop != NULL);
		assert(prop->type == ELEMENT_BOOLEAN);
		class = mapGet(elem, "class");
		assert(class != NULL);
		assert(class->type == ELEMENT_STRING);
		if (boolValue(prop)) {
			result = createString(stringValue(class));
			TAILQ_CONCAT(&result->comments, &comments);
			mapSet(pool, result, "client-class");
			return;
		}
	}

	/* Build name */
	name = makeString(-1, "gen#");
	for (i = 0; i < listSize(allow); i++) {
		elem = listGet(allow, i);
		assert(elem != NULL);
		prop = mapGet(elem, "way");
		assert(prop != NULL);
		assert(prop->type == ELEMENT_BOOLEAN);
		if (!boolValue(prop))
			appendString(name, "!");
		prop = mapGet(elem, "class");
		assert(prop != NULL);
		assert(prop->type == ELEMENT_STRING);
		concatString(name, stringValue(prop));
		appendString(name, "#");
	}
	if (listSize(deny) > 0) {
		appendString(name, "_AND_#");
		for (i = 0; i < listSize(deny); i++) {
			elem = listGet(deny, i);
			assert(elem != NULL);
			prop = mapGet(elem, "way");
			assert(prop != NULL);
			assert(prop->type == ELEMENT_BOOLEAN);
			if (boolValue(prop))
				appendString(name, "!");
			prop = mapGet(elem, "class");
			assert(prop != NULL);
			assert(prop->type == ELEMENT_STRING);
			concatString(name, stringValue(prop));
			appendString(name, "#");
		}
	}

	/* Check if it already exists */
	for (i = 0; i < listSize(classes); i++) {
		struct element *descr;

		class = listGet(classes, i);
		assert(class != NULL);
		descr = mapGet(class, "name");
		assert(descr != NULL);
		assert(descr->type == ELEMENT_STRING);
		if (!eqString(name, stringValue(descr)))
			continue;
		result = createString(name);
		TAILQ_CONCAT(&result->comments, &comments);
		mapSet(pool, result, "client-class");
		return;
	}

	/* Create expression */
	class = createMap();
	depend = createList();
	expr = allocString();

	if ((listSize(allow) > 0) && (listSize(deny) > 0))
		appendString(expr, "(");

	for (i = 0; i < listSize(allow); i++) {
		isc_boolean_t negative;

		if (i > 0)
			appendString(expr, " or ");
		elem = listGet(allow, i);
		prop = mapGet(elem, "way");
		negative = !boolValue(prop);
		prop = mapGet(elem, "class");
		if (negative)
			appendString(expr, "not ");
		appendString(expr, "member('");
		concatString(expr, stringValue(prop));
		appendString(expr, "')");
		listPush(depend, createString(stringValue(prop)));
	}

	if ((listSize(allow) > 0) && (listSize(deny) > 0))
		appendString(expr, ") and ");

	for (i = 0; i < listSize(deny); i++) {
		isc_boolean_t negative;

		if (i > 0)
			appendString(expr, " and ");
		elem = listGet(deny, i);
		prop = mapGet(elem, "way");
		negative = boolValue(prop);
		prop = mapGet(elem, "class");
		if (negative)
			appendString(expr, "not ");
		appendString(expr, "member('");
		concatString(expr, stringValue(prop));
		appendString(expr, "')");
		listPush(depend, createString(stringValue(prop)));
	}

	mapSet(class, createString(name), "name");
	mapSet(class, createString(expr), "test");
	mapSet(class, depend, "depend");
	/* inherit untranslatable cases */
	class->skip |= allow->skip || deny->skip;
	listPush(classes, class);

	result = createString(name);
	TAILQ_CONCAT(&result->comments, &comments);
	mapSet(pool, result, "client-class");
}
