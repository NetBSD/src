/* dns.c

   Domain Name Service subroutines. */

/*
 * Copyright (c) 2000 Internet Software Consortium.
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
 * by Ted Lemon in cooperation with Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: dns.c,v 1.1.1.4.2.1 2000/06/22 18:00:46 minoura Exp $ Copyright (c) 2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include "arpa/nameser.h"

/* This file is kind of a crutch for the BIND 8 nsupdate code, which has
 * itself been cruelly hacked from its original state.   What this code
 * does is twofold: first, it maintains a database of zone cuts that can
 * be used to figure out which server should be contacted to update any
 * given domain name.   Secondly, it maintains a set of named TSIG keys,
 * and associates those keys with zones.   When an update is requested for
 * a particular zone, the key associated with that zone is used for the
 * update.
 *
 * The way this works is that you define the domain name to which an
 * SOA corresponds, and the addresses of some primaries for that domain name:
 *
 *	zone FOO.COM {
 *	  primary 10.0.17.1;
 *	  secondary 10.0.22.1, 10.0.23.1;
 *	  key "FOO.COM Key";
 * 	}
 *
 * If an update is requested for GAZANGA.TOPANGA.FOO.COM, then the name
 * server looks in its database for a zone record for "GAZANGA.TOPANGA.FOO.COM",
 * doesn't find it, looks for one for "TOPANGA.FOO.COM", doesn't find *that*,
 * looks for "FOO.COM", finds it. So it
 * attempts the update to the primary for FOO.COM.   If that times out, it
 * tries the secondaries.   You can list multiple primaries if you have some
 * kind of magic name server that supports that.   You shouldn't list
 * secondaries that don't know how to forward updates (e.g., BIND 8 doesn't
 * support update forwarding, AFAIK).   If no TSIG key is listed, the update
 * is attempted without TSIG.
 *
 * The DHCP server tries to find an existing zone for any given name by
 * trying to look up a local zone structure for each domain containing
 * that name, all the way up to '.'.   If it finds one cached, it tries
 * to use that one to do the update.   That's why it tries to update
 * "FOO.COM" above, even though theoretically it should try GAZANGA...
 * and TOPANGA... first.
 *
 * If the update fails with a predefined or cached zone (we'll get to
 * those in a second), then it tries to find a more specific zone.   This
 * is done by looking first for an SOA for GAZANGA.TOPANGA.FOO.COM.   Then
 * an SOA for TOPANGA.FOO.COM is sought.   If during this search a predefined
 * or cached zone is found, the update fails - there's something wrong
 * somewhere.
 *
 * If a more specific zone _is_ found, that zone is cached for the length of
 * its TTL in the same database as that described above.   TSIG updates are
 * never done for cached zones - if you want TSIG updates you _must_
 * write a zone definition linking the key to the zone.   In cases where you
 * know for sure what the key is but do not want to hardcode the IP addresses
 * of the primary or secondaries, a zone declaration can be made that doesn't
 * include any primary or secondary declarations.   When the DHCP server
 * encounters this while hunting up a matching zone for a name, it looks up
 * the SOA, fills in the IP addresses, and uses that record for the update.
 * If the SOA lookup returns NXRRSET, a warning is printed and the zone is
 * discarded, TSIG key and all.   The search for the zone then continues as if
 * the zone record hadn't been found.   Zones without IP addresses don't
 * match when initially hunting for a predefined or cached zone to update.
 *
 * When an update is attempted and no predefined or cached zone is found
 * that matches any enclosing domain of the domain being updated, the DHCP
 * server goes through the same process that is done when the update to a
 * predefined or cached zone fails - starting with the most specific domain
 * name (GAZANGA.TOPANGA.FOO.COM) and moving to the least specific (the root),
 * it tries to look up an SOA record.   When it finds one, it creates a cached
 * zone and attempts an update, and gives up if the update fails.
 *
 * TSIG keys are defined like this:
 *
 *	key "FOO.COM Key" {
 *		algorithm HMAC-MD5.SIG-ALG.REG.INT;
 *		secret <Base64>;
 *	}
 *
 * <Base64> is a number expressed in base64 that represents the key.
 * It's also permissible to use a quoted string here - this will be
 * translated as the ASCII bytes making up the string, and will not
 * include any NUL termination.  The key name can be any text string,
 * and the key type must be one of the key types defined in the draft
 * or by the IANA.  Currently only the HMAC-MD5... key type is
 * supported.
 */

struct hash_table *tsig_key_hash;
struct hash_table *dns_zone_hash;

#if defined (NSUPDATE)
isc_result_t find_tsig_key (ns_tsig_key **key, const char *zname,
			    struct dns_zone *zone)
{
	isc_result_t status;
	ns_tsig_key *tkey;
#if 0
	struct dns_zone *zone;

	zone = (struct dns_zone *)0;
	status = dns_zone_lookup (&zone, zname);
	if (status != ISC_R_SUCCESS)
		return status;
#else
	if (!zone)
		return ISC_R_NOTFOUND;
#endif
	if (!zone -> key) {
		dns_zone_dereference (&zone, MDL);
		return ISC_R_KEY_UNKNOWN;
	}
	
	if ((!zone -> key -> name ||
	     strlen (zone -> key -> name) > NS_MAXDNAME) ||
	    (!zone -> key -> algorithm ||
	     strlen (zone -> key -> algorithm) > NS_MAXDNAME) ||
	    (!zone -> key -> key.len)) {
		dns_zone_dereference (&zone, MDL);
		return ISC_R_INVALIDKEY;
	}
	tkey = dmalloc (sizeof *tkey, MDL);
	if (!tkey) {
	      nomem:
		dns_zone_dereference (&zone, MDL);
		return ISC_R_NOMEMORY;
	}
	memset (tkey, 0, sizeof *tkey);
	tkey -> data = dmalloc (zone -> key -> key.len, MDL);
	if (!tkey -> data) {
		dfree (tkey, MDL);
		goto nomem;
	}
	strcpy (tkey -> name, zone -> key -> name);
	strcpy (tkey -> alg, zone -> key -> algorithm);
	memcpy (tkey -> data,
		zone -> key -> key.data, zone -> key -> key.len);
	tkey -> len = zone -> key -> key.len;
	*key = tkey;
	return ISC_R_SUCCESS;
}

void tkey_free (ns_tsig_key **key)
{
	if ((*key) -> data)
		dfree ((*key) -> data, MDL);
	dfree ((*key), MDL);
	*key = (ns_tsig_key *)0;
}
#endif

isc_result_t enter_dns_zone (struct dns_zone *zone)
{
	struct dns_zone *tz = (struct dns_zone *)0;

	if (dns_zone_hash) {
		dns_zone_hash_lookup (&tz,
				      dns_zone_hash, zone -> name, 0, MDL);
		if (tz == zone) {
			dns_zone_dereference (&tz, MDL);
			return ISC_R_SUCCESS;
		}
		if (tz) {
			dns_zone_hash_delete (dns_zone_hash,
					      zone -> name, 0, MDL);
			dns_zone_dereference (&tz, MDL);
		}
	} else {
		dns_zone_hash =
			new_hash ((hash_reference)dns_zone_reference,
				  (hash_dereference)dns_zone_dereference, 1);
		if (!dns_zone_hash)
			return ISC_R_NOMEMORY;
	}
	dns_zone_hash_add (dns_zone_hash, zone -> name, 0, zone, MDL);
	return ISC_R_SUCCESS;
}

isc_result_t dns_zone_lookup (struct dns_zone **zone, const char *name)
{
	struct dns_zone *tz = (struct dns_zone *)0;
	unsigned len;
	char *tname = (char *)0;
	isc_result_t status;

	if (!dns_zone_hash)
		return ISC_R_NOTFOUND;

	len = strlen (name);
	if (name [len - 1] != '.') {
		tname = dmalloc (len + 2, MDL);
		if (!tname)
			return ISC_R_NOMEMORY;;
		strcpy (tname, name);
		tname [len] = '.';
		tname [len + 1] = 0;
		name = tname;
	}
	if (!dns_zone_hash_lookup (zone, dns_zone_hash, name, 0, MDL))
		status = ISC_R_NOTFOUND;
	else
		status = ISC_R_SUCCESS;

	if (tname)
		dfree (tname, MDL);
	return status;
}

isc_result_t enter_tsig_key (struct tsig_key *tkey)
{
	struct tsig_key *tk = (struct tsig_key *)0;

	if (tsig_key_hash) {
		tsig_key_hash_lookup (&tk, tsig_key_hash,
				      tkey -> name, 0, MDL);
		if (tk == tkey) {
			tsig_key_dereference (&tk, MDL);
			return ISC_R_SUCCESS;
		}
		if (tk) {
			tsig_key_hash_delete (tsig_key_hash,
					      tkey -> name, 0, MDL);
			tsig_key_dereference (&tk, MDL);
		}
	} else {
		tsig_key_hash =
			new_hash ((hash_reference)tsig_key_reference,
				  (hash_dereference)tsig_key_dereference, 1);
		if (!tsig_key_hash)
			return ISC_R_NOMEMORY;
	}
	tsig_key_hash_add (tsig_key_hash, tkey -> name, 0, tkey, MDL);
	return ISC_R_SUCCESS;
	
}

isc_result_t tsig_key_lookup (struct tsig_key **tkey, const char *name) {
	struct tsig_key *tk;

	if (!tsig_key_hash)
		return ISC_R_NOTFOUND;
	if (!tsig_key_hash_lookup (tkey, tsig_key_hash, name, 0, MDL))
		return ISC_R_NOTFOUND;
	return ISC_R_SUCCESS;
}

int dns_zone_dereference (ptr, file, line)
	struct dns_zone **ptr;
	const char *file;
	int line;
{
	int i;
	struct dns_zone *dns_zone;

	if (!ptr || !*ptr) {
		log_error ("%s(%d): null pointer", file, line);
#if defined (POINTER_DEBUG)
		abort ();
#else
		return 0;
#endif
	}

	dns_zone = *ptr;
	*ptr = (struct dns_zone *)0;
	--dns_zone -> refcnt;
	rc_register (file, line, ptr, dns_zone, dns_zone -> refcnt);
	if (dns_zone -> refcnt > 0)
		return 1;

	if (dns_zone -> refcnt < 0) {
		log_error ("%s(%d): negative refcnt!", file, line);
#if defined (DEBUG_RC_HISTORY)
		dump_rc_history ();
#endif
#if defined (POINTER_DEBUG)
		abort ();
#else
		return 0;
#endif
	}

	if (dns_zone -> name)
		dfree (dns_zone -> name, file, line);
	if (dns_zone -> key)
		tsig_key_dereference (&dns_zone -> key, file, line);
	if (dns_zone -> primary)
		option_cache_dereference (&dns_zone -> primary, file, line);
	if (dns_zone -> secondary)
		option_cache_dereference (&dns_zone -> secondary, file, line);
	dfree (dns_zone, file, line);
	return 1;
}

#if defined (NSUPDATE)
int find_cached_zone (const char *dname, ns_class class,
		      char *zname, size_t zsize,
		      struct in_addr *addrs, int naddrs,
		      struct dns_zone **zcookie)
{
	isc_result_t status = ISC_R_NOTFOUND;
	const char *np;
	struct dns_zone *zone = (struct dns_zone *)0;
	struct data_string nsaddrs;
	int ix;

	/* The absence of the zcookie pointer indicates that we
	   succeeded previously, but the update itself failed, meaning
	   that we shouldn't use the cached zone. */
	if (!zcookie)
		return 0;

	/* For each subzone, try to find a cached zone. */
	for (np = dname - 1; np; np = strchr (np, '.')) {
		np++;
		status = dns_zone_lookup (&zone, np);
		if (status == ISC_R_SUCCESS)
			break;
	}

	if (status != ISC_R_SUCCESS)
		return 0;

	/* Make sure the zone is valid. */
	if (zone -> timeout && zone -> timeout < cur_time) {
		dns_zone_dereference (&zone, MDL);
		return 0;
	}

	/* Make sure the zone name will fit. */
	if (strlen (zone -> name) > zsize) {
		dns_zone_dereference (&zone, MDL);
		return 0;
	}
	strcpy (zname, zone -> name);

	memset (&nsaddrs, 0, sizeof nsaddrs);
	ix = 0;

	if (zone -> primary) {
		if (evaluate_option_cache (&nsaddrs, (struct packet *)0,
					   (struct lease *)0,
					   (struct option_state *)0,
					   (struct option_state *)0,
					   &global_scope,
					   zone -> primary, MDL)) {
			int ip = 0;
			while (ix < naddrs) {
				if (ip + 4 > nsaddrs.len)
					break;
				memcpy (&addrs [ix], &nsaddrs.data [ip], 4);
				ip += 4;
				ix++;
			}
			data_string_forget (&nsaddrs, MDL);
		}
	}
	if (zone -> secondary) {
		if (evaluate_option_cache (&nsaddrs, (struct packet *)0,
					   (struct lease *)0,
					   (struct option_state *)0,
					   (struct option_state *)0,
					   &global_scope,
					   zone -> secondary, MDL)) {
			int ip = 0;
			while (ix < naddrs) {
				if (ip + 4 > nsaddrs.len)
					break;
				memcpy (&addrs [ix], &nsaddrs.data [ip], 4);
				ip += 4;
				ix++;
			}
			data_string_forget (&nsaddrs, MDL);
		}
	}

	/* It's not an error for zcookie to have a value here - actually,
	   it's quite likely, because res_nupdate cycles through all the
	   names in the update looking for their zones. */
	if (!*zcookie)
		dns_zone_reference (zcookie, zone, MDL);
	dns_zone_dereference (&zone, MDL);
	return ix;
}

void forget_zone (struct dns_zone **zone)
{
	dns_zone_dereference (zone, MDL);
}

void repudiate_zone (struct dns_zone **zone)
{
	/* XXX Currently we're not differentiating between a cached
	   XXX zone and a zone that's been repudiated, which means
	   XXX that if we reap cached zones, we blow away repudiated
	   XXX zones.   This isn't a big problem since we're not yet
	   XXX caching zones... :'} */

	(*zone) -> timeout = cur_time - 1;
	dns_zone_dereference (zone, MDL);
}
#endif /* NSUPDATE */

HASH_FUNCTIONS (dns_zone, const char *, struct dns_zone)
HASH_FUNCTIONS (tsig_key, const char *, struct tsig_key)
