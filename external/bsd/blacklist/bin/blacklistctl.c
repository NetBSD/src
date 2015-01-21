
#include <stdio.h>
#include <time.h>
#include <util.h>
#include <fcntl.h>
#include <db.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "conf.h"
#include "state.h"
#include "internal.h"
#include "util.h"

static __dead void
usage(int c)
{
	warnx("Unknown option `%c'", (char)c);
	fprintf(stderr, "Usage: %s [-d]\n", getprogname());
	exit(EXIT_FAILURE);
}
int
main(int argc, char *argv[])
{
	const char *dbname = _PATH_BLSTATE;
	DB *db;
	struct conf c;
	struct sockaddr_storage ss;
	struct dbinfo dbi;
	unsigned int i;
	int o;

	while ((o = getopt(argc, argv, "d")) != -1)
		switch (o) {
		case 'd':
			debug++;
			break;
		default:
			usage(o);
			break;
		}

	db = state_open(dbname, O_RDONLY, 0);
	if (db == NULL)
		err(EXIT_FAILURE, "Can't open `%s'", dbname);

	for (i = 1; state_iterate(db, &ss, &c, &dbi, i) != 0; i = 0) {
		char buf[BUFSIZ];
		printf("conf: %s\n", conf_print(buf, sizeof(buf), "",
		    ":", &c));
		sockaddr_snprintf(buf, sizeof(buf), "%a", (void *)&ss);
		printf("addr: %s\n", buf);
		printf("data: count=%d id=%s time=%s\n", dbi.count,
		    dbi.id, fmttime(buf, sizeof(buf), dbi.last));
	}
	state_close(db);
	return EXIT_SUCCESS;
}
