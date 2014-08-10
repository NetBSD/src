/* $NetBSD: db_machdep.c,v 1.1 2014/08/10 05:47:37 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: db_machdep.c,v 1.1 2014/08/10 05:47:37 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <aarch64/db_machdep.h>
#include <aarch64/locore.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_command.h>

db_regs_t ddb_regs;

const struct db_variable db_regs[] = {
	{ "x0", (long *) &ddb_regs.tf_reg[0], FCN_NULL, NULL },
	{ "x1", (long *) &ddb_regs.tf_reg[1], FCN_NULL, NULL },
	{ "x2", (long *) &ddb_regs.tf_reg[2], FCN_NULL, NULL },
	{ "x3", (long *) &ddb_regs.tf_reg[3], FCN_NULL, NULL },
	{ "x4", (long *) &ddb_regs.tf_reg[4], FCN_NULL, NULL },
	{ "x5", (long *) &ddb_regs.tf_reg[5], FCN_NULL, NULL },
	{ "x6", (long *) &ddb_regs.tf_reg[6], FCN_NULL, NULL },
	{ "x7", (long *) &ddb_regs.tf_reg[7], FCN_NULL, NULL },
	{ "x8", (long *) &ddb_regs.tf_reg[8], FCN_NULL, NULL },
	{ "x9", (long *) &ddb_regs.tf_reg[9], FCN_NULL, NULL },
	{ "x10", (long *) &ddb_regs.tf_reg[10], FCN_NULL, NULL },
	{ "x11", (long *) &ddb_regs.tf_reg[11], FCN_NULL, NULL },
	{ "x12", (long *) &ddb_regs.tf_reg[12], FCN_NULL, NULL },
	{ "x13", (long *) &ddb_regs.tf_reg[13], FCN_NULL, NULL },
	{ "x14", (long *) &ddb_regs.tf_reg[14], FCN_NULL, NULL },
	{ "x15", (long *) &ddb_regs.tf_reg[15], FCN_NULL, NULL },
	{ "x16", (long *) &ddb_regs.tf_reg[16], FCN_NULL, NULL },
	{ "x17", (long *) &ddb_regs.tf_reg[17], FCN_NULL, NULL },
	{ "x18", (long *) &ddb_regs.tf_reg[18], FCN_NULL, NULL },
	{ "x19", (long *) &ddb_regs.tf_reg[19], FCN_NULL, NULL },
	{ "x20", (long *) &ddb_regs.tf_reg[20], FCN_NULL, NULL },
	{ "x21", (long *) &ddb_regs.tf_reg[21], FCN_NULL, NULL },
	{ "x22", (long *) &ddb_regs.tf_reg[22], FCN_NULL, NULL },
	{ "x23", (long *) &ddb_regs.tf_reg[23], FCN_NULL, NULL },
	{ "x24", (long *) &ddb_regs.tf_reg[24], FCN_NULL, NULL },
	{ "x25", (long *) &ddb_regs.tf_reg[25], FCN_NULL, NULL },
	{ "x26", (long *) &ddb_regs.tf_reg[26], FCN_NULL, NULL },
	{ "x27", (long *) &ddb_regs.tf_reg[27], FCN_NULL, NULL },
	{ "x28", (long *) &ddb_regs.tf_reg[28], FCN_NULL, NULL },
	{ "x29", (long *) &ddb_regs.tf_reg[29], FCN_NULL, NULL },
	{ "x30", (long *) &ddb_regs.tf_reg[30], FCN_NULL, NULL },
	{ "sp", (long *) &ddb_regs.tf_sp, FCN_NULL, NULL },
	{ "pc", (long *) &ddb_regs.tf_pc, FCN_NULL, NULL },
	{ "spsr", (long *) &ddb_regs.tf_spsr, FCN_NULL, NULL },
	{ "tpidr", (long *) &ddb_regs.tf_tpidr, FCN_NULL, NULL },
};

const struct db_variable * const db_eregs = db_regs + __arraycount(db_regs);
