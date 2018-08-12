/*	$NetBSD: xml.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_XML_H
#define ISC_XML_H 1

/*
 * This file is here mostly to make it easy to add additional libxml header
 * files as needed across all the users of this file.  Rather than place
 * these libxml includes in each file, one include makes it easy to handle
 * the ifdef as well as adding the ability to add additional functions
 * which may be useful.
 */

#ifdef HAVE_LIBXML2
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#endif

#define ISC_XMLCHAR (const xmlChar *)

#define ISC_XML_RENDERCONFIG		0x00000001 /* render config data */
#define ISC_XML_RENDERSTATS		0x00000002 /* render stats */
#define ISC_XML_RENDERALL		0x000000ff /* render everything */

#endif /* ISC_XML_H */
