/* $Heimdal: asn1-common.h,v 1.2 2001/09/25 13:39:25 assar Exp $
   $NetBSD: asn1-common.h,v 1.1.1.3 2002/09/12 12:41:40 joda Exp $ */

#include <stddef.h>
#include <time.h>

#ifndef __asn1_common_definitions__
#define __asn1_common_definitions__

typedef struct octet_string {
    size_t length;
    void *data;
} octet_string;

typedef char *general_string;

typedef struct oid {
    size_t length;
    unsigned *components;
} oid;

#endif
