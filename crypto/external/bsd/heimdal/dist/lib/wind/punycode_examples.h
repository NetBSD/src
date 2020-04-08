/*	$NetBSD: punycode_examples.h,v 1.2.4.2 2020/04/08 14:03:15 martin Exp $	*/

/* ./punycode_examples.h */
/* Automatically generated at 2019-06-07T02:40:18.438347 */

#ifndef PUNYCODE_EXAMPLES_H
#define PUNYCODE_EXAMPLES_H 1

#include <krb5/krb5-types.h>

#define MAX_LENGTH 40

struct punycode_example {
    size_t len;
    uint32_t val[MAX_LENGTH];
    const char *pc;
    const char *description;
};

extern const struct punycode_example punycode_examples[];

extern const size_t punycode_examples_size;
#endif /* PUNYCODE_EXAMPLES_H */
