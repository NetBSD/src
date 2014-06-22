struct simops 
{
  unsigned long   opcode;
  unsigned long   mask;
  int (* func) (void);
  int    numops;
  int    operands[12];
};
