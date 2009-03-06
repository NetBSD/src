/*	$NetBSD: extern.h,v 1.5 2009/03/06 11:45:03 tteras Exp $	*/



void parse_init __P((void));
int parse __P((FILE **));
int parse_string __P((char *));

int setkeymsg __P((char *, size_t *));
int sendkeymsg __P((char *, size_t));

int yylex __P((void));
int yyparse __P((void));
void yyfatal __P((const char *));
void yyerror __P((const char *));

u_int32_t *sendkeymsg_spigrep __P((unsigned int, struct addrinfo *,
				   struct addrinfo *, int *));

extern int f_rfcmode;
extern int lineno;
extern int last_msg_type;
extern u_int32_t last_priority;
extern int exit_now;

extern u_char m_buf[BUFSIZ];
extern u_int m_len;
extern int f_debug;

#ifdef HAVE_PFKEY_POLICY_PRIORITY
extern int last_msg_type;
extern u_int32_t last_priority;
#endif
