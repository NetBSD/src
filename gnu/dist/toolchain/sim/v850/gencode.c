#include <stdio.h>
#include <ctype.h>
#include "ansidecl.h"
#include "opcode/v850.h"
#include <limits.h>

static void write_header PARAMS ((void));
static void write_opcodes PARAMS ((void));
static void write_template PARAMS ((void));

long Opcodes[512];
static int curop=0;

int
main (argc, argv)
     int argc;
     char *argv[];
{
  if ((argc > 1) && (strcmp (argv[1], "-h") == 0))
    write_header();
  else if ((argc > 1) && (strcmp (argv[1], "-t") == 0))
    write_template ();
  else
    write_opcodes();
  return 0;
}


static void
write_header ()
{
  struct v850_opcode *opcode;

  for (opcode = (struct v850_opcode *)v850_opcodes; opcode->name; opcode++)
    printf("int OP_%X PARAMS ((void));\t\t/* %s */\n",
	   opcode->opcode, opcode->name);
}


/* write_template creates a file all required functions, ready */
/* to be filled out */

static void
write_template ()
{
  struct v850_opcode *opcode;
  int i,j;

  printf ("#include \"sim-main.h\"\n");
  printf ("#include \"v850_sim.h\"\n");
  printf ("#include \"simops.h\"\n");

  for (opcode = (struct v850_opcode *)v850_opcodes; opcode->name; opcode++)
    {
      printf("/* %s */\nvoid\nOP_%X (void)\n{\n", opcode->name, opcode->opcode);
	  
      /* count operands */
      j = 0;
      for (i = 0; i < 6; i++)
	{
	  int flags = v850_operands[opcode->operands[i]].flags;

	  if (flags & (V850_OPERAND_REG | V850_OPERAND_SRG | V850_OPERAND_CC))
	    j++;
	}
      switch (j)
	{
	case 0:
	  printf ("printf(\"   %s\\n\");\n", opcode->name);
	  break;
	case 1:
	  printf ("printf(\"   %s\\t%%x\\n\", OP[0]);\n", opcode->name);
	  break;
	case 2:
	  printf ("printf(\"   %s\\t%%x,%%x\\n\",OP[0],OP[1]);\n",
		  opcode->name);
	  break;
	case 3:
	  printf ("printf(\"   %s\\t%%x,%%x,%%x\\n\",OP[0],OP[1],OP[2]);\n",
		  opcode->name);
	  break;
	default:
	  fprintf (stderr,"Too many operands: %d\n", j);
	}
      printf ("}\n\n");
    }
}

static void
write_opcodes ()
{
  struct v850_opcode *opcode;
  int i, j;
  int numops;
  
  /* write out opcode table */
  printf ("#include \"sim-main.h\"\n");
  printf ("#include \"v850_sim.h\"\n");
  printf ("#include \"simops.h\"\n\n");
  printf ("struct simops Simops[] = {\n");
  
  for (opcode = (struct v850_opcode *)v850_opcodes; opcode->name; opcode++)
    {
      printf ("  { 0x%x,0x%x,OP_%X,",
	      opcode->opcode, opcode->mask, opcode->opcode);
      
      Opcodes[curop++] = opcode->opcode;

      /* count operands */
      j = 0;
      for (i = 0; i < 6; i++)
	{
	  int flags = v850_operands[opcode->operands[i]].flags;

	  if (flags & (V850_OPERAND_REG | V850_OPERAND_SRG | V850_OPERAND_CC))
	    j++;
	}
      printf ("%d,{",j);
	  
      j = 0;
      numops = 0;
      for (i = 0; i < 6; i++)
	{
	  int flags = v850_operands[opcode->operands[i]].flags;
	  int shift = v850_operands[opcode->operands[i]].shift;

	  if (flags & (V850_OPERAND_REG | V850_OPERAND_SRG | V850_OPERAND_CC))
	    {
	      if (j)
		printf (", ");
	      printf ("%d,%d,%d", shift,
		      v850_operands[opcode->operands[i]].bits,flags);
	      j = 1;
	      numops++;
	    }
	}

      switch (numops)
	{
	case 0:
	  printf ("0,0,0");
	case 1:
	  printf (",0,0,0");
	}

      printf ("}},\n");
    }
  printf ("{ 0,0,NULL,0,{0,0,0,0,0,0}},\n};\n");
}
