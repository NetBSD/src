
char	*memcpy(char *from, char *to, unsigned int size)
{
    bcopy(to, from, size);
    return to;
}

extern int ufs_mountroot();
int (*mountroot)() = ufs_mountroot;
     
