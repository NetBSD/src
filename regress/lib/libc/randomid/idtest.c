/* $NetBSD: idtest.c,v 1.2 2003/11/25 23:14:48 itojun Exp $ */

/* If defined, abort at first short period and only test REGRESS times. */
#define	REGRESS		10000000		/* should be enough... */

#include <sys/types.h>

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <randomid.h>

#define	PERIOD		12000

uint64_t last[65536];

uint64_t n = 0;

main()
{
	static randomid_t ctx = NULL;
	uint64_t lowest;
	uint16_t id;
	int i;

	memset(last, 0, sizeof(last));
	ctx = randomid_new(16, (long)3600);

	lowest = 0xffffffffffffffffULL;
	while (n < ULLONG_MAX) {
		id = randomid(ctx);
		if (last[id] > 0) {
			if (n - last[id] <= lowest) {
				if (lowest != 0xffffffffffffffffULL) {
					printf("id %5d  "
					    "last call for id at %9lld, "
					    "current call %9lld (diff %5lld)\n",
					    id, last[id], n, n - last[id]);
#ifdef REGRESS
					assert(lowest >= REGRESS);
#endif
				}
				lowest = n - last[id];
			}
		}
		last[id] = n;
		n++;
#ifdef REGRESS
		if (n > REGRESS)
			break;
#endif
	}
	exit(0);
}

