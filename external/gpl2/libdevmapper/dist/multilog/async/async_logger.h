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

#ifndef LIB_CIRCBUF_H
#define LIB_CIRCBUF_H

void write_to_buf(int priority, const char *file, int line,
		  const char *string);
int start_syslog_thread(pthread_t *thread, long usecs);
int stop_syslog_thread(pthread_t thread);

#endif
