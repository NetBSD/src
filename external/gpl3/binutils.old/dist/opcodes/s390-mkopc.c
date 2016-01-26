/* s390-mkopc.c -- Generates opcode table out of s390-opc.txt
   Copyright 2000, 2001, 2003, 2005, 2007, 2008 Free Software Foundation, Inc.
   Contributed by Martin Schwidefsky (schwidefsky@de.ibm.com).

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Taken from opcodes/s390.h */
enum s390_opcode_mode_val
  {
    S390_OPCODE_ESA = 0,
    S390_OPCODE_ZARCH
  };

enum s390_opcode_cpu_val
  {
    S390_OPCODE_G5 = 0,
    S390_OPCODE_G6,
    S390_OPCODE_Z900,
    S390_OPCODE_Z990,
    S390_OPCODE_Z9_109,
    S390_OPCODE_Z9_EC,
    S390_OPCODE_Z10,
    S390_OPCODE_Z196
  };

struct op_struct
  {
    char  opcode[16];
    char  mnemonic[16];
    char  format[16];
    int   mode_bits;
    int   min_cpu;
    
    unsigned long long sort_value;
    int   no_nibbles;
  };

struct op_struct *op_array;
int max_ops;
int no_ops;

static void
createTable (void)
{
  max_ops = 256;
  op_array = malloc (max_ops * sizeof (struct op_struct));
  no_ops = 0;
}

/* `insertOpcode': insert an op_struct into sorted opcode array.  */

static void
insertOpcode (char *opcode, char *mnemonic, char *format,
	      int min_cpu, int mode_bits)
{
  char *str;
  unsigned long long sort_value;
  int no_nibbles;
  int ix, k;

  while (no_ops >= max_ops)
    {
      max_ops = max_ops * 2;
      op_array = realloc (op_array, max_ops * sizeof (struct op_struct));
    }

  sort_value = 0;
  str = opcode;
  for (ix = 0; ix < 16; ix++)
    {
      if (*str >= '0' && *str <= '9')
	sort_value = (sort_value << 4) + (*str - '0');
      else if (*str >= 'a' && *str <= 'f')
	sort_value = (sort_value << 4) + (*str - 'a' + 10);
      else if (*str >= 'A' && *str <= 'F')
	sort_value = (sort_value << 4) + (*str - 'A' + 10);
      else if (*str == '?')
	sort_value <<= 4;
      else
	break;
      str ++;
    }
  sort_value <<= 4*(16 - ix);
  sort_value += (min_cpu << 8) + mode_bits;
  no_nibbles = ix;
  for (ix = 0; ix < no_ops; ix++)
    if (sort_value > op_array[ix].sort_value)
      break;
  for (k = no_ops; k > ix; k--)
    op_array[k] = op_array[k-1];
  strcpy(op_array[ix].opcode, opcode);
  strcpy(op_array[ix].mnemonic, mnemonic);
  strcpy(op_array[ix].format, format);
  op_array[ix].sort_value = sort_value;
  op_array[ix].no_nibbles = no_nibbles;
  op_array[ix].min_cpu = min_cpu;
  op_array[ix].mode_bits = mode_bits;
  no_ops++;
}

struct s390_cond_ext_format
{
  char nibble;
  char extension[4];
};

/* The mnemonic extensions for conditional jumps used to replace
   the '*' tag.  */
#define NUM_COND_EXTENSIONS 20
const struct s390_cond_ext_format s390_cond_extensions[NUM_COND_EXTENSIONS] =
{ { '1', "o" },    /* jump on overflow / if ones */
  { '2', "h" },    /* jump on A high */
  { '2', "p" },    /* jump on plus */
  { '3', "nle" },  /* jump on not low or equal */
  { '4', "l" },    /* jump on A low */
  { '4', "m" },    /* jump on minus / if mixed */
  { '5', "nhe" },  /* jump on not high or equal */
  { '6', "lh" },   /* jump on low or high */
  { '7', "ne" },   /* jump on A not equal B */
  { '7', "nz" },   /* jump on not zero / if not zeros */
  { '8', "e" },    /* jump on A equal B */
  { '8', "z" },    /* jump on zero / if zeros */
  { '9', "nlh" },  /* jump on not low or high */
  { 'a', "he" },   /* jump on high or equal */
  { 'b', "nl" },   /* jump on A not low */
  { 'b', "nm" },   /* jump on not minus / if not mixed */
  { 'c', "le" },   /* jump on low or equal */
  { 'd', "nh" },   /* jump on A not high */
  { 'd', "np" },   /* jump on not plus */
  { 'e', "no" },   /* jump on not overflow / if not ones */
};

/* The mnemonic extensions for conditional branches used to replace
   the '$' tag.  */
#define NUM_CRB_EXTENSIONS 12
const struct s390_cond_ext_format s390_crb_extensions[NUM_CRB_EXTENSIONS] =
{ { '2', "h" },    /* jump on A high */
  { '2', "nle" },  /* jump on not low or equal */
  { '4', "l" },    /* jump on A low */
  { '4', "nhe" },  /* jump on not high or equal */
  { '6', "ne" },   /* jump on A not equal B */
  { '6', "lh" },   /* jump on low or high */
  { '8', "e" },    /* jump on A equal B */
  { '8', "nlh" },  /* jump on not low or high */
  { 'a', "nl" },   /* jump on A not low */
  { 'a', "he" },   /* jump on high or equal */
  { 'c', "nh" },   /* jump on A not high */
  { 'c', "le" },   /* jump on low or equal */
};

/* As with insertOpcode instructions are added to the sorted opcode
   array.  Additionally mnemonics containing the '*<number>' tag are
   expanded to the set of conditional instructions described by
   s390_cond_extensions with the tag replaced by the respective
   mnemonic extensions.  */

static void
insertExpandedMnemonic (char *opcode, char *mnemonic, char *format,
			int min_cpu, int mode_bits)
{
  char *tag;
  char prefix[15];
  char suffix[15];
  char number[15];
  int mask_start, i = 0, tag_found = 0, reading_number = 0;
  int number_p = 0, suffix_p = 0, prefix_p = 0;
  const struct s390_cond_ext_format *ext_table;
  int ext_table_length;

  if (!(tag = strpbrk (mnemonic, "*$")))
    {
      insertOpcode (opcode, mnemonic, format, min_cpu, mode_bits);
      return;
    }

  while (mnemonic[i] != '\0')
    {
      if (mnemonic[i] == *tag)
	{
	  if (tag_found)
	    goto malformed_mnemonic;

	  tag_found = 1;
	  reading_number = 1;
	}
      else
	switch (mnemonic[i])
	  {
	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8': case '9':
	    if (!tag_found || !reading_number)
	      goto malformed_mnemonic;

	    number[number_p++] = mnemonic[i];
	    break;

	  default:
	    if (reading_number)
	      {
		if (!number_p)
		  goto malformed_mnemonic;
		else
		  reading_number = 0;
	      }

	    if (tag_found)
	      suffix[suffix_p++] = mnemonic[i];
	    else
	      prefix[prefix_p++] = mnemonic[i];
	  }
      i++;
    }

  prefix[prefix_p] = '\0';
  suffix[suffix_p] = '\0';
  number[number_p] = '\0';

  if (sscanf (number, "%d", &mask_start) != 1)
    goto malformed_mnemonic;

  if (mask_start & 3)
    {
      fprintf (stderr, "Conditional mask not at nibble boundary in: %s\n",
	       mnemonic);
      return;
    }

  mask_start >>= 2;

  switch (*tag)
    {
    case '*':
      ext_table = s390_cond_extensions;
      ext_table_length = NUM_COND_EXTENSIONS;
      break;
    case '$':
      ext_table = s390_crb_extensions;
      ext_table_length = NUM_CRB_EXTENSIONS;
      break;
    default: fprintf (stderr, "Unknown tag char: %c\n", *tag);
    }

  for (i = 0; i < ext_table_length; i++)
    {
      char new_mnemonic[15];

      strcpy (new_mnemonic, prefix);
      opcode[mask_start] = ext_table[i].nibble;
      strcat (new_mnemonic, ext_table[i].extension);
      strcat (new_mnemonic, suffix);
      insertOpcode (opcode, new_mnemonic, format, min_cpu, mode_bits);
    }
  return;

 malformed_mnemonic:
  fprintf (stderr, "Malformed mnemonic: %s\n", mnemonic);
}

static char file_header[] =
  "/* The opcode table. This file was generated by s390-mkopc.\n\n"
  "   The format of the opcode table is:\n\n"
  "   NAME	     OPCODE	MASK	OPERANDS\n\n"
  "   Name is the name of the instruction.\n"
  "   OPCODE is the instruction opcode.\n"
  "   MASK is the opcode mask; this is used to tell the disassembler\n"
  "     which bits in the actual opcode must match OPCODE.\n"
  "   OPERANDS is the list of operands.\n\n"
  "   The disassembler reads the table in order and prints the first\n"
  "   instruction which matches.  */\n\n"
  "const struct s390_opcode s390_opcodes[] =\n  {\n";

/* `dumpTable': write opcode table.  */

static void
dumpTable (void)
{
  char *str;
  int  ix;

  /*  Write hash table entries (slots).  */
  printf (file_header);

  for (ix = 0; ix < no_ops; ix++)
    {
      printf ("  { \"%s\", ", op_array[ix].mnemonic);
      for (str = op_array[ix].opcode; *str != 0; str++)
	if (*str == '?')
	  *str = '0';
      printf ("OP%i(0x%sLL), ", 
	      op_array[ix].no_nibbles*4, op_array[ix].opcode);
      printf ("MASK_%s, INSTR_%s, ",
	      op_array[ix].format, op_array[ix].format);
      printf ("%i, ", op_array[ix].mode_bits);
      printf ("%i}", op_array[ix].min_cpu);
      if (ix < no_ops-1)
	printf (",\n");
      else
	printf ("\n");
    }
  printf ("};\n\n");
  printf ("const int s390_num_opcodes =\n");
  printf ("  sizeof (s390_opcodes) / sizeof (s390_opcodes[0]);\n\n");
}

int
main (void)
{
  char currentLine[256];
  
  createTable ();

  /*  Read opcode descriptions from `stdin'.  For each mnemonic,
      make an entry into the opcode table.  */
  while (fgets (currentLine, sizeof (currentLine), stdin) != NULL)
    {
      char  opcode[16];
      char  mnemonic[16];
      char  format[16];
      char  description[80];
      char  cpu_string[16];
      char  modes_string[16];
      int   min_cpu;
      int   mode_bits;
      char  *str;

      if (currentLine[0] == '#')
        continue;
      memset (opcode, 0, 8);
      if (sscanf (currentLine, "%15s %15s %15s \"%79[^\"]\" %15s %15s",
		  opcode, mnemonic, format, description,
		  cpu_string, modes_string) == 6)
	{
	  if (strcmp (cpu_string, "g5") == 0)
	    min_cpu = S390_OPCODE_G5;
	  else if (strcmp (cpu_string, "g6") == 0)
	    min_cpu = S390_OPCODE_G6;
	  else if (strcmp (cpu_string, "z900") == 0)
	    min_cpu = S390_OPCODE_Z900;
	  else if (strcmp (cpu_string, "z990") == 0)
	    min_cpu = S390_OPCODE_Z990;
	  else if (strcmp (cpu_string, "z9-109") == 0)
	    min_cpu = S390_OPCODE_Z9_109;
	  else if (strcmp (cpu_string, "z9-ec") == 0)
	    min_cpu = S390_OPCODE_Z9_EC;
	  else if (strcmp (cpu_string, "z10") == 0)
	    min_cpu = S390_OPCODE_Z10;
	  else if (strcmp (cpu_string, "z196") == 0)
	    min_cpu = S390_OPCODE_Z196;
	  else {
	    fprintf (stderr, "Couldn't parse cpu string %s\n", cpu_string);
	    exit (1);
	  }

	  str = modes_string;
	  mode_bits = 0;
	  do {
	    if (strncmp (str, "esa", 3) == 0
		&& (str[3] == 0 || str[3] == ',')) {
	      mode_bits |= 1 << S390_OPCODE_ESA;
	      str += 3;
	    } else if (strncmp (str, "zarch", 5) == 0
		       && (str[5] == 0 || str[5] == ',')) {
	      mode_bits |= 1 << S390_OPCODE_ZARCH;
	      str += 5;
	    } else {
	      fprintf (stderr, "Couldn't parse modes string %s\n",
		       modes_string);
	      exit (1);
	    }
	    if (*str == ',')
	      str++;
	  } while (*str != 0);

	  insertExpandedMnemonic (opcode, mnemonic, format, min_cpu, mode_bits);
	}
      else
	{
	  fprintf (stderr, "Couldn't scan line %s\n", currentLine);
	  exit (1);
	}
    }

  dumpTable ();
  return 0;
}
