/*	$NetBSD: msg_247.c,v 1.21 2022/06/24 20:02:58 rillig Exp $	*/
# 3 "msg_247.c"

// Test for message: pointer cast from '%s' to '%s' may be troublesome [247]

/* lint1-extra-flags: -c */

/* example taken from Xlib.h */
typedef struct {
	int id;
} *PDisplay;

struct Other {
	int id;
};

void
example(struct Other *arg)
{
	PDisplay display;

	/*
	 * XXX: The target type is reported as 'struct <unnamed>'.  In cases
	 *  like these, it would be helpful to print at least the type name
	 *  of the pointer.  This type name though is discarded immediately
	 *  in the grammar rule 'typespec: T_TYPENAME'.
	 *  After that, the target type of the cast is just an unnamed struct,
	 *  with no hint at all that there is a typedef for a pointer to the
	 *  struct.
	 */
	/* expect+1: warning: pointer cast from 'pointer to struct Other' to 'pointer to struct <unnamed>' may be troublesome [247] */
	display = (PDisplay)arg;
}

/*
 * C code with a long history that has existed in pre-C90 times already often
 * uses 'pointer to char' where modern code would use 'pointer to void'.
 * Since 'char' is the most general underlying type, there is nothing wrong
 * with casting to it.  An example for this type of code is X11.
 *
 * Casting to 'pointer to char' may also be used by programmers who don't know
 * about endianness, but that's not something lint can do anything about.  The
 * code for these two use cases looks exactly the same, so lint errs on the
 * side of fewer false positive warnings here.
 */
char *
cast_to_char_pointer(struct Other *arg)
{
	return (char *)arg;
}

/*
 * In traditional C there was 'unsigned char' as well, so the same reasoning
 * as for plain 'char' applies here.
 */
unsigned char *
cast_to_unsigned_char_pointer(struct Other *arg)
{
	return (unsigned char *)arg;
}

/*
 * Traditional C does not have the type specifier 'signed', which means that
 * this type cannot be used by old code.  Therefore warn about this.  All code
 * that triggers this warning should do the intermediate cast via 'void
 * pointer'.
 */
signed char *
cast_to_signed_char_pointer(struct Other *arg)
{
	/* expect+1: warning: pointer cast from 'pointer to struct Other' to 'pointer to signed char' may be troublesome [247] */
	return (signed char *)arg;
}

char *
cast_to_void_pointer_then_to_char_pointer(struct Other *arg)
{
	return (char *)(void *)arg;
}


/*
 * When implementing types that have a public part that is exposed to the user
 * (in this case 'struct counter') and a private part that is only visible to
 * the implementation (in this case 'struct counter_impl'), a common
 * implementation technique is to use a struct in which the public part is the
 * first member.  C guarantees that the pointer to the first member is at the
 * same address as the pointer to the whole struct.
 *
 * Seen in external/mpl/bind/dist/lib/isc/mem.c for 'struct isc_mem' and
 * 'struct isc__mem'.
 */

struct counter {
	int count;
};

struct counter_impl {
	struct counter public_part;
	int saved_count;
};

void *allocate(void);

struct counter *
counter_new(void)
{
	struct counter_impl *impl = allocate();
	impl->public_part.count = 12345;
	impl->saved_count = 12346;
	return &impl->public_part;
}

void
counter_increment(struct counter *counter)
{
	/*
	 * Before tree.c 1.272 from 2021-04-08, lint warned about the cast
	 * from 'struct counter' to 'struct counter_impl'.
	 */
	struct counter_impl *impl = (struct counter_impl *)counter;
	impl->saved_count = impl->public_part.count;
	impl->public_part.count++;
}


/*
 * In OpenSSL, the hashing API uses the incomplete 'struct lhash_st' for their
 * type-generic hashing API while defining a separate struct for each type to
 * be hashed.
 *
 * Before 2021-04-09, in a typical NetBSD build this led to about 38,000 lint
 * warnings about possibly troublesome pointer casts.
 */

/* expect+1: warning: struct 'lhash_st' never defined [233] */
struct lhash_st;

struct lhash_st *OPENSSL_LH_new(void);

struct lhash_st_OPENSSL_STRING {
	union lh_OPENSSL_STRING_dummy {
		void *d1;
		unsigned long d2;
		int d3;
	} dummy;
};

# 196 "lhash.h" 1 3 4
struct lhash_st_OPENSSL_STRING *
lh_OPENSSL_STRING_new(void)
{
	/*
	 * Since tree.c 1.274 from 2021-04-09, lint does not warn about casts
	 * to or from incomplete structs anymore.
	 */
	return (struct lhash_st_OPENSSL_STRING *)OPENSSL_LH_new();
}
# 160 "msg_247.c" 2

void sink(const void *);

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
 * Before tree.c 1.460 from 2022-06-24, lint warned about pointer casts from
 * unsigned char or plain char to a struct or union type.  These casts often
 * occur in traditional code that does not use void pointers, even 30 years
 * after C90 introduced 'void'.
 */
void
char_to_struct(void *ptr)
{

	sink((struct counter *)(char *)ptr);

	sink((struct counter *)(unsigned char *)ptr);

	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to struct counter' may be troublesome [247] */
	sink((struct counter *)(signed char *)ptr);
}


// The following data types are simplified from various system headers.

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef uint16_t in_port_t;
typedef uint8_t sa_family_t;

struct sockaddr {
	uint8_t sa_len;
	sa_family_t sa_family;
	char sa_data[14];
};

struct in_addr {
	uint32_t s_addr;
};

struct sockaddr_in {
	uint8_t sin_len;
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	uint8_t sin_zero[8];
};

struct sockaddr_in6 {
	uint8_t sin6_len;
	sa_family_t sin6_family;
	in_port_t sin6_port;
	uint32_t sin6_flowinfo;
	union {
		uint8_t u6_addr8[16];
		uint16_t u6_addr16[8];
		uint32_t u6_addr32[4];
	} sin6_addr;
	uint32_t sin6_scope_id;
};

void *
cast_between_sockaddr_variants(void *ptr)
{

	/* expect+1: warning: pointer cast from 'pointer to struct sockaddr' to 'pointer to struct sockaddr_in' may be troublesome [247] */
	void *t1 = (struct sockaddr_in *)(struct sockaddr *)ptr;

	/* expect+1: warning: pointer cast from 'pointer to struct sockaddr_in' to 'pointer to struct sockaddr' may be troublesome [247] */
	void *t2 = (struct sockaddr *)(struct sockaddr_in *)t1;

	/* expect+1: warning: pointer cast from 'pointer to struct sockaddr' to 'pointer to struct sockaddr_in6' may be troublesome [247] */
	void *t3 = (struct sockaddr_in6 *)(struct sockaddr *)t2;

	/* expect+1: warning: pointer cast from 'pointer to struct sockaddr_in6' to 'pointer to struct sockaddr' may be troublesome [247] */
	void *t4 = (struct sockaddr *)(struct sockaddr_in6 *)t3;

	return t4;
}
