/*	$NetBSD: extern.h,v 1.4 2018/01/23 21:06:25 sevan Exp $	*/

extern	int		interrupted;
extern	pr_list		printers;
extern	pr_queue	queue;
extern	char		sp_name[1024];

void		add_printer_alias(char *, char *, char *);
void		add_cache_entry(struct passwd *);
int		build_pr_list(void);
pirstat		build_pr_queue(printername, username, int, int *, int *);
int		check_cache(char *, char *, int *, int *);
void		free_mapreq_results(mapreq_res);
void            fillin_extra_groups(char *, u_int, int *, u_int[]);
#ifdef USE_YP
char	       *find_entry(const char *, const char *);
#endif
void		free_pr_list_item(pr_list);
void		free_pr_queue_item(pr_queue);
struct passwd  *get_password(char *);
pirstat		get_pr_status(printername, bool_t *, bool_t *, int *,
		    bool_t *, char *, size_t);
void	       *grab(int);
pcrstat		pr_cancel(char *, char *, char *);
pirstat		pr_init(char *, char *, char **);
psrstat		pr_start(void);
psrstat		pr_start2(char *, char *, char *, char *, char *,
		    char **);
void		run_ps630(char *, char *);
void     	scramble(char *, char *);
int		strembedded(const char *, const char *);
FILE	       *su_popen(char *, char *, int);
int		su_pclose(FILE *);
#ifdef WTMP
void		wlogin(char *, struct svc_req *);
#endif
