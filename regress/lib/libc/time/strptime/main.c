#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int	main __P((int, char *[]));
void	die __P((void));

void
die()
{

	if (ferror(stdin))
		err(1, "fgetln");
	else
		errx(1, "input is truncated");
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *p, *title, *buf, *format;
	size_t len;
	struct tm tm;

	for (;;) {
		p = fgetln(stdin, &len);
		if (p == 0)
			die();
		title = malloc(len + 1);
		memcpy(title, p, len);
		title[len] = '\0';

		if (!strcmp(title, "EOF\n"))
			return(0);
		if (title[0] == '#' || title[0] == '\n') {
			free(title);
			continue;
		}

		p = fgetln(stdin, &len);
		if (p == 0)
			die();
		buf = malloc(len + 1);
		memcpy(buf, p, len);
		buf[len] = '\0';

		p = fgetln(stdin, &len);
		if (p == 0)
			die();
		format = malloc(len + 1);
		memcpy(format, p, len);
		format[len] = '\0';

		tm.tm_sec = -1;
		tm.tm_min = -1;
		tm.tm_hour = -1;
		tm.tm_mday = -1;
		tm.tm_mon = -1;
		tm.tm_year = -1;
		tm.tm_wday = -1;
		tm.tm_yday = -1;

		p = strptime(buf, format, &tm);

		printf("%s", title);
		if (p) {
			printf("succeeded\n");
			printf("%d %d %d %d %d %d %d %d\n",
			    tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday,
			    tm.tm_mon, tm.tm_year, tm.tm_wday, tm.tm_yday);
			printf("%s\n", p);
		} else {
			printf("failed\n");
		}

		free(title);
		free(buf);
		free(format);
	}
}
