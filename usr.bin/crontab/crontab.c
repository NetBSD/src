#ifndef lint
static char rcsid[] = "$Id: crontab.c,v 1.5 1993/11/10 12:35:06 cgd Exp $";
#endif /* not lint */

#define	MAIN_PROGRAM


#include "cron.h"
#include <pwd.h>
#include <errno.h>
#include <sys/file.h>
#if defined(BSD)
# include <sys/time.h>
#endif  /*BSD*/


static int	Pid;
static char	User[MAX_UNAME], RealUser[MAX_UNAME];
static char	Filename[MAX_FNAME];
static FILE	*NewCrontab;
static int	CheckErrorCount;
static enum	{opt_unknown, opt_list, opt_delete, opt_edit, opt_replace}
		Option;

extern char		*getenv(), *strcpy();
extern int		getuid();
extern struct passwd	*getpwnam();
extern int		getopt(), optind;
extern char		*optarg;
extern void		log_it();	/* misc.c */

#if DEBUGGING
static char	*Options[] = {"???", "list", "delete", "edit", "replace"};
#endif

static void
usage()
{
	fprintf(stderr, "usage:  %s [-u user] ...\n", ProgramName);
	fprintf(stderr, " ... -l         (list user's crontab)\n");
	fprintf(stderr, " ... -d         (delete user's crontab)\n");
	fprintf(stderr, " ... -e         (edit user's crontab)\n");
	fprintf(stderr, " ... -r file    (replace user's crontab)\n");
	exit(ERROR_EXIT);
}


main(argc, argv)
	int	argc;
	char	*argv[];
{
	void	parse_args(), set_cron_uid(), set_cron_cwd(),
		list_cmd(), delete_cmd(), edit_cmd(), replace_cmd();

	Pid = getpid();
	ProgramName = argv[0];
#if defined(BSD)
	setlinebuf(stderr);
#endif
	parse_args(argc, argv);		/* sets many globals, opens a file */
	set_cron_uid();
	set_cron_cwd();
	if (!allowed(User)) {
		fprintf(stderr,
			"You (%s) are not allowed to use this program (%s)\n",
			User, ProgramName);
		fprintf(stderr, "See crontab(1) for more information\n");
		log_it(RealUser, Pid, "AUTH", "crontab command not allowed");
		exit(ERROR_EXIT);
	}
	switch (Option)
	{
	case opt_list:		list_cmd();
				break;
	case opt_delete:	delete_cmd();
				break;
	case opt_edit:		edit_cmd();
				break;
	case opt_replace:	replace_cmd();
				break;
	}
}
	

static void
parse_args(argc, argv)
	int	argc;
	char	*argv[];
{
	void		usage();

	struct passwd	*pw;
	int		argch;

	if (!(pw = getpwuid(getuid())))
	{
		fprintf(stderr, "%s: your UID isn't in the passwd file.\n",
			ProgramName);
		fprintf(stderr, "bailing out.\n");
		exit(ERROR_EXIT);
	}
	strcpy(User, pw->pw_name);
	strcpy(RealUser, User);
	Filename[0] = '\0';
	Option = opt_unknown;
	while (EOF != (argch = getopt(argc, argv, "u:lder:x:")))
	{
		switch (argch)
		{
		case 'x':
			if (!set_debug_flags(optarg))
				usage();
			break;
		case 'u':
			if (getuid() != ROOT_UID)
			{
				fprintf(stderr,
					"must be privileged to use -u\n");
				exit(ERROR_EXIT);
			}
			if ((struct passwd *)NULL == getpwnam(optarg))
			{
				fprintf(stderr, "%s:  user `%s' unknown\n",
					ProgramName, optarg);
				exit(ERROR_EXIT);
			}
			(void) strcpy(User, optarg);
			break;
		case 'l':
			if (Option != opt_unknown)
				usage();
			Option = opt_list;
			break;
		case 'd':
			if (Option != opt_unknown)
				usage();
			Option = opt_delete;
			break;
		case 'e':
			if (Option != opt_unknown)
				usage();
			Option = opt_edit;
			break;
		case 'r':
			if (Option != opt_unknown)
				usage();
			Option = opt_replace;
			(void) strcpy(Filename, optarg);
			break;
		default:
			usage();
		}
	}

	endpwent();

	if (Option == opt_unknown || argv[optind] != NULL)
		usage();

	if (Option == opt_replace) {
		if (!Filename[0]) {
			/* getopt(3) says this can't be true
			 * but I'm feeling extra paranoid today.
			 */
			fprintf(stderr, "filename must be given for -r\n");
			usage();
		}
		/* we have to open the file here because we're going to
		 * chdir(2) into /var/cron before we get around to
		 * reading the file.
		 */
		if (!strcmp(Filename, "-")) {
			NewCrontab = stdin;
		} else {
			if (!(NewCrontab = fopen(Filename, "r"))) {
				perror(Filename);
				exit(ERROR_EXIT);
			}
		}
	}

	Debug(DMISC, ("user=%s, file=%s, option=%s\n",
					User, Filename, Options[(int)Option]))
}


static void
list_cmd()
{
	char	n[MAX_FNAME];
	FILE	*f;
	int	ch;

	log_it(RealUser, Pid, "LIST", User);
	(void) sprintf(n, CRON_TAB(User));
	if (!(f = fopen(n, "r")))
	{
		if (errno == ENOENT)
			fprintf(stderr, "no crontab for %s\n", User);
		else
			perror(n);
		exit(ERROR_EXIT);
	}

	/* file is open. copy to stdout, close.
	 */
	Set_LineNum(1)
	while (EOF != (ch = get_char(f)))
		putchar(ch);
	fclose(f);
}


static void
delete_cmd()
{
	extern	errno;
	int	unlink();
	void	poke_daemon();
	char	n[MAX_FNAME];

	log_it(RealUser, Pid, "DELETE", User);
	(void) sprintf(n, CRON_TAB(User));
	if (unlink(n))
	{
		if (errno == ENOENT)
			fprintf(stderr, "no crontab for %s\n", User);
		else
			perror(n);
		exit(ERROR_EXIT);
	}
	poke_daemon();
}


static void
check_error(msg)
	char	*msg;
{
	CheckErrorCount += 1;
	fprintf(stderr, "\"%s\", line %d: %s\n", Filename, LineNumber, msg);
}


static void
edit_cmd()
{
	char		n[MAX_FNAME], tn[MAX_FNAME], *editor;
	FILE		*f, *tf;
	int		ch, t, pid;
	struct stat	before, after;

	log_it(RealUser, Pid, "BEGIN EDIT", User);
	(void) sprintf(n, CRON_TAB(User));
	if (!(f = fopen(n, "r")))
	{
		if (errno == ENOENT)
			fprintf(stderr, "no crontab for %s\n", User);
		else
			perror(n);
		exit(ERROR_EXIT);
	}

	(void) sprintf(tn, "/tmp/crontab.%d", getpid());
	if (-1 == (t = open(tn, O_CREAT|O_RDWR, 0600))) {
		perror(tn);
		exit(ERROR_EXIT);
	}
	if (fchown(t, getuid(), -1) < OK)
	{
		perror("chown");
		goto fatal;
	}
	if (!(tf = fdopen(t, "r+"))) {
		perror("fdopen");
		goto fatal;
	}

	/* copy the crontab to the temp file.  skip top two comments since we
	 * added them during the 'replace'.
	 */
	Set_LineNum(1)
	t = 0;
	while (EOF != (ch = get_char(f))) {
		if (t < 2) {
			t += (ch == '\n');
		} else {
			putc(ch, tf);
		}
	}
	fflush(tf);  rewind(tf);
	fclose(f);
	if (ferror(tf)) {
		fprintf(stderr, "%s: error while writing new crontab to %s\n",
			ProgramName, tn);
		fclose(tf);
fatal:
		unlink(tn);
		exit(ERROR_EXIT);
	}

	if ((!(editor = getenv("VISUAL")))
	 && (!(editor = getenv("EDITOR")))
	    ) {
		editor = EDITOR;
	}

	(void) fstat(fileno(tf), &before);
	fclose(tf);

	pid = VFORK();
	if (pid < 0) {
		perror("fork");
		goto fatal;
	}
	if (pid == 0) {
		char *argv[3];

		/* CHILD */
		setuid(getuid());	/* give up euid which is root */
		argv[0] = editor;
		argv[1] = tn;
		argv[2] = NULL;
		fprintf(stderr, "[%s %s]\n", editor, tn);
		execvp(editor, argv);
		perror(editor);
		exit(ERROR_EXIT);
	}
	(void) wait((int *)0);

	if (!(tf = fopen(tn, "r"))) {
		perror(tn);
		goto fatal;
	}
	(void) fstat(fileno(tf), &after);
	if (after.st_mtime == before.st_mtime) {
		fprintf(stderr, "[no changes were made to the crontab]\n");
		fclose(tf);
	} else {
		fprintf(stderr, "[new crontab now being installed]\n");
		NewCrontab = tf;
		replace_cmd();		/* closes NewCrontab */
	}
	unlink(tn);
}
	

static void
replace_cmd()
{
	entry	*load_entry();
	int	load_env();
	int	unlink();
	void	free_entry();
	void	check_error();
	void	poke_daemon();
	extern	errno;

	char	n[MAX_FNAME], envstr[MAX_ENVSTR], tn[MAX_FNAME];
	FILE	*tmp;
	int	ch;
	entry	*e;
	int	status;
	time_t	now = time(NULL);

	(void) sprintf(n, "tmp.%d", Pid);
	(void) sprintf(tn, CRON_TAB(n));
	if (!(tmp = fopen(tn, "w"))) {
		perror(tn);
		exit(ERROR_EXIT);
	}

	/* write a signature at the top of the file.  for brian.
	 */
	fprintf(tmp, "# (%s installed on %-24.24s)\n", Filename, ctime(&now));
	fprintf(tmp, "# (Cron version -- %s)\n", rcsid);

	/* copy the crontab to the tmp
	 */
	Set_LineNum(1)
	while (EOF != (ch = get_char(NewCrontab)))
		putc(ch, tmp);
	fclose(NewCrontab);
	fflush(tmp);  rewind(tmp);

	if (ferror(tmp)) {
		fprintf(stderr, "%s: error while writing new crontab to %s\n",
			ProgramName, tn);
		fclose(tmp);  unlink(tn);
		exit(ERROR_EXIT);
	}

	/* check the syntax of the file being installed.
	 */

	/* BUG: was reporting errors after the EOF if there were any errors
	 * in the file proper -- kludged it by stopping after first error.
	 *		vix 31mar87
	 */
	CheckErrorCount = 0;
	while (!CheckErrorCount && (status = load_env(envstr, tmp)) >= OK)
	{
		if (status == FALSE)
		{
			if (NULL != (e = load_entry(NewCrontab, check_error)))
				free((char *) e);
		}
	}

	if (CheckErrorCount != 0)
	{
		fprintf(stderr, "errors in crontab file, can't install.\n");
		fclose(tmp);  unlink(tn);
		exit(ERROR_EXIT);
	}

	if (fchown(fileno(tmp), ROOT_UID, -1) < OK)
	{
		perror("chown");
		fclose(tmp);  unlink(tn);
		exit(ERROR_EXIT);
	}

	if (fchmod(fileno(tmp), 0600) < OK)
	{
		perror("chown");
		fclose(tmp);  unlink(tn);
		exit(ERROR_EXIT);
	}

	if (fclose(tmp) == EOF) {
		perror("fclose");
		unlink(tn);
		exit(ERROR_EXIT);
	}

	(void) sprintf(n, CRON_TAB(User));
	if (rename(tn, n))
	{
		fprintf(stderr, "%s: error renaming %s to %s\n",
			ProgramName, tn, n);
		perror("rename");
		unlink(tn);
		exit(ERROR_EXIT);
	}
	log_it(RealUser, Pid, "REPLACE", User);

	poke_daemon();
}


static void
poke_daemon()
{
#if defined(BSD)
	struct timeval tvs[2];
	struct timezone tz;

	(void) gettimeofday(&tvs[0], &tz);
	tvs[1] = tvs[0];
	if (utimes(SPOOL_DIR, tvs) < OK)
	{
		fprintf(stderr, "crontab: can't update mtime on spooldir\n");
		perror(SPOOL_DIR);
		return;
	}
#endif  /*BSD*/
#if defined(ATT)
	if (utime(SPOOL_DIR, NULL) < OK)
	{
		fprintf(stderr, "crontab: can't update mtime on spooldir\n");
		perror(SPOOL_DIR);
		return;
	}
#endif  /*ATT*/
}
