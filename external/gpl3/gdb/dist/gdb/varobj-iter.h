/* Iterator of varobj.
   Copyright (C) 2013-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* A node or item of varobj, composed of the name and the value.  */

typedef struct varobj_item
{
  /* Name of this item.  */
  std::string name;

  /* Value of this item.  */
  struct value *value;
} varobj_item;

struct varobj_iter_ops;

/* A dynamic varobj iterator "class".  */

struct varobj_iter
{
  /* The 'vtable'.  */
  const struct varobj_iter_ops *ops;

  /* The varobj this iterator is listing children for.  */
  struct varobj *var;

  /* The next raw index we will try to check is available.  If it is
     equal to number_of_children, then we've already iterated the
     whole set.  */
  int next_raw_index;
};

/* The vtable of the varobj iterator class.  */

struct varobj_iter_ops
{
  /* Destructor.  Releases everything from SELF (but not SELF
     itself).  */
  void (*dtor) (struct varobj_iter *self);

  /* Returns the next object or NULL if it has reached the end.  */
  varobj_item *(*next) (struct varobj_iter *self);
};

/* Returns the next varobj or NULL if it has reached the end.  */

#define varobj_iter_next(ITER)	(ITER)->ops->next (ITER)

/* Delete a varobj_iter object.  */

#define varobj_iter_delete(ITER)	       \
  do					       \
    {					       \
      if ((ITER) != NULL)		       \
	{				       \
	  (ITER)->ops->dtor (ITER);	       \
	  xfree (ITER);		       \
	}				       \
    } while (0)
