
char	*memcpy(char *to, char *from, unsigned int size)
{
    bcopy(from, to, size);
    return to;
}
     
