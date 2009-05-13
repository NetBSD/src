/* Interface to "cvs watch add", "cvs watchers", and related features

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

extern const char *const watch_usage[];

/* Flags to pass between the various functions making up the
   add/remove code.  All in a single structure in case there is some
   need to make the code reentrant some day.  */

struct addremove_args {
    /* A flag for each watcher type.  */
    int edit;
    int unedit;
    int commit;

    /* Are we adding or removing (non-temporary) edit,unedit,and/or commit
       watches?  */
    int adding;

    /* Should we add a temporary edit watch?  */
    int add_tedit;
    /* Should we add a temporary unedit watch?  */
    int add_tunedit;
    /* Should we add a temporary commit watch?  */
    int add_tcommit;

    /* Should we remove all temporary watches?  */
    int remove_temp;

    /* Should we set the default?  This is here for passing among various
       routines in watch.c (a good place for it if there is ever any reason
       to make the stuff reentrant), not for watch_modify_watchers. 
       This is only set if there are no arguments specified, e.g. 'cvs watch add' */
    int setting_default;

    /* List of directories specified on the command line, to set the
       default attributes. */
    const char ** dirs;
    int num_dirs;

    /* Is this recursive? */
    int local;

};

/* Modify the watchers for FILE.  *WHAT tells what to do to them.
   If FILE is NULL, modify default args (WHAT->SETTING_DEFAULT is
   not used).  */
void watch_modify_watchers (const char *file, struct addremove_args *what);

int watch_add (int argc, char **argv);
int watch_remove (int argc, char **argv);
