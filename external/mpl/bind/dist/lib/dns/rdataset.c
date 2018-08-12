/*	$NetBSD: rdataset.c,v 1.1.1.1 2018/08/12 12:08:11 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/random.h>
#include <isc/serial.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/ncache.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>

static const char *trustnames[] = {
	"none",
	"pending-additional",
	"pending-answer",
	"additional",
	"glue",
	"answer",
	"authauthority",
	"authanswer",
	"secure",
	"local" /* aka ultimate */
};

const char *
dns_trust_totext(dns_trust_t trust) {
	if (trust >= sizeof(trustnames)/sizeof(*trustnames))
		return ("bad");
	return (trustnames[trust]);
}

void
dns_rdataset_init(dns_rdataset_t *rdataset) {

	/*
	 * Make 'rdataset' a valid, disassociated rdataset.
	 */

	REQUIRE(rdataset != NULL);

	rdataset->magic = DNS_RDATASET_MAGIC;
	rdataset->methods = NULL;
	ISC_LINK_INIT(rdataset, link);
	rdataset->rdclass = 0;
	rdataset->type = 0;
	rdataset->ttl = 0;
	rdataset->trust = 0;
	rdataset->covers = 0;
	rdataset->attributes = 0;
	rdataset->count = ISC_UINT32_MAX;
	rdataset->private1 = NULL;
	rdataset->private2 = NULL;
	rdataset->private3 = NULL;
	rdataset->privateuint4 = 0;
	rdataset->private5 = NULL;
	rdataset->private6 = NULL;
	rdataset->private7 = NULL;
	rdataset->resign = 0;
}

void
dns_rdataset_invalidate(dns_rdataset_t *rdataset) {

	/*
	 * Invalidate 'rdataset'.
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods == NULL);

	rdataset->magic = 0;
	ISC_LINK_INIT(rdataset, link);
	rdataset->rdclass = 0;
	rdataset->type = 0;
	rdataset->ttl = 0;
	rdataset->trust = 0;
	rdataset->covers = 0;
	rdataset->attributes = 0;
	rdataset->count = ISC_UINT32_MAX;
	rdataset->private1 = NULL;
	rdataset->private2 = NULL;
	rdataset->private3 = NULL;
	rdataset->privateuint4 = 0;
	rdataset->private5 = NULL;
}

void
dns_rdataset_disassociate(dns_rdataset_t *rdataset) {

	/*
	 * Disassociate 'rdataset' from its rdata, allowing it to be reused.
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	(rdataset->methods->disassociate)(rdataset);
	rdataset->methods = NULL;
	ISC_LINK_INIT(rdataset, link);
	rdataset->rdclass = 0;
	rdataset->type = 0;
	rdataset->ttl = 0;
	rdataset->trust = 0;
	rdataset->covers = 0;
	rdataset->attributes = 0;
	rdataset->count = ISC_UINT32_MAX;
	rdataset->private1 = NULL;
	rdataset->private2 = NULL;
	rdataset->private3 = NULL;
	rdataset->privateuint4 = 0;
	rdataset->private5 = NULL;
	rdataset->private6 = NULL;
}

isc_boolean_t
dns_rdataset_isassociated(dns_rdataset_t *rdataset) {
	/*
	 * Is 'rdataset' associated?
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));

	if (rdataset->methods != NULL)
		return (ISC_TRUE);

	return (ISC_FALSE);
}

static void
question_disassociate(dns_rdataset_t *rdataset) {
	UNUSED(rdataset);
}

static isc_result_t
question_cursor(dns_rdataset_t *rdataset) {
	UNUSED(rdataset);

	return (ISC_R_NOMORE);
}

static void
question_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	/*
	 * This routine should never be called.
	 */
	UNUSED(rdataset);
	UNUSED(rdata);

	REQUIRE(0);
}

static void
question_clone(dns_rdataset_t *source, dns_rdataset_t *target) {
	*target = *source;
}

static unsigned int
question_count(dns_rdataset_t *rdataset) {
	/*
	 * This routine should never be called.
	 */
	UNUSED(rdataset);
	REQUIRE(0);

	return (0);
}

static dns_rdatasetmethods_t question_methods = {
	question_disassociate,
	question_cursor,
	question_cursor,
	question_current,
	question_clone,
	question_count,
	NULL, /* addnoqname */
	NULL, /* getnoqname */
	NULL, /* addclosest */
	NULL, /* getclosest */
	NULL, /* settrust */
	NULL, /* expire */
	NULL, /* clearprefetch */
	NULL, /* setownercase */
	NULL, /* getownercase */
	NULL  /* addglue */
};

void
dns_rdataset_makequestion(dns_rdataset_t *rdataset, dns_rdataclass_t rdclass,
			  dns_rdatatype_t type)
{

	/*
	 * Make 'rdataset' a valid, associated, question rdataset, with a
	 * question class of 'rdclass' and type 'type'.
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods == NULL);

	rdataset->methods = &question_methods;
	rdataset->rdclass = rdclass;
	rdataset->type = type;
	rdataset->attributes |= DNS_RDATASETATTR_QUESTION;
}

unsigned int
dns_rdataset_count(dns_rdataset_t *rdataset) {

	/*
	 * Return the number of records in 'rdataset'.
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	return ((rdataset->methods->count)(rdataset));
}

void
dns_rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target) {

	/*
	 * Make 'target' refer to the same rdataset as 'source'.
	 */

	REQUIRE(DNS_RDATASET_VALID(source));
	REQUIRE(source->methods != NULL);
	REQUIRE(DNS_RDATASET_VALID(target));
	REQUIRE(target->methods == NULL);

	(source->methods->clone)(source, target);
}

isc_result_t
dns_rdataset_first(dns_rdataset_t *rdataset) {

	/*
	 * Move the rdata cursor to the first rdata in the rdataset (if any).
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	return ((rdataset->methods->first)(rdataset));
}

isc_result_t
dns_rdataset_next(dns_rdataset_t *rdataset) {

	/*
	 * Move the rdata cursor to the next rdata in the rdataset (if any).
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	return ((rdataset->methods->next)(rdataset));
}

void
dns_rdataset_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {

	/*
	 * Make 'rdata' refer to the current rdata.
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	(rdataset->methods->current)(rdataset, rdata);
}

#define MAX_SHUFFLE	32
#define WANT_FIXED(r)	(((r)->attributes & DNS_RDATASETATTR_FIXEDORDER) != 0)
#define WANT_RANDOM(r)	(((r)->attributes & DNS_RDATASETATTR_RANDOMIZE) != 0)
#define WANT_CYCLIC(r)	(((r)->attributes & DNS_RDATASETATTR_CYCLIC) != 0)

struct towire_sort {
	int key;
	dns_rdata_t *rdata;
};

static int
towire_compare(const void *av, const void *bv) {
	const struct towire_sort *a = (const struct towire_sort *) av;
	const struct towire_sort *b = (const struct towire_sort *) bv;
	return (a->key - b->key);
}

static isc_result_t
towiresorted(dns_rdataset_t *rdataset, const dns_name_t *owner_name,
	     dns_compress_t *cctx, isc_buffer_t *target,
	     dns_rdatasetorderfunc_t order, const void *order_arg,
	     isc_boolean_t partial, unsigned int options,
	     unsigned int *countp, void **state)
{
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_region_t r;
	isc_result_t result;
	unsigned int i, count = 0, added, choice;
	isc_buffer_t savedbuffer, rdlen, rrbuffer;
	unsigned int headlen;
	isc_boolean_t question = ISC_FALSE;
	isc_boolean_t shuffle = ISC_FALSE, sort = ISC_FALSE;
	isc_boolean_t want_random, want_cyclic;
	dns_rdata_t in_fixed[MAX_SHUFFLE];
	dns_rdata_t *in = in_fixed;
	struct towire_sort out_fixed[MAX_SHUFFLE];
	struct towire_sort *out = out_fixed;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_uint16_t offset;

	UNUSED(state);

	/*
	 * Convert 'rdataset' to wire format, compressing names as specified
	 * in cctx, and storing the result in 'target'.
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);
	REQUIRE(countp != NULL);
	REQUIRE(cctx != NULL && cctx->mctx != NULL);

	want_random = WANT_RANDOM(rdataset);
	want_cyclic = WANT_CYCLIC(rdataset);

	if ((rdataset->attributes & DNS_RDATASETATTR_QUESTION) != 0) {
		question = ISC_TRUE;
		count = 1;
		result = dns_rdataset_first(rdataset);
		INSIST(result == ISC_R_NOMORE);
	} else if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) != 0) {
		/*
		 * This is a negative caching rdataset.
		 */
		unsigned int ncache_opts = 0;
		if ((options & DNS_RDATASETTOWIRE_OMITDNSSEC) != 0)
			ncache_opts |= DNS_NCACHETOWIRE_OMITDNSSEC;
		return (dns_ncache_towire(rdataset, cctx, target, ncache_opts,
					  countp));
	} else {
		count = (rdataset->methods->count)(rdataset);
		result = dns_rdataset_first(rdataset);
		if (result == ISC_R_NOMORE)
			return (ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	/*
	 * Do we want to sort and/or shuffle this answer?
	 */
	if (!question && count > 1 && rdataset->type != dns_rdatatype_rrsig) {
		if (order != NULL) {
			sort = ISC_TRUE;
		}
		if (want_random || want_cyclic) {
			shuffle = ISC_TRUE;
		}
	}

	if ((shuffle || sort) && count > MAX_SHUFFLE) {
		in = isc_mem_get(cctx->mctx, count * sizeof(*in));
		out = isc_mem_get(cctx->mctx, count * sizeof(*out));
		if (in == NULL || out == NULL)
			shuffle = sort = ISC_FALSE;
	}

	if (shuffle || sort) {
		/*
		 * First we get handles to all of the rdata.
		 */
		i = 0;
		do {
			INSIST(i < count);
			dns_rdata_init(&in[i]);
			dns_rdataset_current(rdataset, &in[i]);
			i++;
			result = dns_rdataset_next(rdataset);
		} while (result == ISC_R_SUCCESS);
		if (result != ISC_R_NOMORE)
			goto cleanup;
		INSIST(i == count);
	}

	if (shuffle) {
		if (want_random) {
			/*
			 * 'Random' order.
			 */
			for (i = 0; i < count; i++) {
				isc_uint32_t val;

				isc_random_get(&val);
				choice = i + (val % (count - i));
				rdata = in[i];
				in[i] = in[choice];
				in[choice] = rdata;
				if (order != NULL)
					out[i].key = (*order)(&in[i],
								 order_arg);
				else
					out[i].key = 0; /* Unused */
				out[i].rdata = &in[i];
			}
		} else if (want_cyclic) {
			/*
			 * 'Cyclic' order.
			 */
			isc_uint32_t val;
			unsigned int j;

			val = rdataset->count;
			if (val == ISC_UINT32_MAX)
				isc_random_get(&val);
			j = val % count;
			for (i = 0; i < count; i++) {
				if (order != NULL)
					out[i].key = (*order)(&in[j],
								 order_arg);
				else
					out[i].key = 0; /* Unused */
				out[i].rdata = &in[j];
				j++;
				if (j == count)
					j = 0; /* Wrap around. */
			}
		}
	} else if (sort) {
		for (i = 0; i < count; i++) {
			if (order != NULL)
				out[i].key = (*order)(&in[i], order_arg);
			else
				out[i].key = 0; /* Unused */
			out[i].rdata = &in[i];
		}
	}

	/*
	 * Sortlist order.
	 */
	if (sort) {
		qsort(out, count, sizeof(out[0]), towire_compare);
	}

	savedbuffer = *target;
	i = 0;
	added = 0;

	name = dns_fixedname_initname(&fixed);
	dns_name_copy(owner_name, name, NULL);
	dns_rdataset_getownercase(rdataset, name);
	offset = 0xffff;

	name->attributes |= owner_name->attributes &
		DNS_NAMEATTR_NOCOMPRESS;

	do {
		/*
		 * Copy out the name, type, class, ttl.
		 */

		rrbuffer = *target;
		dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);
		result = dns_name_towire2(name, cctx, target, &offset);
		if (result != ISC_R_SUCCESS)
			goto rollback;
		headlen = sizeof(dns_rdataclass_t) + sizeof(dns_rdatatype_t);
		if (!question)
			headlen += sizeof(dns_ttl_t)
				+ 2;  /* XXX 2 for rdata len */
		isc_buffer_availableregion(target, &r);
		if (r.length < headlen) {
			result = ISC_R_NOSPACE;
			goto rollback;
		}
		isc_buffer_putuint16(target, rdataset->type);
		isc_buffer_putuint16(target, rdataset->rdclass);
		if (!question) {
			isc_buffer_putuint32(target, rdataset->ttl);

			/*
			 * Save space for rdlen.
			 */
			rdlen = *target;
			isc_buffer_add(target, 2);

			/*
			 * Copy out the rdata
			 */
			if (shuffle || sort) {
				rdata = *(out[i].rdata);
			} else {
				dns_rdata_reset(&rdata);
				dns_rdataset_current(rdataset, &rdata);
			}
			result = dns_rdata_towire(&rdata, cctx, target);
			if (result != ISC_R_SUCCESS)
				goto rollback;
			INSIST((target->used >= rdlen.used + 2) &&
			       (target->used - rdlen.used - 2 < 65536));
			isc_buffer_putuint16(&rdlen,
					     (isc_uint16_t)(target->used -
							    rdlen.used - 2));
			added++;
		}

		if (shuffle || sort) {
			i++;
			if (i == count)
				result = ISC_R_NOMORE;
			else
				result = ISC_R_SUCCESS;
		} else {
			result = dns_rdataset_next(rdataset);
		}
	} while (result == ISC_R_SUCCESS);

	if (result != ISC_R_NOMORE)
		goto rollback;

	*countp += count;

	result = ISC_R_SUCCESS;
	goto cleanup;

 rollback:
	if (partial && result == ISC_R_NOSPACE) {
		INSIST(rrbuffer.used < 65536);
		dns_compress_rollback(cctx, (isc_uint16_t)rrbuffer.used);
		*countp += added;
		*target = rrbuffer;
		goto cleanup;
	}
	INSIST(savedbuffer.used < 65536);
	dns_compress_rollback(cctx, (isc_uint16_t)savedbuffer.used);
	*countp = 0;
	*target = savedbuffer;

 cleanup:
	if (out != NULL && out != out_fixed)
		isc_mem_put(cctx->mctx, out, count * sizeof(*out));
	if (in != NULL && in != in_fixed)
		isc_mem_put(cctx->mctx, in, count * sizeof(*in));
	return (result);
}

isc_result_t
dns_rdataset_towiresorted(dns_rdataset_t *rdataset,
			  const dns_name_t *owner_name,
			  dns_compress_t *cctx,
			  isc_buffer_t *target,
			  dns_rdatasetorderfunc_t order,
			  const void *order_arg,
			  unsigned int options,
			  unsigned int *countp)
{
	return (towiresorted(rdataset, owner_name, cctx, target,
			     order, order_arg, ISC_FALSE, options,
			     countp, NULL));
}

isc_result_t
dns_rdataset_towirepartial(dns_rdataset_t *rdataset,
			   const dns_name_t *owner_name,
			   dns_compress_t *cctx,
			   isc_buffer_t *target,
			   dns_rdatasetorderfunc_t order,
			   const void *order_arg,
			   unsigned int options,
			   unsigned int *countp,
			   void **state)
{
	REQUIRE(state == NULL);	/* XXX remove when implemented */
	return (towiresorted(rdataset, owner_name, cctx, target,
			     order, order_arg, ISC_TRUE, options,
			     countp, state));
}

isc_result_t
dns_rdataset_towire(dns_rdataset_t *rdataset,
		    const dns_name_t *owner_name,
		    dns_compress_t *cctx,
		    isc_buffer_t *target,
		    unsigned int options,
		    unsigned int *countp)
{
	return (towiresorted(rdataset, owner_name, cctx, target,
			     NULL, NULL, ISC_FALSE, options, countp, NULL));
}

isc_result_t
dns_rdataset_additionaldata(dns_rdataset_t *rdataset,
			    dns_additionaldatafunc_t add, void *arg)
{
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;

	/*
	 * For each rdata in rdataset, call 'add' for each name and type in the
	 * rdata which is subject to additional section processing.
	 */

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE((rdataset->attributes & DNS_RDATASETATTR_QUESTION) == 0);

	result = dns_rdataset_first(rdataset);
	if (result != ISC_R_SUCCESS)
		return (result);

	do {
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_additionaldata(&rdata, add, arg);
		if (result == ISC_R_SUCCESS)
			result = dns_rdataset_next(rdataset);
		dns_rdata_reset(&rdata);
	} while (result == ISC_R_SUCCESS);

	if (result != ISC_R_NOMORE)
		return (result);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_rdataset_addnoqname(dns_rdataset_t *rdataset, dns_name_t *name) {

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);
	if (rdataset->methods->addnoqname == NULL)
		return (ISC_R_NOTIMPLEMENTED);
	return((rdataset->methods->addnoqname)(rdataset, name));
}

isc_result_t
dns_rdataset_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
			dns_rdataset_t *neg, dns_rdataset_t *negsig)
{
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	if (rdataset->methods->getnoqname == NULL)
		return (ISC_R_NOTIMPLEMENTED);
	return((rdataset->methods->getnoqname)(rdataset, name, neg, negsig));
}

isc_result_t
dns_rdataset_addclosest(dns_rdataset_t *rdataset, const dns_name_t *name) {

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);
	if (rdataset->methods->addclosest == NULL)
		return (ISC_R_NOTIMPLEMENTED);
	return((rdataset->methods->addclosest)(rdataset, name));
}

isc_result_t
dns_rdataset_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
			dns_rdataset_t *neg, dns_rdataset_t *negsig)
{
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	if (rdataset->methods->getclosest == NULL)
		return (ISC_R_NOTIMPLEMENTED);
	return((rdataset->methods->getclosest)(rdataset, name, neg, negsig));
}

void
dns_rdataset_settrust(dns_rdataset_t *rdataset, dns_trust_t trust) {
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	if (rdataset->methods->settrust != NULL)
		(rdataset->methods->settrust)(rdataset, trust);
	else
		rdataset->trust = trust;
}

void
dns_rdataset_expire(dns_rdataset_t *rdataset) {
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	if (rdataset->methods->expire != NULL)
		(rdataset->methods->expire)(rdataset);
}

void
dns_rdataset_clearprefetch(dns_rdataset_t *rdataset) {
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	if (rdataset->methods->clearprefetch != NULL)
		(rdataset->methods->clearprefetch)(rdataset);
}

void
dns_rdataset_setownercase(dns_rdataset_t *rdataset, const dns_name_t *name) {
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	if (rdataset->methods->setownercase != NULL)
		(rdataset->methods->setownercase)(rdataset, name);
}

void
dns_rdataset_getownercase(const dns_rdataset_t *rdataset, dns_name_t *name) {
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);

	if (rdataset->methods->getownercase != NULL)
		(rdataset->methods->getownercase)(rdataset, name);
}

void
dns_rdataset_trimttl(dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		     dns_rdata_rrsig_t *rrsig, isc_stdtime_t now,
		     isc_boolean_t acceptexpired)
{
	isc_uint32_t ttl = 0;

	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(DNS_RDATASET_VALID(sigrdataset));
	REQUIRE(rrsig != NULL);

	/*
	 * If we accept expired RRsets keep them for no more than 120 seconds.
	 */
	if (acceptexpired &&
	    (isc_serial_le(rrsig->timeexpire, ((now + 120) & 0xffffffff)) ||
	     isc_serial_le(rrsig->timeexpire, now)))
		ttl = 120;
	else if (isc_serial_ge(rrsig->timeexpire, now))
		ttl = rrsig->timeexpire - now;

	ttl = ISC_MIN(ISC_MIN(rdataset->ttl, sigrdataset->ttl),
		      ISC_MIN(rrsig->originalttl, ttl));
	rdataset->ttl = ttl;
	sigrdataset->ttl = ttl;
}

isc_result_t
dns_rdataset_addglue(dns_rdataset_t *rdataset,
		     dns_dbversion_t *version,
		     unsigned int options,
		     dns_message_t *msg)
{
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);
	REQUIRE(rdataset->type == dns_rdatatype_ns);

	if (rdataset->methods->addglue == NULL)
		return (ISC_R_NOTIMPLEMENTED);

	return ((rdataset->methods->addglue)(rdataset, version,
					     options, msg));
}
