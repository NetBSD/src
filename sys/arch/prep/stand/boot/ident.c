/*      $NetBSD: ident.c,v 1.1 2001/06/17 15:57:12 nonaka Exp $        */

/*-
 * Copyright (C) 2001 NONAKA Kimihiro.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>

#include <string.h>

#include <machine/preptype.h>
#include <machine/residual.h>

#include "boot.h"

static struct prep_model_ibm_tag {
	char	*str;
	int	model;
} prep_model_ibm[] = {
	{ IBM_6050_STR,	IBM_6050, },
	{ IBM_7248_STR,	IBM_7248, },
	{ IBM_6040_STR,	IBM_6040, },
};

int
prep_identify(void *resp)
{
	char *p;
	int i;

	if (resp == NULL)
		return PREP_UNKNOWN;

	p = ((RESIDUAL *)resp)->VitalProductData.PrintableModel;
	if (strncmp(p, "IBM", 3) == 0) {
		for (i = 0; i < NELEMS(prep_model_ibm); i++) {
			if (strncmp(p, prep_model_ibm[i].str,
			    strlen(prep_model_ibm[i].str)) == 0)
				return prep_model_ibm[i].model;
		}
	}
	return PREP_UNKNOWN;
}
