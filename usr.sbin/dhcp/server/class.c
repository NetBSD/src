/* class.c

   Handling for client classes. */

/*
 * Copyright (c) 1998-2000 Internet Software Consortium.
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
"$Id: class.c,v 1.1.1.1.2.1 2000/06/22 18:00:53 minoura Exp $ Copyright (c) 1998-2000 The Internet Software Consortium.  All rights reserved.\n";

#endif /* not lint */

#include "dhcpd.h"

struct collection default_collection = {
	(struct collection *)0,
	"default",
	(struct class *)0,
};

struct collection *collections = &default_collection;
struct executable_statement *default_classification_rules;

int have_billing_classes;

/* Build the default classification rule tree. */

void classification_setup ()
{
	struct executable_statement *rules;

	/* eval ... */
	rules = (struct executable_statement *)0;
	if (!executable_statement_allocate (&rules, MDL))
		log_fatal ("Can't allocate check of default collection");
	rules -> op = eval_statement;

	/* check-collection "default" */
	if (!expression_allocate (&rules -> data.eval, MDL))
		log_fatal ("Can't allocate default check expression");
	rules -> data.eval -> op = expr_check;
	rules -> data.eval -> data.check = &default_collection;
	
	default_classification_rules = rules;
}

void classify_client (packet)
	struct packet *packet;
{
	execute_statements (packet, (struct lease *)0, packet -> options,
			    (struct option_state *)0, &global_scope,
			    default_classification_rules);
}

int check_collection (packet, lease, collection)
	struct packet *packet;
	struct lease *lease;
	struct collection *collection;
{
	struct class *class, *nc;
	struct data_string data;
	int matched = 0;
	int status;
	int ignorep;

	for (class = collection -> classes; class; class = class -> nic) {
#if defined (DEBUG_CLASS_MATCHING)
		log_info ("checking against class %s...", class -> name);
#endif
		memset (&data, 0, sizeof data);
		/* If a class is for billing, don't put the client in the
		   class if we've already billed it to a different class. */
		if (class -> submatch) {
			status = (evaluate_data_expression
				  (&data, packet, lease,
				   packet -> options, (struct option_state *)0,
				   &lease -> scope, class -> submatch));
			if (status) {
				nc = (struct class *)0;
				if (class_hash_lookup (&nc, class -> hash,
						       (const char *)data.data,
						       data.len, MDL)) {
#if defined (DEBUG_CLASS_MATCHING)
					log_info ("matches subclass %s.",
					      print_hex_1 (data.len,
							   data.data, 60));
#endif
					data_string_forget (&data, MDL);
					classify (packet, nc);
					matched = 1;
					continue;
				}
				if (!class -> spawning) {
					data_string_forget (&data, MDL);
					continue;
				}
#if defined (DEBUG_CLASS_MATCHING)
				log_info ("spawning subclass %s.",
				      print_hex_1 (data.len, data.data, 60));
#endif
				nc = (struct class *)
					dmalloc (sizeof (struct class), MDL);
				memset (nc, 0, sizeof *nc);
				nc -> group = class -> group;
				nc -> superclass = class;
				nc -> lease_limit = class -> lease_limit;
				nc -> dirty = 1;
				if (nc -> lease_limit) {
					nc -> billed_leases =
						(dmalloc
						 (nc -> lease_limit *
						  sizeof (struct lease *),
						  MDL));
					if (!nc -> billed_leases) {
						log_error ("no memory for%s",
							   " billing");
						data_string_forget
							(&nc -> hash_string,
							 MDL);
						dfree (nc, MDL);
						data_string_forget (&data,
								    MDL);
						continue;
					}
					memset (nc -> billed_leases, 0,
						(nc -> lease_limit *
						 sizeof nc -> billed_leases));
				}
				data_string_copy (&nc -> hash_string, &data,
						  MDL);
				data_string_forget (&data, MDL);
				if (!class -> hash)
					class -> hash = new_hash (0, 0, 0);
				class_hash_add (class -> hash,
						(const char *)
						nc -> hash_string.data,
						nc -> hash_string.len,
						nc, MDL);
				classify (packet, nc);
			}
		}

		status = (evaluate_boolean_expression_result
			  (&ignorep, packet, lease,
			   packet -> options, (struct option_state *)0,
			   &lease -> scope, class -> expr));
		if (status) {
			matched = 1;
#if defined (DEBUG_CLASS_MATCHING)
			log_info ("matches class.");
#endif
			classify (packet, class);
		}
	}
	return matched;
}

void classify (packet, class)
	struct packet *packet;
	struct class *class;
{
	if (packet -> class_count < PACKET_MAX_CLASSES)
		class_reference (&packet -> classes [packet -> class_count++],
				 class, MDL);
	else
		log_error ("too many groups for %s",
		      print_hw_addr (packet -> raw -> htype,
				     packet -> raw -> hlen,
				     packet -> raw -> chaddr));
}

isc_result_t find_class (struct class **class, const char *name,
			 const char *file, int line)
{
	struct collection *lp;
	struct class *cp;

	for (lp = collections; lp; lp = lp -> next) {
		for (cp = lp -> classes; cp; cp = cp -> nic)
			if (cp -> name && !strcmp (name, cp -> name)) {
				return class_reference (class, cp, file, line);
			}
	}
	return ISC_R_NOTFOUND;
}

int unbill_class (lease, class)
	struct lease *lease;
	struct class *class;
{
	int i;

	for (i = 0; i < class -> lease_limit; i++)
		if (class -> billed_leases [i] == lease)
			break;
	if (i == class -> lease_limit) {
		log_error ("lease %s unbilled with no billing arrangement.",
		      piaddr (lease -> ip_addr));
		return 0;
	}
	class_dereference (&lease -> billing_class, MDL);
	lease_dereference (&class -> billed_leases [i], MDL);
	class -> leases_consumed--;
	return 1;
}

int bill_class (lease, class)
	struct lease *lease;
	struct class *class;
{
	int i;

	if (class -> leases_consumed == class -> lease_limit)
		return 0;

	for (i = 0; i < class -> lease_limit; i++)
		if (!class -> billed_leases [i])
			break;

	if (i == class -> lease_limit) {
		log_error ("class billing consumption disagrees with leases.");
		return 0;
	}

	lease_reference (&class -> billed_leases [i], lease, MDL);
	class_reference (&lease -> billing_class, class, MDL);
	class -> leases_consumed++;
	return 1;
}
