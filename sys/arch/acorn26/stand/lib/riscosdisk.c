/*	$NetBSD: riscosdisk.c,v 1.1.164.1 2014/08/20 00:02:40 tls Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <riscoscalls.h>

#include <stdarg.h>

struct riscosdisk {
	int	drive;
	/* SWI numbers */
	int	describe_disc;
	int	disc_op;
};

int
rodisk_open(struct open_file *f, ...)
{
	va_list ap;
	char const *fsname;
	int drive;
	size_t buflen;
	char *buf;
	struct riscosdisk *rd;

	va_start(ap, f);
	fsname = va_arg(ap, char const *);
	drive = va_arg(ap, int);
	va_end(ap);

	rd = (struct riscosdisk *) alloc(sizeof(*rd));

	buflen = strlen(fsname) + 13;
	buf = alloc(buflen);
	snprintf(buf, buflen, "%s_DescribeDisc", fsname);
	if (xos_swi_number_from_string(buf, &rd->describe_disc) != NULL)
		return ENODEV;
	snprintf(buf, buflen, "%s_DiscOp", fsname);
	if (xos_swi_number_from_string(buf, &rd->disc_op) != NULL)
		return ENODEV;
	


