#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>
#include <usb.h>
#include <util.h>

int verbose = 0;

struct command {
	struct command *next;
	int line;

	struct hid_item item;
	int value;
	char anyvalue;
	char *name;
	char *action;
};
struct command *commands;

#define SIZE 4000

void usage(void);
void parse_conf(const char *conf, report_desc_t repd, int ignore);
void docmd(struct command *cmd, int value, const char *hid,
	   int argc, char **argv);

int
main(int argc, char **argv)
{
	const char *conf = NULL;
	const char *hid = NULL;
	int fd, ch, sz, n, val;
	int demon, ignore;
	report_desc_t repd;
	char buf[100];
	struct command *cmd;

	demon = 1;
	ignore = 0;
	while ((ch = getopt(argc, argv, "c:dif:v")) != -1) {
		switch(ch) {
		case 'c':
			conf = optarg;
			break;
		case 'd':
			demon ^= 1;
			break;
		case 'i':
			ignore++;
			break;
		case 'f':
			hid = optarg;
			break;
		case 'v':
			demon = 0;
			verbose++;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (conf == NULL || hid == NULL)
		usage();

	hid_init(NULL);

	fd = open(hid, O_RDWR);
	if (fd < 0)
		err(1, "%s", hid);
	repd = hid_get_report_desc(fd);
	if (repd == NULL)
		err(1, "hid_get_report_desc() failed\n");

	parse_conf(conf, repd, ignore);

	sz = hid_report_size(repd, hid_input, NULL);
	hid_dispose_report_desc(repd);

	if (verbose)
		printf("report size %d\n", sz);
	if (sz > sizeof buf)
		errx(1, "report too large");

	if (demon) {
		if (daemon(0, 0) < 0)
			err(1, "daemon()");
		pidfile(NULL);
	}

	for(;;) {
		n = read(fd, buf, sz);
		if (verbose > 2)
			printf("read %d bytes [%02x]\n", n, buf[0]);
		if (n < 0) {
			if (verbose)
				err(1, "read");
			else
				exit(1);
		}
		for (cmd = commands; cmd; cmd = cmd->next) {
			val = hid_get_data(buf, &cmd->item);
			if (cmd->value == val || cmd->anyvalue)
				docmd(cmd, val, hid, argc, argv);
		}
	}

	exit(0);
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "Usage: %s -c config_file [-d] -f hid_dev "
	    " [-v]\n", __progname);
	exit(1);
}

static int
peek(FILE *f)
{
	int c;

	c = getc(f);
	if (c != EOF)
		ungetc(c, f);
	return c;
}

void
parse_conf(const char *conf, report_desc_t repd, int ignore)
{
	FILE *f;
	char *p;
	int line;
	char buf[SIZE], name[SIZE], value[SIZE], action[SIZE];
	char usage[SIZE], coll[SIZE];
	struct command *cmd;
	struct hid_data *d;
	struct hid_item h;

	f = fopen(conf, "r");
	if (f == NULL)
		err(1, "%s", conf);

	for (line = 1; ; line++) {
		if (fgets(buf, sizeof buf, f) == NULL)
			break;
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		p = strchr(buf, '\n');
		while (p && isspace(peek(f))) {
			if (fgets(p, sizeof buf - strlen(buf), f) == NULL)
				break;
			p = strchr(buf, '\n');
		}
		if (p)
			*p = 0;
		if (sscanf(buf, "%s %s %[^\n]", name, value, action) != 3) {
			errx(1, "config file `%s', line %d, syntax error: %s",
			     conf, line, buf);
		}

		cmd = malloc(sizeof *cmd);
		if (cmd == NULL)
			err(1, "malloc failed");
		cmd->next = commands;
		commands = cmd;
		cmd->line = line;

		if (strcmp(value, "*") == 0) {
			cmd->anyvalue = 1;
		} else {
			cmd->anyvalue = 0;
			if (sscanf(value, "%d", &cmd->value) != 1)
				errx(1, "config file `%s', line %d, "
				     "bad value: %s\n", conf, line, value);
		}

		coll[0] = 0;
		for (d = hid_start_parse(repd, 1 << hid_input);
		     hid_get_item(d, &h); ) {
			if (verbose > 2)
				printf("kind=%d usage=%x\n", h.kind, h.usage);
			if (h.flags & HIO_CONST)
				continue;
			switch (h.kind) {
			case hid_input:
				snprintf(usage, sizeof usage,  "%s:%s",
					 hid_usage_page(HID_PAGE(h.usage)), 
					 hid_usage_in_page(h.usage));
				if (verbose > 2)
					printf("usage %s\n", usage);
				if (strcasecmp(usage, name) == 0)
					goto foundhid;
				if (coll[0]) {
					snprintf(usage, sizeof usage,
					    "%s.%s:%s", coll+1,
					    hid_usage_page(HID_PAGE(h.usage)), 
					    hid_usage_in_page(h.usage));
					if (verbose > 2)
						printf("usage %s\n", usage);
					if (strcasecmp(usage, name) == 0)
						goto foundhid;
				}
				break;
			case hid_collection:
				snprintf(coll + strlen(coll),
				    sizeof coll - strlen(coll),  ".%s:%s",
				    hid_usage_page(HID_PAGE(h.usage)), 
				    hid_usage_in_page(h.usage));
				break;
			case hid_endcollection:
				if (coll[0])
					*strrchr(coll, '.') = 0;
				break;
			default:
				break;
			}
		}
		if (ignore) {
			if (verbose)
				warnx("ignore item '%s'\n", name);
			continue;
		}
		errx(1, "config file `%s', line %d, HID item not found: `%s'\n",
		     conf, line, name);

	foundhid:
		hid_end_parse(d);
		cmd->item = h;
		cmd->name = strdup(name);
		cmd->action = strdup(action);

		if (verbose)
			printf("PARSE:%d %s, %d, '%s'\n", cmd->line, name,
			       cmd->value, cmd->action);
	}
	fclose(f);
}

void
docmd(struct command *cmd, int value, const char *hid, int argc, char **argv)
{
	char cmdbuf[SIZE], *p, *q;
	size_t len;
	int n, r;

	for (p = cmd->action, q = cmdbuf; *p && q < &cmdbuf[SIZE-1]; ) {
		if (*p == '$') {
			p++;
			len = &cmdbuf[SIZE-1] - q;
			if (isdigit(*p)) {
				n = strtol(p, &p, 10) - 1;
				if (n >= 0 && n < argc) {
					strncpy(q, argv[n], len);
					q += strlen(q);
				}
			} else if (*p == 'V') {
				p++;
				snprintf(q, len, "%d", value);
				q += strlen(q);
			} else if (*p == 'N') {
				p++;
				strncpy(q, cmd->name, len);
				q += strlen(q);
			} else if (*p == 'H') {
				p++;
				strncpy(q, hid, len);
				q += strlen(q);
			} else if (*p) {
				*q++ = *p++;
			}
		} else {
			*q++ = *p++;
		}
	}
	*q = 0;

	if (verbose)
		printf("system '%s'\n", cmdbuf);
	r = system(cmdbuf);
	if (verbose > 1 && r)
		printf("return code = 0x%x\n", r);
}
