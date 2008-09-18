/*
 * Copyright (C) 2005-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "pthread_lock.h"

void *nop(void *data)
{
	pthread_exit(NULL);
}

void *init_locking(void)
{
	pthread_t thread;
	pthread_mutex_t *mutex;
	if(!(mutex = malloc(sizeof(*mutex))))
		return NULL;
	memset(mutex, 0, sizeof(*mutex));

	/*
	 * Hack to ensure DSO loading fails if not linked with
	 * libpthread
	 */
	pthread_create(&thread, NULL, nop, NULL);

	return mutex;
}

int lock_fn(void *handle)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *)handle;
	if(mutex)
		return pthread_mutex_lock(mutex);
	else
		return 0;
}


int unlock_fn(void *handle)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *)handle;
	if(mutex)
		return pthread_mutex_unlock(mutex);
	else
		return 0;
}


void destroy_locking(void *handle)
{
        pthread_mutex_t *mutex = (pthread_mutex_t *)handle;
	free(mutex);
}

