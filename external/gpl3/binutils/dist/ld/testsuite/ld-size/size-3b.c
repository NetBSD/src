static char bar[10];
extern char size_of_bar asm ("bar@SIZE");
char *bar_size = &size_of_bar;
char *bar_p = bar;
