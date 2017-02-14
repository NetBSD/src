/*	$NetBSD: attr_print_plain.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	attr_print_plain 3
/* SUMMARY
/*	send attributes over byte stream
/* SYNOPSIS
/*	#include <attr.h>
/*
/*	int	attr_print_plain(fp, flags, type, name, ..., ATTR_TYPE_END)
/*	VSTREAM	fp;
/*	int	flags;
/*	int	type;
/*	char	*name;
/*
/*	int	attr_vprint_plain(fp, flags, ap)
/*	VSTREAM	fp;
/*	int	flags;
/*	va_list	ap;
/* DESCRIPTION
/*	attr_print_plain() takes zero or more (name, value) simple attributes
/*	and converts its input to a byte stream that can be recovered with
/*	attr_scan_plain(). The stream is not flushed.
/*
/*	attr_vprint_plain() provides an alternate interface that is convenient
/*	for calling from within variadic functions.
/*
/*	Attributes are sent in the requested order as specified with the
/*	attr_print_plain() argument list. This routine satisfies the formatting
/*	rules as outlined in attr_scan_plain(3).
/*
/*	Arguments:
/* .IP fp
/*	Stream to write the result to.
/* .IP flags
/*	The bit-wise OR of zero or more of the following.
/* .RS
/* .IP ATTR_FLAG_MORE
/*	After sending the requested attributes, leave the output stream in
/*	a state that is usable for more attribute sending operations on
/*	the same output attribute list.
/*	By default, attr_print_plain() automatically appends an attribute list
/*	terminator when it has sent the last requested attribute.
/* .RE
/* .IP List of attributes followed by terminator:
/* .RS
/* .IP "SEND_ATTR_INT(const char *name, int value)"
/*	The arguments are an attribute name and an integer.
/* .IP "SEND_ATTR_LONG(const char *name, long value)"
/*	The arguments are an attribute name and a long integer.
/* .IP "SEND_ATTR_STR(const char *name, const char *value)"
/*	The arguments are an attribute name and a null-terminated
/*	string.
/* .IP "SEND_ATTR_DATA(const char *name, ssize_t len, const void *value)"
/*	The arguments are an attribute name, an attribute value
/*	length, and an attribute value pointer.
/* .IP "SEND_ATTR_FUNC(ATTR_PRINT_SLAVE_FN, const void *value)"
/*	The arguments are a function pointer and generic data
/*	pointer. The caller-specified function returns whatever the
/*	specified attribute printing function returns.
/* .IP "SEND_ATTR_HASH(const HTABLE *table)"
/* .IP "SEND_ATTR_NAMEVAL(const NVTABLE *table)"
/*	The content of the table is sent as a sequence of string-valued
/*	attributes with names equal to the table lookup keys.
/* .IP ATTR_TYPE_END
/*	This terminates the attribute list.
/* .RE
/* DIAGNOSTICS
/*	The result value is 0 in case of success, VSTREAM_EOF in case
/*	of trouble.
/*
/*	Panic: interface violation. All system call errors are fatal.
/* SEE ALSO
/*	attr_scan_plain(3) recover attributes from byte stream
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <htable.h>
#include <base64_code.h>
#include <vstring.h>
#include <attr.h>

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* attr_vprint_plain - send attribute list to stream */

int     attr_vprint_plain(VSTREAM *fp, int flags, va_list ap)
{
    const char *myname = "attr_print_plain";
    int     attr_type;
    char   *attr_name;
    unsigned int_val;
    unsigned long long_val;
    char   *str_val;
    HTABLE_INFO **ht_info_list;
    HTABLE_INFO **ht;
    static VSTRING *base64_buf;
    ssize_t len_val;
    ATTR_PRINT_SLAVE_FN print_fn;
    void   *print_arg;

    /*
     * Sanity check.
     */
    if (flags & ~ATTR_FLAG_ALL)
	msg_panic("%s: bad flags: 0x%x", myname, flags);

    /*
     * Iterate over all (type, name, value) triples, and produce output on
     * the fly.
     */
    while ((attr_type = va_arg(ap, int)) != ATTR_TYPE_END) {
	switch (attr_type) {
	case ATTR_TYPE_INT:
	    attr_name = va_arg(ap, char *);
	    int_val = va_arg(ap, int);
	    vstream_fprintf(fp, "%s=%u\n", attr_name, (unsigned) int_val);
	    if (msg_verbose)
		msg_info("send attr %s = %u", attr_name, (unsigned) int_val);
	    break;
	case ATTR_TYPE_LONG:
	    attr_name = va_arg(ap, char *);
	    long_val = va_arg(ap, long);
	    vstream_fprintf(fp, "%s=%lu\n", attr_name, long_val);
	    if (msg_verbose)
		msg_info("send attr %s = %lu", attr_name, long_val);
	    break;
	case ATTR_TYPE_STR:
	    attr_name = va_arg(ap, char *);
	    str_val = va_arg(ap, char *);
	    vstream_fprintf(fp, "%s=%s\n", attr_name, str_val);
	    if (msg_verbose)
		msg_info("send attr %s = %s", attr_name, str_val);
	    break;
	case ATTR_TYPE_DATA:
	    attr_name = va_arg(ap, char *);
	    len_val = va_arg(ap, ssize_t);
	    str_val = va_arg(ap, char *);
	    if (base64_buf == 0)
		base64_buf = vstring_alloc(10);
	    base64_encode(base64_buf, str_val, len_val);
	    vstream_fprintf(fp, "%s=%s\n", attr_name, STR(base64_buf));
	    if (msg_verbose)
		msg_info("send attr %s = [data %ld bytes]",
			 attr_name, (long) len_val);
	    break;
	case ATTR_TYPE_FUNC:
	    print_fn = va_arg(ap, ATTR_PRINT_SLAVE_FN);
	    print_arg = va_arg(ap, void *);
	    print_fn(attr_print_plain, fp, flags | ATTR_FLAG_MORE, print_arg);
	    break;
	case ATTR_TYPE_HASH:
	    vstream_fwrite(fp, ATTR_NAME_OPEN, sizeof(ATTR_NAME_OPEN));
	    VSTREAM_PUTC('\n', fp);
	    ht_info_list = htable_list(va_arg(ap, HTABLE *));
	    for (ht = ht_info_list; *ht; ht++) {
		vstream_fprintf(fp, "%s=%s\n", ht[0]->key, (char *) ht[0]->value);
		if (msg_verbose)
		    msg_info("send attr name %s value %s",
			     ht[0]->key, (char *) ht[0]->value);
	    }
	    myfree((void *) ht_info_list);
	    vstream_fwrite(fp, ATTR_NAME_CLOSE, sizeof(ATTR_NAME_CLOSE));
	    VSTREAM_PUTC('\n', fp);
	    break;
	default:
	    msg_panic("%s: unknown type code: %d", myname, attr_type);
	}
    }
    if ((flags & ATTR_FLAG_MORE) == 0)
	VSTREAM_PUTC('\n', fp);
    return (vstream_ferror(fp));
}

int     attr_print_plain(VSTREAM *fp, int flags,...)
{
    va_list ap;
    int     ret;

    va_start(ap, flags);
    ret = attr_vprint_plain(fp, flags, ap);
    va_end(ap);
    return (ret);
}

#ifdef TEST

 /*
  * Proof of concept test program.  Mirror image of the attr_scan_plain test
  * program.
  */
#include <msg_vstream.h>

int     main(int unused_argc, char **argv)
{
    HTABLE *table = htable_create(1);

    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_verbose = 1;
    htable_enter(table, "foo-name", mystrdup("foo-value"));
    htable_enter(table, "bar-name", mystrdup("bar-value"));
    attr_print_plain(VSTREAM_OUT, ATTR_FLAG_NONE,
		     SEND_ATTR_INT(ATTR_NAME_INT, 4711),
		     SEND_ATTR_LONG(ATTR_NAME_LONG, 1234L),
		     SEND_ATTR_STR(ATTR_NAME_STR, "whoopee"),
	       SEND_ATTR_DATA(ATTR_NAME_DATA, strlen("whoopee"), "whoopee"),
		     SEND_ATTR_HASH(table),
		     SEND_ATTR_LONG(ATTR_NAME_LONG, 4321L),
		     ATTR_TYPE_END);
    attr_print_plain(VSTREAM_OUT, ATTR_FLAG_NONE,
		     SEND_ATTR_INT(ATTR_NAME_INT, 4711),
		     SEND_ATTR_LONG(ATTR_NAME_LONG, 1234L),
		     SEND_ATTR_STR(ATTR_NAME_STR, "whoopee"),
	       SEND_ATTR_DATA(ATTR_NAME_DATA, strlen("whoopee"), "whoopee"),
		     ATTR_TYPE_END);
    if (vstream_fflush(VSTREAM_OUT) != 0)
	msg_fatal("write error: %m");

    htable_free(table, myfree);
    return (0);
}

#endif
