/*	$NetBSD: bt478var.h,v 1.3 1999/04/24 08:01:02 simonb Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

/*
 * External declarations exported from the bt478 low-level
 * chipset driver.
 */

int bt478init __P((struct fbinfo *fi));
void bt478BlankCursor __P((struct fbinfo *fi));
void bt478RestoreCursorColor __P((struct fbinfo *fi));
void bt478InitColorMap __P((struct fbinfo *fi));
int bt478LoadColorMap __P ((struct fbinfo *fi, caddr_t bits,
			    int index, int count));
int bt478GetColorMap __P ((struct fbinfo *fi, caddr_t bits,
			   int index, int count));
