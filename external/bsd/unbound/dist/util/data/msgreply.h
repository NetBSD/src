/*
 * util/data/msgreply.h - store message and reply data. 
 *
 * Copyright (c) 2007, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file contains a data structure to store a message and its reply.
 */

#ifndef UTIL_DATA_MSGREPLY_H
#define UTIL_DATA_MSGREPLY_H
#include "util/storage/lruhash.h"
#include "util/data/packed_rrset.h"
struct sldns_buffer;
struct comm_reply;
struct alloc_cache;
struct iovec;
struct regional;
struct edns_data;
struct msg_parse;
struct rrset_parse;

/** calculate the prefetch TTL as 90% of original. Calculation
 * without numerical overflow (uin32_t) */
#define PREFETCH_TTL_CALC(ttl) ((ttl) - (ttl)/10)

/**
 * Structure to store query information that makes answers to queries
 * different.
 */
struct query_info {
	/** 
	 * Salient data on the query: qname, in wireformat. 
	 * can be allocated or a pointer to outside buffer.
	 * User has to keep track on the status of this.
	 */
	uint8_t* qname;
	/** length of qname (including last 0 octet) */
	size_t qname_len;
	/** qtype, host byte order */
	uint16_t qtype;
	/** qclass, host byte order */
	uint16_t qclass;
};

/**
 * Information to reference an rrset
 */
struct rrset_ref {
	/** the key with lock, and ptr to packed data. */
	struct ub_packed_rrset_key* key;
	/** id needed */
	rrset_id_t id;
};

/**
 * Structure to store DNS query and the reply packet.
 * To use it, copy over the flags from reply and modify using flags from
 * the query (RD,CD if not AA). prepend ID. 
 *
 * Memory layout is:
 *	o struct
 *	o rrset_ref array
 *	o packed_rrset_key* array.
 *
 * Memory layout is sometimes not packed, when the message is synthesized,
 * for easy of the generation. It is allocated packed when it is copied
 * from the region allocation to the malloc allocation.
 */
struct reply_info {
	/** the flags for the answer, host byte order. */
	uint16_t flags;

	/**
	 * This flag informs unbound the answer is authoritative and 
	 * the AA flag should be preserved. 
	 */
	uint8_t authoritative;

	/**
	 * Number of RRs in the query section.
	 * If qdcount is not 0, then it is 1, and the data that appears
	 * in the reply is the same as the query_info.
	 * Host byte order.
	 */
	uint8_t qdcount;

	/** 32 bit padding to pad struct member alignment to 64 bits. */
	uint32_t padding;

	/** 
	 * TTL of the entire reply (for negative caching).
	 * only for use when there are 0 RRsets in this message.
	 * if there are RRsets, check those instead.
	 */
	time_t ttl;

	/**
	 * TTL for prefetch. After it has expired, a prefetch is suitable.
	 * Smaller than the TTL, otherwise the prefetch would not happen.
	 */
	time_t prefetch_ttl;

	/**
	 * The security status from DNSSEC validation of this message.
	 */
	enum sec_status security;

	/**
	 * Number of RRsets in each section.
	 * The answer section. Add up the RRs in every RRset to calculate
	 * the number of RRs, and the count for the dns packet. 
	 * The number of RRs in RRsets can change due to RRset updates.
	 */
	size_t an_numrrsets;

	/** Count of authority section RRsets */
	size_t ns_numrrsets; 
	/** Count of additional section RRsets */
	size_t ar_numrrsets;

	/** number of RRsets: an_numrrsets + ns_numrrsets + ar_numrrsets */
	size_t rrset_count;

	/** 
	 * List of pointers (only) to the rrsets in the order in which 
	 * they appear in the reply message.  
	 * Number of elements is ancount+nscount+arcount RRsets.
	 * This is a pointer to that array. 
	 * Use the accessor function for access.
	 */
	struct ub_packed_rrset_key** rrsets;

	/** 
	 * Packed array of ids (see counts) and pointers to packed_rrset_key.
	 * The number equals ancount+nscount+arcount RRsets. 
	 * These are sorted in ascending pointer, the locking order. So
	 * this list can be locked (and id, ttl checked), to see if 
	 * all the data is available and recent enough.
	 *
	 * This is defined as an array of size 1, so that the compiler 
	 * associates the identifier with this position in the structure.
	 * Array bound overflow on this array then gives access to the further
	 * elements of the array, which are allocated after the main structure.
	 *
	 * It could be more pure to define as array of size 0, ref[0].
	 * But ref[1] may be less confusing for compilers.
	 * Use the accessor function for access.
	 */
	struct rrset_ref ref[1];
};

/**
 * Structure to keep hash table entry for message replies.
 */
struct msgreply_entry {
	/** the hash table key */
	struct query_info key;
	/** the hash table entry, data is struct reply_info* */
	struct lruhash_entry entry;
};

/**
 * Constructor for replyinfo.
 * @param region: where to allocate the results, pass NULL to use malloc.
 * @param flags: flags for the replyinfo.
 * @param qd: qd count
 * @param ttl: TTL of replyinfo
 * @param prettl: prefetch ttl
 * @param an: an count
 * @param ns: ns count
 * @param ar: ar count
 * @param total: total rrset count (presumably an+ns+ar).
 * @param sec: security status of the reply info.
 * @return the reply_info base struct with the array for putting the rrsets
 * in.  The array has been zeroed.  Returns NULL on malloc failure.
 */
struct reply_info*
construct_reply_info_base(struct regional* region, uint16_t flags, size_t qd,
		time_t ttl, time_t prettl, size_t an, size_t ns, size_t ar,
		size_t total, enum sec_status sec);

/** 
 * Parse wire query into a queryinfo structure, return 0 on parse error. 
 * initialises the (prealloced) queryinfo structure as well.
 * This query structure contains a pointer back info the buffer!
 * This pointer avoids memory allocation. allocqname does memory allocation.
 * @param m: the prealloced queryinfo structure to put query into.
 *    must be unused, or _clear()ed.
 * @param query: the wireformat packet query. starts with ID.
 * @return: 0 on format error.
 */
int query_info_parse(struct query_info* m, struct sldns_buffer* query);

/**
 * Parse query reply.
 * Fills in preallocated query_info structure (with ptr into buffer).
 * Allocates reply_info and packed_rrsets. These are not yet added to any
 * caches or anything, this is only parsing. Returns formerror on qdcount > 1.
 * @param pkt: the packet buffer. Must be positioned after the query section.
 * @param alloc: creates packed rrset key structures.
 * @param rep: allocated reply_info is returned (only on no error).
 * @param qinf: query_info is returned (only on no error).
 * @param region: where to store temporary data (for parsing).
 * @param edns: where to store edns information, does not need to be inited.
 * @return: zero is OK, or DNS error code in case of error
 *	o FORMERR for parse errors.
 *	o SERVFAIL for memory allocation errors.
 */
int reply_info_parse(struct sldns_buffer* pkt, struct alloc_cache* alloc,
	struct query_info* qinf, struct reply_info** rep, 
	struct regional* region, struct edns_data* edns);

/**
 * Allocate and decompress parsed message and rrsets.
 * @param pkt: for name decompression.
 * @param msg: parsed message in scratch region.
 * @param alloc: alloc cache for special rrset key structures.
 *	Not used if region!=NULL, it can be NULL in that case.
 * @param qinf: where to store query info.
 *	qinf itself is allocated by the caller.
 * @param rep: reply info is allocated and returned.
 * @param region: if this parameter is NULL then malloc and the alloc is used.
 *	otherwise, everything is allocated in this region.
 *	In a region, no special rrset key structures are needed (not shared),
 *	and no rrset_ref array in the reply is built up.
 * @return 0 if allocation failed.
 */
int parse_create_msg(struct sldns_buffer* pkt, struct msg_parse* msg,
        struct alloc_cache* alloc, struct query_info* qinf,
	struct reply_info** rep, struct regional* region);

/**
 * Sorts the ref array.
 * @param rep: reply info. rrsets must be filled in.
 */
void reply_info_sortref(struct reply_info* rep);

/**
 * Set TTLs inside the replyinfo to absolute values.
 * @param rep: reply info. rrsets must be filled in. 
 *	Also refs must be filled in.
 * @param timenow: the current time.
 */
void reply_info_set_ttls(struct reply_info* rep, time_t timenow);

/** 
 * Delete reply_info and packed_rrsets (while they are not yet added to the
 * hashtables.). Returns rrsets to the alloc cache.
 * @param rep: reply_info to delete.
 * @param alloc: where to return rrset structures to.
 */
void reply_info_parsedelete(struct reply_info* rep, struct alloc_cache* alloc);

/**
 * Compare two queryinfo structures, on query and type, class. 
 * It is _not_ sorted in canonical ordering.
 * @param m1: struct query_info* , void* here to ease use as function pointer.
 * @param m2: struct query_info* , void* here to ease use as function pointer.
 * @return: 0 = same, -1 m1 is smaller, +1 m1 is larger.
 */
int query_info_compare(void* m1, void* m2);

/** clear out query info structure */
void query_info_clear(struct query_info* m);

/** calculate size of struct query_info + reply_info */
size_t msgreply_sizefunc(void* k, void* d);

/** delete msgreply_entry key structure */
void query_entry_delete(void *q, void* arg);

/** delete reply_info data structure */
void reply_info_delete(void* d, void* arg);

/** calculate hash value of query_info, lowercases the qname,
 * uses CD flag for AAAA qtype */
hashvalue_t query_info_hash(struct query_info *q, uint16_t flags);

/**
 * Setup query info entry
 * @param q: query info to copy. Emptied as if clear is called.
 * @param r: reply to init data.
 * @param h: hash value.
 * @return: newly allocated message reply cache item.
 */
struct msgreply_entry* query_info_entrysetup(struct query_info* q,
	struct reply_info* r, hashvalue_t h);

/**
 * Copy reply_info and all rrsets in it and allocate.
 * @param rep: what to copy, probably inside region, no ref[] array in it.
 * @param alloc: how to allocate rrset keys.
 *	Not used if region!=NULL, it can be NULL in that case.
 * @param region: if this parameter is NULL then malloc and the alloc is used.
 *	otherwise, everything is allocated in this region.
 *	In a region, no special rrset key structures are needed (not shared),
 *	and no rrset_ref array in the reply is built up.
 * @return new reply info or NULL on memory error.
 */
struct reply_info* reply_info_copy(struct reply_info* rep, 
	struct alloc_cache* alloc, struct regional* region);

/**
 * Copy a parsed rrset into given key, decompressing and allocating rdata.
 * @param pkt: packet for decompression
 * @param msg: the parser message (for flags for trust).
 * @param pset: the parsed rrset to copy.
 * @param region: if NULL - malloc, else data is allocated in this region.
 * @param pk: a freshly obtained rrsetkey structure. No dname is set yet,
 *	will be set on return.
 *	Note that TTL will still be relative on return.
 * @return false on alloc failure.
 */
int parse_copy_decompress_rrset(struct sldns_buffer* pkt, struct msg_parse* msg,
	struct rrset_parse *pset, struct regional* region, 
	struct ub_packed_rrset_key* pk);

/**
 * Find final cname target in reply, the one matching qinfo. Follows CNAMEs.
 * @param qinfo: what to start with.
 * @param rep: looks in answer section of this message.
 * @return: pointer dname, or NULL if not found.
 */
uint8_t* reply_find_final_cname_target(struct query_info* qinfo,
	struct reply_info* rep);

/**
 * Check if cname chain in cached reply is still valid.
 * @param qinfo: query info with query name.
 * @param rep: reply to check.
 * @return: true if valid, false if invalid.
 */
int reply_check_cname_chain(struct query_info* qinfo, struct reply_info* rep);

/**
 * Check security status of all RRs in the message.
 * @param rep: reply to check
 * @return: true if all RRs are secure. False if not.
 *    True if there are zero RRs.
 */
int reply_all_rrsets_secure(struct reply_info* rep);

/**
 * Find answer rrset in reply, the one matching qinfo. Follows CNAMEs, so the
 * result may have a different owner name.
 * @param qinfo: what to look for.
 * @param rep: looks in answer section of this message.
 * @return: pointer to rrset, or NULL if not found.
 */
struct ub_packed_rrset_key* reply_find_answer_rrset(struct query_info* qinfo,
	struct reply_info* rep);

/**
 * Find rrset in reply, inside the answer section. Does not follow CNAMEs.
 * @param rep: looks in answer section of this message.
 * @param name: what to look for.
 * @param namelen: length of name.
 * @param type: looks for (host order).
 * @param dclass: looks for (host order).
 * @return: pointer to rrset, or NULL if not found.
 */
struct ub_packed_rrset_key* reply_find_rrset_section_an(struct reply_info* rep,
	uint8_t* name, size_t namelen, uint16_t type, uint16_t dclass);

/**
 * Find rrset in reply, inside the authority section. Does not follow CNAMEs.
 * @param rep: looks in authority section of this message.
 * @param name: what to look for.
 * @param namelen: length of name.
 * @param type: looks for (host order).
 * @param dclass: looks for (host order).
 * @return: pointer to rrset, or NULL if not found.
 */
struct ub_packed_rrset_key* reply_find_rrset_section_ns(struct reply_info* rep,
	uint8_t* name, size_t namelen, uint16_t type, uint16_t dclass);

/**
 * Find rrset in reply, inside any section. Does not follow CNAMEs.
 * @param rep: looks in answer,authority and additional section of this message.
 * @param name: what to look for.
 * @param namelen: length of name.
 * @param type: looks for (host order).
 * @param dclass: looks for (host order).
 * @return: pointer to rrset, or NULL if not found.
 */
struct ub_packed_rrset_key* reply_find_rrset(struct reply_info* rep,
	uint8_t* name, size_t namelen, uint16_t type, uint16_t dclass);

/**
 * Debug send the query info and reply info to the log in readable form.
 * @param str: descriptive string printed with packet content.
 * @param qinfo: query section.
 * @param rep: rest of message.
 */
void log_dns_msg(const char* str, struct query_info* qinfo, 
	struct reply_info* rep);

/**
 * Print string with neat domain name, type, class from query info.
 * @param v: at what verbosity level to print this.
 * @param str: string of message.
 * @param qinf: query info structure with name, type and class.
 */
void log_query_info(enum verbosity_value v, const char* str, 
	struct query_info* qinf);

/**
 * Append edns option to edns data structure
 */
int edns_opt_append(struct edns_data* edns, struct regional* region,
	uint16_t code, size_t len, uint8_t* data);

/**
 * Find edns option in edns list
 * @param list: list of edns options (eg. edns.opt_list)
 * @param code: opt code to find.
 * @return NULL or the edns_option element.
 */
struct edns_option* edns_opt_find(struct edns_option* list, uint16_t code);

/**
 * Transform edns data structure from query structure into reply structure.
 * In place transform, for errors and cache replies.
 * @param edns: on input contains the edns from the query.  On output contains
 *   the edns for the answer.  Add new options to the opt_list to put them
 *   in the answer (allocated in the region, with edns_opt_append).
 * @param region: to allocate stuff in.
 * @return false on failure (servfail to client, or for some error encodings,
 *   no EDNS options in the answer).
 */
int edns_opt_inplace_reply(struct edns_data* edns, struct regional* region);

/**
 * Copy edns option list allocated to the new region
 */
struct edns_option* edns_opt_copy_region(struct edns_option* list,
	struct regional* region);

/**
 * Copy edns option list allocated with malloc
 */
struct edns_option* edns_opt_copy_alloc(struct edns_option* list);

/**
 * Free edns option list allocated with malloc
 */
void edns_opt_list_free(struct edns_option* list);

/**
 * Compare an edns option. (not entire list).  Also compares contents.
 */
int edns_opt_compare(struct edns_option* p, struct edns_option* q);

/**
 * Compare edns option lists, also the order and contents of edns-options.
 */
int edns_opt_list_compare(struct edns_option* p, struct edns_option* q);

#endif /* UTIL_DATA_MSGREPLY_H */
