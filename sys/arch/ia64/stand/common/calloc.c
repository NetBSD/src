/*	$NetBSD: calloc.c,v 1.1.6.2 2006/04/22 11:37:38 simonb Exp $	*/

#include <sys/cdefs.h>
#include <sys/types.h>

#include <lib/libsa/stand.h>

void *
calloc(u_int size1, u_int size2)
{
	u_int total_size = size1 * size2;
	void *ptr;

	if(( (ptr = alloc(total_size)) != NULL)) {
		bzero(ptr, total_size);
	}
	
	/* alloc will crib for me. */

	return(ptr);
}
