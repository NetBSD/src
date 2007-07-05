/* $NetBSD: sysmon_envsysvar.h,v 1.3 2007/07/05 23:48:22 xtraeme Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines.
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
 *      This product includes software developed by Juan Romero Pardines
 *      for the NetBSD Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DEV_SYSMON_ENVSYSVAR_H_
#define _DEV_SYSMON_ENVSYSVAR_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/workqueue.h>

#include <dev/sysmon/sysmonvar.h>
#include <prop/proplib.h>

#ifdef ENVSYS_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

#ifdef ENVSYS_OBJECTS_DEBUG
#define DPRINTFOBJ(x)	printf x
#else
#define DPRINTFOBJ(x)
#endif

/* convenience macros to avoid writing same code many times */
#define SENSOR_OBJUPDATED(a, b)						\
do {									\
	DPRINTFOBJ(("%s: obj (%s:%d) updated\n", __func__, (a), (b)));	\
} while (/* CONSTCOND */ 0)

#define SENSOR_DICTSETFAILED(a, b)					\
do {									\
	DPRINTF(("%s: dict_set (%s:%d) failed\n", __func__, (a), (b)));	\
} while (/* CONSTCOND */ 0)

#define SENSOR_SETTYPE(a, b, c, d)					\
do {									\
	if (!prop_dictionary_set_ ## d((a), (b), (c))) {		\
		SENSOR_DICTSETFAILED((b), (c));				\
		if ((a)) 						\
			prop_object_release((a));			\
		goto out;						\
	}								\
} while (/* CONSTCOND */ 0)

#define SENSOR_SINT32(a, b, c)	SENSOR_SETTYPE(a, b, c, int32)
#define SENSOR_SUINT32(a, b, c)	SENSOR_SETTYPE(a, b, c, uint32)
#define SENSOR_SBOOL(a, b, c)	SENSOR_SETTYPE(a, b, c, bool)
#define SENSOR_SSTRING(a, b, c)						\
do {									\
	if (!prop_dictionary_set_cstring_nocopy((a), (b), (c))) {	\
		DPRINTF(("%s: set_cstring (%s) failed.\n",		\
		    __func__, (c)));					\
		if ((a))						\
			prop_object_release((a));			\
		goto out;						\
	}								\
} while (/* CONSTCOND */ 0)

#define SENSOR_UPTYPE(a, b, c, d, e)					\
do {									\
	obj = prop_dictionary_get((a), (b));				\
	if (!prop_number_equals_ ## e(obj, (c))) {			\
		if (!prop_dictionary_set_ ## d((a), (b), (c))) { 	\
			SENSOR_DICTSETFAILED((b), (c));			\
			return EINVAL;					\
		}							\
		SENSOR_OBJUPDATED((b), (c));				\
	}								\
} while (/* CONSTCOND */ 0)

#define SENSOR_UPINT32(a, b, c)		\
	SENSOR_UPTYPE(a, b, c, int32, integer)
#define SENSOR_UPUINT32(a, b, c)	\
	SENSOR_UPTYPE(a, b, c, uint32, unsigned_integer)
#define SENSOR_UPSTRING(a, b, c)					\
do {									\
	obj = prop_dictionary_get((a), (b));				\
	if (obj == NULL) {						\
		SENSOR_SSTRING((a), (b), (c));				\
	} else {							\
		if (!prop_string_equals_cstring((obj), (c))) {		\
			SENSOR_SSTRING((a), (b), (c));			\
		}							\
	}								\
} while (/* CONSTCOND */ 0)

/* struct used by a sysmon envsys event */
typedef struct sme_event {
	/* to add works into our workqueue */
	union {
		struct work u_work;
		TAILQ_ENTRY(sme_event) u_q;
	} see_u;
#define see_wk	see_u.u_work
#define see_q	see_u.u_q
	LIST_ENTRY(sme_event)	see_list;
	struct penvsys_state	pes;		/* our power envsys */
	int32_t			critval;	/* critical value set */
	int			type;		/* type of the event */
	int			snum;		/* sensor number */
	int			evsent;		/* event already sent */
} sme_event_t;

/* struct by a sysmon envsys event set by a driver */
typedef struct sme_event_drv {
	struct sysmon_envsys	*sme;
	prop_dictionary_t	sdict;
	envsys_data_t		*edata;
	int			powertype;
} sme_event_drv_t;

/* common */
extern	kmutex_t sme_mtx;	/* mutex for the sysmon envsys devices */
extern	kmutex_t sme_event_mtx;	/* mutex for the sysmon envsys events */
extern	kmutex_t sme_event_init_mtx;	/* mutex to initialize/destroy see */

/* linked list for the sysmon envsys devices */
LIST_HEAD(, sysmon_envsys) sysmon_envsys_list;

/* linked list for the sysmon envsys events */
LIST_HEAD(, sme_event) sme_events_list;

/* functions to handle sysmon envsys devices */
int	sysmon_envsys_createplist(struct sysmon_envsys *);
int	sme_make_dictionary(struct sysmon_envsys *, prop_array_t,
			    envsys_data_t *);
int	sme_update_dictionary(struct sysmon_envsys *);
int	sme_userset_dictionary(struct sysmon_envsys *,
			       prop_dictionary_t, prop_array_t);

/* functions to handle sysmon envsys events */
int	sme_event_register(struct sme_event *);
int	sme_event_unregister(const char *, int);
void	sme_event_drvadd(void *);
int	sme_event_add(prop_dictionary_t, envsys_data_t *,
		      const char *, const char *, int32_t, int, int);
int	sme_events_init(void);
void	sme_events_check(void *);
void	sme_events_worker(struct work *, void *);

#endif /* _DEV_SYSMON_ENVSYSVAR_H_ */
