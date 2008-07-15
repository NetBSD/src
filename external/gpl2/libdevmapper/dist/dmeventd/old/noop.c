/*
 * Copyright (C) 2005 Red Hat, Inc. All rights reserved.
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

#include "libdm-event.h"
#include "libmultilog.h"


void process_event(char *device, enum event_type event)
{
	log_err("[%s] %s(%d) - Device: %s, Event %d\n",
		__FILE__, __func__, __LINE__, device, event);
}

int register_device(char *device)
{
	log_err("[%s] %s(%d) - Device: %s\n",
		__FILE__, __func__, __LINE__, device);

	return 1;
}

int unregister_device(char *device)
{
	log_err("[%s] %s(%d) - Device: %s\n",
		__FILE__, __func__, __LINE__, device);

	return 1;
}
