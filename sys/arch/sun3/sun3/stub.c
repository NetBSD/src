
ether_output() {}
     

char	*memcpy(char *from, char *to, unsigned int size)
{
    bcopy(to, from, size);
    return to;
}

