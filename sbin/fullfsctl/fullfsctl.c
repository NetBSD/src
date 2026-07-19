/*	$NetBSD$	*/

#include <sys/cdefs.h>
__RCSID("$NetBSD$");

#include <sys/ioctl.h>

#include <miscfs/fullfs/full_ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

static void usage(void) __dead;

static const struct {
	const char	*name;
	int			value;
} modes[] = {
	{ "pass",	FULLFS_MODE_PASS },
	{ "fail",	FULLFS_MODE_FAIL },
	{ "count",	FULLFS_MODE_COUNT },
	{ "bytes",	FULLFS_MODE_BYTES },
	{ "random",	FULLFS_MODE_RANDOM },
};

static const struct {
	const char	*name;
	int			value;
} errnos[] = {
	{ "enospc",	ENOSPC },
	{ "eio",	EIO },
	{ "edquot",	EDQUOT },
	{ "efbig",	EFBIG },
	{ "eperm",	EPERM },
	{ "eacces",	EACCES },
	{ "erofs",	EROFS },
};

static const struct {
	const char		*name;
	int				value;
} ops[] = {
	{ "write",		FULLFS_OP_WRITE },
	{ "create",		FULLFS_OP_CREATE },
	{ "mkdir",		FULLFS_OP_MKDIR },
	{ "mknod",		FULLFS_OP_MKNOD },
	{ "symlink",	FULLFS_OP_SYMLINK },
	{ "link",		FULLFS_OP_LINK },
	{ "all",		FULLFS_OP_ALL },
	{ "none",		FULLFS_OP_NONE },
};

static int
parse_mode(const char *s)
{
	for (size_t i = 0; i < __arraycount(modes); i++)
		if (strcasecmp(s, modes[i].name) == 0)
			return modes[i].value;

	errx(EXIT_FAILURE, "unknown mode: %s", s);
}

static const char *
mode_name(int mode)
{
	for (size_t i = 0; i < __arraycount(modes); i++)
		if (modes[i].value == mode)
			return modes[i].name;

	return "unknown";
}

static int
parse_error(const char *s)
{
	for (size_t i = 0; i < __arraycount(errnos); i++)
		if (strcasecmp(s, errnos[i].name) == 0)
			return errnos[i].value;

	return (int)estrtoi(s, 10, 1, INT_MAX);
}

static int
parse_opmask(const char *s)
{
	int mask;
	char *copy, *token, *last;

	copy = estrdup(s);

	mask = 0;
	for (token = strtok_r(copy, ",", &last); token != NULL;
	    token = strtok_r(NULL, ",", &last)) {
		int found = 0;
		for (size_t i = 0; i < __arraycount(ops); i++) {
			if (strcasecmp(token, ops[i].name) == 0) {
				mask |= ops[i].value;
				found = 1;
				break;
			}
		}
		if (!found)
			errx(EXIT_FAILURE, "unknown op: %s", token);
	}

	free(copy);
	return mask;
}

static unsigned int
parse_doom(const char *s)
{
	return (unsigned int)estrtou(s, 10, 0, UINT_MAX);
}

static unsigned int
parse_rate(const char *s)
{
	return (unsigned int)estrtou(s, 10, 0, UINT_MAX);
}

static void
show_state(int fd)
{
	struct fullfs_ctl fc;

	if (ioctl(fd, FULLFS_GETSTATE, &fc) == -1)
		err(EXIT_FAILURE, "FULLFS_GETSTATE");

	printf("mode=%s error=%d opmask=0x%x doom=%u rate=%u\n",
			mode_name(fc.fc_mode), fc.fc_error, fc.fc_opmask,
			fc.fc_doom, fc.fc_rate);
}

int
main(int argc, char **argv)
{
	int ch, fd, mflag, sflag;
	const char *marg, *earg, *oarg, *darg, *rarg;
	struct fullfs_ctl fc;

	mflag = sflag = 0;
	marg = earg = oarg = darg = rarg = NULL;
	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "d:e:m:o:r:s")) != -1) {
		switch (ch) {
		case 'd':
			darg = optarg;
			break;
		case 'e':
			earg = optarg;
			break;
		case 'm':
			mflag = 1;
			marg = optarg;
			break;
		case 'o':
			oarg = optarg;
			break;
		case 'r':
			rarg = optarg;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (mflag && sflag)
		usage();

	if (!mflag && !sflag)
		usage();

	if (!mflag && (earg || oarg || darg || rarg))
		usage();

	fd = open(argv[0], O_RDONLY);
	if (fd == -1)
		err(EXIT_FAILURE, "%s", argv[0]);

	if (sflag) {
		show_state(fd);
		close(fd);
		return 0;
	}

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = parse_mode(marg);
	fc.fc_error = earg ? parse_error(earg) : ENOSPC;
	fc.fc_opmask = oarg ? parse_opmask(oarg) : FULLFS_OP_ALL;
	fc.fc_doom = darg ? parse_doom(darg) : 0;
	fc.fc_rate = rarg ? parse_rate(rarg) : 0;

	if (ioctl(fd, FULLFS_SETSTATE, &fc) == -1)
		err(EXIT_FAILURE, "FULLFS_SETSTATE");

	close(fd);
	return 0;
}

static void
usage(void)
{
	fprintf(stderr,
		"usage: %s -m mode [-e errno] [-o ops] [-d doom] [-r rate]"
		" mountpoint\n"
		"       %s -s mountpoint\n",
		getprogname(), getprogname());
	exit(EXIT_FAILURE);
}
