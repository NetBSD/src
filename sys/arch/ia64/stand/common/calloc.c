/*	$NetBSD: calloc.c,v 1.2 2009/03/18 16:00:12 cegger Exp $	*/

#include <sys/cdefs.h>
#include <sys/types.h>

#include <lib/libsa/stand.h>

void *
calloc(u_int size1, u_int size2)
{
	u_int total_size = size1 * size2;
	void *ptr;

	if(( (ptr = alloc(total_size)) != NULL)) {
		memset(ptr, 0, total_size);
	}
	
	/* alloc will crib for me. */

	return(ptr);
}
