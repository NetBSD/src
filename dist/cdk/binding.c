#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/04 19:58:29 $
 * $Revision: 1.1.1.1 $
 */

/*
 * Declare file local prototypes.
 */
static int mapChtype (chtype key);

/*
 * This inserts a binding.
 */
void bindCDKObject (EObjectType cdktype, void *object, chtype key, BINDFN function, void * data)
{
   int Index = mapChtype (key);
   CDKOBJS *obj = (CDKOBJS *)object;

  /*
   * When an alarm is set and this function is entered, a very wild
   * value for the key is provided, and the index gets messed up big time.
   * So we will make sure that index is a valid value before using it.
   */
   if ((Index >= 0) && (Index < MAX_BINDINGS))
   {
      if (cdktype == vFSELECT)
      {
	 bindCDKObject (vENTRY, ((CDKFSELECT *)object)->entryField, key, function, data);
      }
      else if (cdktype == vALPHALIST)
      {
	 bindCDKObject (vENTRY, ((CDKALPHALIST *)object)->entryField, key, function, data);
      }
      else
      {
	 if (Index >= obj->bindingCount)
	 {
	    unsigned next = (Index + 1);
	    unsigned need = next * sizeof(CDKBINDING);

	    if (obj->bindingList != 0)
	       obj->bindingList = (CDKBINDING *)realloc(obj->bindingList, need);
	    else
	       obj->bindingList = (CDKBINDING *)malloc(need);

	    memset(&(obj->bindingList[obj->bindingCount]), 0,
		   (next - obj->bindingCount) * sizeof(CDKBINDING));
	    obj->bindingCount = next;
	 }

	 if (obj->bindingList != 0)
	 {
	    obj->bindingList[Index].bindFunction = function;
	    obj->bindingList[Index].bindData	 = data;
	 }
      }
   }
}

/*
 * This removes a binding on an object.
 */
void unbindCDKObject (EObjectType cdktype, void *object, chtype key)
{
   int Index = mapChtype(key);
   CDKOBJS *obj = (CDKOBJS *)object;

  /*
   * When an alarm is set and this function is entered, a very wild
   * value for the key is provided, and the index gets messed up big time.
   * So we will make sure that index is a valid value before using it.
   */
   if (cdktype == vFSELECT)
   {
      unbindCDKObject (vENTRY, ((CDKFSELECT *)object)->entryField, key);
   }
   else if (cdktype == vALPHALIST)
   {
      unbindCDKObject (vENTRY, ((CDKALPHALIST *)object)->entryField, key);
   }
   else if (Index >= 0 && Index < obj->bindingCount)
   {
      obj->bindingList[Index].bindFunction	= 0;
      obj->bindingList[Index].bindData		= 0;
   }
}

/*
 * This sets all the bindings for the given objects.
 */
void cleanCDKObjectBindings (EObjectType cdktype, void *object)
{
  /*
   * Since dereferencing a void pointer is a no-no, we have to cast
   * our pointer correctly.
   */
   if (cdktype == vFSELECT)
   {
      cleanCDKObjectBindings (vENTRY, ((CDKFSELECT *)object)->entryField);
      cleanCDKObjectBindings (vSCROLL, ((CDKFSELECT *)object)->scrollField);
   }
   else if (cdktype == vALPHALIST)
   {
      cleanCDKObjectBindings (vENTRY, ((CDKALPHALIST *)object)->entryField);
      cleanCDKObjectBindings (vSCROLL, ((CDKALPHALIST *)object)->scrollField);
   }
   else
   {
      int x;
      CDKOBJS *obj = (CDKOBJS *)object;
      for (x=0; x < obj->bindingCount; x++)
      {
	 (obj)->bindingList[x].bindFunction	= 0;
	 (obj)->bindingList[x].bindData		= 0;
      }
   }
}

/*
 * This checks to see iof the binding for the key exists. If it does then it
 * runs the command and returns a TRUE. If it doesn't it returns a FALSE. This
 * way we can 'overwrite' coded bindings.
 */
int checkCDKObjectBind (EObjectType cdktype, void *object, chtype key)
{
   int Index = mapChtype (key);
   CDKOBJS *obj = (CDKOBJS *)object;

  /*
   * When an alarm is set and this function is entered, a very wild
   * value for the key is provided, and the index gets messed up big time.
   * So we will make sure that index is a valid value before using it.
   */
   if ((Index >= 0) && (Index < obj->bindingCount))
   {
      if ( (obj)->bindingList[Index].bindFunction != 0 )
      {
	 BINDFN function	= obj->bindingList[Index].bindFunction;
	 void * data		= obj->bindingList[Index].bindData;
	 function (cdktype, object, data, key);
	 return (TRUE);
      }
   }
   return (FALSE);
}

/*
 * This translates non ascii characters like KEY_UP to an 'equivalent'
 * ascii value.
 */
static int mapChtype (chtype key)
{
   static const struct {
      int key_out;
      chtype key_in;
   } table[] = {
      { 257, KEY_UP },
      { 258, KEY_DOWN },
      { 259, KEY_LEFT },
      { 260, KEY_RIGHT },
      { 261, KEY_NPAGE },
      { 262, KEY_PPAGE },
      { 263, KEY_HOME },
      { 264, KEY_END },
      { 265, KEY_F0 },
      { 266, KEY_F1 },
      { 267, KEY_F2 },
      { 268, KEY_F3 },
      { 269, KEY_F4 },
      { 270, KEY_F5 },
      { 271, KEY_F6 },
      { 272, KEY_F7 },
      { 273, KEY_A1 },
      { 274, KEY_A3 },
      { 275, KEY_B2 },
      { 276, KEY_C1 },
      { 277, KEY_C3 },
      { 278, KEY_ESC },
   };
   unsigned n;

   for (n = 0; n < sizeof(table)/sizeof(table[0]); n++)
   {
      if (table[n].key_in == key)
      {
	 key = table[n].key_out;
	 break;
      }
   }
   return (key);
}
