/*
 *	$Id: genpar.c,v 1.4 1994/02/11 07:01:34 chopps Exp $
 */

#define bset(i,b) ((i & (1<<b))?1:0)

main()
{
  int i;
  
  printf ("u_char even_parity[] = {\n   ");
  for (i = 0; i < 0x80; i++)
    {
      unsigned char par = bset(i,0)+bset(i,1)+bset(i,2)+bset(i,3)+bset(i,4)+
      		   bset(i,5)+bset(i,6)+bset(i,7);

      printf ("%2d, ", par & 1);

      if ((i & 15) == 15)
        printf ("\n   ");
    }
  printf ("};\n");
}
