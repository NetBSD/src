/* $NetBSD: pass2.h,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $	 */

#include <sys/types.h>

struct exfatfs;

void pass2(struct exfatfs *, uint8_t *);
