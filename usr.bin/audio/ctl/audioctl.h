extern audio_info_t info;

int audioctl __P((int, char *[]));
int audiorecord __P((int, char *[]));
int audioplay __P((int, char *[]));
void audioctl_write __P((int, int, char *[]));
