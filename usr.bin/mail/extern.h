/*	$NetBSD: extern.h,v 1.20 2005/07/19 01:38:38 christos Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)extern.h	8.2 (Berkeley) 4/20/95 
 *	$NetBSD: extern.h,v 1.20 2005/07/19 01:38:38 christos Exp $
 */

struct name;
struct name *cat(struct name *, struct name *);
struct name *delname(struct name *, char []);
struct name *elide(struct name *);
struct name *extract(char [], int);
struct grouphead;
struct name *gexpand(struct name *, struct grouphead *, int, int);
struct name *nalloc(char [], int);
struct header;
struct name *outof(struct name *, FILE *, struct header *);
struct name *put(struct name *, struct name *);
struct name *tailof(struct name *);
struct name *usermap(struct name *);
FILE	*Fdopen(int, const char *);
FILE	*Fopen(const char *, const char *);
FILE	*Popen(const char *, const char *);
FILE	*collect(struct header *, int);
char	*copy(char *, char *);
char	*copyin(char *, char **);
char	*detract(struct name *, int);
const char *expand(const char *);
const char *getdeadletter(void);
const	char *getname(int);
struct message;
char	*hfield(const char [], const struct message *);
FILE	*infix(struct header *, FILE *);
char	*ishfield(const char [], char[], const char *);
char	*name1(struct message *, int);
char	*nameof(struct message *, int);
char	*nextword(char *, char *);
char	*readtty(const char [], char []);
char 	*reedit(char *);
FILE	*run_editor(FILE *, off_t, int, int);
void	*salloc(int);
char	*savestr(const char *);
FILE	*setinput(const struct message *);
char	*skin(char *);
char	*skip_comment(char *);
char	*snarf(char [], int *);
const	char *username(void);
char	*value(const char []);
char	*vcopy(const char []);
char	*yankword(char *, char []);
int	 Fclose(FILE *);
int	 More(void *);
int	 Pclose(FILE *);
int	 Respond(void *);
int	 Type(void *);
int	 _Respond(int []);
int	 _respond(int *);
void	 alter(char *);
int	 alternates(void *);
void	 announce(void);
int	 append(struct message *, FILE *);
int	 argcount(char **);
void	 assign(const char [], const char []);
int	 bangexp(char *);
int	 blankline(char []);
void	 brokpipe(int);
int	 charcount(char *, int);
int	 check(int, int);
void	 clob1(int);
int	 clobber(void *);
void	 close_all_files(void);
int	 cmatch(const char *, char *);
void	 collhup(int);
void	 collint(int);
void	 collstop(int);
void	 commands(void);
int	 copycmd(void *);
int	 core(void *);
int	 count(struct name *);
int	 delete(void *);
int	 delm(int []);
int	 deltype(void *);
void	 demail(void);
int	 dosh(void *);
int	 dot_lock(const char *, int, FILE *, const char *);
void	 dot_unlock(const char *);
int	 echo(void *);
int	 edit1(int *, int);
int	 editor(void *);
void	 edstop(void);
int	 elsecmd(void *);
int	 endifcmd(void *);
int	 evalcol(int);
int	 execute(char [], int);
int	 exwrite(const char [], FILE *, int);
void	 fail(char [], const char []);
int	 file(void *);
struct grouphead *
	 findgroup(char []);
void	 findmail(const char *, char *);
int	 first(int, int);
void	 fixhead(struct header *, struct name *);
void	 fmt(const char *, struct name *, FILE *, int);
int	 folders(void *);
int	 forward(char [], FILE *, char *, int);
void	 free_child(int);
int	 from(void *);
off_t	 fsize(FILE *);
int	 getfold(char *);
int	 gethfield(FILE *, char [], int, char **);
int	 getmsglist(char *, int *, int);
int	 getrawlist(const char [], char **, int);
int	 getuserid(char []);
int	 grabh(struct header *, int);
int	 group(void *);
void	 hangup(int);
int	 hash(const char *);
void	 hdrstop(int);
int	 headers(void *);
int	 help(void *);
void	 holdsigs(void);
int	 ifcmd(void *);
int	 igfield(void *);
struct ignoretab;
int	 ignore1(char *[], struct ignoretab *, const char *);
int	 igshow(struct ignoretab *, const char *);
void	 intr(int);
int	 inc(void *);
int	 incfile(void);
int	 isdate(const char []);
int	 isdir(const char []);
int	 isfileaddr(char *);
int	 ishead(char []);
int	 isign(const char *, struct ignoretab []);
int	 isprefix(char *, const char *);
void	 istrcpy(char *, const char *);
const struct cmd *
	 lex(char []);
void	 load(const char *);
struct var *
	 lookup(const char []);
int	 mail(struct name *,
	      struct name *, struct name *, struct name *, char *);
void	 mail1(struct header *, int);
void	 makemessage(FILE *, int);
void	 mark(int);
int	 markall(char [], int);
int	 matchsender(char *, int);
int	 matchsubj(char *, int);
int	 mboxit(void *);
int	 member(char *, struct ignoretab *);
void	 mesedit(FILE *, int);
void	 mespipe(FILE *, char []);
int	 messize(void *);
int	 metamess(int, int);
int	 more(void *);
int	 newfileinfo(int);
int	 next(void *);
int	 null(void *);
struct headline;
void	 parse(char [], struct headline *, char []);
int	 pcmdlist(void *);
int	 pdot(void *);
void	 prepare_child(sigset_t *, int, int);
int	 preserve(void *);
void	 prettyprint(struct name *);
void	 printgroup(char []);
void	 printhead(int);
int	 puthead(struct header *, FILE *, int);
int	 putline(FILE *, const char *, int);
int	 pversion(void *);
void	 quit(void);
int	 quitcmd(void *);
int	 upcase(int);
int	 readline(FILE *, char *, int);
void	 register_file(FILE *, int, int);
void	 regret(int);
void	 relsesigs(void);
int	 respond(void *);
int	 retfield(void *);
int	 rexit(void *);
int	 rm(char *);
int	 run_command(const char *, sigset_t *, int, int, ...);
int	 save(void *);
int	 Save(void *);
int	 save1(char [], int, const char *, struct ignoretab *);
void	 savedeadletter(FILE *);
int	 saveigfield(void *);
int	 savemail(const char [], FILE *);
int	 saveretfield(void *);
int	 scan(char **);
void	 scaninit(void);
int	 schdir(void *);
int	 screensize(void);
int	 scroll(void *);
int	 sendmessage(struct message *, FILE *, struct ignoretab *,
    const char *);
int	 sendmail(void *);
int	 set(void *);
int	 setfile(const char *);
void	 setmsize(int);
void	 setptr(FILE *, off_t);
void	 setscreensize(void);
int	 shell(void *);
void	 sigchild(int);
void	 sort(char **);
int	 source(void *);
void	 spreserve(void);
void	 sreset(void);
int	 start_command(const char *, sigset_t *, int, int, ...);
void	 statusput(struct message *, FILE *, const char *);
void	 stop(int);
int	 stouch(void *);
int	 swrite(void *);
void	 tinit(void);
int	 top(void *);
void	 touch(struct message *);
void	 ttyint(int);
void	 ttystop(int);
int	 type(void *);
int	 type1(int *, int, int);
int	 undeletecmd(void *);
void	 unmark(int);
const char **unpack(struct name *);
int	 unread(void *);
void	 unregister_file(FILE *);
int	 unset(void *);
int	 unstack(void);
void	 v_free(char *);
int	 visual(void *);
int	 wait_child(int);
int	 wait_command(int);
int	 writeback(FILE *);
