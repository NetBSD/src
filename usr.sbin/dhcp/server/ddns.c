/* ddns.c

   Dynamic DNS updates. */

/*
 * Copyright (c) 2000-2001 Internet Software Consortium.
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
 * This software has been donated to the Internet Software Consortium
 * by Damien Neil of Nominum, Inc.
 *
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: ddns.c,v 1.1.1.1.2.2 2001/04/04 20:56:11 he Exp $ Copyright (c) 2000-2001 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include "dst/md5.h"
#include "minires/minires.h"

#ifdef NSUPDATE

/* Have to use TXT records for now. */
#define T_DHCID T_TXT

/* DN: No way of checking that there is enough space in a data_string's
   buffer.  Be certain to allocate enough!
   TL: This is why the expression evaluation code allocates a *new*
   data_string.   :') */
static void data_string_append (struct data_string *ds1,
				struct data_string *ds2)
{
	memcpy (ds1 -> buffer -> data + ds1 -> len,
		ds2 -> data,
		ds2 -> len);
	ds1 -> len += ds2 -> len;
}

static isc_result_t ddns_update_a (struct data_string *ddns_fwd_name,
				   struct iaddr ddns_addr,
				   struct data_string *ddns_dhcid,
				   unsigned long ttl)
{
	ns_updque updqueue;
	ns_updrec *updrec;
	isc_result_t result;
	char ddns_address [16];

	if (ddns_addr.len != 4)
		return ISC_R_INVALIDARG;
#ifndef NO_SNPRINTF
	snprintf (ddns_address, 16, "%d.%d.%d.%d",
		  ddns_addr.iabuf[0], ddns_addr.iabuf[1],
		  ddns_addr.iabuf[2], ddns_addr.iabuf[3]);
#else
	sprintf (ddns_address, "%d.%d.%d.%d",
		 ddns_addr.iabuf[0], ddns_addr.iabuf[1],
		 ddns_addr.iabuf[2], ddns_addr.iabuf[3]);
#endif

	/*
	 * When a DHCP client or server intends to update an A RR, it first
	 * prepares a DNS UPDATE query which includes as a prerequisite the
	 * assertion that the name does not exist.  The update section of the
	 * query attempts to add the new name and its IP address mapping (an A
	 * RR), and the DHCID RR with its unique client-identity.
	 *   -- "Interaction between DHCP and DNS"
	 */

	ISC_LIST_INIT (updqueue);

	/*
	 * A RR does not exist.
	 */
	updrec = minires_mkupdrec (S_PREREQ,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_A, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)0;
	updrec -> r_size = 0;
	updrec -> r_opcode = NXDOMAIN;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * Add A RR.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_A, ttl);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)ddns_address;
	updrec -> r_size = strlen (ddns_address);
	updrec -> r_opcode = ADD;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * Add DHCID RR.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_DHCID, ttl);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = ddns_dhcid -> data;
	updrec -> r_size = ddns_dhcid -> len;
	updrec -> r_opcode = ADD;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));

	print_dns_status ((int)result, &updqueue);

	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}


	/*
	 * If this update operation succeeds, the updater can conclude that it
	 * has added a new name whose only RRs are the A and DHCID RR records.
	 * The A RR update is now complete (and a client updater is finished,
	 * while a server might proceed to perform a PTR RR update).
	 *   -- "Interaction between DHCP and DNS"
	 */

	if (result == ISC_R_SUCCESS)
		return result;


	/*
	 * If the first update operation fails with YXDOMAIN, the updater can
	 * conclude that the intended name is in use.  The updater then
	 * attempts to confirm that the DNS name is not being used by some
	 * other host. The updater prepares a second UPDATE query in which the
	 * prerequisite is that the desired name has attached to it a DHCID RR
	 * whose contents match the client identity.  The update section of
	 * this query deletes the existing A records on the name, and adds the
	 * A record that matches the DHCP binding and the DHCID RR with the
	 * client identity.
	 *   -- "Interaction between DHCP and DNS"
	 */

	if (result != ISC_R_YXDOMAIN)
		return result;


	/*
	 * DHCID RR exists, and matches client identity.
	 */
	updrec = minires_mkupdrec (S_PREREQ,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_DHCID, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = ddns_dhcid -> data;
	updrec -> r_size = ddns_dhcid -> len;
	updrec -> r_opcode = YXRRSET;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * Delete A RRset.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_A, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)0;
	updrec -> r_size = 0;
	updrec -> r_opcode = DELETE;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * Add A RR.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_A, ttl);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)ddns_address;
	updrec -> r_size = strlen (ddns_address);
	updrec -> r_opcode = ADD;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));

	print_dns_status ((int)result, &updqueue);

	/*
	 * If this query succeeds, the updater can conclude that the current
	 * client was the last client associated with the domain name, and that
	 * the name now contains the updated A RR. The A RR update is now
	 * complete (and a client updater is finished, while a server would
	 * then proceed to perform a PTR RR update).
	 *   -- "Interaction between DHCP and DNS"
	 */

	/*
	 * If the second query fails with NXRRSET, the updater must conclude
	 * that the client's desired name is in use by another host.  At this
	 * juncture, the updater can decide (based on some administrative
	 * configuration outside of the scope of this document) whether to let
	 * the existing owner of the name keep that name, and to (possibly)
	 * perform some name disambiguation operation on behalf of the current
	 * client, or to replace the RRs on the name with RRs that represent
	 * the current client. If the configured policy allows replacement of
	 * existing records, the updater submits a query that deletes the
	 * existing A RR and the existing DHCID RR, adding A and DHCID RRs that
	 * represent the IP address and client-identity of the new client.
	 *   -- "Interaction between DHCP and DNS"
	 */

  error:
	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}

	return result;
}


static isc_result_t ddns_update_ptr (struct data_string *ddns_fwd_name,
				     struct data_string *ddns_rev_name,
				     unsigned long ttl)
{
	ns_updque updqueue;
	ns_updrec *updrec;
	isc_result_t result = ISC_R_UNEXPECTED;

	/*
	 * The DHCP server submits a DNS query which deletes all of the PTR RRs
	 * associated with the lease IP address, and adds a PTR RR whose data
	 * is the client's (possibly disambiguated) host name. The server also
	 * adds a DHCID RR specified in Section 4.3.
	 *   -- "Interaction between DHCP and DNS"
	 */

	ISC_LIST_INIT (updqueue);

	/*
	 * Delete all PTR RRs.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_rev_name -> data,
				   C_IN, T_PTR, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)0;
	updrec -> r_size = 0;
	updrec -> r_opcode = DELETE;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Add PTR RR.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_rev_name -> data,
				   C_IN, T_PTR, ttl);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = ddns_fwd_name -> data;
	updrec -> r_size = ddns_fwd_name -> len;
	updrec -> r_opcode = ADD;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));
	print_dns_status ((int)result, &updqueue);

	/* Fall through. */
      error:

	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}

	return result;
}


static isc_result_t ddns_remove_a (struct data_string *ddns_fwd_name,
				   struct iaddr ddns_addr,
				   struct data_string *ddns_dhcid)
{
	ns_updque updqueue;
	ns_updrec *updrec;
	isc_result_t result = SERVFAIL;
	char ddns_address [16];

	if (ddns_addr.len != 4)
		return ISC_R_INVALIDARG;

#ifndef NO_SNPRINTF
	snprintf (ddns_address, 16, "%d.%d.%d.%d",
		  ddns_addr.iabuf[0], ddns_addr.iabuf[1],
		  ddns_addr.iabuf[2], ddns_addr.iabuf[3]);
#else
	sprintf (ddns_address, "%d.%d.%d.%d",
		 ddns_addr.iabuf[0], ddns_addr.iabuf[1],
		 ddns_addr.iabuf[2], ddns_addr.iabuf[3]);
#endif


	/*
	 * The entity chosen to handle the A record for this client (either the
	 * client or the server) SHOULD delete the A record that was added when
	 * the lease was made to the client.
	 *
	 * In order to perform this delete, the updater prepares an UPDATE
	 * query which contains two prerequisites.  The first prerequisite
	 * asserts that the DHCID RR exists whose data is the client identity
	 * described in Section 4.3. The second prerequisite asserts that the
	 * data in the A RR contains the IP address of the lease that has
	 * expired or been released.
	 *   -- "Interaction between DHCP and DNS"
	 */

	ISC_LIST_INIT (updqueue);

	/*
	 * DHCID RR exists, and matches client identity.
	 */
	updrec = minires_mkupdrec (S_PREREQ,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_DHCID,0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = ddns_dhcid -> data;
	updrec -> r_size = ddns_dhcid -> len;
	updrec -> r_opcode = YXRRSET;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * A RR matches the expiring lease.
	 */
	updrec = minires_mkupdrec (S_PREREQ,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_A, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)ddns_address;
	updrec -> r_size = strlen (ddns_address);
	updrec -> r_opcode = YXRRSET;

	ISC_LIST_APPEND (updqueue, updrec, r_link);


	/*
	 * Delete appropriate A RR.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_A, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)ddns_address;
	updrec -> r_size = strlen (ddns_address);
	updrec -> r_opcode = DELETE;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));
	print_dns_status ((int)result, &updqueue);

	/*
	 * If the query fails, the updater MUST NOT delete the DNS name.  It
	 * may be that the host whose lease on the server has expired has moved
	 * to another network and obtained a lease from a different server,
	 * which has caused the client's A RR to be replaced. It may also be
	 * that some other client has been configured with a name that matches
	 * the name of the DHCP client, and the policy was that the last client
	 * to specify the name would get the name.  In this case, the DHCID RR
	 * will no longer match the updater's notion of the client-identity of
	 * the host pointed to by the DNS name.
	 *   -- "Interaction between DHCP and DNS"
	 */

	if (result != ISC_R_SUCCESS)
		goto error;

	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}

	/* If the deletion of the A succeeded, and there are no A records
	   left for this domain, then we can blow away the DHCID record
	   as well.   We can't blow away the DHCID record above because
	   it's possible that more than one A has been added to this
	   domain name. */
	ISC_LIST_INIT (updqueue);

	/*
	 * A RR does not exist.
	 */
	updrec = minires_mkupdrec (S_PREREQ,
				   (const char *)ddns_fwd_name -> data,
				   C_IN, T_A, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)0;
	updrec -> r_size = 0;
	updrec -> r_opcode = NXRRSET;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Delete appropriate DHCID RR.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				  (const char *)ddns_fwd_name -> data,
				   C_IN, T_DHCID, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = ddns_dhcid -> data;
	updrec -> r_size = ddns_dhcid -> len;
	updrec -> r_opcode = DELETE;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));
	print_dns_status ((int)result, &updqueue);

	/* Fall through. */
  error:

	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}

	return result;
}


static isc_result_t ddns_remove_ptr (struct data_string *ddns_rev_name)
{
	ns_updque updqueue;
	ns_updrec *updrec;
	isc_result_t result;

	/*
	 * When a lease expires or a DHCP client issues a DHCPRELEASE request,
	 * the DHCP server SHOULD delete the PTR RR that matches the DHCP
	 * binding, if one was successfully added. The server's update query
	 * SHOULD assert that the name in the PTR record matches the name of
	 * the client whose lease has expired or been released.
	 *   -- "Interaction between DHCP and DNS"
	 */

	ISC_LIST_INIT (updqueue);

	/*
	 * Delete the PTR RRset for the leased address.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_rev_name -> data,
				   C_IN, T_PTR, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)0;
	updrec -> r_size = 0;
	updrec -> r_opcode = DELETE;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));
	print_dns_status ((int)result, &updqueue);

	/* Fall through. */
      error:

	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}

	return result;
}


int ddns_updates (struct packet *packet,
		  struct lease *lease, struct lease *old,
		  struct lease_state *state)
{
	unsigned long ddns_ttl = DEFAULT_DDNS_TTL;
	struct data_string ddns_hostname;
	struct data_string ddns_domainname;
	struct data_string old_ddns_fwd_name;
	struct data_string ddns_fwd_name;
	struct data_string ddns_rev_name;
	struct data_string ddns_dhcid;
	unsigned len;
	struct data_string d1;
	struct option_cache *oc;
	int s1, s2;
	int result = 0;
	isc_result_t rcode1 = ISC_R_SUCCESS, rcode2 = ISC_R_SUCCESS;
	int server_updates_a = 1;
	struct buffer *bp = (struct buffer *)0;
	int ignorep = 0;

	if (ddns_update_style != 2)
		return 0;

	/* Can only cope with IPv4 addrs at the moment. */
	if (lease -> ip_addr . len != 4)
		return 0;

	memset (&ddns_hostname, 0, sizeof (ddns_hostname));
	memset (&ddns_domainname, 0, sizeof (ddns_domainname));
	memset (&old_ddns_fwd_name, 0, sizeof (ddns_fwd_name));
	memset (&ddns_fwd_name, 0, sizeof (ddns_fwd_name));
	memset (&ddns_rev_name, 0, sizeof (ddns_rev_name));
	memset (&ddns_dhcid, 0, sizeof (ddns_dhcid));

	/* If we are allowed to accept the client's update of its own A
	   record, see if the client wants to update its own A record. */
	if (!(oc = lookup_option (&server_universe, state -> options,
				  SV_CLIENT_UPDATES)) ||
	    evaluate_boolean_option_cache (&ignorep, packet, lease,
					   (struct client_state *)0,
					   packet -> options,
					   state -> options,
					   &lease -> scope, oc, MDL)) {
		/* If there's no fqdn.no-client-update or if it's
		   nonzero, don't try to use the client-supplied
		   XXX */
		if (!(oc = lookup_option (&fqdn_universe, packet -> options,
					  FQDN_NO_CLIENT_UPDATE)) ||
		    evaluate_boolean_option_cache (&ignorep, packet, lease,
						   (struct client_state *)0,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL))
			goto noclient;
		/* Win98 and Win2k will happily claim to be willing to
		   update an unqualified domain name. */
		if (!(oc = lookup_option (&fqdn_universe, packet -> options,
					  FQDN_DOMAINNAME)))
			goto noclient;
		if (!(oc = lookup_option (&fqdn_universe, packet -> options,
					  FQDN_FQDN)) ||
		    !evaluate_option_cache (&ddns_fwd_name, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL))
			goto noclient;
		server_updates_a = 0;
		goto client_updates;
	}
      noclient:

	/* If it's a static lease, then don't do the DNS update unless we're
	   specifically configured to do so.   If the client asked to do its
	   own update and we allowed that, we don't do this test. */
	if (lease -> flags & STATIC_LEASE) {
		if (!(oc = lookup_option (&fqdn_universe, packet -> options,
					  SV_UPDATE_STATIC_LEASES)) ||
		    !evaluate_boolean_option_cache (&ignorep, packet, lease,
						    (struct client_state *)0,
						    packet -> options,
						    state -> options,
						    &lease -> scope, oc, MDL))
			return 0;
	}

	/*
	 * Compute the name for the A record.
	 */
	s1 = s2 = 0;

	oc = lookup_option (&server_universe, state -> options,
			    SV_DDNS_HOST_NAME);
	if (oc)
		s1 = evaluate_option_cache (&ddns_hostname, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL);

	oc = lookup_option (&server_universe, state -> options,
			    SV_DDNS_DOMAIN_NAME);
	if (oc)
		s2 = evaluate_option_cache (&ddns_domainname, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL);

	if (s1 && s2) {
		buffer_allocate (&ddns_fwd_name.buffer,
				 ddns_hostname.len + ddns_domainname.len + 2,
				 MDL);
		if (ddns_fwd_name.buffer) {
			ddns_fwd_name.data = ddns_fwd_name.buffer -> data;
			data_string_append (&ddns_fwd_name, &ddns_hostname);
			ddns_fwd_name.buffer -> data [ddns_fwd_name.len] = '.';
			ddns_fwd_name.len++;
			data_string_append (&ddns_fwd_name, &ddns_domainname);
			ddns_fwd_name.buffer -> data [ddns_fwd_name.len] ='\0';
			ddns_fwd_name.terminated = 1;
		}
	}
      client_updates:

	/* See if there's a name already stored on the lease. */
	if (find_bound_string (&old_ddns_fwd_name,
			       lease -> scope, "ddns-fwd-name")) {
		/* If there is, see if it's different. */
		if (old_ddns_fwd_name.len != ddns_fwd_name.len ||
		    memcmp (old_ddns_fwd_name.data, ddns_fwd_name.data,
			    old_ddns_fwd_name.len)) {
			/* If the name is different, try to delete
			   the old A record. */
			if (!ddns_removals (lease))
				goto out;
			/* If the delete succeeded, go install the new
			   record. */
			goto in;
		}

		/* See if there's a DHCID on the lease. */
		if (!find_bound_string (&ddns_dhcid,
					lease -> scope, "ddns-txt")) {
			/* If there's no DHCID, the update was probably
			   done with the old-style ad-hoc DDNS updates.
			   So if the expiry and release events look like
			   they're the same, run them.   This should delete
			   the old DDNS data. */
			if (old -> on_expiry == old -> on_release) {
				execute_statements ((struct binding_value **)0,
						    (struct packet *)0, lease,
						    (struct client_state *)0,
						    (struct option_state *)0,
						    (struct option_state *)0,
						    &lease -> scope,
						    old -> on_expiry);
				if (old -> on_expiry)
					executable_statement_dereference
						(&old -> on_expiry, MDL);
				if (old -> on_release)
					executable_statement_dereference
						(&old -> on_release, MDL);
				/* Now, install the DDNS data the new way. */
				goto in;
			}
		}

		/* See if the administrator wants to do updates even
		   in cases where the update already appears to have been
		   done. */
		if (!(oc = lookup_option (&server_universe, state -> options,
					  SV_UPDATE_OPTIMIZATION)) ||
		    evaluate_boolean_option_cache (&ignorep, packet, lease,
						   (struct client_state *)0,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
			result = 1;
			goto noerror;
		}
	}

	/* If there's no ddns-fwd-name on the lease, see if there's
	   a ddns-client-fqdn, indicating a prior client FQDN update.
	   If there is, and if we're still doing the client update,
	   see if the name has changed.   If it hasn't, don't do the
	   PTR update. */
	if (find_bound_string (&old_ddns_fwd_name,
			       lease -> scope, "ddns-client-fqdn")) {
		if (old_ddns_fwd_name.len == ddns_fwd_name.len &&
		    !memcmp (old_ddns_fwd_name.data, ddns_fwd_name.data,
			     old_ddns_fwd_name.len)) {
			/* If the name is not different, no need to update
			   the PTR record. */
			goto noerror;
		}
	}
      in:
		
	/*
	 * Compute the RR TTL.
	 */
	ddns_ttl = DEFAULT_DDNS_TTL;
	memset (&d1, 0, sizeof d1);
	if ((oc = lookup_option (&server_universe, state -> options,
				 SV_DDNS_TTL))) {
		if (evaluate_option_cache (&d1, packet, lease,
					   (struct client_state *)0,
					   packet -> options,
					   state -> options,
					   &lease -> scope, oc, MDL)) {
			if (d1.len == sizeof (u_int32_t))
				ddns_ttl = getULong (d1.data);
			data_string_forget (&d1, MDL);
		}
	}


	/*
	 * Compute the reverse IP name.
	 */
	oc = lookup_option (&server_universe, state -> options,
			    SV_DDNS_REV_DOMAIN_NAME);
	if (oc)
		s1 = evaluate_option_cache (&d1, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL);
	
	if (oc && s1) {
		/* Buffer length:
		   XXX.XXX.XXX.XXX.<ddns-rev-domain-name>\0 */
		buffer_allocate (&ddns_rev_name.buffer,
				 d1.len + 17, MDL);
		if (ddns_rev_name.buffer) {
			ddns_rev_name.data = ddns_rev_name.buffer -> data;
#ifndef NO_SNPRINTF
			snprintf ((char *)ddns_rev_name.buffer -> data, 17,
				  "%d.%d.%d.%d.",
				  lease -> ip_addr . iabuf[3],
				  lease -> ip_addr . iabuf[2],
				  lease -> ip_addr . iabuf[1],
				  lease -> ip_addr . iabuf[0]);
#else
			sprintf ((char *)ddns_rev_name.buffer -> data,
				 "%d.%d.%d.%d.",
				 lease -> ip_addr . iabuf[3],
				 lease -> ip_addr . iabuf[2],
				 lease -> ip_addr . iabuf[1],
				 lease -> ip_addr . iabuf[0]);
#endif
			ddns_rev_name.len =
				strlen ((const char *)ddns_rev_name.data);
			data_string_append (&ddns_rev_name, &d1);
			ddns_rev_name.buffer -> data [ddns_rev_name.len] ='\0';
			ddns_rev_name.terminated = 1;
		}
		
		data_string_forget (&d1, MDL);
	}

	/*
	 * If we are updating the A record, compute the DHCID value.
	 */
	if (server_updates_a) {
		memset (&ddns_dhcid, 0, sizeof ddns_dhcid);
		get_dhcid (&ddns_dhcid, lease);
	}

	/*
	 * Start the resolver, if necessary.
	 */
	if (!resolver_inited) {
		minires_ninit (&resolver_state);
		resolver_inited = 1;
		resolver_state.retrans = 1;
		resolver_state.retry = 1;
	}

	/*
	 * Perform updates.
	 */
	if (ddns_fwd_name.len && ddns_dhcid.len)
		rcode1 = ddns_update_a (&ddns_fwd_name, lease -> ip_addr,
					&ddns_dhcid, ddns_ttl);
	
	if (rcode1 == ISC_R_SUCCESS) {
		if (ddns_fwd_name.len && ddns_rev_name.len)
			rcode2 = ddns_update_ptr (&ddns_fwd_name,
						  &ddns_rev_name, ddns_ttl);
	} else
		rcode2 = rcode1;

	if (rcode1 == ISC_R_SUCCESS &&
	    (server_updates_a || rcode2 == ISC_R_SUCCESS)) {
		bind_ds_value (&lease -> scope, 
			       (server_updates_a
				? "ddns-fwd-name" : "ddns-client-fqdn"),
			       &ddns_fwd_name);
		if (server_updates_a)
			bind_ds_value (&lease -> scope, "ddns-txt",
				       &ddns_dhcid);
	}

	if (rcode2 == ISC_R_SUCCESS) {
		bind_ds_value (&lease -> scope, "ddns-rev-name",
			       &ddns_rev_name);
	}

	/* Set up the outgoing FQDN option if there was an incoming
	   FQDN option.  If there's a valid FQDN option, there should
	   be an FQDN_ENCODED suboption, so we test the latter to
	   detect the presence of the former. */
      noerror:
	if ((oc = lookup_option (&fqdn_universe,
				 packet -> options, FQDN_ENCODED))
	    && buffer_allocate (&bp, ddns_fwd_name.len + 5, MDL)) {
		bp -> data [0] = server_updates_a;
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [0], 1,
					 &fqdn_options [FQDN_SERVER_UPDATE],
					 0))
			goto badfqdn;
		bp -> data [1] = server_updates_a;
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [1], 1,
					 &fqdn_options [FQDN_NO_CLIENT_UPDATE],
					 0))
			goto badfqdn;
		/* Do the same encoding the client did. */
		oc = lookup_option (&fqdn_universe, packet -> options,
				    FQDN_ENCODED);
		if (oc &&
		    evaluate_boolean_option_cache (&ignorep, packet, lease,
						   (struct client_state *)0,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL))
			bp -> data [2] = 1;
		else
			bp -> data [2] = 0;
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [2], 1,
					 &fqdn_options [FQDN_ENCODED],
					 0))
			goto badfqdn;
		bp -> data [3] = isc_rcode_to_ns (rcode1);
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [3], 1,
					 &fqdn_options [FQDN_RCODE1],
					 0))
			goto badfqdn;
		bp -> data [4] = isc_rcode_to_ns (rcode2);
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [4], 1,
					 &fqdn_options [FQDN_RCODE2],
					 0))
			goto badfqdn;
		if (ddns_fwd_name.len) {
		    memcpy (&bp -> data [5],
			    ddns_fwd_name.data, ddns_fwd_name.len);
		    if (!save_option_buffer (&fqdn_universe, state -> options,
					     bp, &bp -> data [5],
					     ddns_fwd_name.len,
					     &fqdn_options [FQDN_FQDN],
					     0))
			goto badfqdn;
		}
	}

      badfqdn:
      out:
	/*
	 * Final cleanup.
	 */
	data_string_forget (&ddns_hostname, MDL);
	data_string_forget (&ddns_domainname, MDL);
	data_string_forget (&old_ddns_fwd_name, MDL);
	data_string_forget (&ddns_fwd_name, MDL);
	data_string_forget (&ddns_rev_name, MDL);
	data_string_forget (&ddns_dhcid, MDL);
	if (bp)
		buffer_dereference (&bp, MDL);

	return result;
}

int ddns_removals (struct lease *lease)
{
	struct data_string ddns_fwd_name;
	struct data_string ddns_rev_name;
	struct data_string ddns_dhcid;
	isc_result_t rcode;
	struct binding *binding;
	int result = 0;
	int client_updated = 0;

	/* No scope implies that DDNS has not been performed for this lease. */
	if (!lease -> scope)
		return 0;

	/*
	 * Look up stored names.
	 */
	memset (&ddns_fwd_name, 0, sizeof (ddns_fwd_name));
	memset (&ddns_rev_name, 0, sizeof (ddns_rev_name));
	memset (&ddns_dhcid, 0, sizeof (ddns_dhcid));

	/*
	 * Start the resolver, if necessary.
	 */
	if (!resolver_inited) {
		minires_ninit (&resolver_state);
		resolver_inited = 1;
	}

	/* We need the fwd name whether we are deleting both records or just
	   the PTR record, so if it's not there, we can't proceed. */
	if (!find_bound_string (&ddns_fwd_name,
				lease -> scope, "ddns-fwd-name")) {
		/* If there's no ddns-fwd-name, look for the client fqdn,
		   in case the client did the update. */
		if (!find_bound_string (&ddns_fwd_name,
					lease -> scope, "ddns-client-fqdn"))
			return 0;
		client_updated = 1;
		goto try_rev;
	}

	/* If the ddns-txt binding isn't there, this isn't an interim
	   or rfc3??? record, so we can't delete the A record using
	   this mechanism, but we can delete the PTR record. */
	if (!find_bound_string (&ddns_dhcid, lease -> scope, "ddns-txt")) {
		result = 1;
		goto try_rev;
	}

	/*
	 * Perform removals.
	 */
	rcode = ddns_remove_a (&ddns_fwd_name, lease -> ip_addr, &ddns_dhcid);

	if (rcode == ISC_R_SUCCESS) {
		result = 1;
		unset (lease -> scope, "ddns-fwd-name");
		unset (lease -> scope, "ddns-txt");
	      try_rev:
		if (find_bound_string (&ddns_rev_name,
				       lease -> scope, "ddns-rev-name")) {
			if (ddns_remove_ptr(&ddns_rev_name) == NOERROR) {
				unset (lease -> scope, "ddns-rev-name");
				if (client_updated)
					unset (lease -> scope,
					       "ddns-client-fqdn");
			} else
				result = 0;
		}
	}

	data_string_forget (&ddns_fwd_name, MDL);
	data_string_forget (&ddns_rev_name, MDL);
	data_string_forget (&ddns_dhcid, MDL);

	return result;
}

#endif /* NSUPDATE */
