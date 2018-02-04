/*	$NetBSD: db_access.c,v 1.23 2018/02/04 09:17:54 mrg Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_access.c,v 1.23 2018/02/04 09:17:54 mrg Exp $");

#if defined(_KERNEL_OPT)
#include "opt_kgdb.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/endian.h>

#include <ddb/ddb.h>

/*
 * Access unaligned data items on aligned (longword)
 * boundaries.
 *
 * This file is shared by ddb, kgdb and crash(8).
 */

#if defined(DDB) || !defined(DDB) && !defined(KGDB)
#define	_COMPILE_THIS
#endif

#if defined(_COMPILE_THIS) || defined(KGDB) && defined(SOFTWARE_SSTEP)

db_expr_t
db_get_value(db_addr_t addr, size_t size, bool is_signed)
{
	char data[sizeof(db_expr_t)] __aligned(sizeof(db_expr_t));
	db_expr_t value;
	size_t i;

	db_read_bytes(addr, size, data);

	value = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
	for (i = size; i-- > 0;)
#else /* BYTE_ORDER == BIG_ENDIAN */
	for (i = 0; i < size; i++)
#endif /* BYTE_ORDER */
		value = (value << 8) + (data[i] & 0xFF);

	if (size < sizeof(db_expr_t) && is_signed
	    && (value & ((db_expr_t)1 << (8*size - 1)))) {
		value |= (unsigned long)~(db_expr_t)0 << (8*size - 1);
	}
	return (value);
}

void
db_put_value(db_addr_t addr, size_t size, db_expr_t value)
{
	char data[sizeof(db_expr_t)] __aligned(sizeof(db_expr_t));
	size_t i;

#if BYTE_ORDER == LITTLE_ENDIAN
	for (i = 0; i < size; i++)
#else /* BYTE_ORDER == BIG_ENDIAN */
	for (i = size; i-- > 0;)
#endif /* BYTE_ORDER */
	{
		data[i] = value & 0xFF;
		value >>= 8;
	}

	db_write_bytes(addr, size, data);
}

#endif	/* _COMPILE_THIS || KGDB && SOFTWARE_SSTEP */

#ifdef	_COMPILE_THIS

void *
db_read_ptr(const char *name)
{
	db_expr_t val;
	void *p;

	if (!db_value_of_name(name, &val)) {
		db_printf("db_read_ptr: cannot find `%s'\n", name);
		db_error(NULL);
		/* NOTREACHED */
	}
	db_read_bytes((db_addr_t)val, sizeof(p), (char *)&p);
	return p;
}

int
db_read_int(const char *name)
{
	db_expr_t val;
	int p;

	if (!db_value_of_name(name, &val)) {
		db_printf("db_read_int: cannot find `%s'\n", name);
		db_error(NULL);
		/* NOTREACHED */
	}
	db_read_bytes((db_addr_t)val, sizeof(p), (char *)&p);
	return p;
}

#endif	/* _COMPILE_THIS */
