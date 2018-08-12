/*	$NetBSD: internal.h,v 1.1.1.1 2018/08/12 12:08:25 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


#ifndef PK11_INTERNAL_H
#define PK11_INTERNAL_H 1

/*! \file pk11/internal.h */

ISC_LANG_BEGINDECLS

const char *pk11_get_lib_name(void);

void *pk11_mem_get(size_t size);

void pk11_mem_put(void *ptr, size_t size);

CK_SLOT_ID pk11_get_best_token(pk11_optype_t optype);

unsigned int pk11_numbits(CK_BYTE_PTR data, unsigned int bytecnt);

CK_ATTRIBUTE *pk11_attribute_first(const pk11_object_t *obj);

CK_ATTRIBUTE *pk11_attribute_next(const pk11_object_t *obj,
				  CK_ATTRIBUTE *attr);

CK_ATTRIBUTE *pk11_attribute_bytype(const pk11_object_t *obj,
				    CK_ATTRIBUTE_TYPE type);

ISC_LANG_ENDDECLS

#endif /* PK11_INTERNAL_H */
