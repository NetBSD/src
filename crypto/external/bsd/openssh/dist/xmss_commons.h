/*	$NetBSD: xmss_commons.h,v 1.2.2.2 2018/04/16 01:57:31 pgoyette Exp $	*/
/* $OpenBSD: xmss_commons.h,v 1.3 2018/02/26 03:56:44 dtucker Exp $ */
/*
xmss_commons.h 20160722
Andreas Hülsing
Joost Rijneveld
Public domain.
*/
#ifndef XMSS_COMMONS_H
#define XMSS_COMMONS_H

#include <stdlib.h>
#include <stdint.h>

void to_byte(unsigned char *output, unsigned long long in, uint32_t bytes);
void hexdump(const unsigned char *a, size_t len);
#endif
