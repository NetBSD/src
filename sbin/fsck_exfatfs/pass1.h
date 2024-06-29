/* $NetBSD: pass1.h,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $	 */

#include <sys/types.h>

struct exfatfs;
struct dup;

void pass1(struct exfatfs *, struct dup *, uint8_t *);
