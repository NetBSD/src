/*	$NetBSD: db_autoconf.c,v 1.1 2018/03/04 07:14:50 mlelstv Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_autoconf.c,v 1.1 2018/03/04 07:14:50 mlelstv Exp $");

#ifndef _KERNEL
#include <stdbool.h>
#endif

#include <sys/param.h>
#include <sys/device.h>

#include <ddb/ddb.h>

static const char *classnames[] = {
	"dull", "cpu", "disk", "ifnet", "tape", "tty",
	"audiodev", "displaydev", "bus", "virtual"
};

struct device *
db_device_first(void)
{

	return db_read_ptr("alldevs");
}

struct device *
db_device_next(struct device *dv)
{

	db_read_bytes((db_addr_t)&dv->dv_list.tqe_next, sizeof(dv),
	    (char *)&dv);
	return dv;
}

void
db_show_all_devices(db_expr_t addr, bool haddr, db_expr_t count,
		  const char *modif)
{
	struct device buf;
	struct device *dv;
	const char *cl;

	db_printf("%-16s %10s %4s %18s %18s\n",
		"NAME","CLASS","UNIT","DEVICE_T","PRIVATE");

	for (dv = db_device_first(); dv != NULL; dv = db_device_next(dv)) {
		db_read_bytes((db_addr_t)dv, sizeof(buf), (char *)&buf);

		if (buf.dv_class < 0 ||
		    buf.dv_class > __arraycount(classnames))
			cl = "????";
		else
			cl = classnames[buf.dv_class];

		db_printf("%-16.16s", buf.dv_xname);
		db_printf(" %10s", cl);
		db_printf(" %4u", buf.dv_unit);
		db_printf(" %18" PRIxPTR, (uintptr_t)dv);
		db_printf(" %18" PRIxPTR, (uintptr_t)buf.dv_private);
		db_printf("\n");
	}
}
