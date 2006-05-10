/* $NetBSD: idtest.c,v 1.5 2006/05/10 19:07:22 mrg Exp $ */

/* If defined, abort at first short period and only test REGRESS times. */
#define	REGRESS		10000000		/* should be enough... */

#include <sys/types.h>

#include <assert.h>
#include <randomid.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define	PERIOD		30000

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

	lowest = UINT64_MAX;
	while (n < UINT64_MAX) {
		id = randomid(ctx);
		if (last[id] > 0) {
			if (n - last[id] <= lowest) {
				if (lowest != UINT64_MAX) {
					printf("id %5d  "
					    "last call for id at %9lld, "
					    "current call %9lld (diff %5lld)\n",
					    id, last[id], n, n - last[id]);
#ifdef REGRESS
					if (n - last[id] < PERIOD) {
						printf("diff (%"PRIu64") less "
						    "than minimum period "
						    "(%d)\n", n - last[id],
						    PERIOD);
						abort();
					}
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
