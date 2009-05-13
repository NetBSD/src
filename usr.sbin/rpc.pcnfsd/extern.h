/*	$NetBSD: extern.h,v 1.1.54.1 2009/05/13 19:20:38 jym Exp $	*/

extern	int		interrupted;
extern	pr_list		printers;
extern	pr_queue	queue;
extern	char		sp_name[1024];

void		add_printer_alias __P((char *, char *, char *));
void		add_cache_entry __P((struct passwd *));
int		build_pr_list __P((void));
pirstat		build_pr_queue __P((printername, username, int, int *, int *));
int		check_cache __P((char *, char *, int *, int *));
void		free_mapreq_results __P((mapreq_res));
void            fillin_extra_groups __P((char *, u_int, int *, u_int[]));
#ifdef USE_YP
char	       *find_entry __P((const char *, const char *));
#endif
void		free_pr_list_item __P((pr_list));
void		free_pr_queue_item __P((pr_queue));
struct passwd  *get_password __P((char *));
pirstat		get_pr_status __P((printername, bool_t *, bool_t *, int *,
		    bool_t *, char *));
void	       *grab __P((int));
pcrstat		pr_cancel __P((char *, char *, char *));
pirstat		pr_init __P((char *, char *, char **));
psrstat		pr_start __P((void));
psrstat		pr_start2 __P((char *, char *, char *, char *, char *,
		    char **));
void		run_ps630 __P((char *, char *));
void     	scramble __P((char *, char *));
int		strembedded __P((const char *, const char *));
FILE	       *su_popen __P((char *, char *, int));
int		su_pclose __P((FILE *));
#ifdef WTMP
void		wlogin __P((char *, struct svc_req *));
#endif
