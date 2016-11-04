extern char bar[];
extern char size_of_bar asm ("bar@SIZE");
char *bar_size = &size_of_bar;
