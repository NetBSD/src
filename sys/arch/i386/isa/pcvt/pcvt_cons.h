int pccngetc __P((dev_t));
void pccnputc __P((dev_t, int));
void pccnpollc __P((dev_t, int));
void pccninit __P((struct consdev *));
