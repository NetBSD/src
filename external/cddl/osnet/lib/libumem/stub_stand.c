/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident"%Z%%M%%I%%E% SMI"

/*
 * Stubs for the standalone to reduce the dependence on external libraries
 */

#include <string.h>
#include "misc.h"

/*ARGSUSED*/
int
mutex_init(mutex_t *mp, int type, void *arg)
{

	return (0);
}

/*ARGSUSED*/
int
mutex_destroy(mutex_t *mp)
{
	return (0);
}

/*ARGSUSED*/
int
_mutex_held(mutex_t *mp)
{
	return (1);
}

int
mutex_owned(mutex_t *mp)
{
	return (1);
}

/*ARGSUSED*/
int
mutex_lock(mutex_t *mp)
{
	return (0);
}

/*ARGSUSED*/
int
mutex_trylock(mutex_t *mp)
{
	return (0);
}

/*ARGSUSED*/
int
mutex_unlock(mutex_t *mp)
{
	return (0);
}

int
issetugid(void)
{
	return (1);
}
