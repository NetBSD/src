/*
 * Copyright (C) 2006 Rackable Systems All rights reserved.  
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LVM_TIMESTAMP_H
#define _LVM_TIMESTAMP_H

struct timestamp;

struct timestamp *get_timestamp(void);

/* cmp_timestamp: Compare two timestamps
 * 
 * Return: -1 if t1 is less than t2
 *  	    0 if t1 is equal to t2
 *          1 if t1 is greater than t2
 */
int cmp_timestamp(struct timestamp *t1, struct timestamp *t2);

void destroy_timestamp(struct timestamp *t);

#endif /* _LVM_TIMESTAMP_H */

