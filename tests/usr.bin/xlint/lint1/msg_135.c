/*	$NetBSD: msg_135.c,v 1.13 2023/04/22 19:45:04 rillig Exp $	*/
# 3 "msg_135.c"

// Test for message: converting '%s' to '%s' increases alignment from %u to %u [135]

/* lint1-extra-flags: -h -X 351 */

void sink(const void *);

unsigned
read_uint(const unsigned short **pp)
{
	unsigned val;

	/* expect+1: warning: converting 'pointer to const unsigned short' to 'pointer to const unsigned int' increases alignment from 2 to 4 [135] */
	val = *(const unsigned *)(*pp);
	pp += sizeof(unsigned);
	return val;
}

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

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

/*
 * Before tree.c 1.316 from 2021-07-15, lint warned about pointer casts from
 * unsigned char or plain char to another type.  These casts often occur in
 * traditional code that does not use void pointers, even 30 years after C90
 * introduced 'void'.
 */
void
unsigned_char_to_unsigned_type(unsigned char *ucp)
{
	unsigned short *usp;

	usp = (unsigned short *)ucp;
	sink(usp);
}

/*
 * Before tree.c 1.316 from 2021-07-15, lint warned about pointer casts from
 * unsigned char or plain char to another type.  These casts often occur in
 * traditional code that does not use void pointers, even 30 years after C90
 * introduced 'void'.
 */
void
plain_char_to_unsigned_type(char *cp)
{
	unsigned short *usp;

	usp = (unsigned short *)cp;
	sink(usp);
}

/*
 * Converting a pointer with a low alignment requirement to a union that
 * includes other types with higher alignment requirements is safe.  While
 * accessing any other member of the union might trigger an alignment
 * violation, such an access would invoke undefined behavior anyway.
 *
 * A practical case for this pattern are tagged unions, in which the first
 * member of the struct determines how the remaining members are interpreted.
 * See sbin/newfs_udf, function udf_validate_tag_and_crc_sums for an example.
 */
double
cast_to_union(void)
{
	int align_4 = 0;
	double align_8 = 0.0;
	union both {
		int p_align_4;
		double p_align_8;
	} *both;

	/* TODO: don't warn here. */
	/* expect+1: warning: converting 'pointer to int' to 'pointer to union both' increases alignment from 4 to 8 [135] */
	both = (union both *)&align_4;
	both = (union both *)&align_8;
	return both->p_align_8;
}
