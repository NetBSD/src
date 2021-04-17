/*	$NetBSD: msg_135.c,v 1.7 2021/04/17 16:58:04 rillig Exp $	*/
# 3 "msg_135.c"

// Test for message: converting '%s' to '%s' may cause alignment problem [135]

/* lint1-extra-flags: -h */

void sink(const void *);

unsigned
read_uint(const unsigned char **pp)
{
	unsigned val;

	val = *(const unsigned *)(*pp);	/* expect: 135 */
	pp += sizeof(unsigned);
	return val;
}

struct incomplete;	/* expect: never defined */

struct complete {
    int member;
};

/*
 * These types of conversions are typically seen in OpenSSL, when converting
 * from the publicly visible, incomplete 'struct lhash_st' to a private
 * implementation type such as 'struct lhash_st_OPENSSL_STRING'.
 *
 * Before tree.c 1.277 from 2021-04-17, lint warned about this, even though
 * there was not enough evidence that there really was an alignment problem,
 * resulting in many false positives.
 *
 * See openssl/lhash.h.
 */
void
pointer_to_structs(struct incomplete *incomplete)
{
	struct complete *complete;

	complete = (struct complete *)incomplete;
	sink(complete);
}
