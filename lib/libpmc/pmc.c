/*	$NetBSD: pmc.c,v 1.3 2003/05/21 20:29:51 kristerw Exp $	*/

/*
 * Copyright (c 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pmc.c,v 1.3 2003/05/21 20:29:51 kristerw Exp $");

#include <sys/param.h>
#include <errno.h>
#include <string.h>
#include <pmc.h>

#include "pmc_private.h"

static const struct pmc_class2evid *
_pmc_type_lookup(int classval)
{
	const struct pmc_class2evid *class;

	for (class = _pmc_md_classes; class->name != NULL; class++) {
		if (class->class == classval)
			return (class);
	}
	return (NULL);
}

static const struct pmc_event *
_pmc_evid_lookup(const char *evname)
{
	const struct pmc_class2evid *class;
	const struct pmc_event *evid;
	int classval;

	if (pmc_get_info(-1, PMC_INFO_CPUCTR_TYPE, &classval) == -1) {
		/* Sets errno for us. */
		return (NULL);
	}

	if ((class = _pmc_type_lookup(classval)) == NULL) {
		errno = ESRCH;
		return (NULL);
	}

	for (evid = class->evids; evid->name != NULL; evid++) {
		if (evname[0] == evid->name[0] &&
		    strcmp(evname, evid->name) == 0)
			break;
	}
	if (evid->name == NULL) {
		errno = ESRCH;
		return (NULL);
	}

	return (evid);
}

int
pmc_configure_counter(int ctr, const char *evname, pmc_ctr_t reset_val,
    uint32_t flags)
{
	struct pmc_counter_cfg pcfg;
	const struct pmc_event *evid;

	evid = _pmc_evid_lookup(evname);
	if (evid == NULL) {
		/* _pmc_evid_lookup() set errno */
		return (-1);
	}

	pcfg.event_id = evid->val;
	pcfg.reset_value = reset_val;
	pcfg.flags = flags;

	return (pmc_control(ctr, PMC_OP_CONFIGURE, &pcfg));
}

int
pmc_start_counter(int ctr)
{
	return (pmc_control(ctr, PMC_OP_START, NULL));
}

int
pmc_stop_counter(int ctr)
{
	return (pmc_control(ctr, PMC_OP_STOP, NULL));
}

int
pmc_get_num_counters(void)
{
	int rv;

	if (pmc_get_info(-1, PMC_INFO_NCOUNTERS, &rv) == -1)
		return (0);

	return (rv);
}

int
pmc_get_counter_class(void)
{
	int rv;

	if (pmc_get_info(-1, PMC_INFO_CPUCTR_TYPE, &rv) == -1)
		return (0);

	return (rv);
}

int
pmc_get_counter_type(int ctr, int *typep)
{
	/* If they want class, they should call pmc_get_counter_class(). */
	if (ctr < 0) {
		errno = ENODEV;
		return (-1);
	}

	return (pmc_get_info(ctr, PMC_INFO_CPUCTR_TYPE, typep));
}

int
pmc_get_counter_value(int ctr, uint64_t *valp)
{
	return (pmc_get_info(ctr, PMC_INFO_COUNTER_VALUE, valp));
}

int
pmc_get_accumulated_counter_value(int ctr, uint64_t *valp)
{
	return (pmc_get_info(ctr, PMC_INFO_ACCUMULATED_COUNTER_VALUE, valp));
}

const char *
pmc_get_counter_class_name(int classval)
{
	const struct pmc_class2evid *class;

	class = _pmc_type_lookup(classval);
	if (class == NULL) {
		errno = ESRCH;
		return (NULL);
	}

	return (class->name);
}

const char *
pmc_get_counter_type_name(int type)
{
	const struct pmc_class2evid *class;

	class = _pmc_type_lookup(type);
	if (class == NULL) {
		errno = ESRCH;
		return (NULL);
	}

	return (class->name);
}

const char *
pmc_get_counter_event_name(pmc_evid_t event)
{
	const struct pmc_class2evid *class;
	const struct pmc_event *evid;
	int classval;

	if (pmc_get_info(-1, PMC_INFO_CPUCTR_TYPE, &classval) == -1) {
		/* Sets errno for us. */
		return (NULL);
	}

	if ((class = _pmc_type_lookup(classval)) == NULL) {
		errno = ESRCH;
		return (NULL);
	}

	for (evid = class->evids; evid->name != NULL; evid++) {
		if (evid->val == event)
			break;
	}
	if (evid->name == NULL) {
		errno = ESRCH;
		return (NULL);
	}

	return (evid->name);
}

const struct pmc_event *
pmc_get_counter_event_list(void)
{
	const struct pmc_class2evid *class;
	int classval;

	if (pmc_get_info(-1, PMC_INFO_CPUCTR_TYPE, &classval) == -1) {
		/* Sets errno for us. */
		return (NULL);
	}

	if ((class = _pmc_type_lookup(classval)) == NULL) {
		errno = ESRCH;
		return (NULL);
	}

	return (class->evids);
}
