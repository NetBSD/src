void init_prom_calls __P((void));
int prom_open __P((char*, int));
void OSFpal __P((void));
void halt __P((void));
u_int64_t prom_dispatch __P((int, ...));
int cpu_number __P((void));
void switch_palcode __P((void));
