/*	$NetBSD: resultclass.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_RESULTCLASS_H
#define ISC_RESULTCLASS_H 1


/*! \file isc/resultclass.h
 * \brief Registry of Predefined Result Type Classes
 *
 * A result class number is an unsigned 16 bit number.  Each class may
 * contain up to 65536 results.  A result code is formed by adding the
 * result number within the class to the class number multiplied by 65536.
 *
 * Classes < 1024 are reserved for ISC use.
 * Result classes >= 1024 and <= 65535 are reserved for application use.
 */

#define ISC_RESULTCLASS_FROMNUM(num)		((num) << 16)
#define ISC_RESULTCLASS_TONUM(rclass)		((rclass) >> 16)
#define ISC_RESULTCLASS_SIZE			65536
#define ISC_RESULTCLASS_INCLASS(rclass, result) \
	((rclass) == ((result) & 0xFFFF0000))


#define	ISC_RESULTCLASS_ISC		ISC_RESULTCLASS_FROMNUM(0)
#define	ISC_RESULTCLASS_DNS		ISC_RESULTCLASS_FROMNUM(1)
#define	ISC_RESULTCLASS_DST		ISC_RESULTCLASS_FROMNUM(2)
#define	ISC_RESULTCLASS_DNSRCODE	ISC_RESULTCLASS_FROMNUM(3)
#define	ISC_RESULTCLASS_OMAPI		ISC_RESULTCLASS_FROMNUM(4)
#define	ISC_RESULTCLASS_ISCCC		ISC_RESULTCLASS_FROMNUM(5)
#define	ISC_RESULTCLASS_DHCP		ISC_RESULTCLASS_FROMNUM(6)
#define	ISC_RESULTCLASS_PK11		ISC_RESULTCLASS_FROMNUM(7)

#endif /* ISC_RESULTCLASS_H */
