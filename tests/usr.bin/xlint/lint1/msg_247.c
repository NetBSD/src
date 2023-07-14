/*	$NetBSD: msg_247.c,v 1.32 2023/07/14 08:53:52 rillig Exp $	*/
# 3 "msg_247.c"

// Test for message: pointer cast from '%s' to '%s' may be troublesome [247]

/*
 * The word 'may' in the message text means that the trouble is not necessarily
 * on this platform with its specific type sizes, but on other platforms.
 *
 * See also:
 *	msg_247_portable.c
 *	msg_247_lp64.c
 */

/* lint1-extra-flags: -c -X 351 */

/* example taken from Xlib.h */
typedef struct {
	int id;
} *PDisplay;

struct Other {
	int id;
};

PDisplay
example(struct Other *arg)
{
	/*
	 * Before tree.c 1.461 from 2022-06-24, lint warned about the cast
	 * between the structs.
	 *
	 * XXX: The target type was reported as 'struct <unnamed>'.  In cases
	 *  like these, it would be helpful to print at least the type name
	 *  of the pointer.  This type name though is discarded immediately
	 *  in the grammar rule 'typespec: T_TYPENAME'.
	 *  After that, the target type of the cast is just an unnamed struct,
	 *  with no hint at all that there is a typedef for a pointer to the
	 *  struct.
	 */
	return (PDisplay)arg;
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
 * In traditional C, there was 'unsigned char' as well, so the same reasoning
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
counter_new_typesafe(void)
{
	struct counter_impl *impl = allocate();
	impl->public_part.count = 12345;
	impl->saved_count = 12346;
	return &impl->public_part;
}

struct counter *
counter_new_cast(void)
{
	struct counter_impl *impl = allocate();
	impl->public_part.count = 12345;
	impl->saved_count = 12346;
	/* Before tree.c 1.462 from 2022-06-24, lint warned about this cast. */
	return (struct counter *)impl;
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
# 179 "msg_247.c" 2

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

/*
 * Before tree.c 1.461 from 2022-06-24, lint warned about the cast between the
 * sockaddr variants.  Since then, lint allows casts between pointers to
 * structs if the initial members have compatible types and either of the
 * struct types continues with a byte array.
 */
void *
cast_between_sockaddr_variants(void *ptr)
{

	void *t1 = (struct sockaddr_in *)(struct sockaddr *)ptr;
	void *t2 = (struct sockaddr *)(struct sockaddr_in *)t1;
	void *t3 = (struct sockaddr_in6 *)(struct sockaddr *)t2;
	void *t4 = (struct sockaddr *)(struct sockaddr_in6 *)t3;

	/* expect+1: warning: pointer cast from 'pointer to struct sockaddr_in6' to 'pointer to struct sockaddr_in' may be troublesome [247] */
	void *t5 = (struct sockaddr_in *)(struct sockaddr_in6 *)t4;

	/* expect+1: warning: pointer cast from 'pointer to struct sockaddr_in' to 'pointer to struct sockaddr_in6' may be troublesome [247] */
	void *t6 = (struct sockaddr_in6 *)(struct sockaddr_in *)t5;

	return t6;
}


// From jemalloc.

typedef struct ctl_node_s {
	_Bool named;
} ctl_node_t;

typedef struct ctl_named_node_s {
	ctl_node_t node;
	const char *name;
} ctl_named_node_t;

void *
cast_between_first_member_struct(void *ptr)
{
	/* Before tree.c 1.462 from 2022-06-24, lint warned about this cast. */
	/* expect+1: warning: 't1' set but not used in function 'cast_between_first_member_struct' [191] */
	void *t1 = (ctl_node_t *)(ctl_named_node_t *)ptr;

	void *t2 = (ctl_named_node_t *)(ctl_node_t *)ptr;

	return t2;
}

double *
unnecessary_cast_from_array_to_pointer(int dim)
{
	static double storage_1d[10];
	static double storage_2d[10][5];

	if (dim == 1)
		return (double *)storage_1d;

	if (dim == -1)
		return storage_1d;

	if (dim == 2)
		/* expect+1: warning: illegal combination of 'pointer to double' and 'pointer to array[5] of double' [184] */
		return storage_2d;

	/*
	 * C11 6.3.2.1p3 says that an array is converted to a pointer to its
	 * first element.  That paragraph doesn't say 'recursively', that
	 * word is only used two paragraphs above, in 6.3.2.1p1.
	 */
	if (dim == -2)
		return storage_2d[0];

	return (double *)storage_2d;
}


typedef void (*function)(void);

typedef struct {
	function m_function_array[5];
} struct_function_array;

typedef union {
	int um_int;
	double um_double;
	struct_function_array um_function_array;
} anything;

static int *p_int;
static double *p_double;
static function p_function;
static struct_function_array *p_function_array;
static anything *p_anything;

void
conversions_from_and_to_union(void)
{
	/* Self-assignment, disguised by a cast to its own type. */
	p_int = (int *)p_int;
	/* Self-assignment, disguised by a cast to a pointer. */
	p_int = (void *)p_int;

	/* expect+1: warning: illegal combination of 'pointer to int' and 'pointer to double', op '=' [124] */
	p_int = p_double;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to int' may be troublesome [247] */
	p_int = (int *)p_double;

	/* expect+1: warning: illegal combination of 'pointer to union typedef anything' and 'pointer to double', op '=' [124] */
	p_anything = p_double;
	/* OK, since the union 'anything' has a 'double' member. */
	p_anything = (anything *)p_double;
	/* expect+1: warning: illegal combination of 'pointer to double' and 'pointer to union typedef anything', op '=' [124] */
	p_double = p_anything;
	/* OK, since the union 'anything' has a 'double' member. */
	p_double = (double *)p_anything;

	/*
	 * Casting to an intermediate union does not make casting between two
	 * incompatible types better.
	 */
	/* expect+1: warning: illegal combination of 'pointer to function(void) returning void' and 'pointer to union typedef anything', op '=' [124] */
	p_function = (anything *)p_int;

	/* expect+2: warning: converting 'pointer to function(void) returning void' to 'pointer to union typedef anything' is questionable [229] */
	/* expect+1: warning: illegal combination of 'pointer to function(void) returning void' and 'pointer to union typedef anything', op '=' [124] */
	p_function = (anything *)p_function_array->m_function_array[0];

	/* expect+1: warning: illegal combination of 'pointer to int' and 'pointer to function(void) returning void', op '=' [124] */
	p_int = p_function;
}
