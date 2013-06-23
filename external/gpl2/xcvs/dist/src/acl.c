/*
 * Copyright (C) 2006 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 2006, Baris Sahin <sbaris at users.sourceforge.net>
 *                                          <http://cvsacl.sourceforge.net>
 *
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * 
 *
 * CVS ACCESS CONTROL LIST EXTENSION
 *
 * It provides advanced access control definitions per modules,
 * directories, and files on branch/tag for remote cvs repository
 * connections.Execution of all CVS subcommands can be controlled
 * with eight different permissions.
 *
 * Permission Types:
 * - no permission      (n) (1)
 * - all permissions    (a) (2)
 * - write permission   (w) (3)
 * - tag permission     (t) (4)
 * - read permission    (r) (5)
 * - add permission     (c) (6)
 * - remove permission  (d) (7)
 * - permission	change  (p) (8)
 * 
 */
#include "cvs.h"
#include "getline.h"
#include <pwd.h>
#include <grp.h>

static int acl_fileproc (void *callerdat, struct file_info *finfo);

static Dtype acl_dirproc (void *callerdat, const char *dir, const char *repos,
			  const char *update_dir, List *entries);

static int acllist_fileproc (void *callerdat, struct file_info *finfo);
static Dtype acllist_dirproc (void *callerdat, const char *dir,
			      const char *repos, const char *update_dir,
			      List *entries);

static void acllist_print (char *line, const char *obj);

static int racl_proc (int argc, char **argv, char *xwhere,
		      char *mwhere, char *mfile, int shorten,
		      int local_specified, char *mname, char *msg);

static FILE *open_accessfile (char *xmode, const char *repos, char **fname);
static FILE *open_groupfile (char *xmode);

static char *get_perms (const char *xperms);
static char *make_perms (char *xperms, char *xfounduserpart, char **xerrmsg);

static char *findusername (const char *string1, const char *string2);
static char *findgroupname (const char *string1, const char *string2);
static int valid_tag (const char *part_tag, const char *tag);
static int valid_perm (const char *part_perms, int perm);
static int write_perms (const char *user, const char *perms,
			const char *founduserpart, int foundline,
			char *otheruserparts, const char *part_type,
			const char *part_object, const char *part_tag, int pos,
			const char *arepos);

static char *cache_repository;
static int cache_retval;
static int founddeniedfile;
static int cache_perm;

static int is_racl;
static int debug = 0;

int use_cvs_acl = 0;
char *cvs_acl_default_permissions;
int use_cvs_groups = 0;
int use_system_groups = 0;
int use_separate_acl_file_for_each_dir = 0;
char *cvs_acl_file_location = NULL;
char *cvs_groups_file_location = NULL;
char *cvs_server_run_as = NULL;
int stop_at_first_permission_denied = 0;

char *tag = NULL;

char *muser;
char *mperms;
static int defaultperms;

static char *default_perms_object;
char *default_part_perms_accessfile;
int aclconfig_default_used;

int acldir = 0;
int aclfile = 0;
int listacl = 0;

int userfound = 0;
int groupfound = 0;

/* directory depth ... */
char *dirs[255];

static const char *const acl_usage[] =
        {
                "Usage: %s %s [user||group:permissions] [-Rl] [-r tag] [directories...] [files...]\n",
                "\t-R\tProcess directories recursively.\n",
                "\t-r rev\tExisting revision/tag.\n",
                "\t-l\tList defined ACLs.\n",
                "(Specify the --help global option for a list of other help options)\n",
                NULL
        };

static const char *const racl_usage[] =
{
    "Usage: %s %s [user||group:permissions] [-Rl] [-r tag] [directories...]"
    " [files...]\n",
    "\t-R\tProcess directories recursively.\n",
    "\t-r rev\tExisting revision/tag.\n",
    "\t-l\tList defined ACLs.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};


int
access_allowed (const char *file, const char *repos, const char *tag,
		int perm, char **mline, int *mpos, int usecache)
{
    int retval = 0;
    int foundline = 0;
    FILE *accessfp;

    int flag = 1;

    char *iline;
    char *tempv;
    char *tempc;
    size_t tempsize;
    
    int intcount;
    int accessfilecount;
    int signlevel = -1;
    int dadmin = 0;

    const char *repository;
    char *filefullname = NULL;
    userfound = 0;
    groupfound = 0;

    if (defaultperms)
    {
	repository = xstrdup ("ALL");
    }
    else {
	if (strlen(repository = Short_Repository (repos)) == 0)
	{
	    repository = xstrdup (".");
	}
    }

    /* cache */
    if (usecache && cache_repository != NULL &&
	strcmp (cache_repository, repository) == 0 && !founddeniedfile
	&& perm == cache_perm)
	return (cache_retval);
    else
    {
	free (cache_repository);
	cache_repository = xstrdup (repository);
	cache_perm = perm;
    }

    iline = xstrdup(repository);

    tempv = strtok(iline, "/\t");
    tempc = xstrdup(tempv);
    tempsize = ( tempc != NULL ) ? strlen(tempc) : 0;

    intcount = 0;
    /* store paths from object to cvsroot */
    dirs[intcount] = xstrdup(tempc);
    while ((tempv = strtok(NULL, "/\t")) != NULL)
    {
	intcount++;

	xrealloc_and_strcat(&tempc, &tempsize, "/");
	xrealloc_and_strcat(&tempc, &tempsize, tempv);

	dirs[intcount] = xstrdup(tempc);
    }

    /* free not needed variables here */
    free (tempv);
    free (tempc);
    free (iline);

    /* accessfilecount will used
     * if UseSeparateACLFile keyword is set to yes*/
    accessfilecount = intcount;

    /* if file is not null add it to dirs array */
    if (file != NULL)
    {
	filefullname = Xasprintf("%s/%s", repository, file);
	intcount++;
	dirs[intcount] = xstrdup(filefullname);
    }

    for (; accessfilecount >= 0 && flag; accessfilecount--)
    {
	if (!use_separate_acl_file_for_each_dir) {
	    flag = 0;
	    accessfp = open_accessfile ("r", repository, NULL);
	}
	else
	{
	    flag = 1;
	    accessfp = open_accessfile ("r", dirs[accessfilecount], NULL);
	}

	if (accessfp != NULL)
	{
	    char *line = NULL;
	    size_t line_allocated = 0;

	    char *xline;
	    char *part_type = NULL;
	    char *part_object = NULL;
	    char *part_tag = NULL;
	    char *part_perms = NULL;

	    int x;

	    while (getline (&line, &line_allocated, accessfp) >= 0)
	    {

		if (line[0] == '#' || line[0] == '\0' || line[0] == '\n')
			continue;

		xline = xstrdup (line);
		part_type = strtok (line, ":\t");
		part_object = strtok (NULL, ":\t");
		part_tag = strtok (NULL, ":\t");
		part_perms = strtok (NULL, ":\t");

		if (part_type == NULL || part_object == NULL ||
		    part_tag == NULL || part_perms == NULL)
		{
		    free (line);
		    error(1, 0, "access file is corrupted or has invalid"
				" format");
		}

		if (debug) fprintf (stderr, "type %s object %s tag %s perms"
				    "%s\n", part_type, part_object, part_tag,
				    part_perms);
		for (x = intcount; x >= signlevel && x != -1; x--)
		{
		    if (debug) fprintf (stderr, "dirs[%d] = %s, part_object="
					"%s\n", x, dirs[x], part_object);
		    if (strcmp (dirs[x], part_object) == 0)
		    {
			if (debug) fprintf(stderr, "tag %s \n", tag);
			if (valid_tag (part_tag, tag))
			{
			    foundline  = 1;
			    if (debug) fprintf(stderr, "foundline\n");

			    if (listacl || ((acldir || aclfile) &&
					    x == intcount) &&
				strcmp(part_tag, tag) == 0)
			    {
				*mline = xstrdup (xline);
				*mpos = ftell (accessfp);
			    }

			    if (debug) fprintf(stderr, "perm %d\n", perm);
			    if (valid_perm (part_perms, perm))
			    {
				if (debug) fprintf(stderr, "signlevel=%d "
				    " x=%d, aclconfig_default_used=%d\n",
				    signlevel, x, aclconfig_default_used);
				if (signlevel == x)
				{
				    if (strcmp(part_tag, "ALL") != 0 &&
					!aclconfig_default_used) {
					retval = 1;
					if (debug) fprintf(stderr,
					    "%s, %d: %d\n", __FILE__, __LINE__,
					    retval);
				    }
				}
				else if (!aclconfig_default_used)
				{
				    signlevel = x;
				    retval = 1;
				    if (debug) fprintf(stderr,
					"%s, %d: %d\n", __FILE__, __LINE__,
					retval);
				}
				else {
				    /* nothing... */
				}
			    }
			    else
			    {
				if (signlevel == x)
				{
				    if (strcmp(part_tag, "ALL") != 0 &&
					!aclconfig_default_used) {
					retval = 0;
					if (debug) fprintf(stderr,
					    "%s, %d: %d\n", __FILE__, __LINE__,
					    retval);
				    }
				}
				else if (!aclconfig_default_used)
				{
				    signlevel = x;
				    retval = 0;
				    if (debug) fprintf(stderr,
					"%s, %d: %d\n", __FILE__, __LINE__,
					retval);

				    if (strncmp (part_type, "f", 1) == 0)
					founddeniedfile = 1;
				}
				else {
				}
			    }
			}
		    }
		}
		
		if (debug) fprintf (stderr, "xline tag = %s %d %d\n", xline,
				    groupfound, userfound);
		if (strncmp (xline, "d:ALL:", 6) == 0 &&
		    ((!groupfound && !userfound) || listacl))
		{
		    if (debug) fprintf (stderr, "ALL tag = %s\n", tag);
		    /* a default found */
		    if (valid_tag (part_tag, tag) > 0)
		    {
			foundline = 1;

			default_part_perms_accessfile = xstrdup (part_perms);

			if (debug) fprintf (stderr, "valid perm = %d\n", perm);
			if (valid_perm (part_perms, perm))
			{
			    retval = 1;
			    if (debug) fprintf(stderr,
				"%s, %d: %d\n", __FILE__, __LINE__,
				retval);
			    if (perm == 8)
				dadmin = 1;
			}
			else {
			    retval = 0;
			    if (debug) fprintf(stderr,
				"%s, %d: %d\n", __FILE__, __LINE__,
				retval);
			}
		    }
		}

	    }
	    
	    if (fclose (accessfp) == EOF)
		error (1, errno, "cannot close 'access' file");
	}
    }

    if (!foundline)
    {
	if (debug) fprintf(stderr, "not found line\n");
	/* DEFAULT */
	if (valid_perm (NULL, perm)) {
	    retval = 1;
	    if (debug) fprintf(stderr,
		"%s, %d: %d\n", __FILE__, __LINE__,
		retval);
	} else {
	    retval = 0;
	    if (debug) fprintf(stderr,
		"%s, %d: %d\n", __FILE__, __LINE__,
		retval);
	}
    }

    /* acl admin rigths 'p' */
    if (dadmin)
    {
	retval = dadmin;
    }

    cache_retval = retval;

    free (filefullname);
    /* free directories array */
    while (intcount >= 0)
    {
	free (dirs[intcount]);
	intcount--;
    }

    return retval;
}

/* Returns 1 if tag is valid, 0 if not */
static int
valid_tag (const char *part_tag, const char *tag)
{
    int retval;

    if (tag == NULL)
	tag = "HEAD";

    if (strcmp (tag, part_tag) == 0 || strcmp (part_tag, "ALL") == 0)
	retval = 1;
    else
	retval = 0;

    return retval;
}

/* Returns 1 if successful, 0 if not. */
static int
valid_perm (const char *part_perms, int perm)
{
    char *perms;
    int retval = 0;

    perms = get_perms (part_perms);

    /* Allow, if nothing found. */
    if (perms[0] == '\0')
	return (1);

    /* no access allowed, exit */
    if (strstr (perms, "n"))
	retval = 0;
    
    if (strstr (perms, "p"))
	/* admin rights */
	retval = 1;
    else if (strstr (perms, "a") && perm != 8)
	/* all access allowed, exit */
	retval = 1;
    else
	switch (perm)
	{
	case 3:/* write permission */
	    if (strstr (perms, "w"))
		retval = 1;
	    break;
	case 4:/* tag permission */
	    if (strstr (perms, "t"))
		retval = 1;
	    break;
	case 5:/* read permission */
	    if (strstr (perms, "w") || strstr (perms, "t") ||
		strstr (perms, "c") || strstr (perms, "d") ||
		strstr (perms, "r"))
		retval = 1;
	    break;
	case 6:/* create permission */
	    if (strstr (perms, "c"))
		retval = 1;
	    break;
	case 7:/* delete permission */
	    if (strstr (perms, "d"))
		retval = 1;
	    break;
	case 8:/* permission change */
	    if (strstr (perms, "p"))
		retval = 1;
	    break;
	default:/* never reached */
	    retval = 0;
	    break;
	}

    free (perms);

    return (retval);
}

/* returns permissions found */
char *
get_perms (const char *part_perms)
{
    char *username;
    char *xperms;
    size_t xperms_len = 1;

    FILE *groupfp;

    char *founduser = NULL;
    char *foundall = NULL;
    int default_checked = 0;

    if (debug) fprintf (stderr, "get_perms %s...", part_perms);
    aclconfig_default_used = 0;

    xperms = xmalloc (xperms_len);
    xperms[0] = '\0';

    /* use CVS_Username if set */
    if (CVS_Username == NULL)
	username = getcaller ();
    else
	username = CVS_Username;

    /* no defined acl, no default acl in access file,
     * or no access file at all */
    if (part_perms == NULL) {
	if (cvs_acl_default_permissions)
	{
	    aclconfig_default_used = 1;
	    if (debug) fprintf (stderr, "default %s\n",
			        cvs_acl_default_permissions);
	    return xstrdup(cvs_acl_default_permissions);
	}
	else {
	    if (debug) fprintf (stderr, "early %s\n", xperms);
	    return xperms;
	}
    }

check_default:
    founduser = findusername (part_perms, username);
    foundall = strstr (part_perms, "ALL!");

    if (debug) fprintf (stderr, "founduser=%s foundALL=%s\n",
		        founduser, foundall);
    if (founduser)
    {
	char *usr;
	char *per;
			
	usr = strtok (founduser, "!\t");
	per = strtok (NULL, ",\t");

	free(xperms);
	xperms = xstrdup (per);
	xperms_len = strlen (xperms);

	userfound = 1;
	free (founduser);
    }
    else
    {
	if (debug) fprintf (stderr, "usesystemgroups=%d\n", use_system_groups);
	if (use_system_groups) {
	    struct group *griter;
	    struct passwd *pwd;
	    gid_t gid = (pwd = getpwnam(username)) != NULL ? pwd->pw_gid : -1;
	    setgrent ();
	    while (griter = getgrent ())
	    {
		char *userchk;
		if (gid == griter->gr_gid) {
		    userchk = username;
		} else  {
		    char **users = griter->gr_mem;
		    int index = 0;
		    userchk = users [index++];
		    while(userchk != NULL) {
			if(strcmp (userchk, username) == 0)
			    break;
			userchk = users[index++];
		    }
		}
		if (userchk != NULL) {
		    char *grp;
		    if ((grp = findusername (part_perms, griter->gr_name)))
		    {
			char *gperm = strtok (grp, "!\t");
			if (debug) fprintf (stderr, "usercheck=%s, grp=%s\n",
					    userchk, grp);
			gperm = strtok (NULL, ",\t");
			xrealloc_and_strcat (&xperms, &xperms_len, gperm);

			groupfound = 1;
			free (grp);
		    }
		}
	    }
	    endgrent ();
	}
	else if (use_cvs_groups) {
	    groupfp = open_groupfile ("r");
	    if (groupfp != NULL)
	    {
		char *line = NULL;
		char *grp;
		char *gperm;
		int read;
		
		size_t line_allocated = 0;

		while ((read = getline (&line, &line_allocated, groupfp)) >= 0)
		{
		    char *user;
		    if (line[0] == '#' || line[0] == '\0' || line[0] == '\n')
			continue;
		    
		    if (line[read - 1] == '\n')
			line[--read] = '\0';

		    if ((grp = findgroupname (line, username)) &&
			(user = findusername (part_perms, grp)))
					    
		    {
			gperm = strtok (user, "!\t");
			gperm = strtok (NULL, ",\t");
			xrealloc_and_strcat (&xperms, &xperms_len, gperm);
			groupfound = 1;
			free (grp);
			free (user);
		    }
		}
		
		free (line);
						
		if (fclose (groupfp) == EOF)
		    error (1, errno, "cannot close 'group' file");
	    }
	}
    }

    if (foundall)
    {
	char *usr;
	char *per;
	
	usr = strtok (strstr (part_perms, "ALL!"), "!\t");
	per = strtok (NULL, ",\t");

	if (!default_checked)
	    default_perms_object = xstrdup (per);

	if (xperms[0] == '\0')
	{
	    xperms = xstrdup (per);
	    xperms_len = strlen (xperms);
	}
	
	/* You don't free pointers from strtok()! */
	//free(usr);
	//free(per);
    }
    
    if (xperms[0] == '\0' && !default_checked && default_part_perms_accessfile)
    {
	part_perms = xstrdup (default_part_perms_accessfile);
	default_checked = 1;

	goto check_default;
    }

    if (xperms[0] != '\0' && strcmp (xperms, "x") == 0)
    {
	if (default_perms_object)
	    xperms = xstrdup (default_perms_object);
	else if (default_part_perms_accessfile)
	{
	    part_perms = default_part_perms_accessfile;
	    default_checked = 1;
	    goto check_default;
	}
	else if (cvs_acl_default_permissions)
	{
	    aclconfig_default_used = 1;
	    xperms = xstrdup (cvs_acl_default_permissions);
	}
    }

    if (xperms[0] == '\0' && cvs_acl_default_permissions)
    {
	aclconfig_default_used = 1;
	xperms = xstrdup (cvs_acl_default_permissions);
    }

    if (debug) fprintf (stderr, "late %s\n", xperms);
    return xperms;
}


int
cvsacl (int argc, char **argv)
{
    char *chdirrepository;
    int c;
    int err = 0;
    int usetag = 0;
    int recursive = 0;

    int which;
    char *where;

    is_racl = (strcmp (cvs_cmd_name, "racl") == 0);

    if (argc == -1)
	usage (is_racl ? racl_usage : acl_usage);

    /* parse the args */
    optind = 0;

    while ((c = getopt (argc, argv, "dRr:l")) != -1)
    {
	switch (c)
	{
	case 'd':
	    debug++;
	    break;
	case 'R':
	    recursive = 1;
	    break;
	case 'r': // baris
	    tag = xstrdup (optarg);
	    break;
	case 'l':
	    listacl = 1;
	    break;
	case '?':
	default:
	    usage (is_racl ? racl_usage : acl_usage);
	    break;
	}
    }

    argc -= optind;
    argv += optind;

    if (argc < (is_racl ? 1 : 1))
	usage (is_racl ? racl_usage : acl_usage);
    if (listacl) {
	if (strstr (argv[0], ":"))
	    usage (is_racl ? racl_usage : acl_usage);
    } else {
	if (!strstr (argv[0], ":"))
	    usage (is_racl ? racl_usage : acl_usage);
    }


#ifdef CLIENT_SUPPORT

    if (current_parsed_root->isremote)
    {
	start_server ();
	ign_setup ();

	if(recursive)
	    send_arg ("-R");

	if (listacl)
	    send_arg ("-l");

	if(tag)
	{
	    option_with_arg ("-r", tag);
	}

	send_arg ("--");

	if (!listacl)
	{
	    send_arg (argv[0]);

	    argc--;
	    argv++;
	}

	if (is_racl)
	{
	    int i;
	    for (i = 0; i < argc; ++i)
		send_arg (argv[i]);

	    send_to_server ("racl\012",0);
	}
	else
	{
	    send_files (argc, argv, recursive, 0, SEND_NO_CONTENTS);
	    send_file_names (argc, argv, SEND_EXPAND_WILD);
	    send_to_server ("acl\012", 0);
	}
	
	return get_responses_and_close ();
    }
#endif

#ifdef SERVER_SUPPORT

    if (!listacl)
    {
	muser = strtok (argv[0], ":\t");
	mperms = strtok (NULL, ":\t");

	/* if set to 'default' */
	if ((strlen (mperms) == 7) && (strncmp (mperms, "default", 7) == 0))
	    mperms = xstrdup ("x");

	/* Check that the given permissions are valid. */
	if (!given_perms_valid (mperms))
	    error (1,0,"Invalid permissions: `%s'", mperms);

	argc--;
	argv++;
    }


    if (!tag)
	tag = xstrdup ("HEAD");

    if (!strcasecmp (argv[0], "ALL"))
    {
	argv[0] = xstrdup (".");
	defaultperms = 1;
	if (!use_separate_acl_file_for_each_dir)
	{
	    recursive = 0;
	}

    }

    if (is_racl)
    {
	DBM *db;
	int i;
	db = open_module ();
	for (i = 0; i < argc; i++)
	{
	    err += do_module (db, argv[i], MISC, "ACL ing: ",
			      racl_proc, NULL, 0, !recursive, 0,
			      0, NULL);
	}
	close_module (db);
    }
    else
    {
	err = racl_proc (argc + 1, argv - 1, NULL, NULL, NULL, 0, !recursive,
			 NULL, NULL);
    }

    return err;

#endif
}

static int
racl_proc (int argc, char **argv, char *xwhere, char *mwhere,
	   char *mfile, int shorten, int local, char *mname, char *msg)
{
    char *myargv[2];
    int err = 0;
    int which;
    char *repository;
    char *where;
    char *obj;
    size_t objlen = 0;

    if (!use_cvs_acl)
    {
	error(1, 0, "CVSACL extension is not enabled, set `UseCVSACL=yes'"
	      " in aclconfig file");
    }

    if (is_racl)
    {
	char *v;
	repository = Xasprintf ("%s/%s", current_parsed_root->directory,
				argv[0]);
	where = xstrdup (argv[0]);

	/* if mfile isn't null, we need to set up to do only part of the
	 * module */
	if (mfile != NULL)
	{
	    char *cp;
	    char *path;

	    /* if the portion of the module is a path, put the dir part on
	     * repos */
	    if ((cp = strrchr (mfile, '/')) != NULL)
	    {
		*cp = '\0';
		v = Xasprintf ("%s/%s", repository, mfile);
		free (repository);
		repository = v;
		v = Xasprintf ("%s/%s", where, mfile);
		free(where);
		where = v;
		mfile = cp + 1;
	    }

	    /* take care of the rest */
	    path = Xasprintf ("%s/%s", repository, mfile);
	    if (isdir (path))
	    {
		/* directory means repository gets the dir tacked on */
		free(repository);
		repository = path;
		v = Xasprintf ("%s/%s", where, mfile);
		free(where);
		where = v;
	    }
	    else
	    {
		free (path);
		myargv[0] = argv[0];
		myargv[1] = mfile;
		argc = 2;
		argv = myargv;
	    }
	}

	/* cd to the starting repository */
	if ( CVS_CHDIR (repository) < 0)
	{
	    error (0, errno, "cannot chdir to %s", repository);
	    free (repository);
	    free (where);
	    return 1;
	}

	/* End section which is identical to patch_proc.  */

	which = W_REPOS | W_ATTIC;

	if (argc > 1)
	{
		obj = Xasprintf ("%s/%s", repository, argv[1]);
	}
	else
	{
		obj = xstrdup(repository);
	}
    }
    else
    {
	where = NULL;
	repository = NULL;
	which = W_LOCAL | W_REPOS | W_ATTIC;

	obj = xstrdup (argv[1]);
    }

    if (isdir (obj))
	acldir = 1;
    else if (isfile (obj))
	aclfile = 1;
    else
	error(1, 0, "no such file or directory");
    
    free (obj);

    if (listacl)
	err = start_recursion (acllist_fileproc, NULL, acllist_dirproc, NULL,
			       NULL, argc - 1, argv + 1, local, which, 0, 0,
			       where, 1, repository);
    else
	err = start_recursion (acl_fileproc, NULL, acl_dirproc, NULL, NULL,
			       argc - 1, argv + 1, local, which, 0, 0,
			       where, 1, repository);

    if (repository != NULL)
	free (repository);
    if (where != NULL)
	free (where);
    
    return err;
}


static int
acl_fileproc (void *callerdat, struct file_info *finfo)
{
    char *filefullname;
    char *founduserpart = NULL;
    char *otheruserparts = NULL;
    size_t otherslen = 0;

    const char *frepository;
    int foundline = 0;

    char *line = NULL;
    size_t line_allocated = 0;
    int linelen;

    char *wperms;
    char *errmsg;

    int pos;
    
    if (!aclfile)
	return 0;

    frepository = Short_Repository (finfo->repository);

    filefullname = Xasprintf("%s/%s", frepository, finfo->file);

    
    if (!access_allowed (finfo->file, finfo->repository, tag, 8, &line, &pos,
			 0))
	error (1, 0, "You do not have acl admin rights on '%s'", frepository);

    if (line != NULL)
    {
	char *part_type = NULL;
	char *part_object = NULL;
	char *part_tag = NULL;
	char *part_perms = NULL;
	char *userpart;
		
	part_type = strtok (line, ":\t");
	part_object = strtok (NULL, ":\t");
	part_tag = strtok (NULL, ":\t");
	part_perms = strtok (NULL, ":\t");

	foundline = 1;
	userpart = strtok (part_perms, ",\t");

	do
	{
	    if (strncmp (userpart, muser, strlen (muser)) == 0)
		founduserpart = xstrdup (userpart);
	    else
	    {
		if (otheruserparts != NULL)
		{
		    xrealloc_and_strcat (&otheruserparts, &otherslen, ",");
		    xrealloc_and_strcat (&otheruserparts, &otherslen, userpart);
		}
		else
		{
		    otheruserparts = xstrdup (userpart);
		    otherslen = strlen (otheruserparts);
		}
	    }
	} while ((userpart = strtok (NULL, ",\t")) != NULL);
	
	free (userpart);
    }

    wperms = make_perms (mperms, founduserpart, &errmsg);
    if (wperms == NULL)
    {
	if (errmsg)
	    error (0, 0, "`%s' %s", filefullname, errmsg);
    }
    else
    {
	cvs_output ("X ", 0);
	cvs_output (filefullname, 0);
	cvs_output ("\n", 0);

	write_perms (muser, wperms, founduserpart, foundline,
		     otheruserparts, "f", filefullname, tag, pos,
		     Short_Repository(finfo->repository));
    }

    free (line);
    free (founduserpart);
    free (otheruserparts);
    free (wperms);
    free (filefullname);
    
    return 0;
}

static Dtype
acl_dirproc (void *callerdat, const char *dir, const char *repos,
	     const char *update_dir, List *entries)
{
    const char *drepository;
    char *founduserpart = NULL;
    char *otheruserparts = NULL;
    size_t otherslen = 0;
    int foundline = 0;

    char *line = NULL;
    size_t line_allocated = 0;
    int linelen;

    int pos;

    char *wperms;
    char *errmsg;

    if (!acldir)
	return 0;

    if (repos[0] == '\0')
	repos = Name_Repository (dir, NULL);

    if (!access_allowed (NULL, repos, tag, 8, &line, &pos, 0))
	error (1, 0, "You do not have admin rights on '%s'",
	       Short_Repository (repos));

    drepository = Short_Repository (repos);

    if (line != NULL)
    {
	char *part_type = NULL;
	char *part_object = NULL;
	char *part_tag = NULL;
	char *part_perms = NULL;
	char *userpart;
			
	part_type = strtok (line, ":\t");
	part_object = strtok (NULL, ":\t");
	part_tag = strtok (NULL, ":\t");
	part_perms = strtok (NULL, ":\t");
				
	foundline = 1;
	userpart = strtok (part_perms, ",\t");

	do
	{
	    if (strncmp (userpart, muser, strlen (muser)) == 0)
		founduserpart = xstrdup (userpart);
	    else
	    {
		if (otheruserparts != NULL)
		{
		    xrealloc_and_strcat (&otheruserparts, &otherslen, ",");
		    xrealloc_and_strcat (&otheruserparts, &otherslen, userpart);
		}
		else
		{
		    otheruserparts = xstrdup (userpart);
		    otherslen = strlen (otheruserparts);
		}
	    }
	} while ((userpart = strtok (NULL, ",\t")) != NULL);
    }

    wperms = make_perms (mperms, founduserpart, &errmsg);
    if (wperms == NULL)
    {
	if (errmsg)
	    error (0, 0, "`%s' %s", drepository, errmsg);
    }
    else
    {
	if (defaultperms)
	{
	    cvs_output ("X ", 0);
	    cvs_output ("ALL", 0);
	    cvs_output ("\n", 0);
	    write_perms (muser, wperms, founduserpart, foundline,
			 otheruserparts, "d", "ALL", tag, pos, drepository);

	}
	else
	{
	    cvs_output ("X ", 0);
	    cvs_output (drepository, 0);
	    cvs_output ("\n", 0);
	    write_perms (muser, wperms, founduserpart, foundline,
			 otheruserparts, "d", drepository, tag, pos,
			 drepository);
	}
    }
    
    free (line);
    free (founduserpart);
    free (otheruserparts);
    free (wperms);
    
    return 0;
}

/* Open CVSROOT/access or defined CVSACLFileLocation file 
 * Open access file In each directory if UseSeparateACLFileForEachDir=yes
 * returns file pointer to access file or NULL if access file not found */
FILE *
open_accessfile (char *fmode, const char *adir, char **fname)
{
    char *accessfile = NULL;
    FILE *accessfp;

    if (!use_separate_acl_file_for_each_dir)
    {
	if (cvs_acl_file_location == NULL)
	{
	    accessfile = Xasprintf("%s/%s/%s", current_parsed_root->directory,
				   CVSROOTADM, CVSROOTADM_ACCESS);
	}
	else
	{
	    accessfile = xstrdup(cvs_acl_file_location);
	}
    }
    else
    {
	size_t accessfilelen = 0;
	xrealloc_and_strcat (&accessfile, &accessfilelen,
			     current_parsed_root->directory);
	xrealloc_and_strcat (&accessfile, &accessfilelen, "/");
	xrealloc_and_strcat (&accessfile, &accessfilelen, adir);
	xrealloc_and_strcat (&accessfile, &accessfilelen, "/access");
    }

    accessfp = CVS_FOPEN (accessfile, fmode);

    if (fname != NULL)
	*fname = xstrdup (accessfile);

    free (accessfile);

    return accessfp;
}

/* Open /etc/group file if UseSystemGroups=yes in config file
 * Open CVSROOT/group file if UseCVSGroups=yes in config file
 * Open group file if specified in CVSGroupsFileLocation 
 * returns group file pointer if UseSystemGroups=yes
 * returns NULL if UseSystemGroups=no or group file not found */
FILE *
open_groupfile (char *fmode)
{
    char *groupfile;
    FILE *groupfp;

    if (use_cvs_groups)
    {
	if (cvs_groups_file_location != NULL)
	{
	    groupfile = xstrdup (cvs_groups_file_location);
	}
	else
	{
	    groupfile = Xasprintf("%s/%s/%s", current_parsed_root->directory,
				  CVSROOTADM, CVSROOTADM_GROUP);
	}
    }
    else
    {
	    return NULL;
    }

    groupfp = CVS_FOPEN (groupfile, "r");

    if (groupfp == NULL)
	error (0, 0, "cannot open file: %s", groupfile);

    free (groupfile);

    return groupfp;
}


/* Check whether given permissions are valid or not
 * Returns 1 if permissions are valid
 * Returns 0 if permissions are NOT valid */
int
given_perms_valid (const char *cperms)
{
    int cperms_len;
    int retval;
    int index, i;

    if (cperms[0] == '+' || cperms[0] == '-')
	index = 1;
    else
	index = 0;

    cperms_len = strlen (cperms);

    switch (cperms[index])
    {
    case 'x':
	if ((cperms_len - index) == 1 && cperms_len == 1)
	    retval = 1;
	else
	    retval = 0;
	break;
    case 'n':
	if ((cperms_len - index) == 1 && cperms_len == 1)
	    retval = 1;
	else
	    retval = 0;
	break;
    case 'p':
	if ((cperms_len - index) == 1)
	    retval = 1;
	else
	    retval = 0;
	break;
    case 'a':
	if ((cperms_len - index) == 1)
	    retval = 1;
	else
	    retval = 0;
	break;
    case 'r':
	if ((cperms_len - index) == 1)
	    retval = 1;
	else
	    retval = 0;
	break;
    case 'w':
	if ((cperms_len - index) == 1)
		retval = 1;
	else
	    for (i = index + 1; i < cperms_len; i++)
		if (cperms[i] == 't' || cperms[i] == 'c' || cperms[i] == 'd')
		    retval = 1;
		else
		    retval = 0;
	break;
    case 't':
	if ((cperms_len - index) == 1)
	    retval = 1;
	else
	    for (i = index + 1; i < cperms_len; i++)
		if (cperms[i] == 'w' || cperms[i] == 'c' || cperms[i] == 'd')
		    retval = 1;
		else
		    retval = 0;
	break;
    case 'c':
	if ((cperms_len - index) == 1)
	    retval = 1;
	else
	    for (i = index + 1; i < cperms_len; i++)
		if (cperms[i] == 't' || cperms[i] == 'w' || cperms[i] == 'd')
		    retval = 1;
		else
		    retval = 0;
	break;
    case 'd':
	if ((cperms_len - index) == 1)
	    retval = 1;
	else
	    for (i = index + 1; i < cperms_len; i++)
		if (cperms[i] == 't' || cperms[i] == 'c' || cperms[i] == 'w')
		    retval = 1;
		else
		    retval = 0;
	break;
    default:
	retval = 0;
	break;
    }
    
    return retval;
}

/* prepare permsissions string to be written to access file
 * returns permissions or NULL if */
char *
make_perms (char *perms, char *founduserpart, char **xerrmsg)
{
    char *fperms = NULL;
    size_t perms_len;
    size_t fperms_len;
    
    int i, j;
    int err = 0;
    char *errmsg = NULL;
    
    char *retperms;
    size_t retperms_len;

    perms_len = strlen (perms);
    if (perms[0] == '+' || perms[0] == '-')
    {
	retperms = xmalloc (retperms_len);
	retperms[0] = '\0';
	retperms_len = 1;

	if (founduserpart)
	{
	    char *tempfperms;
	    size_t tempfperms_len;
	    
	    char *temp;
	    int per = 0;
	    temp = strtok (founduserpart, "!\t");
	    fperms = strtok (NULL, "!\t");
	    fperms_len = strlen (fperms);

	    if (strncmp (fperms, "x", 1) == 0)
	    {
		err = 1;
		if (perms[0] == '+')
		    *xerrmsg = xstrdup ("cannot add default permission 'x'");
		else
		    *xerrmsg = xstrdup ("cannot remove default permission 'x'");
	    }

	    /* go through perms */
	    for (i = 1; i < perms_len && !err; i++)
	    {
		switch (perms[i])
		{
		case 'n':
		    err = 1;
		    break;
		case 'p':
		    if (perms[0] == '+')
			fperms = xstrdup ("p");
		    else if (perms[0] == '-')
		    {
			fperms_len = 1;
			fperms = xmalloc (fperms_len);
			fperms[0] = '\0';
		    }
		    break;
		case 'a':
		    for (j = 0; j < fperms_len; j++)
		    {
			if (fperms[j] == 'p')
			{
			    err = 1;
			    *xerrmsg = xstrdup ("user have admin rights,"
						" cannot use +/- permissions");
			}
			else if (fperms[j] == 'a' && perms[0] == '+')
			{
			    err = 1;
			    *xerrmsg = xstrdup ("user already has all ('a')"
						" permission");
			}
			else if (fperms[j] != 'a' && perms[0] == '-')
			{
			    err = 1;
			    *xerrmsg = xstrdup ("user does not have all "
						"('a') permission");
			}
		    }
		    if (perms[0] == '+')
		    {
			fperms = xstrdup ("a");
			fperms_len = strlen (fperms);
		    }
		    else if (perms[0] == '-')
		    {
			fperms_len = 1;
			fperms = xmalloc (fperms_len);
			fperms[0] = '\0';
		    }
		    break;
		case 'r':
		    for (i = 0; i < fperms_len; i++)
		    {
			if (fperms[i] == 'n' && perms[0] == '+')
			{
			    fperms = xstrdup ("r");
			    fperms_len = strlen (fperms);
			}
			else if (fperms[i] == 'r' && perms[0] == '-')
			{
			    fperms_len = 1;
			    fperms = xmalloc (fperms_len);
			    fperms[0] = '\0';
			}
			else if (perms[0] == '-')
			{
			    err = 1;
			    *xerrmsg = xstrdup ("user has other permissions,"
						" cannot remove read ('r')"
						" permission");
			}
			else
			{
			    err = 1;
			    *xerrmsg = xstrdup ("user has other permissions,"
						" cannot remove read ('r')"
						" permission");
			}
		    }
		    break;
		case 'w':
		    {
			tempfperms_len = 1;

			tempfperms = xmalloc (tempfperms_len);
			tempfperms[0] = '\0';

			for (j = 0; j < fperms_len; j++)
			{
			    if (fperms[j] == 't' || fperms[j] == 'c' ||
				fperms[j] == 'd')
			    {
				char *temp;
				temp = xmalloc (2);
				temp[0] = fperms[j];
				temp[1] = '\0';

				xrealloc_and_strcat (&tempfperms,
						     &tempfperms_len, temp);
				free (temp);
			    }
			    else if (fperms[j] == 'a' || fperms[j] == 'p')
			    {
				err = 1;
				*xerrmsg = xstrdup ("user has higher"
						    " permissions, cannot use"
						    " +/- write permissions");
			    }
			    else if (fperms[j] == 'n' || fperms[j] == 'r')
			    {
				if (perms[0] == '-')
				{
				    err = 1;
				    *xerrmsg = xstrdup ("user does not have"
							" write ('w')"
							" permission");
				}
			    }
			    else if (fperms[j] == 'w')
			    {
				per = 1;
				if (perms[0] == '+') {
				    err = 1;
				    *xerrmsg = xstrdup ("user already have"
							" write ('w')"
							"permission");
				}
			    }
			}

			fperms = tempfperms;
			fperms_len = strlen (fperms);

			if (!per && !err && (perms[0] == '-')) {
			    err = 1;
			    *xerrmsg = xstrdup ("user does not have write"
						" ('w') permission");
			}

			if (perms[0] == '+')
			{
			    xrealloc_and_strcat (&fperms, &fperms_len, "w");
			}
		    }
		    break;
		case 't':
		    {
			tempfperms_len = 1;

			tempfperms = xmalloc (tempfperms_len);
			tempfperms[0] = '\0';

			for (j = 0; j < fperms_len; j++)
			{
			    if (fperms[j] == 'w' || fperms[j] == 'c' ||
				fperms[j] == 'd')
			    {
				char *temp;
				temp = xmalloc (2);
				temp[0] = fperms[j];
				temp[1] = '\0';

				xrealloc_and_strcat (&tempfperms,
						     &tempfperms_len, temp);
				free (temp);
			    }
			    else if (fperms[j] == 'a' || fperms[j] == 'p')
			    {
				err = 1;
				*xerrmsg = xstrdup ("user has higher"
						    " permissions, cannot use"
						    " +/- tag permissions");
			    }
			    else if (fperms[j] == 'n' || fperms[i] == 'r')
			    {
				if (perms[0] == '-')
				    *xerrmsg = xstrdup ("user does not have tag"
							" ('t') permission");
			    }
			    else if (fperms[j] == 't')
			    {
				per = 1;
				if (perms[0] == '+')
				{
				    err = 1;
				    *xerrmsg = xstrdup ("user already have tag"
							" ('t') permission");
				}
			    }
			}

			fperms = tempfperms;
			fperms_len = strlen (fperms);

			if (!per && !err && (perms[0] == '-'))
			{
			    err = 1;
			    *xerrmsg = xstrdup ("user does not have tag ('t')"
						" permission");
			}

			if (perms[0] == '+')
			{
			    xrealloc_and_strcat (&fperms, &fperms_len, "t");
			}
		    }
		    break;
		case 'c':
		    {
			tempfperms_len = 1;

			tempfperms = xmalloc (tempfperms_len);
			tempfperms[0] = '\0';

			for (j = 0; j < fperms_len; j++)
			{
			    if (fperms[j] == 'w' || fperms[j] == 't' ||
				fperms[j] == 'd')
			    {
				char *temp;
				temp = xmalloc (2);
				temp[0] = fperms[j];
				temp[1] = '\0';

				xrealloc_and_strcat (&tempfperms,
						     &tempfperms_len, temp);
				free (temp);
			    }
			    else if (fperms[j] == 'a' || fperms[j] == 'p')
			    {
				err = 1;
				*xerrmsg = xstrdup ("user has higher"
						    " permissions, cannot use"
						    " +/- create permissions");
			    }
			    else if (fperms[j] == 'n' || fperms[i] == 'r')
			    {
				if (perms[0] == '-')
				    err = 1;
				*xerrmsg = xstrdup ("user does not have create"
						    " ('c') permission");
			    }
			    else if (fperms[j] == 'c')
			    {
				per = 1;
				if (perms[0] == '+') {
				    err = 1;
				    *xerrmsg = xstrdup ("user already have"
							" create ('c')"
							" permission");
				}
			    }
			}

			fperms = tempfperms;
			fperms_len = strlen (fperms);

			if (!per && !err && (perms[0] == '-')) {
			    err = 1;
			    *xerrmsg = xstrdup ("user does not have create"
						" ('c') permission");
			}

			if (perms[0] == '+')
			{
			    xrealloc_and_strcat (&fperms, &fperms_len, "c");
			}
		    }
		    break;
		case 'd':
		    {
			tempfperms_len = 1;

			tempfperms = xmalloc (tempfperms_len);
			tempfperms[0] = '\0';

			for (j = 0; j < fperms_len; j++)
			{
			    if (fperms[j] == 'w' || fperms[j] == 'c' ||
				fperms[j] == 't')
			    {
				char *temp;
				temp = xmalloc (2);
				temp[0] = fperms[j];
				temp[1] = '\0';

				xrealloc_and_strcat (&tempfperms,
						     &tempfperms_len, temp);
				free (temp);
			    }
			    else if (fperms[j] == 'a' || fperms[j] == 'p')
			    {
				err = 1;
				*xerrmsg = xstrdup ("user has higher"
						    " permissions, cannot use"
						    " +/- delete permissions");
			    }
			    else if (fperms[j] == 'n' || fperms[i] == 'r')
			    {
				if (perms[0] == '-')
				    err = 1;
				*xerrmsg = xstrdup ("user does not have delete"
						    " ('d') permission");
			    }
			    else if (fperms[j] == 'd')
			    {
				per = 1;
				if (perms[0] == '+') {
				    err = 1;
				    *xerrmsg = xstrdup ("user already have"
							" delete ('d')"
							" permission");
				}
			    }
			}

			fperms = tempfperms;
			fperms_len = strlen (fperms);
			
			if (!per && !err && (perms[0] == '-')) {
				err = 1;
				*xerrmsg = xstrdup ("user does not have delete"
						    " ('d') permission");
			}

			if (perms[0] == '+')
			{
				xrealloc_and_strcat (&fperms, &fperms_len, "d");
			}
		    }
		    break;
		default:
		    err  = 1;
		    *xerrmsg = xstrdup ("error in 'access' file format");
		    break;
		}
		
		if (fperms[0] == '\0')
		    retperms = xstrdup ("none");
		else
		    retperms = xstrdup (fperms);
	    }
	}
	else
	{
	    err = 1;
	    *xerrmsg = xstrdup("user is not given any permissions to remove/add");
	}
    }
    else
    {
	retperms = xstrdup (perms);
    }
    if (fperms)
	free (fperms);
    if (err && retperms)
	free (retperms);
    
    return (err ? NULL : retperms);
}

/* prepare and write resulting permissions to access file */
static int
write_perms (const char *user, const char *perms, const char *founduserpart,
	     int foundline, char *otheruserparts,
	     const char *part_type, const char *part_object,
	     const char *part_tag, int pos, const char *arepos)
{
    char *accessfile;
    char *tmpaccessout;
    FILE *accessfpin;
    FILE *accessfpout;

    char *newline = NULL;
    size_t newlinelen = 1;
    char *object;

    char *line = NULL;
    size_t line_allocated = 0;

    newline = xmalloc (newlinelen);
    newline[0] = '\0';

    if (!strcasecmp (part_tag, "ALL"))
	part_tag = "ALL";

    /* strip any trailing slash if found */
    object = xstrdup (part_object);
    if (object[strlen (object) - 1] == '/')
	object[strlen (object) - 1] = '\0';

    /* first parts, part type, object, and tag */
    xrealloc_and_strcat (&newline, &newlinelen, part_type);
    xrealloc_and_strcat (&newline, &newlinelen, ":");
    xrealloc_and_strcat (&newline, &newlinelen, object);
    xrealloc_and_strcat (&newline, &newlinelen, ":");
    xrealloc_and_strcat (&newline, &newlinelen, part_tag);
    xrealloc_and_strcat (&newline, &newlinelen, ":");

    if (strncmp (perms, "none", 4) != 0)
    {
	xrealloc_and_strcat (&newline, &newlinelen, user);
	xrealloc_and_strcat (&newline, &newlinelen, "!");
	xrealloc_and_strcat (&newline, &newlinelen, perms);
	if (otheruserparts != NULL)
	    xrealloc_and_strcat (&newline, &newlinelen, ",");
    }

    if (otheruserparts != NULL)
    {
	if (otheruserparts[strlen (otheruserparts) - 1] == '\n')
	    otheruserparts[strlen (otheruserparts) - 1] = '\0';

	xrealloc_and_strcat (&newline, &newlinelen, otheruserparts);
    }

    xrealloc_and_strcat (&newline, &newlinelen, ":");

    if (foundline)
    {
	accessfpout = cvs_temp_file (&tmpaccessout);
	if (accessfpout == NULL)
	    error (1, errno, "cannot open temporary file: %s", tmpaccessout);

	accessfpin = open_accessfile ("r", arepos, &accessfile);
	if (accessfpout == NULL)
	    error (1, errno, "cannot open access file: %s", accessfile);

	while (getline (&line, &line_allocated, accessfpin) >= 0)
	{
	    if (pos != ftell (accessfpin))
	    {
		if (fprintf (accessfpout, "%s", line) < 0)
		    error (1, errno, "writing temporary file: %s", tmpaccessout);
	    }
	    else
	    {
		if (fprintf (accessfpout, "%s\n", newline) < 0)
		    error (1, errno, "writing temporary file: %s", tmpaccessout);
	    }

	}
	if (fclose (accessfpin) == EOF)
		error (1, errno, "cannot close access file: %s", accessfile);

	if (fclose (accessfpout) == EOF)
	    error (1, errno, "cannot close temporary file: %s", tmpaccessout);

	if (CVS_UNLINK (accessfile) < 0)
	    error (0, errno, "cannot remove %s", accessfile);

	copy_file (tmpaccessout, accessfile);

	if (CVS_UNLINK (tmpaccessout) < 0)
	    error (0, errno, "cannot remove temporary file: %s", tmpaccessout);
    }
    else
    {
	accessfpout = open_accessfile ("r+", arepos, &accessfile);

	if (accessfpout == NULL)
	{
	    if (existence_error (errno))
	    {
		accessfpout = open_accessfile ("w+", arepos, &accessfile);
	    }
	    if (accessfpout == NULL)
		error (1, errno, "cannot open access file: %s", accessfile);
	}
	else {
	    if (fseek (accessfpout, 0, 2) != 0)
		error (1, errno, "cannot fseek access file: %s", accessfile);
	}

	if (fprintf (accessfpout, "%s\n", newline) < 0)
	    error (1, errno, "writing access file: %s", accessfile);

	if (fclose (accessfpout) == EOF)
	    error (1, errno, "cannot close access file: %s", accessfile);
    }

    free (line);
    free (newline);

    chmod (accessfile, 0644);

    return 0;
}

static int
acllist_fileproc (void *callerdat, struct file_info *finfo)
{
    char *filefullname;
    const char *frepository;
    char *line = NULL;
    int pos;

    if (!aclfile)
	return 0;

    frepository = Short_Repository (finfo->repository);

    filefullname = Xasprintf("%s/%s", frepository, finfo->file);

    /* check that user, which run acl/racl command, has admin permisson,
     * and also return the line with permissions from access file.  */
    if (!access_allowed (finfo->file, finfo->repository, tag, 5, &line, &pos,
			 0))
	error (1, 0, "You do not have admin rights on '%s'", frepository);

    acllist_print (line, filefullname);

    free (filefullname);

    return 0;
}

static Dtype
acllist_dirproc (void *callerdat, const char *dir, const char *repos,
		 const char *update_dir, List *entries)
{
    char *line = NULL;
    const char *drepository;
    int pos;

    if (repos[0] == '\0')
	repos = Name_Repository (dir, NULL);

    if (!acldir)
	return 0;

    drepository = Short_Repository (repos);
    
    /* check that user, which run acl/racl command, has admin permisson,
     * and also return the line with permissions from access file.  */
    if (!access_allowed (NULL, repos, tag, 5, &line, &pos, 0))
	error (1, 0, "You do not have admin rights on '%s'", drepository);

    acllist_print (line, drepository);

    return 0;
}

/* Prints permissions to screen with -l option */
void
acllist_print (char *line, const char *obj)
{
    char *temp;
    int c = 0;
    int def = 0;

    char *printedusers[255];
    printedusers[0] = NULL;

    if (line != NULL)
    {
	temp = strtok (line, ":\t");

	if (acldir)
	    cvs_output ("d ", 0);
	else if (aclfile)
	    cvs_output ("f ", 0);

	temp = strtok (NULL, ":\t");

	cvs_output(obj, 0);
	cvs_output (" | ", 0);

	temp = strtok (NULL, ":\t");
	cvs_output (temp, 0);
	cvs_output (" | ", 0);

	while ((temp = strtok (NULL, "!\t")) != NULL)
	{
	    if (strncmp (temp, ":", 1) == 0)
		break;

	    if (strcmp (temp, "ALL") == 0)
	    {
		temp = strtok (NULL, ",\t");
		continue;
	    }

	    cvs_output (temp, 0);
	    cvs_output (":", 0);

	    while (printedusers[c] != NULL)
		c++;

	    printedusers[c] = xstrdup (temp);
	    c++;
	    printedusers[c] = NULL;

	    temp = strtok (NULL, ",\t");

	    if (temp != NULL && temp[strlen (temp) - 2] == ':')
		temp[strlen (temp) - 2] = '\0';

	    cvs_output (temp, 0);
	    cvs_output (" ", 0);
	}

	if (default_perms_object)
	{
	    cvs_output ("| defaults ", 0);
	    cvs_output ("ALL:", 0);
	    cvs_output (default_perms_object, 0);
	    def = 1;
	}
	if (default_part_perms_accessfile)
	{
	    size_t i;
	    i = strlen (default_part_perms_accessfile);
	    xrealloc_and_strcat (&default_part_perms_accessfile, &i, ",");

	    free (line);
	    line = xstrdup (default_part_perms_accessfile);

	    if (!def)
		cvs_output ("| defaults ", 0);
	    else
		cvs_output (" ", 0);

	    temp = strtok (line, "!\t");
	    cvs_output (temp, 0);
	    cvs_output (":", 0);

	    temp = strtok (NULL, ",\t");

	    cvs_output (temp, 0);
	    cvs_output (" ", 0);

	    while ((temp = strtok (NULL, "!\t")) != NULL)
	    {
		int printed = 0;
		int c2 = 0;
		while (printedusers[c2] != NULL && printed == 0)
		{
		    if (strcmp (printedusers[c2], temp) == 0)
		    {
			printed = 1;
			break;
		    }
		    c2++;
		}

		if (printed == 0)
		{
		    cvs_output (temp, 0);
		    cvs_output (":", 0);
		}

		temp = strtok (NULL, ",\t");

		if (temp[strlen (temp) - 2] == ':')
		    temp[strlen (temp) - 2] = '\0';

		if (printed == 0)
		{
		    cvs_output (temp, 0);
		    cvs_output (" ", 0);
		}
	    }
	    def = 1;
	}
	else if (cvs_acl_default_permissions)
	{
	    cvs_output ("| defaults ", 0);
	    cvs_output ("ALL: ", 0);
	    cvs_output (cvs_acl_default_permissions, 0);
	}
    }
    else
    {
	if (acldir)
	    cvs_output ("d ", 0);
	else if (aclfile)
	    cvs_output ("f ", 0);
	cvs_output (obj, 0);
	cvs_output (" | ", 0);
	cvs_output (tag, 0);
	cvs_output (" | ", 0);

	if (default_perms_object)
	{
	    cvs_output ("| defaults ", 0);
	    cvs_output ("ALL:", 0);
	    cvs_output (default_perms_object, 0);
	    def = 1;
	}
	if (default_part_perms_accessfile)
	{
	    free (line);
	    line = xstrdup (default_part_perms_accessfile);

	    if (!def)
		cvs_output ("| defaults ", 0);
	    else
		cvs_output (" ", 0);

	    temp = strtok (line, "!\t");
	    cvs_output (temp, 0);
	    cvs_output (":", 0);

	    temp = strtok (NULL, ",\t");

	    if (temp[strlen (temp) - 2] == ':')
		temp[strlen (temp) - 2] = '\0';

	    cvs_output (temp, 0);
	    cvs_output (" ", 0);

	    while ((temp = strtok (NULL, "!\t")) != NULL)
	    {
		cvs_output (temp, 0);
		cvs_output (":", 0);

		if ((temp = strtok (NULL, ",\t")) != NULL)
		{
		    if (temp[strlen (temp) - 2] == ':')
			temp[strlen (temp) - 2] = '\0';

		    cvs_output (temp, 0);
		    cvs_output (" ", 0);
		}
	    }
	    cvs_output ("\n", 0);
	}
	else if (cvs_acl_default_permissions)
	{
	    cvs_output ("| defaults ", 0);
	    cvs_output ("ALL: ", 0);
	    cvs_output (cvs_acl_default_permissions, 0);
	}
	else
	    cvs_output ("default:p (no perms)", 0);
    }
    cvs_output ("\n", 0);
	
    while (c >= 0)  {
	free (printedusers[c]);
	c--;
    }
	
    free (line);
}

/* find username
 * returns username with its permissions if user found
 * returns NULL if user not found */
char *findusername (const char *string1, const char *string2)
{
    char *tmp1, *tmp2;

    if (string1 != NULL && string2 != NULL)
    {
	tmp1 = xstrdup (string1);
	tmp2 = strtok (tmp1, ",\t");
	
	do
	{
	    if (strncmp (tmp2, string2, strlen (string2)) == 0 &&
				     tmp2[strlen (string2)] == '!')
	    {
		tmp2 = xstrdup (tmp2);
		free (tmp1);
		return tmp2;
	    }
	    tmp2 = strtok (NULL, ",\t");
	}
	while (tmp2 != NULL);

	free (tmp1);
	
	return NULL;
    }
    else
	return NULL;
}

/* find user name in group file
 * returns group name if user found
 * returns NULL if user not found */
char *findgroupname (const char *string1, const char *string2)
{
    char *tmp1, *tmp2;
    char *grpname;
    
    if (string1 != NULL && string2 != NULL)
    {
	tmp1 = xstrdup (string1);
	grpname = strtok (tmp1, ":\t");

	while (tmp2 = strtok(NULL, ",\t"))
	{
	    if (strcmp (tmp2, string2) == 0)
	    {
		grpname = xstrdup (grpname);
		free (tmp1);
		return grpname;
	    }
	}
	
	free (tmp1);
	
	return NULL;
    }
    else
	return NULL;
}
