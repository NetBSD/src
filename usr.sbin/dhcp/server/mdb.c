/* mdb.c

   Server-specific in-memory database support. */

/*
 * Copyright (c) 1996-2000 Internet Software Consortium.
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
"$Id: mdb.c,v 1.1.1.4 2000/07/09 07:04:10 mellon Exp $ Copyright (c) 1996-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

struct subnet *subnets;
struct shared_network *shared_networks;
struct hash_table *host_hw_addr_hash;
struct hash_table *host_uid_hash;
struct hash_table *lease_uid_hash;
struct hash_table *lease_ip_addr_hash;
struct hash_table *lease_hw_addr_hash;
struct hash_table *host_name_hash;

omapi_object_type_t *dhcp_type_host;

isc_result_t enter_host (hd, dynamicp, commit)
	struct host_decl *hd;
	int dynamicp;
	int commit;
{
	struct host_decl *hp = (struct host_decl *)0;
	struct host_decl *np = (struct host_decl *)0;
	struct executable_statement *esp;

	if (!host_name_hash) {
		host_name_hash =
			new_hash ((hash_reference)host_reference,
				  (hash_dereference)host_dereference, 0);
		if (!host_name_hash)
			log_fatal ("Can't allocate host name hash");
		host_hash_add (host_name_hash,
			       (unsigned char *)hd -> name,
			       strlen (hd -> name), hd, MDL);
	} else {
		host_hash_lookup (&hp, host_name_hash,
				  (unsigned char *)hd -> name,
				  strlen (hd -> name), MDL);

		/* If it's deleted, we can supersede it. */
		if (hp && (hp -> flags & HOST_DECL_DELETED)) {
			host_hash_delete (host_name_hash,
					  (unsigned char *)hd -> name,
					  strlen (hd -> name), MDL);
			/* If the old entry wasn't dynamic, then we
			   always have to keep the deletion. */
			if (!hp -> flags & HOST_DECL_DYNAMIC)
				hd -> flags |= HOST_DECL_STATIC;
		}

		/* If there isn't already a host decl matching this
		   address, add it to the hash table. */
		if (!hp) {
			host_hash_add (host_name_hash,
				       (unsigned char *)hd -> name,
				       strlen (hd -> name), hd, MDL);
		} else {
			/* XXX actually, we have to delete the old one
			   XXX carefully and replace it.   Not done yet. */
			host_dereference (&hp, MDL);
			return ISC_R_EXISTS;
		}
	}

	if (hd -> n_ipaddr)
		host_dereference (&hd -> n_ipaddr, MDL);

	if (!hd -> type)
		hd -> type = dhcp_type_host;

	if (hd -> interface.hlen) {
		if (!host_hw_addr_hash) {
			host_hw_addr_hash =
				new_hash ((hash_reference)host_reference,
					  (hash_dereference)host_dereference,
					  0);
			if (!host_hw_addr_hash)
				log_fatal ("Can't allocate host/hw hash");
		} else {
			/* If there isn't already a host decl matching this
			   address, add it to the hash table. */
			host_hash_lookup (&hp, host_hw_addr_hash,
					  hd -> interface.hbuf,
					  hd -> interface.hlen, MDL);
		}
		if (!hp)
			host_hash_add (host_hw_addr_hash, hd -> interface.hbuf,
				       hd -> interface.hlen, hd, MDL);
		else {
			/* If there was already a host declaration for
			   this hardware address, add this one to the
			   end of the list. */
			for (np = hp; np -> n_ipaddr; np = np -> n_ipaddr)
				;
			host_reference (&np -> n_ipaddr, hd, MDL);
			host_dereference (&hp, MDL);
		}
	}

	/* See if there's a statement that sets the client identifier.
	   This is a kludge - the client identifier really shouldn't be
	   set with an executable statement. */
	for (esp = hd -> group -> statements; esp; esp = esp -> next) {
		if (esp -> op == supersede_option_statement &&
		    esp -> data.option &&
		    (esp -> data.option -> option -> universe ==
		     &dhcp_universe) &&
		    (esp -> data.option -> option -> code ==
		     DHO_DHCP_CLIENT_IDENTIFIER)) {
			evaluate_option_cache
				(&hd -> client_identifier, (struct packet *)0,
				 (struct lease *)0, (struct option_state *)0,
				 (struct option_state *)0, &global_scope,
				 esp -> data.option, MDL);
			break;
		}
	}

	/* If we got a client identifier, hash this entry by
	   client identifier. */
	if (hd -> client_identifier.len) {
		/* If there's no uid hash, make one; otherwise, see if
		   there's already an entry in the hash for this host. */
		if (!host_uid_hash) {
			host_uid_hash =
				new_hash ((hash_reference)host_reference,
					  (hash_dereference)host_dereference,
					  0);
			if (!host_uid_hash)
				log_fatal ("Can't allocate host/uid hash");

			host_hash_add (host_uid_hash,
				       hd -> client_identifier.data,
				       hd -> client_identifier.len,
				       hd, MDL);
		} else {
			/* If there's already a host declaration for this
			   client identifier, add this one to the end of the
			   list.  Otherwise, add it to the hash table. */
			if (host_hash_lookup (&hp, host_uid_hash,
					      hd -> client_identifier.data,
					      hd -> client_identifier.len,
					      MDL)) {
				/* Don't link it in twice... */
				if (!np) {
					for (np = hp; np -> n_ipaddr;
					     np = np -> n_ipaddr) {
						if (hd == np)
						    break;
					}
					if (hd != np)
					    host_reference (&np -> n_ipaddr,
							    hd, MDL);
				}
				host_dereference (&hp, MDL);
			} else {
				host_hash_add (host_uid_hash,
					       hd -> client_identifier.data,
					       hd -> client_identifier.len,
					       hd, MDL);
			}
		}
	}

	if (dynamicp && commit) {
		if (!write_host (hd))
			return ISC_R_IOERROR;
		if (!commit_leases ())
			return ISC_R_IOERROR;
	}

	return ISC_R_SUCCESS;
}

isc_result_t delete_host (hd, commit)
	struct host_decl *hd;
	int commit;
{
	struct host_decl *hp = (struct host_decl *)0;
	struct host_decl *np = (struct host_decl *)0;
	struct host_decl *foo;
	struct executable_statement *esp;
	int hw_head = 0, uid_head = 1;

	/* Don't need to do it twice. */
	if (hd -> flags & HOST_DECL_DELETED)
		return ISC_R_SUCCESS;

	/* But we do need to do it once!   :') */
	hd -> flags |= HOST_DECL_DELETED;

	if (hd -> interface.hlen) {
	    if (host_hw_addr_hash) {
		if (host_hash_lookup (&hp, host_hw_addr_hash,
			hd -> interface.hbuf,
			hd -> interface.hlen, MDL)) {
		    if (hp == hd) {
			host_hash_delete (host_hw_addr_hash,
					  hd -> interface.hbuf,
					  hd -> interface.hlen, MDL);
			hw_head = 1;
		    } else {
			for (foo = hp; foo; foo = foo -> n_ipaddr) {
			    if (foo == hd)
				    break;
			    np = foo;
			}
			if (foo) {
			    host_dereference (&np -> n_ipaddr, MDL);
			    if (hd -> n_ipaddr)
				host_reference (&np -> n_ipaddr,
						hd -> n_ipaddr, MDL);
			}
		    }
		    host_dereference (&hp, MDL);
		}
	    }
	}

	/* If we got a client identifier, hash this entry by
	   client identifier. */
	if (hd -> client_identifier.len) {
	    if (host_uid_hash) {
		if (host_hash_lookup (&hp, host_uid_hash,
				      hd -> client_identifier.data,
				      hd -> client_identifier.len, MDL)) {
		    if (hp == hd) {
			host_hash_delete (host_uid_hash,
					  hd -> client_identifier.data,
					  hd -> client_identifier.len, MDL);
			uid_head = 1;
		    } else {
			for (foo = hp; foo; foo = foo -> n_ipaddr) {
			    if (foo == hd)
				break;
			    np = foo;
			}
			if (foo) {
			    if (np -> n_ipaddr)
				host_dereference (&np -> n_ipaddr, MDL);
			    host_reference (&np -> n_ipaddr,
					    hd -> n_ipaddr, MDL);
			}
		    }
		    host_dereference (&hp, MDL);
		}
	    }
	}

	if (hd -> n_ipaddr) {
		if (uid_head && hd -> n_ipaddr -> client_identifier.len) {
			host_hash_add
				(host_uid_hash,
				 hd -> n_ipaddr -> client_identifier.data,
				 hd -> n_ipaddr -> client_identifier.len,
				 hd -> n_ipaddr, MDL);
		}
		if (hw_head && hd -> n_ipaddr -> interface.hlen) {
			host_hash_add (host_hw_addr_hash,
				       hd -> n_ipaddr -> interface.hbuf,
				       hd -> n_ipaddr -> interface.hlen,
				       hd -> n_ipaddr, MDL);
		}
		host_dereference (&hd -> n_ipaddr, MDL);
	}

	if (host_name_hash) {
		if (host_hash_lookup (&hp, host_name_hash,
				      (unsigned char *)hd -> name,
				      strlen (hd -> name), MDL)) {
			if (hp == hd && !(hp -> flags & HOST_DECL_STATIC)) {
				host_hash_delete (host_name_hash,
						  (unsigned char *)hd -> name,
						  strlen (hd -> name), MDL);
			}
			host_dereference (&hp, MDL);
		}
	}

	if (commit) {
		if (!write_host (hd))
			return ISC_R_IOERROR;
		if (!commit_leases ())
			return ISC_R_IOERROR;
	}
	return ISC_R_SUCCESS;
}

int find_hosts_by_haddr (struct host_decl **hp, int htype,
			 const unsigned char *haddr, unsigned hlen,
			 const char *file, int line)
{
	struct host_decl *foo;
	struct hardware h;

	h.hlen = hlen + 1;
	h.hbuf [0] = htype;
	memcpy (&h.hbuf [1], haddr, hlen);

	return host_hash_lookup (hp, host_hw_addr_hash,
				 h.hbuf, h.hlen, file, line);
}

int find_hosts_by_uid (struct host_decl **hp,
		       const unsigned char *data, unsigned len,
		       const char *file, int line)
{
	return host_hash_lookup (hp, host_uid_hash, data, len, file, line);
}

/* More than one host_decl can be returned by find_hosts_by_haddr or
   find_hosts_by_uid, and each host_decl can have multiple addresses.
   Loop through the list of hosts, and then for each host, through the
   list of addresses, looking for an address that's in the same shared
   network as the one specified.    Store the matching address through
   the addr pointer, update the host pointer to point at the host_decl
   that matched, and return the subnet that matched. */

int find_host_for_network (struct subnet **sp, struct host_decl **host,
			   struct iaddr *addr, struct shared_network *share)
{
	int i;
	struct subnet *subnet;
	struct iaddr ip_address;
	struct host_decl *hp;
	struct data_string fixed_addr;

	memset (&fixed_addr, 0, sizeof fixed_addr);

	for (hp = *host; hp; hp = hp -> n_ipaddr) {
		if (!hp -> fixed_addr)
			continue;
		if (!evaluate_option_cache (&fixed_addr, (struct packet *)0,
					    (struct lease *)0,
					    (struct option_state *)0,
					    (struct option_state *)0,
					    &global_scope,
					    hp -> fixed_addr, MDL))
			continue;
		for (i = 0; i < fixed_addr.len; i += 4) {
			ip_address.len = 4;
			memcpy (ip_address.iabuf,
				fixed_addr.data + i, 4);
			if (find_grouped_subnet (sp, share, ip_address, MDL)) {
				struct host_decl *tmp = (struct host_decl *)0;
				*addr = ip_address;
				/* This is probably not necessary, but
				   just in case *host is the only reference
				   to that host declaration, make a temporary
				   reference so that dereferencing it doesn't
				   dereference hp out from under us. */
				host_reference (&tmp, *host, MDL);
				host_dereference (host, MDL);
				host_reference (host, hp, MDL);
				host_dereference (&tmp, MDL);
				data_string_forget (&fixed_addr, MDL);
				return 1;
			}
		}
		data_string_forget (&fixed_addr, MDL);
	}
	return 0;
}

void new_address_range (low, high, subnet, pool)
	struct iaddr low, high;
	struct subnet *subnet;
	struct pool *pool;
{
	struct lease *address_range, *lp, *plp;
	struct iaddr net;
	unsigned min, max, i;
	char lowbuf [16], highbuf [16], netbuf [16];
	struct shared_network *share = subnet -> shared_network;
	isc_result_t status;
	struct lease *lt = (struct lease *)0;

	/* All subnets should have attached shared network structures. */
	if (!share) {
		strcpy (netbuf, piaddr (subnet -> net));
		log_fatal ("No shared network for network %s (%s)",
		       netbuf, piaddr (subnet -> netmask));
	}

	/* Initialize the hash table if it hasn't been done yet. */
	if (!lease_uid_hash) {
		lease_uid_hash =
			new_hash ((hash_reference)lease_reference,
				  (hash_dereference)lease_dereference, 0);
		if (!lease_uid_hash)
			log_fatal ("Can't allocate lease/uid hash");
	}
	if (!lease_ip_addr_hash) {
		lease_ip_addr_hash =
			new_hash ((hash_reference)lease_reference,
				  (hash_dereference)lease_dereference, 0);
		if (!lease_uid_hash)
			log_fatal ("Can't allocate lease/ip hash");
	}
	if (!lease_hw_addr_hash) {
		lease_hw_addr_hash =
			new_hash ((hash_reference)lease_reference,
				  (hash_dereference)lease_dereference, 0);
		if (!lease_uid_hash)
			log_fatal ("Can't allocate lease/hw hash");
	}

	/* Make sure that high and low addresses are in same subnet. */
	net = subnet_number (low, subnet -> netmask);
	if (!addr_eq (net, subnet_number (high, subnet -> netmask))) {
		strcpy (lowbuf, piaddr (low));
		strcpy (highbuf, piaddr (high));
		strcpy (netbuf, piaddr (subnet -> netmask));
		log_fatal ("Address range %s to %s, netmask %s spans %s!",
		       lowbuf, highbuf, netbuf, "multiple subnets");
	}

	/* Make sure that the addresses are on the correct subnet. */
	if (!addr_eq (net, subnet -> net)) {
		strcpy (lowbuf, piaddr (low));
		strcpy (highbuf, piaddr (high));
		strcpy (netbuf, piaddr (subnet -> netmask));
		log_fatal ("Address range %s to %s not on net %s/%s!",
		       lowbuf, highbuf, piaddr (subnet -> net), netbuf);
	}

	/* Get the high and low host addresses... */
	max = host_addr (high, subnet -> netmask);
	min = host_addr (low, subnet -> netmask);

	/* Allow range to be specified high-to-low as well as low-to-high. */
	if (min > max) {
		max = min;
		min = host_addr (high, subnet -> netmask);
	}

	/* Get a lease structure for each address in the range. */
#if defined (COMPACT_LEASES)
	address_range = new_leases (max - min + 1, MDL);
	if (!address_range) {
		strcpy (lowbuf, piaddr (low));
		strcpy (highbuf, piaddr (high));
		log_fatal ("No memory for address range %s-%s.",
			   lowbuf, highbuf);
	}
#endif

	/* Fill out the lease structures with some minimal information. */
	for (i = 0; i < max - min + 1; i++) {
		struct lease *lp = (struct lease *)0;
#if defined (COMPACT_LEASES)
		omapi_object_initialize ((omapi_object_t *)&address_range [i],
					 dhcp_type_lease,
					 0, sizeof (struct lease), MDL);
		lease_reference (&lp, &address_range [i], MDL);
#else
		status = lease_allocate (&lp, MDL);
		if (status != ISC_R_SUCCESS)
			log_fatal ("No memory for lease %s: %s",
				   piaddr (ip_addr (subnet -> net,
						    subnet -> netmask,
						    i + min)),
				   isc_result_totext (status));
#endif
		lp -> ip_addr = ip_addr (subnet -> net,
					 subnet -> netmask, i + min);
		lp -> starts = lp -> timestamp = MIN_TIME;
		lp -> ends = MIN_TIME;
		subnet_reference (&lp -> subnet, subnet, MDL);
		pool_reference (&lp -> pool, pool, MDL);
		lp -> binding_state = FTS_FREE;
		lp -> next_binding_state = FTS_FREE;
		lp -> flags = 0;

		/* Remember the lease in the IP address hash. */
		if (find_lease_by_ip_addr (&lt, lp -> ip_addr, MDL)) {
			if (lt -> pool) {
				log_error ("duplicate entries for lease %s",
					   piaddr (lp -> ip_addr));
			} else
				pool_reference (&lt -> pool, pool, MDL);
			lease_dereference (&lt, MDL);
		} else
			lease_hash_add (lease_ip_addr_hash,
					lp -> ip_addr.iabuf,
					lp -> ip_addr.len, lp, MDL);
		lease_dereference (&lp, MDL);
	}
}


#if defined (COMPACT_LEASES)
struct lease *free_leases;

/* If we are allocating leases in aggregations, there's really no way
   to free one, although perhaps we can maintain a free list. */

isc_result_t dhcp_lease_free (omapi_object_t *lo,
			      const char *file, int line)
{
	struct lease *lease;
	if (lo -> type != dhcp_type_lease)
		return ISC_R_INVALIDARG;
	lease = (struct lease *)lo;
	if (free_leases) {
		lease_reference (&lease -> next, free_leases, file, line);
		lease_dereference (&free_leases, file, line);
	}
	lease_reference (&free_leases, lease, MDL);
	return ISC_R_SUCCESS;
}

isc_result_t dhcp_lease_get (omapi_object_t **lp,
			     const char *file, int line)
{
	struct lease **lease = (struct lease **)lp;

	if (free_leases) {
		lease_reference (lease, free_leases, file, line);
		lease_dereference (&free_leases, file, line);
		if ((*lease) -> next) {
			lease_reference (&free_leases, (*lease) -> next,
					 file, line);
			lease_dereference (&(*lease) -> next, file, line);
		}
		return ISC_R_SUCCESS;
	}
	return ISC_R_NOMEMORY;
}
#endif

int find_subnet (struct subnet **sp,
		 struct iaddr addr, const char *file, int line)
{
	struct subnet *rv;

	for (rv = subnets; rv; rv = rv -> next_subnet) {
		if (addr_eq (subnet_number (addr, rv -> netmask), rv -> net)) {
			if (subnet_reference (sp, rv,
					      file, line) != ISC_R_SUCCESS)
				return 0;
			return 1;
		}
	}
	return 0;
}

int find_grouped_subnet (struct subnet **sp,
			 struct shared_network *share, struct iaddr addr,
			 const char *file, int line)
{
	struct subnet *rv;

	for (rv = share -> subnets; rv; rv = rv -> next_sibling) {
		if (addr_eq (subnet_number (addr, rv -> netmask), rv -> net)) {
			if (subnet_reference (sp, rv,
					      file, line) != ISC_R_SUCCESS)
				return 0;
			return 1;
		}
	}
	return 0;
}

int subnet_inner_than (subnet, scan, warnp)
	struct subnet *subnet, *scan;
	int warnp;
{
	if (addr_eq (subnet_number (subnet -> net, scan -> netmask),
		     scan -> net) ||
	    addr_eq (subnet_number (scan -> net, subnet -> netmask),
		     subnet -> net)) {
		char n1buf [16];
		int i, j;
		for (i = 0; i < 32; i++)
			if (subnet -> netmask.iabuf [3 - (i >> 3)]
			    & (1 << (i & 7)))
				break;
		for (j = 0; j < 32; j++)
			if (scan -> netmask.iabuf [3 - (j >> 3)] &
			    (1 << (j & 7)))
				break;
		strcpy (n1buf, piaddr (subnet -> net));
		if (warnp)
			log_error ("%ssubnet %s/%d overlaps subnet %s/%d",
			      "Warning: ", n1buf, 32 - i,
			      piaddr (scan -> net), 32 - j);
		if (i < j)
			return 1;
	}
	return 0;
}

/* Enter a new subnet into the subnet list. */
void enter_subnet (subnet)
	struct subnet *subnet;
{
	struct subnet *scan, *prev = (struct subnet *)0;

	/* Check for duplicates... */
	for (scan = subnets; scan; scan = scan -> next_subnet) {
		/* When we find a conflict, make sure that the
		   subnet with the narrowest subnet mask comes
		   first. */
		if (subnet_inner_than (subnet, scan, 1)) {
			if (prev) {
				subnet_reference (&prev -> next_subnet,
						  subnet, MDL);
			} else
				subnet_reference (&subnets, subnet, MDL);
			subnet_reference (&subnet -> next_subnet, scan, MDL);
			return;
		}
		prev = scan;
	}

	/* XXX use the BSD radix tree code instead of a linked list. */
	subnet -> next_subnet = subnets;
	subnets = subnet;
}
	
/* Enter a new shared network into the shared network list. */

void enter_shared_network (share)
	struct shared_network *share;
{
	if (shared_networks) {
		shared_network_reference (&share -> next,
					  shared_networks, MDL);
		shared_network_dereference (&shared_networks, MDL);
	}
	shared_network_reference (&shared_networks, share, MDL);
}
	
void new_shared_network_interface (cfile, share, name)
	struct parse *cfile;
	struct shared_network *share;
	const char *name;
{
	struct interface_info *ip;
	isc_result_t status;

	if (share -> interface) {
		parse_warn (cfile, 
			    "A subnet or shared network can't be connected %s",
			    "to two interfaces.");
		return;
	}
	
	for (ip = interfaces; ip; ip = ip -> next)
		if (!strcmp (ip -> name, name))
			break;
	if (!ip) {
		status = interface_allocate (&ip, MDL);
		if (status != ISC_R_SUCCESS)
			log_fatal ("new_shared_network_interface %s: %s",
				   name, isc_result_totext (status));
		if (strlen (name) > sizeof ip -> name) {
			memcpy (ip -> name, name, (sizeof ip -> name) - 1);
			ip -> name [(sizeof ip -> name) - 1] = 0;
		} else
			strcpy (ip -> name, name);
		if (interfaces) {
			interface_reference (&ip -> next, interfaces, MDL);
			interface_dereference (&interfaces, MDL);
		}
		interface_reference (&interfaces, ip, MDL);
		ip -> flags = INTERFACE_REQUESTED;
		/* XXX this is a reference loop. */
		shared_network_reference (&ip -> shared_network, share, MDL);
		interface_reference (&share -> interface, ip, MDL);
	}
}

/* Enter a lease into the system.   This is called by the parser each
   time it reads in a new lease.   If the subnet for that lease has
   already been read in (usually the case), just update that lease;
   otherwise, allocate temporary storage for the lease and keep it around
   until we're done reading in the config file. */

void enter_lease (lease)
	struct lease *lease;
{
	struct lease *comp = (struct lease *)0;
	isc_result_t status;

	if (find_lease_by_ip_addr (&comp, lease -> ip_addr, MDL)) {
		if (!comp -> pool) {
			log_error ("undeclared lease found in database: %s",
				   piaddr (lease -> ip_addr));
		} else
			pool_reference (&lease -> pool, comp -> pool, MDL);

		if (comp -> subnet)
			subnet_reference (&lease -> subnet,
					  comp -> subnet, MDL);

		lease_hash_delete (lease_ip_addr_hash,
				   lease -> ip_addr.iabuf,
				   lease -> ip_addr.len, MDL);
		lease_dereference (&comp, MDL);
	}
	lease_hash_add (lease_ip_addr_hash,
			lease -> ip_addr.iabuf,
			lease -> ip_addr.len, lease, MDL);
}

/* Replace the data in an existing lease with the data in a new lease;
   adjust hash tables to suit, and insertion sort the lease into the
   list of leases by expiry time so that we can always find the oldest
   lease. */

int supersede_lease (comp, lease, commit, propogate, pimmediate)
	struct lease *comp, *lease;
	int commit;
	int propogate;
	int pimmediate;
{
	int enter_uid = 0;
	int enter_hwaddr = 0;
	struct lease *lp, **lq, *prev;
	TIME lp_next_state;

	/* If there is no sample lease, just do the move. */
	if (!lease)
		goto just_move_it;

	/* Static leases are not currently kept in the database... */
	if (lease -> flags & STATIC_LEASE)
		return 1;

	/* If the existing lease hasn't expired and has a different
	   unique identifier or, if it doesn't have a unique
	   identifier, a different hardware address, then the two
	   leases are in conflict.  If the existing lease has a uid
	   and the new one doesn't, but they both have the same
	   hardware address, and dynamic bootp is allowed on this
	   lease, then we allow that, in case a dynamic BOOTP lease is
	   requested *after* a DHCP lease has been assigned. */

	if (lease -> binding_state != FTS_ABANDONED &&
	    (comp -> binding_state == FTS_ACTIVE ||
	     comp -> binding_state == FTS_RESERVED ||
	     comp -> binding_state == FTS_BOOTP) &&
	    (((comp -> uid && lease -> uid) &&
	      (comp -> uid_len != lease -> uid_len ||
	       memcmp (comp -> uid, lease -> uid, comp -> uid_len))) ||
	     (!comp -> uid &&
	      ((comp -> hardware_addr.hlen !=
		lease -> hardware_addr.hlen) ||
	       memcmp (comp -> hardware_addr.hbuf,
		       lease -> hardware_addr.hbuf,
		       comp -> hardware_addr.hlen))))) {
		log_error ("Lease conflict at %s",
		      piaddr (comp -> ip_addr));
		return 0;
	}

	/* If there's a Unique ID, dissociate it from the hash
	   table and free it if necessary. */
	if (comp -> uid) {
		uid_hash_delete (comp);
		enter_uid = 1;
		if (comp -> uid != &comp -> uid_buf [0]) {
			dfree (comp -> uid, MDL);
			comp -> uid_max = 0;
			comp -> uid_len = 0;
		}
		comp -> uid = (unsigned char *)0;
	} else
		enter_uid = 1;
	
	if (comp -> hardware_addr.hlen &&
	    ((comp -> hardware_addr.hlen !=
	      lease -> hardware_addr.hlen) ||
	     memcmp (comp -> hardware_addr.hbuf,
		     lease -> hardware_addr.hbuf,
		     comp -> hardware_addr.hlen))) {
		hw_hash_delete (comp);
		enter_hwaddr = 1;
	} else if (!comp -> hardware_addr.hlen)
		enter_hwaddr = 1;
	
	/* If the lease has been billed to a class, remove the billing. */
	if (comp -> billing_class != lease -> billing_class) {
		if (comp -> billing_class)
			unbill_class (comp, comp -> billing_class);
		if (lease -> billing_class)
			bill_class (comp, lease -> billing_class);
	}

	/* Copy the data files, but not the linkages. */
	comp -> starts = lease -> starts;
	if (lease -> uid) {
		if (lease -> uid_len <= sizeof (lease -> uid_buf)) {
			memcpy (comp -> uid_buf,
				lease -> uid, lease -> uid_len);
			comp -> uid = &comp -> uid_buf [0];
			comp -> uid_max = sizeof comp -> uid_buf;
		} else if (lease -> uid != &lease -> uid_buf [0]) {
			comp -> uid = lease -> uid;
			comp -> uid_max = lease -> uid_max;
			lease -> uid = (unsigned char *)0;
			lease -> uid_max = 0;
		} else {
			log_fatal ("corrupt lease uid."); /* XXX */
		}
	} else {
		comp -> uid = (unsigned char *)0;
		comp -> uid_max = 0;
	}
	comp -> uid_len = lease -> uid_len;
	if (comp -> host)
		host_dereference (&comp -> host, MDL);
	host_reference (&comp -> host, lease -> host, MDL);
	comp -> hardware_addr = lease -> hardware_addr;
	comp -> flags = ((lease -> flags & ~PERSISTENT_FLAGS) |
			 (comp -> flags & ~EPHEMERAL_FLAGS));
	if (comp -> scope.bindings)
		free_bindings (&comp -> scope, MDL);
	comp -> scope.bindings = lease -> scope.bindings;
	lease -> scope.bindings = (struct binding *)0;

	/* Record the hostname information in the lease. */
	if (comp -> hostname)
		dfree (comp -> hostname, MDL);
	comp -> hostname = lease -> hostname;
	lease -> hostname = (char *)0;
	if (comp -> client_hostname)
		dfree (comp -> client_hostname, MDL);
	comp -> client_hostname = lease -> client_hostname;
	lease -> client_hostname = (char *)0;

	if (lease -> on_expiry) {
		if (comp -> on_expiry)
			executable_statement_dereference (&comp -> on_expiry,
							  MDL);
		executable_statement_reference (&comp -> on_expiry,
						lease -> on_expiry,
						MDL);
	}
	if (lease -> on_commit) {
		if (comp -> on_commit)
			executable_statement_dereference (&comp -> on_commit,
							  MDL);
		executable_statement_reference (&comp -> on_commit,
						lease -> on_commit,
						MDL);
	}
	if (lease -> on_release) {
		if (comp -> on_release)
			executable_statement_dereference (&comp -> on_release,
							  MDL);
		executable_statement_reference (&comp -> on_release,
						lease -> on_release, MDL);
	}
	
	/* Record the lease in the uid hash if necessary. */
	if (enter_uid && lease -> uid) {
		uid_hash_add (comp);
	}
	
	/* Record it in the hardware address hash if necessary. */
	if (enter_hwaddr && lease -> hardware_addr.hlen) {
		hw_hash_add (comp);
	}
	
#if defined (FAILOVER_PROTOCOL)
	comp -> cltt = lease -> cltt;
	comp -> tstp = lease -> tstp;
	comp -> tsfp = lease -> tsfp;
#endif /* FAILOVER_PROTOCOL */
	comp -> ends = lease -> ends;
	comp -> next_binding_state = lease -> next_binding_state;

      just_move_it:
	/* Figure out which queue it's on. */
	switch (comp -> binding_state) {
	      case FTS_FREE:
		lq = &comp -> pool -> free;
		comp -> pool -> free_leases--;
		break;

	      case FTS_ACTIVE:
	      case FTS_RESERVED:
	      case FTS_BOOTP:
		lq = &comp -> pool -> active;
		break;

	      case FTS_EXPIRED:
	      case FTS_RELEASED:
	      case FTS_RESET:
		lq = &comp -> pool -> expired;
		break;

	      case FTS_ABANDONED:
		lq = &comp -> pool -> abandoned;
		break;

	      case FTS_BACKUP:
		lq = &comp -> pool -> backup;
		comp -> pool -> backup_leases--;
		break;

	      default:
		log_error ("Lease with bogus binding state: %d",
			   comp -> binding_state);
		return 0;
	}

	/* Remove the lease from its current place in its current
	   timer sequence. */
	prev = (struct lease *)0;
	for (lp = *lq; lp; lp = lp -> next) {
		if (lp == comp)
			break;
		prev = lp;
	}

	if (!lp) {
		log_error ("Lease with binding state %s not on its queue.",
			   (comp -> binding_state < 1 &&
			    comp -> binding_state < FTS_BOOTP)
			   ? "unknown"
			   : binding_state_names [comp -> binding_state - 1]);
		return 0;
	}
	
	if (prev) {
		lease_dereference (&prev -> next, MDL);
		if (comp -> next) {
			lease_reference (&prev -> next, comp -> next, MDL);
			lease_dereference (&comp -> next, MDL);
		}
	} else {
		lease_dereference (lq, MDL);
		if (comp -> next) {
			lease_reference (lq, comp -> next, MDL);
			lease_dereference (&comp -> next, MDL);
		}
	}

	/* Make the state transition. */
	if (commit)
		process_state_transition (comp);

	/* Put the lease back on the appropriate queue.    If the lease
	   is corrupt (as detected by lease_enqueue), don't go any farther. */
	if (!lease_enqueue (comp))
		return 0;

	/* If this is the next lease that will timeout on the pool,
	   zap the old timeout and set the timeout on this pool to the
	   time that the lease's next event will happen.
		   
	   We do not actually set the timeout unless commit is true -
	   we don't want to thrash the timer queue when reading the
	   lease database.  Instead, the database code calls the
	   expiry event on each pool after reading in the lease file,
	   and the expiry code sets the timer if there's anything left
	   to expire after it's run any outstanding expiry events on
	   the pool. */
	if (commit &&
	    comp -> sort_time != MIN_TIME &&
	    comp -> sort_time > cur_time &&
	    (comp -> sort_time < comp -> pool -> next_event_time ||
	     comp -> pool -> next_event_time == MIN_TIME)) {
		comp -> pool -> next_event_time = comp -> sort_time;
		add_timeout (comp -> pool -> next_event_time,
			     pool_timer, comp -> pool,
			     (tvref_t)pool_reference,
			     (tvunref_t)pool_dereference);
	}

	/* Return zero if we didn't commit the lease to permanent storage;
	   nonzero if we did. */
	return commit && write_lease (comp) && commit_leases ()
#if defined (FAILOVER_PROTOCOL)
		&& (!propogate ||
		    dhcp_failover_queue_update (comp, pimmediate))
#endif
		;
}

void process_state_transition (struct lease *lease)
{
	/* If the lease was active and is now no longer active, but isn't
	   released, then it just expired, so do the expiry event. */
	if (lease -> next_binding_state != lease -> binding_state &&
	    (lease -> binding_state == FTS_ACTIVE ||
	     lease -> binding_state == FTS_BOOTP ||
	     lease -> binding_state == FTS_RESERVED) &&
	    lease -> next_binding_state != FTS_RELEASED) {
		if (lease -> on_expiry) {
			execute_statements ((struct packet *)0, lease,
					    (struct option_state *)0,
					    (struct option_state *)0, /* XXX */
					    &lease -> scope,
					    lease -> on_expiry);
			if (lease -> on_expiry)
				executable_statement_dereference
					(&lease -> on_expiry, MDL);
		}
		
		/* No sense releasing a lease after it's expired. */
		if (lease -> on_release)
			executable_statement_dereference (&lease -> on_release,
							  MDL);
		/* Send the expiry time to the peer. */
		lease -> tstp = lease -> ends;
	}

	/* If the lease was active and is now released, do the release
	   event. */
	if ((lease -> binding_state == FTS_ACTIVE ||
	     lease -> binding_state == FTS_BOOTP ||
	     lease -> binding_state == FTS_RESERVED) &&
	    lease -> next_binding_state == FTS_RELEASED) {
		if (lease -> on_release) {
			execute_statements ((struct packet *)0, lease,
					    (struct option_state *)0,
					    (struct option_state *)0, /* XXX */
					    &lease -> scope,
					    lease -> on_release);
			executable_statement_dereference (&lease -> on_release,
							  MDL);
		}
		
		/* A released lease can't expire. */
		if (lease -> on_expiry)
			executable_statement_dereference (&lease -> on_expiry,
							  MDL);

		/* Send the release time (should be == cur_time) to the
		   peer. */
		lease -> tstp = lease -> ends;
	}

	lease -> binding_state = lease -> next_binding_state;
	if (lease -> binding_state == FTS_ACTIVE ||
	    lease -> binding_state == FTS_BACKUP) {
#if defined (FAILOVER_PROTOCOL)
		if (lease -> pool && lease -> pool -> failover_peer)
			lease -> next_binding_state = FTS_EXPIRED;
		else
#endif
			lease -> next_binding_state = FTS_FREE;
	}
}

/* Copy the contents of one lease into another, correctly maintaining
   reference counts. */
int lease_copy (struct lease **lp,
		struct lease *lease, const char *file, int line)
{
	struct lease *lt = (struct lease *)0;
	isc_result_t status;

	status = lease_allocate (&lt, MDL);
	if (status != ISC_R_SUCCESS)
		return 0;

	lt -> ip_addr = lease -> ip_addr;
	lt -> starts = lease -> starts;
	lt -> ends = lease -> ends;
	lt -> timestamp = lease -> timestamp;
	lt -> uid_len = lease -> uid_len;
	lt -> uid_max = lease -> uid_max;
	if (lease -> uid == lease -> uid_buf) {
		lt -> uid = lt -> uid_buf;
		memcpy (lt -> uid_buf, lease -> uid_buf, sizeof lt -> uid_buf);
	} else {
		lt -> uid = dmalloc (lt -> uid_max, MDL);
		if (!lt -> uid) {
			lease_dereference (&lt, MDL);
			return 0;
		}
		memcpy (lt -> uid, lease -> uid, lease -> uid_max);
	}
	if (lease -> hostname) {
		lt -> hostname = dmalloc (strlen (lease -> hostname) + 1, MDL);
		if (!lt -> hostname) {
			lease_dereference (&lt, MDL);
			return 0;
		}
		strcpy (lt -> hostname, lease -> hostname);
	}
	if (lease -> client_hostname) {
		lt -> client_hostname =
			dmalloc (strlen (lease -> client_hostname) + 1, MDL);
		if (!lt -> client_hostname) {
			lease_dereference (&lt, MDL);
			return 0;
		}
		strcpy (lt -> client_hostname, lease -> client_hostname);
	}
	lt -> scope = lease -> scope;
	host_reference (&lt -> host, lease -> host, file, line);
	subnet_reference (&lt -> subnet, lease -> subnet, file, line);
	pool_reference (&lt -> pool, lease -> pool, file, line);
	class_reference (&lt -> billing_class,
			 lease -> billing_class, file, line);
	lt -> hardware_addr = lease -> hardware_addr;
	if (lease -> on_expiry)
		executable_statement_reference (&lt -> on_expiry,
						lease -> on_expiry,
						file, line);
	if (lease -> on_commit)
		executable_statement_reference (&lt -> on_commit,
						lease -> on_commit,
						file, line);
	if (lease -> on_release)
		executable_statement_reference (&lt -> on_release,
						lease -> on_release,
						file, line);
	lt -> flags = lease -> flags;
	lt -> tstp = lease -> tstp;
	lt -> tsfp = lease -> tsfp;
	lt -> cltt = lease -> cltt;
	lt -> binding_state = lease -> binding_state;
	lt -> next_binding_state = lease -> next_binding_state;
	status = lease_reference (lp, lt, file, line);
	lease_dereference (&lt, MDL);
	return status == ISC_R_SUCCESS;
}

/* Release the specified lease and re-hash it as appropriate. */
void release_lease (lease, packet)
	struct lease *lease;
	struct packet *packet;
{
	/* If there are statements to execute when the lease is
	   released, execute them. */
	if (lease -> on_release) {
		execute_statements (packet, lease, packet -> options,
				    (struct option_state *)0, /* XXX */
				    &lease -> scope, lease -> on_release);
		if (lease -> on_release)
			executable_statement_dereference (&lease -> on_release,
							  MDL);
	}

	/* We do either the on_release or the on_expiry events, but
	   not both (it's possible that they could be the same,
	   in any case). */
	if (lease -> on_expiry)
		executable_statement_dereference (&lease -> on_expiry, MDL);

	if (lease -> ends > cur_time) {
		if (lease -> on_commit)
			executable_statement_dereference (&lease -> on_commit,
							  MDL);

		/* Blow away any bindings. */
		/* XXX free them?!? */
		lease -> scope.bindings = (struct binding *)0;
		lease -> ends = cur_time;
#if defined (FAILOVER_PROTOCOL)
		if (lease -> pool && lease -> pool -> failover_peer) {
			lease -> next_binding_state = FTS_RELEASED;
		} else {
			lease -> next_binding_state = FTS_FREE;
		}
#else
		lease -> next_binding_state = FTS_FREE;
#endif
		if (lease -> billing_class)
			class_dereference (&lease -> billing_class, MDL);
		supersede_lease (lease, (struct lease *)0, 1, 1, 1);
	}
}

/* Abandon the specified lease (set its timeout to infinity and its
   particulars to zero, and re-hash it as appropriate. */

void abandon_lease (lease, message)
	struct lease *lease;
	const char *message;
{
	struct lease *lt = (struct lease *)0;

	if (!lease_copy (&lt, lease, MDL))
		return;

	if (lt -> on_expiry)
		executable_statement_dereference (&lease -> on_expiry, MDL);
	if (lt -> on_release)
		executable_statement_dereference (&lease -> on_release, MDL);
	if (lt -> on_commit)
		executable_statement_dereference (&lease -> on_commit, MDL);

	/* Blow away any bindings. */
	lt -> scope.bindings = (struct binding *)0;
	lt -> ends = cur_time; /* XXX */
	lt -> next_binding_state = FTS_ABANDONED;

	log_error ("Abandoning IP address %s: %s",
	      piaddr (lease -> ip_addr), message);
	lt -> hardware_addr.hlen = 0;
	if (lt -> uid != lt -> uid_buf)
		dfree (lt -> uid, MDL);
	lt -> uid = (unsigned char *)0;
	lt -> uid_len = 0;
	lt -> uid_max = 0;
	if (lt -> billing_class)
		class_dereference (&lt -> billing_class, MDL);
	supersede_lease (lease, lt, 1, 1, 1);
	lease_dereference (&lt, MDL);
}

/* Abandon the specified lease (set its timeout to infinity and its
   particulars to zero, and re-hash it as appropriate. */

void dissociate_lease (lease)
	struct lease *lease;
{
	struct lease *lt = (struct lease *)0;

	if (!lease_copy (&lt, lease, MDL))
		return;

	if (lt -> on_expiry)
		executable_statement_dereference (&lease -> on_expiry, MDL);
	if (lt -> on_release)
		executable_statement_dereference (&lease -> on_release, MDL);
	if (lt -> on_commit)
		executable_statement_dereference (&lease -> on_commit, MDL);

	/* Blow away any bindings. */
	lt -> scope.bindings = (struct binding *)0;

#if defined (FAILOVER_PROTOCOL)
	if (lease -> pool && lease -> pool -> failover_peer) {
		lt -> next_binding_state = FTS_RESET;
	} else {
		lt -> next_binding_state = FTS_FREE;
	}
#else
	lt -> next_binding_state = FTS_FREE;
#endif
	lt -> ends = cur_time; /* XXX */
	lt -> hardware_addr.hlen = 0;
	if (lt -> uid != lt -> uid_buf)
		dfree (lt -> uid, MDL);
	lt -> uid = (unsigned char *)0;
	lt -> uid_len = 0;
	lt -> uid_max = 0;
	if (lt -> billing_class)
		class_dereference (&lt -> billing_class, MDL);
	supersede_lease (lease, lt, 1, 1, 1);
	lease_dereference (&lt, MDL);
}

/* Timer called when a lease in a particular pool expires. */
void pool_timer (vpool)
	void *vpool;
{
	struct pool *pool;
	struct lease *lt = (struct lease *)0;
	struct lease *next = (struct lease *)0;
	struct lease *lease = (struct lease *)0;
	struct lease **lptr [5];
	TIME next_expiry = MAX_TIME;
	int i;

	pool = (struct pool *)vpool;

#define FREE_LEASES 0
	lptr [FREE_LEASES] = &pool -> free;
#define ACTIVE_LEASES 1
	lptr [ACTIVE_LEASES] = &pool -> active;
#define EXPIRED_LEASES 2
	lptr [EXPIRED_LEASES] = &pool -> expired;
#define ABANDONED_LEASES 3
	lptr [ABANDONED_LEASES] = &pool -> abandoned;
#define BACKUP_LEASES 4
	lptr [BACKUP_LEASES] = &pool -> backup;

	for (i = FREE_LEASES; i <= BACKUP_LEASES; i++) {
		/* If there's nothing on the queue, skip it. */
		if (!*(lptr [i]))
			continue;
		
		lease_reference (&lease, *(lptr [i]), MDL);

		while (lease) {
			/* Remember the next lease in the list. */
			if (next)
				lease_dereference (&next, MDL);
			if (lease -> next)
				lease_reference (&next, lease -> next, MDL);

			/* If we've run out of things to expire on this list,
			   stop. */
			if (lease -> sort_time > cur_time) {
				if (lease -> sort_time < next_expiry)
					next_expiry = lease -> sort_time;
				break;
			}

			/* If there is a pending state change, and
			   this lease has gotten to the time when the
			   state change should happen, just call
			   supersede_lease on it to make the change
			   happen. */
			if (lease -> next_binding_state !=
			    lease -> binding_state)
				supersede_lease (lease,
						 (struct lease *)0, 1, 1, 1);

			lease_dereference (&lease, MDL);
			if (next)
				lease_reference (&lease, next, MDL);
		}
		if (next)
			lease_dereference (&next, MDL);
		if (lease)
			lease_dereference (&lease, MDL);
	}
	if (next_expiry != MAX_TIME) {
		pool -> next_event_time = next_expiry;
		add_timeout (pool -> next_event_time, pool_timer, pool,
			     (tvref_t)pool_reference,
			     (tvunref_t)pool_dereference);
	} else
		pool -> next_event_time = MIN_TIME;

}

/* Locate the lease associated with a given IP address... */

int find_lease_by_ip_addr (struct lease **lp, struct iaddr addr,
			   const char *file, int line)
{
	return lease_hash_lookup (lp, lease_ip_addr_hash,
				  addr.iabuf, addr.len, file, line);
}

int find_lease_by_uid (struct lease **lp, const unsigned char *uid,
		       unsigned len, const char *file, int line)
{
	if (len == 0)
		return 0;
	return lease_hash_lookup (lp, lease_uid_hash, uid, len, file, line);
}

int find_lease_by_hw_addr (struct lease **lp,
			   const unsigned char *hwaddr, unsigned hwlen,
			   const char *file, int line)
{
	if (hwlen == 0)
		return 0;
	return lease_hash_lookup (lp, lease_hw_addr_hash,
				  hwaddr, hwlen, file, line);
}

/* Add the specified lease to the uid hash. */

void uid_hash_add (lease)
	struct lease *lease;
{
	struct lease *head = (struct lease *)0;
	struct lease *scan;


	/* If it's not in the hash, just add it. */
	if (!find_lease_by_uid (&head, lease -> uid, lease -> uid_len, MDL))
		lease_hash_add (lease_uid_hash, lease -> uid,
				lease -> uid_len, lease, MDL);
	else {
		/* Otherwise, attach it to the end of the list. */
		for (scan = head; scan -> n_uid; scan = scan -> n_uid)
			;
		lease_reference (&scan -> n_uid, lease, MDL);
		lease_dereference (&head, MDL);
	}
}

/* Delete the specified lease from the uid hash. */

void uid_hash_delete (lease)
	struct lease *lease;
{
	struct lease *head = (struct lease *)0;
	struct lease *scan;

	/* If it's not in the hash, we have no work to do. */
	if (!find_lease_by_uid (&head, lease -> uid, lease -> uid_len, MDL)) {
		if (lease -> n_uid)
			lease_dereference (&lease -> n_uid, MDL);
		return;
	}

	/* If the lease we're freeing is at the head of the list,
	   remove the hash table entry and add a new one with the
	   next lease on the list (if there is one). */
	if (head == lease) {
		lease_hash_delete (lease_uid_hash,
				   lease -> uid, lease -> uid_len, MDL);
		if (lease -> n_uid) {
			lease_hash_add (lease_uid_hash,
					lease -> n_uid -> uid,
					lease -> n_uid -> uid_len,
					lease -> n_uid, MDL);
			lease_dereference (&lease -> n_uid, MDL);
		}
	} else {
		/* Otherwise, look for the lease in the list of leases
		   attached to the hash table entry, and remove it if
		   we find it. */
		for (scan = head; scan -> n_uid; scan = scan -> n_uid) {
			if (scan -> n_uid == lease) {
				lease_dereference (&scan -> n_uid, MDL);
				if (lease -> n_uid) {
					lease_reference (&scan -> n_uid,
							 lease -> n_uid, MDL);
					lease_dereference (&lease -> n_uid,
							   MDL);
				}
				break;
			}
		}
		lease_dereference (&head, MDL);
	}
}

/* Add the specified lease to the hardware address hash. */

void hw_hash_add (lease)
	struct lease *lease;
{
	struct lease *head = (struct lease *)0;
	struct lease *scan;

	/* If it's not in the hash, just add it. */
	if (!find_lease_by_hw_addr (&head, lease -> hardware_addr.hbuf,
				    lease -> hardware_addr.hlen, MDL))
		lease_hash_add (lease_hw_addr_hash,
				lease -> hardware_addr.hbuf,
				lease -> hardware_addr.hlen,
				lease, MDL);
	else {
		/* Otherwise, attach it to the end of the list. */
		for (scan = head; scan -> n_hw; scan = scan -> n_hw)
			;
		lease_reference (&scan -> n_hw, lease, MDL);
		lease_dereference (&head, MDL);
	}
}

/* Delete the specified lease from the hardware address hash. */

void hw_hash_delete (lease)
	struct lease *lease;
{
	struct lease *head = (struct lease *)0;
	struct lease *scan;

	/* If it's not in the hash, we have no work to do. */
	if (!find_lease_by_hw_addr (&head, lease -> hardware_addr.hbuf,
				    lease -> hardware_addr.hlen, MDL)) {
		if (lease -> n_hw)
			lease_dereference (&lease -> n_hw, MDL);
		return;
	}

	/* If the lease we're freeing is at the head of the list,
	   remove the hash table entry and add a new one with the
	   next lease on the list (if there is one). */
	if (head == lease) {
		lease_hash_delete (lease_hw_addr_hash,
				   lease -> hardware_addr.hbuf,
				   lease -> hardware_addr.hlen, MDL);
		if (lease -> n_hw) {
			lease_hash_add (lease_hw_addr_hash,
					lease -> n_hw -> hardware_addr.hbuf,
					lease -> n_hw -> hardware_addr.hlen,
					lease -> n_hw, MDL);
			lease_dereference (&lease -> n_hw, MDL);
		}
	} else {
		/* Otherwise, look for the lease in the list of leases
		   attached to the hash table entry, and remove it if
		   we find it. */
		for (scan = head; scan -> n_hw; scan = scan -> n_hw) {
			if (scan -> n_hw == lease) {
				lease_dereference (&scan -> n_hw, MDL);
				if (lease -> n_hw) {
					lease_reference (&scan -> n_hw,
							 lease -> n_hw, MDL);
					lease_dereference (&lease -> n_hw,
							   MDL);
				}
				break;
			}
		}
	}
	lease_dereference (&head, MDL);
}

/* Write all interesting leases to permanent storage. */

void write_leases ()
{
	struct lease *l;
	struct shared_network *s;
	struct pool *p;
	struct host_decl *hp;
	struct group_object *gp;
	struct hash_bucket *hb;
	int i;
	int num_written;
	struct lease **lptr [5];

	/* Write all the dynamically-created group declarations. */
	if (group_name_hash) {
	    num_written = 0;
	    for (i = 0; i < group_name_hash -> hash_count; i++) {
		for (hb = group_name_hash -> buckets [i];
		     hb; hb = hb -> next) {
			gp = (struct group_object *)hb -> value;
			if ((gp -> flags & GROUP_OBJECT_DYNAMIC) ||
			    ((gp -> flags & GROUP_OBJECT_STATIC) &&
			     (gp -> flags & GROUP_OBJECT_DELETED))) {
				write_group (gp);
				++num_written;
			}
		}
	    }
	    log_info ("Wrote %d group decls to leases file.", num_written);
	}

	/* Write all the deleted host declarations. */
	if (host_name_hash) {
	    num_written = 0;
	    for (i = 0; i < host_name_hash -> hash_count; i++) {
		for (hb = host_name_hash -> buckets [i];
		     hb; hb = hb -> next) {
			hp = (struct host_decl *)hb -> value;
			if (((hp -> flags & HOST_DECL_STATIC) &&
			     (hp -> flags & HOST_DECL_DELETED))) {
				write_host (hp);
				++num_written;
			}
		}
	    }
	    log_info ("Wrote %d deleted host decls to leases file.",
		      num_written);
	}

	/* Write all the new, dynamic host declarations. */
	if (host_name_hash) {
	    num_written = 0;
	    for (i = 0; i < host_name_hash -> hash_count; i++) {
		for (hb = host_name_hash -> buckets [i];
		     hb; hb = hb -> next) {
			hp = (struct host_decl *)hb -> value;
			if ((hp -> flags & HOST_DECL_DYNAMIC)) {
				write_host (hp);
				++num_written;
			}
		}
	    }
	    log_info ("Wrote %d new dynamic host decls to leases file.",
		      num_written);
	}

#if defined (FAILOVER_PROTOCOL)
	/* Write all the failover states. */
	dhcp_failover_write_all_states ();
#endif

	/* Write all the leases. */
	num_written = 0;
	for (s = shared_networks; s; s = s -> next) {
	    for (p = s -> pools; p; p = p -> next) {
		lptr [FREE_LEASES] = &p -> free;
		lptr [ACTIVE_LEASES] = &p -> active;
		lptr [EXPIRED_LEASES] = &p -> expired;
		lptr [ABANDONED_LEASES] = &p -> abandoned;
		lptr [BACKUP_LEASES] = &p -> backup;

		for (i = FREE_LEASES; i <= BACKUP_LEASES; i++) {
		    for (l = *(lptr [i]); l; l = l -> next) {
			if (l -> hardware_addr.hlen ||
			    l -> uid_len ||
			    (l -> binding_state != FTS_FREE)) {
			    if (!write_lease (l))
				log_fatal ("Can't rewrite lease database");
			    num_written++;
			}
		    }
		}
	    }
	}
	log_info ("Wrote %d leases to leases file.", num_written);
	if (!commit_leases ())
		log_fatal ("Can't commit leases to new database: %m");
}

int lease_enqueue (struct lease *comp)
{
	struct lease **lq, *prev, *lp;

	/* No queue to put it on? */
	if (!comp -> pool)
		return 0;

	/* Figure out which queue it's going to. */
	switch (comp -> binding_state) {
	      case FTS_FREE:
		lq = &comp -> pool -> free;
		comp -> pool -> free_leases++;
		comp -> sort_time = comp -> ends;
		break;

	      case FTS_ACTIVE:
	      case FTS_RESERVED:
	      case FTS_BOOTP:
		lq = &comp -> pool -> active;
		comp -> sort_time = comp -> ends;
		break;

	      case FTS_EXPIRED:
	      case FTS_RELEASED:
	      case FTS_RESET:
		lq = &comp -> pool -> expired;
		comp -> sort_time = comp -> ends;

		break;

	      case FTS_ABANDONED:
		lq = &comp -> pool -> abandoned;
		comp -> sort_time = comp -> ends;
		break;

	      case FTS_BACKUP:
		lq = &comp -> pool -> backup;
		comp -> pool -> backup_leases++;
		comp -> sort_time = comp -> ends;
		break;

	      default:
		log_error ("Lease with bogus binding state: %d",
			   comp -> binding_state);
		return 0;
	}

	/* Insertion sort the lease onto the appropriate queue. */
	prev = (struct lease *)0;
	for (lp = *lq; lp; lp = lp -> next) {
		if (lp -> sort_time >= comp -> sort_time)
			break;
		prev = lp;
	}
	if (prev) {
		if (prev -> next) {
			lease_reference (&comp -> next, prev -> next, MDL);
			lease_dereference (&prev -> next, MDL);
		}
		lease_reference (&prev -> next, comp, MDL);
	} else {
		if (*lq) {
			lease_reference (&comp -> next, *lq, MDL);
			lease_dereference (lq, MDL);
		}
		lease_reference (lq, comp, MDL);
	}
	return 1;
}

/* For a given lease, sort it onto the right list in its pool and put it
   in each appropriate hash, understanding that it's already by definition
   in lease_ip_addr_hash. */

void lease_instantiate (const unsigned char *val, unsigned len,
			struct lease *lease)
{

	/* XXX If the lease doesn't have a pool at this point, it's an
	   XXX orphan, which we *should* keep around until it expires,
	   XXX but which right now we just forget. */
	if (!lease -> pool) {
		lease_hash_delete (lease_ip_addr_hash,
				   lease -> ip_addr.iabuf,
				   lease -> ip_addr.len, MDL);
		return;
	}
		
	/* Put the lease on the right queue. */
	lease_enqueue (lease);

	/* Record the lease in the uid hash if possible. */
	if (lease -> uid) {
		uid_hash_add (lease);
	}
	
	/* Record it in the hardware address hash if possible. */
	if (lease -> hardware_addr.hlen) {
		hw_hash_add (lease);
	}
	
	return;
}

/* Run expiry events on every pool.   This is called on startup so that
   any expiry events that occurred after the server stopped and before it
   was restarted can be run.   At the same time, if failover support is
   compiled in, we compute the balance of leases for the pool. */

void expire_all_pools ()
{
	struct shared_network *s;
	struct pool *p;
	struct hash_bucket *hb;
	int i;
	struct lease *l;
	struct lease **lptr [5];

	/* First, go over the hash list and actually put all the leases
	   on the appropriate lists. */
	lease_hash_foreach (lease_ip_addr_hash, lease_instantiate);

	/* Loop through each pool in each shared network and call the
	   expiry routine on the pool. */
	for (s = shared_networks; s; s = s -> next) {
	    for (p = s -> pools; p; p = p -> next) {
		pool_timer (p);

		p -> lease_count = 0;
		p -> free_leases = 0;
		p -> backup_leases = 0;
		
		lptr [FREE_LEASES] = &p -> free;
		lptr [ACTIVE_LEASES] = &p -> active;
		lptr [EXPIRED_LEASES] = &p -> expired;
		lptr [ABANDONED_LEASES] = &p -> abandoned;
		lptr [BACKUP_LEASES] = &p -> backup;

		for (i = FREE_LEASES; i <= BACKUP_LEASES; i++) {
		    for (l = *(lptr [i]); l; l = l -> next) {
			p -> lease_count++;
			if (l -> ends <= cur_time) {
				if (l -> binding_state == FTS_FREE)
					p -> free_leases++;
				else if (l -> binding_state == FTS_BACKUP)
					p -> backup_leases++;
			}
#if defined (FAILOVER_PROTOCOL)
			if (p -> failover_peer &&
			    l -> tstp > l -> tsfp &&
			    !(l -> flags & ON_UPDATE_QUEUE))
				dhcp_failover_queue_update (l, 1);
#endif
		    }
		}
	    }
	}
}

void dump_subnets ()
{
	struct lease *l;
	struct shared_network *s;
	struct subnet *n;
	struct pool *p;
	struct lease **lptr [5];
	int i;

	log_info ("Subnets:");
	for (n = subnets; n; n = n -> next_subnet) {
		log_debug ("  Subnet %s", piaddr (n -> net));
		log_debug ("     netmask %s",
		       piaddr (n -> netmask));
	}
	log_info ("Shared networks:");
	for (s = shared_networks; s; s = s -> next) {
	    log_info ("  %s", s -> name);
	    for (p = s -> pools; p; p = p -> next) {
		lptr [FREE_LEASES] = &p -> free;
		lptr [ACTIVE_LEASES] = &p -> active;
		lptr [EXPIRED_LEASES] = &p -> expired;
		lptr [ABANDONED_LEASES] = &p -> abandoned;
		lptr [BACKUP_LEASES] = &p -> backup;

		for (i = FREE_LEASES; i <= BACKUP_LEASES; i++) {
		    for (l = *(lptr [i]); l; l = l -> next) {
			    print_lease (l);
		    }
		}
	    }
	}
}

HASH_FUNCTIONS (lease, const unsigned char *, struct lease)
HASH_FUNCTIONS (host, const unsigned char *, struct host_decl)
HASH_FUNCTIONS (class, const char *, struct class)
