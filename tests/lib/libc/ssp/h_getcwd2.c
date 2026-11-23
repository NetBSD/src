#include <err.h>
#include <errno.h>
#include <unistd.h>

char*
getcwd(char* buf, size_t buflen)
{
       errno = ENOSYS;
       return NULL;
}
int
main(void)
{
       char buf[256];  
       if (getcwd(buf, sizeof buf) == NULL)
               err(1, "getcwd failed");
       return 0;
}
