/*	$NetBSD: qsubst.c,v 1.4 2001/04/22 05:35:35 simonb Exp $	*/

/*
 * qsubst -- designed for renaming routines existing in a whole bunch
 *  of files.  Needs -ltermcap.
 *
 * Usage:
 *
 * qsubst str1 str2 [ options ]
 *
 * qsubst reads its options (see below) to get a list of files.  For
 *  each file on this list, it then replaces str1 with str2 wherever
 *  possible in that file, depending on user input (see below).  The
 *  result is written back onto the original file.
 *
 * For each possible substitution, the user is prompted with a few
 *  lines before and after the line containing the string to be
 *  substituted.  The string itself is displayed using the terminal's
 *  standout mode, if any.  Then one character is read from the
 *  terminal.  This is then interpreted as follows (this is designed to
 *  be like Emacs' query-replace-string):
 *
 *	space	replace this occurrence and go on to the next one
 *	.	replace this occurrence and don't change any more in
 *		this file (ie, go on to the next file).
 *	,	tentatively replace this occurrence.  The lines as they
 *		would look if the substitution were made are printed
 *		out.  Then another character is read and it is used to
 *		decide the result (possibly undoing the tentative
 *		replacement).
 *	n	don't change this one, but go on to the next one
 *	^G	don't change this one or any others in this file, but
 *		instead go on to the next file.
 *	!	change the rest in this file without asking, then go on
 *		to the next file (at which point qsubst will start
 *		asking again).
 *	?	print out the current filename and ask again.
 *
 * The first two arguments to qsubst are always the string to replace
 *  and the string to replace it with.  The options are as follows:
 *
 *	-w	The search string is considered as a C symbol; it must
 *		be bounded by non-symbol characters.  This option
 *		toggles.  (`w' for `word'.)
 *	-!	Enter ! mode automatically at the beginning of each
 *		file.
 *	-go	Same as -!
 *	-noask	Same as -!
 *	-nogo	Negate -go
 *	-ask	Negate -noask (same as -nogo)
 *	-cN	(N is a number) Give N lines of context above and below
 *		the line with the match when prompting the user.
 *	-CAN	(N is a number) Give N lines of context above the line
 *		with the match when prompting the user.
 *	-CBN	(N is a number) Give N lines of context below the line
 *		with the match when prompting the user.
 *	-f filename
 *		The filename following the -f argument is one of the
 *		files qsubst should perform substitutions in.
 *	-F filename
 *		qsubst should read the named file to get the names of
 *		files to perform substitutions in.  The names should
 *		appear one to a line.
 *
 * The default amount of context is -c2, that is, two lines above and
 *  two lines below the line with the match.
 *
 * Arguments not beginning with a - sign in the options field are
 *  implicitly preceded by -f.  Thus, -f is really needed only when the
 *  file name begins with a - sign.
 *
 * qsubst reads its options in order and processes files as it gets
 *  them.  This means, for example, that a -go will affect only files
 *  from -f or -F options appearing after the -go option.
 *
 * The most context you can get is ten lines each, above and below
 *  (corresponding to -c10).
 *
 * Str1 is limited to 512 characters; there is no limit on the size of
 *  str2.  Neither one may contain a NUL.
 *
 * NULs in the file may cause qsubst to make various mistakes.
 *
 * If any other program modifies the file while qsubst is running, all
 *  bets are off.
 *
 * This program is in the public domain.  Anyone may use it in any way
 *  for any purpose.  Of course, it's also up to you to determine
 *  whether what it does is suitable for you; the above comments may
 *  help, but I can't promise they're accurate.  It's free, and you get
 *  what you pay for.
 *
 * If you find any bugs I would appreciate hearing about them,
 *  especially if you also fix them.
 *
 *					der Mouse
 *
 *			       mouse@rodents.montreal.qc.ca
 */

#include <sys/file.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <termcap.h>
#include <termios.h>
#include <unistd.h>

extern const char *__progname;

#define MAX_C_A 10
#define MAX_C_B 10
#define BUF_SIZ 1024

static int debugging;
static FILE *tempf;
static long tbeg;
static FILE *workf;
static char *str1;
static char *str2;
static int s1l;
static int s2l;
static long nls[MAX_C_A+1];
static char buf[(BUF_SIZ*2)+2];
static char *bufp;
static char *bufp0;
static char *bufpmax;
static int rahead;
static int cabove;
static int cbelow;
static int wordmode;
static int flying;
static int flystate;
static int allfly;
static const char *nullstr = "";
static int ul_;
static char *current_file;
static const char *beginul;
static const char *endul;
static char tcp_buf[1024];
static char cap_buf[1024];
static struct termios orig_tio;

static void tstp_self(void)
{
 void (*old_tstp)(int);
 int mask;

 mask = sigblock(0);
 kill(getpid(),SIGTSTP);
 old_tstp = signal(SIGTSTP,SIG_DFL);
 sigsetmask(mask&~sigmask(SIGTSTP));
 signal(SIGTSTP,old_tstp);
}

/* ARGSUSED */
static void sigtstp(int sig)
{
 struct termios tio;

 if (tcgetattr(0,&tio) < 0)
  { tstp_self();
    return;
  }
 tcsetattr(0,TCSAFLUSH|TCSASOFT,&orig_tio);
 tstp_self();
 tcsetattr(0,TCSADRAIN|TCSASOFT,&tio);
}

static void limit_above_below(void)
{
 if (cabove > MAX_C_A)
  { cabove = MAX_C_A;
  }
 if (cbelow > MAX_C_B)
  { cbelow = MAX_C_B;
  }
}

static int issymchar(char c)
{
 return( isascii(c) &&
	 ( isalnum(c) ||
	   (c == '_') ||
	   (c == '$') ) );
}

static int foundit(void)
{
 if (wordmode)
  { return( !issymchar(bufp[-1]) &&
	    !issymchar(bufp[-2-s1l]) &&
	    !bcmp(bufp-1-s1l,str1,s1l) );
  }
 else
  { return(!bcmp(bufp-s1l,str1,s1l));
  }
}

static int putcharf(int c)
{
 return(putchar(c));
}

static void put_ul(char *s)
{
 if (ul_)
  { for (;*s;s++)
     { printf("_\b%c",*s);
     }
  }
 else
  { tputs(beginul,1,putcharf);
    fputs(s,stdout);
    tputs(endul,1,putcharf);
  }
}

static int getc_cbreak(void)
{
 struct termios tio;
 struct termios otio;
 char c;

 if (tcgetattr(0,&tio) < 0) return(getchar());
 otio = tio;
 tio.c_lflag &= ~(ICANON|ECHOKE|ECHOE|ECHO|ECHONL);
 tio.c_cc[VMIN] = 1;
 tio.c_cc[VTIME] = 0;
 tcsetattr(0,TCSANOW|TCSASOFT,&tio);
 switch (read(0,&c,1))
  { case -1:
       break;
    case 0:
       break;
    case 1:
       break;
  }
 tcsetattr(0,TCSANOW|TCSASOFT,&otio);
 return(c);
}

static int doit(void)
{
 long save;
 int i;
 int lastnl;
 int use_replacement;

 if (flying)
  { return(flystate);
  }
 use_replacement = 0;
 save = ftell(workf);
 do
  { for (i=MAX_C_A-cabove;nls[i]<0;i++) ;
    fseek(workf,nls[i],0);
    for (i=save-nls[i]-rahead;i;i--)
     { putchar(getc(workf));
     }
    put_ul(use_replacement?str2:str1);
    fseek(workf,save+s1l-rahead,0);
    lastnl = 0;
    i = cbelow + 1;
    while (i > 0)
     { int c;
       c = getc(workf);
       if (c == EOF)
	{ clearerr(workf);
	  break;
	}
       putchar(c);
       lastnl = 0;
       if (c == '\n')
	{ i --;
	  lastnl = 1;
	}
     }
    if (! lastnl) printf("\n[no final newline] ");
    fseek(workf,save,0);
    i = -1;
    while (i == -1)
     { switch (getc_cbreak())
	{ case ' ':
	     i = 1;
	     break;
	  case '.':
	     i = 1;
	     flying = 1;
	     flystate = 0;
	     break;
	  case 'n':
	     i = 0;
	     break;
	  case '\7':
	     i = 0;
	     flying = 1;
	     flystate = 0;
	     break;
	  case '!':
	     i = 1;
	     flying = 1;
	     flystate = 1;
	     break;
	  case ',':
	     use_replacement = ! use_replacement;
	     i = -2;
	     printf("(using %s string gives)\n",use_replacement?"new":"old");
	     break;
	  case '?':
	     printf("File is `%s'\n",current_file);
	     break;
	  default:
	     putchar('\7');
	     break;
	}
     }
  } while (i < 0);
 if (i)
  { printf("(replacing");
  }
 else
  { printf("(leaving");
  }
 if (flying)
  { if (flystate == i)
     { printf(" this and all the rest");
     }
    else if (flystate)
     { printf(" this, replacing all the rest");
     }
    else
     { printf(" this, leaving all the rest");
     }
  }
 printf(")\n");
 return(i);
}

static void add_shift(long *a, long e, int n)
{
 int i;

 n --;
 for (i=0;i<n;i++)
  { a[i] = a[i+1];
  }
 a[n] = e;
}

static void process_file(char *fn)
{
 int i;
 long n;
 int c;

 workf = fopen(fn,"r+");
 if (workf == NULL)
  { fprintf(stderr,"%s: cannot read %s\n",__progname,fn);
    return;
  }
 printf("(file: %s)\n",fn);
 current_file = fn;
 for (i=0;i<=MAX_C_A;i++)
  { nls[i] = -1;
  }
 nls[MAX_C_A] = 0;
 tbeg = -1;
 if (wordmode)
  { bufp0 = &buf[1];
    rahead = s1l + 1;
    buf[0] = '\0';
  }
 else
  { bufp0 = &buf[0];
    rahead = s1l;
  }
 if (debugging)
  { printf("[rahead = %d, bufp0-buf = %ld]\n",rahead,(long)(bufp0-&buf[0]));
  }
 n = 0;
 bufp = bufp0;
 bufpmax = &buf[sizeof(buf)-s1l-2];
 flying = allfly;
 flystate = 1;
 while (1)
  { c = getc(workf);
    if (c == EOF)
     { if (tbeg >= 0)
	{ if (bufp > bufp0) fwrite(bufp0,1,bufp-bufp0,tempf);
	  fseek(workf,tbeg,0);
	  n = ftell(tempf);
	  fseek(tempf,0L,0);
	  for (;n;n--)
	   { putc(getc(tempf),workf);
	   }
	  fflush(workf);
	  ftruncate(fileno(workf),ftell(workf));
	}
       fclose(workf);
       return;
     }
    *bufp++ = c;
    n ++;
    if (debugging)
     { printf("[got %c, n now %ld, bufp-buf %ld]\n",c,n,(long)(bufp-bufp0));
     }
    if ((n >= rahead) && foundit() && doit())
     { int wbehind;
       if (debugging)
	{ printf("[doing change]\n");
	}
       wbehind = 1;
       if (tbeg < 0)
	{ tbeg = ftell(workf) - rahead;
	  fseek(tempf,0L,0);
	  if (debugging)
	   { printf("[tbeg set to %d]\n",(int)tbeg);
	   }
	  wbehind = 0;
	}
       if (bufp[-1] == '\n') add_shift(nls,ftell(workf),MAX_C_A+1);
       if ((n > rahead) && wbehind)
	{ fwrite(bufp0,1,n-rahead,tempf);
	  if (debugging)
	   { printf("[writing %ld from bufp0]\n",n-rahead);
	   }
	}
       fwrite(str2,1,s2l,tempf);
       n = rahead - s1l;
       if (debugging)
	{ printf("[n now %ld]\n",n);
	}
       if (n > 0)
	{ bcopy(bufp-n,bufp0,n);
	  if (debugging)
	   { printf("[copying %ld back]\n",n);
	   }
	}
       bufp = bufp0 + n;
     }
    else
     { if (bufp[-1] == '\n') add_shift(nls,ftell(workf),MAX_C_A+1);
       if (bufp >= bufpmax)
	{ if (tbeg >= 0)
	   { fwrite(bufp0,1,n-rahead,tempf);
	     if (debugging)
	      { printf("[flushing %ld]\n",n-rahead);
	      }
	   }
	  n = rahead;
	  bcopy(bufp-n,bufp0,n);
	  if (debugging)
	   { printf("[n now %ld]\n[copying %ld back]\n",n,n);
	   }
	  bufp = bufp0 + n;
	}
     }
  }
}

static void process_indir_file(char *fn)
{
 char newfn[1024];
 FILE *f;

 f = fopen(fn,"r");
 if (f == NULL)
  { fprintf(stderr,"%s: cannot read %s\n",__progname,fn);
    return;
  }
 while (fgets(newfn,sizeof(newfn),f) == newfn)
  { newfn[strlen(newfn)-1] = '\0';
    process_file(newfn);
  }
 fclose(f);
}

int main(int ac, char **av)
{
 int skip;
 char *cp;

 if (ac < 3)
  { fprintf(stderr,"Usage: %s str1 str2 [ -w -! -noask -go -f file -F file ]\n",
				__progname);
    exit(1);
  }
 cp = getenv("TERM");
 if (cp == 0)
  { beginul = nullstr;
    endul = nullstr;
  }
 else
  { if (tgetent(tcp_buf,cp) != 1)
     { beginul = nullstr;
       endul = nullstr;
     }
    else
     { cp = cap_buf;
       if (tgetflag("os") || tgetflag("ul"))
	{ ul_ = 1;
	}
       else
	{ ul_ = 0;
	  beginul = tgetstr("us",&cp);
	  if (beginul == 0)
	   { beginul = tgetstr("so",&cp);
	     if (beginul == 0)
	      { beginul = nullstr;
		endul = nullstr;
	      }
	     else
	      { endul = tgetstr("se",&cp);
	      }
	   }
	  else
	   { endul = tgetstr("ue",&cp);
	   }
	}
     }
  }
  { static char tmp[] = "/tmp/qsubst.XXXXXX";
    int fd;
    fd = mkstemp(&tmp[0]);
    if (fd < 0)
     { fprintf(stderr,"%s: cannot create temp file: %s\n",__progname,strerror(errno));
       exit(1);
     }
    tempf = fdopen(fd,"w+");
  }
 if ( (access(av[1],R_OK|W_OK) == 0) &&
      (access(av[ac-1],R_OK|W_OK) < 0) &&
      (access(av[ac-2],R_OK|W_OK) < 0) )
  { fprintf(stderr,"%s: argument order has changed, it's now: str1 str2 files...\n",__progname);
  }
 str1 = av[1];
 str2 = av[2];
 av += 2;
 ac -= 2;
 s1l = strlen(str1);
 s2l = strlen(str2);
 if (s1l > BUF_SIZ)
  { fprintf(stderr,"%s: search string too long (max %d chars)\n",__progname,BUF_SIZ);
    exit(1);
  }
 tcgetattr(0,&orig_tio);
 signal(SIGTSTP,sigtstp);
 allfly = 0;
 cabove = 2;
 cbelow = 2;
 skip = 0;
 for (ac--,av++;ac;ac--,av++)
  { if (skip > 0)
     { skip --;
       continue;
     }
    if (**av == '-')
     { ++*av;
       if (!strcmp(*av,"debug"))
	{ debugging ++;
	}
       else if (!strcmp(*av,"w"))
	{ wordmode = ! wordmode;
	}
       else if ( (strcmp(*av,"!") == 0) ||
		 (strcmp(*av,"go") == 0) ||
		 (strcmp(*av,"noask") == 0) )
	{ allfly = 1;
	}
       else if ( (strcmp(*av,"nogo") == 0) ||
		 (strcmp(*av,"ask") == 0) )
	{ allfly = 0;
	}
       else if (**av == 'c')
	{ cabove = atoi(++*av);
	  cbelow = cabove;
	  limit_above_below();
	}
       else if (**av == 'C')
	{ ++*av;
	  if (**av == 'A')
	   { cabove = atoi(++*av);
	     limit_above_below();
	   }
	  else if (**av == 'B')
	   { cbelow = atoi(++*av);
	     limit_above_below();
	   }
	  else
	   { fprintf(stderr,"%s: -C must be -CA or -CB\n",__progname);
	   }
	}
       else if ( (strcmp(*av,"f") == 0) ||
		 (strcmp(*av,"F") == 0) )
	{ if (++skip >= ac)
	   { fprintf(stderr,"%s: -%s what?\n",__progname,*av);
	   }
	  else
	   { if (**av == 'f')
	      { process_file(av[skip]);
	      }
	     else
	      { process_indir_file(av[skip]);
	      }
	   }
	}
     }
    else
     { process_file(*av);
     }
  }
 exit(0);
}

