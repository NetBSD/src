
#define iprintf(ident,...)	do { 		\
	for (size_t i = 0; i < ident; i++)	\
		putc(' ', stdout);		\
	printf(__VA_ARGS__);			\
} while (/*CONSTCOND*/0)

extern int verbose;

int  cmd_dumptree(int argc, char *argv[]);
int  cmd_login(int argc, char *argv[]);
int  cmd_logout(int argc, char *argv[]);
int  cmd_lookup(int argc, char *argv[]);
int  cmd_print(int argc, char *argv[]);
int  cmd_view(int argc, char *argv[]);
void login_usage(void);
void logout_usage(void);
void lookup_usage(void);
void print_usage(void);
void view_usage(void);
#ifdef APPLE
extern int loadsmbvfs();
#endif
