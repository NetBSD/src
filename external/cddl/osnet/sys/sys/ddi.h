/*	$NetBSD: ddi.h,v 1.1 2009/08/07 20:57:57 haad Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _UTS_DDI_H
#define	_UTS_DDI_H

#include <sys/u8_textprep.h>

#define ddi_name_to_major(name) devsw_name2blk(name, NULL, 0)

int	ddi_strtoul(const char *, char **, int, unsigned long *);
int	ddi_strtol(const char *, char **, int, long *);
int	ddi_soft_state_init(void **, size_t, size_t);
int	ddi_soft_state_zalloc(void *, int);
void	*ddi_get_soft_state(void *, int);
void	ddi_soft_state_free(void *, int);
void	ddi_soft_state_fini(void **);
int	ddi_create_minor_node(dev_info_t *, char *, int,
                              minor_t, char *, int);
void	ddi_remove_minor_node(dev_info_t *, char *);

typedef void 	*ldi_ident_t;

#define	DDI_SUCCESS	0
#define	DDI_FAILURE	-1

#define	DDI_PSEUDO	""

#define	ddi_prop_update_int64(a, b, c, d)	DDI_SUCCESS
#define	ddi_prop_update_string(a, b, c, d)	DDI_SUCCESS

#define	bioerror(bp, er)	((bp)->b_error = (er))
#define	b_edev			b_dev

#endif	/* _UTS_DDI_H_ */
