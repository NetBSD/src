
#define iprintf(ident,...)	do { 		\
	for (size_t i = 0; i < ident; i++)	\
		putc(' ', stdout);		\
	printf(__VA_ARGS__);			\
} while (/*CONSTCOND*/0)

extern int verbose;

int  cmd_dumptree(int, char *[]);
int  cmd_login(int, char *[]);
int  cmd_logout(int, char *[]);
int  cmd_lookup(int, char *[]);
int  cmd_print(int, char *[]);
int  cmd_view(int, char *[]);
void login_usage(void) __dead;
void logout_usage(void) __dead;
void lookup_usage(void) __dead;
void print_usage(void) __dead;
void view_usage(void) __dead;
#ifdef APPLE
extern int loadsmbvfs(void);
#endif
