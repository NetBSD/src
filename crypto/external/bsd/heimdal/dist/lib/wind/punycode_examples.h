/*	$NetBSD: punycode_examples.h,v 1.2.10.1 2023/08/11 13:40:02 martin Exp $	*/

/* ./punycode_examples.h */
/* Automatically generated at 2022-11-15T14:04:18.893502 */

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
