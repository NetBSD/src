#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int c, char *argv[]) {
  int i;
  char filename[64];

  for (i=0; i<256; i++) {
    sprintf(filename, "/dev/so%c%c", 'a'+i/26, 'a'+i%26);
    sprintf(filename, "/dev/so%i", i);
    printf("%i: %s (%i/%i)\n", i, filename, 232, i);
    if (mknod(filename, S_IFBLK, (232<<8)|(i&255))==-1) {
      printf("mknod() failed: errno %i\n", errno);
      return -1;
    }
    getchar();
  }

  return 0;
}

