/* Tests fpr intrusive double linked list for GDB, the GNU debugger.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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


#include "gdbsupport/intrusive_list.h"
#include "gdbsupport/owning_intrusive_list.h"
#include "gdbsupport/selftest.h"
#include <unordered_set>

/* Count of how many item_with_base or item_with_member objects are
   currently alive.  */

static int items_alive = 0;

/* An item type using intrusive_list_node by inheriting from it and its
   corresponding list type.  Put another base before intrusive_list_node
   so that a pointer to the node != a pointer to the item.  */

struct other_base
{
  int n = 1;
};

struct item_with_base : public other_base,
			public intrusive_list_node<item_with_base>
{
  explicit item_with_base (const char *name)
    : name (name)
  {
    ++items_alive;
  }

  DISABLE_COPY_AND_ASSIGN (item_with_base);

  ~item_with_base () { --items_alive; }

  const char *const name;
};

using item_with_base_list = intrusive_list<item_with_base>;

/* An item type using intrusive_list_node as a field and its corresponding
   list type.  Put the other field before the node, so that a pointer to the
   node != a pointer to the item.  */

struct item_with_member
{
  explicit item_with_member (const char *name)
    : name (name)
  {
    ++items_alive;
  }

  DISABLE_COPY_AND_ASSIGN (item_with_member);

  ~item_with_member () { --items_alive; }

  const char *const name;
  intrusive_list_node<item_with_member> node;
};

/* Verify that LIST contains exactly the items in EXPECTED.

   Traverse the list forward and backwards to exercise all links.  */

template <typename ListType>
static void
verify_items (const ListType &list,
	      gdb::array_view<const typename ListType::value_type *> expected)
{
  using item_type = typename ListType::value_type;

  int i = 0;

  for (typename ListType::iterator it = list.begin (); it != list.end (); ++it)
    {
      const item_type &item = *it;

      SELF_CHECK (i < expected.size ());
      SELF_CHECK (&item == expected[i]);

      /* Access the item, to make sure the object is still alive.  */
      SELF_CHECK (strcmp (item.name, expected[i]->name) == 0);

      ++i;
    }

  SELF_CHECK (i == expected.size ());

  for (typename ListType::reverse_iterator it = list.rbegin ();
       it != list.rend (); ++it)
    {
      const item_type &item = *it;

      --i;

      SELF_CHECK (i >= 0);
      SELF_CHECK (&item == expected[i]);

      /* Access the item, to make sure the object is still alive.  */
      SELF_CHECK (strcmp (item.name, expected[i]->name) == 0);
    }

  SELF_CHECK (i == 0);
}

/* intrusive_list tests

   To run all tests using both the base and member methods, all tests are
   declared in this templated class, which is instantiated once for each
   list type.  */

using item_with_member_node
  = intrusive_member_node<item_with_member, &item_with_member::node>;
using item_with_member_list
  = intrusive_list<item_with_member, item_with_member_node>;

template <typename ListType>
struct intrusive_list_test
{
  using item_type = typename ListType::value_type;

  static void
  test_move_constructor ()
  {
    {
      /* Other list is not empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      ListType list2 (std::move (list1));

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* Other list contains 1 element.  */
      item_type a ("a");
      ListType list1;
      std::vector<const item_type *> expected;

      list1.push_back (a);

      ListType list2 (std::move (list1));

      expected = {};
      verify_items (list1, expected);

      expected = {&a};
      verify_items (list2, expected);
    }

    {
      /* Other list is empty.  */
      ListType list1;
      std::vector<const item_type *> expected;

      ListType list2 (std::move (list1));

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_move_assignment ()
  {
    {
      /* Both lists are not empty.  */
      item_type a ("a"), b ("b"), c ("c"), d ("d"), e ("e");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2.push_back (d);
      list2.push_back (e);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* rhs list is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2.push_back (a);
      list2.push_back (b);
      list2.push_back (c);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* lhs list is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* Both lists contain 1 item.  */
      item_type a ("a"), b ("b");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list2.push_back (b);

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {&a};
      verify_items (list2, expected);
    }

    {
      /* Both lists are empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2 = std::move (list1);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_swap ()
  {
    {
      /* Two non-empty lists.  */
      item_type a ("a"), b ("b"), c ("c"), d ("d"), e ("e");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2.push_back (d);
      list2.push_back (e);

      std::swap (list1, list2);

      expected = {&d, &e};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* Other is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      std::swap (list1, list2);

      expected = {};
      verify_items (list1, expected);

      expected = {&a, &b, &c};
      verify_items (list2, expected);
    }

    {
      /* *this is empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2.push_back (a);
      list2.push_back (b);
      list2.push_back (c);

      std::swap (list1, list2);

      expected = {&a, &b, &c};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Both lists empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      std::swap (list1, list2);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Swap one element twice.  */
      item_type a ("a");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);

      std::swap (list1, list2);

      expected = {};
      verify_items (list1, expected);

      expected = {&a};
      verify_items (list2, expected);

      std::swap (list1, list2);

      expected = {&a};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_front_back ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    const ListType &clist = list;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    SELF_CHECK (&list.front () == &a);
    SELF_CHECK (&clist.front () == &a);
    SELF_CHECK (&list.back () == &c);
    SELF_CHECK (&clist.back () == &c);
  }

  static void
  test_push_front ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    expected = {};
    verify_items (list, expected);

    list.push_front (a);
    expected = {&a};
    verify_items (list, expected);

    list.push_front (b);
    expected = {&b, &a};
    verify_items (list, expected);

    list.push_front (c);
    expected = {&c, &b, &a};
    verify_items (list, expected);
  }

  static void
  test_push_back ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    expected = {};
    verify_items (list, expected);

    list.push_back (a);
    expected = {&a};
    verify_items (list, expected);

    list.push_back (b);
    expected = {&a, &b};
    verify_items (list, expected);

    list.push_back (c);
    expected = {&a, &b, &c};
    verify_items (list, expected);
  }

  static void
  test_insert ()
  {
    std::vector<const item_type *> expected;

    {
      /* Insert at beginning.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list;


      auto a_it = list.insert (list.begin (), a);
      SELF_CHECK (&*a_it == &a);
      expected = {&a};
      verify_items (list, expected);

      auto b_it = list.insert (list.begin (), b);
      SELF_CHECK (&*b_it == &b);
      expected = {&b, &a};
      verify_items (list, expected);

      auto c_it = list.insert (list.begin (), c);
      SELF_CHECK (&*c_it == &c);
      expected = {&c, &b, &a};
      verify_items (list, expected);
    }

    {
      /* Insert at end.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list;


      auto a_it = list.insert (list.end (), a);
      SELF_CHECK (&*a_it == &a);
      expected = {&a};
      verify_items (list, expected);

      auto b_it = list.insert (list.end (), b);
      SELF_CHECK (&*b_it == &b);
      expected = {&a, &b};
      verify_items (list, expected);

      auto c_it = list.insert (list.end (), c);
      SELF_CHECK (&*c_it == &c);
      expected = {&a, &b, &c};
      verify_items (list, expected);
    }

    {
      /* Insert in the middle.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list;

      list.push_back (a);
      list.push_back (b);

      auto c_it = list.insert (list.iterator_to (b), c);
      SELF_CHECK (&*c_it == &c);
      expected = {&a, &c, &b};
      verify_items (list, expected);
    }

    {
      /* Insert in empty list. */
      item_type a ("a");
      ListType list;

      auto a_it = list.insert (list.end (), a);
      SELF_CHECK (&*a_it == &a);
      expected = {&a};
      verify_items (list, expected);
    }
  }

  static void
  test_splice ()
  {
    {
      /* Two non-empty lists.  */
      item_type a ("a"), b ("b"), c ("c"), d ("d"), e ("e");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list2.push_back (d);
      list2.push_back (e);

      list1.splice (std::move (list2));

      expected = {&a, &b, &c, &d, &e};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Receiving list empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2.push_back (a);
      list2.push_back (b);
      list2.push_back (c);

      list1.splice (std::move (list2));

      expected = {&a, &b, &c};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Giving list empty.  */
      item_type a ("a"), b ("b"), c ("c");
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.push_back (a);
      list1.push_back (b);
      list1.push_back (c);

      list1.splice (std::move (list2));

      expected = {&a, &b, &c};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Both lists empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list1.splice (std::move (list2));

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void
  test_pop_front ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.pop_front ();
    expected = {&b, &c};
    verify_items (list, expected);

    list.pop_front ();
    expected = {&c};
    verify_items (list, expected);

    list.pop_front ();
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_pop_back ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.pop_back();
    expected = {&a, &b};
    verify_items (list, expected);

    list.pop_back ();
    expected = {&a};
    verify_items (list, expected);

    list.pop_back ();
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_erase ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.erase (list.iterator_to (b));
    expected = {&a, &c};
    verify_items (list, expected);

    list.erase (list.iterator_to (c));
    expected = {&a};
    verify_items (list, expected);

    list.erase (list.iterator_to (a));
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_clear ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    list.clear ();
    expected = {};
    verify_items (list, expected);

    /* Verify idempotency.  */
    list.clear ();
    expected = {};
    verify_items (list, expected);
  }

  static void
  test_clear_and_dispose ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    std::vector<const item_type *> expected;
    std::unordered_set<const item_type *> disposer_seen;
    int disposer_calls = 0;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    auto disposer = [&] (const item_type *item)
      {
	disposer_seen.insert (item);
	disposer_calls++;
      };
    list.clear_and_dispose (disposer);

    expected = {};
    verify_items (list, expected);
    SELF_CHECK (disposer_calls == 3);
    SELF_CHECK (disposer_seen.find (&a) != disposer_seen.end ());
    SELF_CHECK (disposer_seen.find (&b) != disposer_seen.end ());
    SELF_CHECK (disposer_seen.find (&c) != disposer_seen.end ());

    /* Verify idempotency.  */
    list.clear_and_dispose (disposer);
    SELF_CHECK (disposer_calls == 3);
  }

  static void
  test_empty ()
  {
    item_type a ("a");
    ListType list;

    SELF_CHECK (list.empty ());
    list.push_back (a);
    SELF_CHECK (!list.empty ());
    list.erase (list.iterator_to (a));
    SELF_CHECK (list.empty ());
  }

  static void
  test_begin_end ()
  {
    item_type a ("a"), b ("b"), c ("c");
    ListType list;
    const ListType &clist = list;

    list.push_back (a);
    list.push_back (b);
    list.push_back (c);

    SELF_CHECK (&*list.begin () == &a);
    SELF_CHECK (&*list.cbegin () == &a);
    SELF_CHECK (&*clist.begin () == &a);
    SELF_CHECK (&*list.rbegin () == &c);
    SELF_CHECK (&*list.crbegin () == &c);
    SELF_CHECK (&*clist.rbegin () == &c);

    /* At least check that they compile.  */
    list.end ();
    list.cend ();
    clist.end ();
    list.rend ();
    list.crend ();
    clist.end ();
  }
};

template <typename ListType>
static void
test_intrusive_list_1 ()
{
  intrusive_list_test<ListType> tests;

  tests.test_move_constructor ();
  tests.test_move_assignment ();
  tests.test_swap ();
  tests.test_front_back ();
  tests.test_push_front ();
  tests.test_push_back ();
  tests.test_insert ();
  tests.test_splice ();
  tests.test_pop_front ();
  tests.test_pop_back ();
  tests.test_erase ();
  tests.test_clear ();
  tests.test_clear_and_dispose ();
  tests.test_empty ();
  tests.test_begin_end ();
}

/* owning_intrusive_list tests

   To run all tests using both the base and member methods, all tests are
   declared in this templated class, which is instantiated once for each
   list type.  */

using item_with_base_owning_list = owning_intrusive_list<item_with_base>;
using item_with_member_owning_list
  = owning_intrusive_list<item_with_member, item_with_member_node>;

template<typename ListType>
struct owning_intrusive_list_test
{
  using item_type = typename ListType::value_type;

  static void test_move_constructor ()
  {
    {
      /* Other list is not empty.  */
      ListType list1;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      auto &b = list1.emplace_back ("b");
      auto &c = list1.emplace_back ("c");

      SELF_CHECK (items_alive == 3);
      ListType list2 (std::move (list1));
      SELF_CHECK (items_alive == 3);

      expected = {};
      verify_items (list1, expected);

      expected = { &a, &b, &c };
      verify_items (list2, expected);
    }

    {
      /* Other list contains 1 element.  */
      ListType list1;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");

      SELF_CHECK (items_alive == 1);
      ListType list2 (std::move (list1));
      SELF_CHECK (items_alive == 1);

      expected = {};
      verify_items (list1, expected);

      expected = { &a };
      verify_items (list2, expected);
    }

    {
      /* Other list is empty.  */
      ListType list1;
      std::vector<const item_type *> expected;

      SELF_CHECK (items_alive == 0);
      ListType list2 (std::move (list1));
      SELF_CHECK (items_alive == 0);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void test_move_assignment ()
  {
    {
      /* Both lists are not empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      auto &b = list1.emplace_back ("b");
      auto &c = list1.emplace_back ("c");

      list2.emplace_back ("d");
      list2.emplace_back ("e");

      SELF_CHECK (items_alive == 5);
      list2 = std::move (list1);
      SELF_CHECK (items_alive == 3);

      expected = {};
      verify_items (list1, expected);

      expected = { &a, &b, &c };
      verify_items (list2, expected);
    }

    {
      /* rhs list is empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      list2.emplace_back ("a");
      list2.emplace_back ("b");
      list2.emplace_back ("c");

      SELF_CHECK (items_alive == 3);
      list2 = std::move (list1);
      SELF_CHECK (items_alive == 0);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* lhs list is empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      auto &b = list1.emplace_back ("b");
      auto &c = list1.emplace_back ("c");

      SELF_CHECK (items_alive == 3);
      list2 = std::move (list1);
      SELF_CHECK (items_alive == 3);

      expected = {};
      verify_items (list1, expected);

      expected = { &a, &b, &c };
      verify_items (list2, expected);
    }

    {
      /* Both lists contain 1 item.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      list2.emplace_back ("b");

      SELF_CHECK (items_alive == 2);
      list2 = std::move (list1);
      SELF_CHECK (items_alive == 1);

      expected = {};
      verify_items (list1, expected);

      expected = { &a };
      verify_items (list2, expected);
    }

    {
      /* Both lists are empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      SELF_CHECK (items_alive == 0);
      list2 = std::move (list1);
      SELF_CHECK (items_alive == 0);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void test_swap ()
  {
    {
      /* Two non-empty lists.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      auto &b = list1.emplace_back ("b");
      auto &c = list1.emplace_back ("c");

      auto &d = list2.emplace_back ("d");
      auto &e = list2.emplace_back ("e");

      SELF_CHECK (items_alive == 5);
      std::swap (list1, list2);
      SELF_CHECK (items_alive == 5);

      expected = { &d, &e };
      verify_items (list1, expected);

      expected = { &a, &b, &c };
      verify_items (list2, expected);
    }

    {
      /* Other is empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      auto &b = list1.emplace_back ("b");
      auto &c = list1.emplace_back ("c");

      SELF_CHECK (items_alive == 3);
      std::swap (list1, list2);
      SELF_CHECK (items_alive == 3);

      expected = {};
      verify_items (list1, expected);

      expected = { &a, &b, &c };
      verify_items (list2, expected);
    }

    {
      /* *this is empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list2.emplace_back ("a");
      auto &b = list2.emplace_back ("b");
      auto &c = list2.emplace_back ("c");

      SELF_CHECK (items_alive == 3);
      std::swap (list1, list2);
      SELF_CHECK (items_alive == 3);

      expected = { &a, &b, &c };
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Both lists empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      SELF_CHECK (items_alive == 0);
      std::swap (list1, list2);
      SELF_CHECK (items_alive == 0);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Swap one element twice.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");

      SELF_CHECK (items_alive == 1);
      std::swap (list1, list2);
      SELF_CHECK (items_alive == 1);

      expected = {};
      verify_items (list1, expected);

      expected = { &a };
      verify_items (list2, expected);

      std::swap (list1, list2);
      SELF_CHECK (items_alive == 1);

      expected = { &a };
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void test_front_back ()
  {
    ListType list;
    const ListType &clist = list;

    auto &a = list.emplace_back ("a");
    list.emplace_back ("b");
    auto &c = list.emplace_back ("c");

    SELF_CHECK (&list.front () == &a);
    SELF_CHECK (&clist.front () == &a);
    SELF_CHECK (&list.back () == &c);
    SELF_CHECK (&clist.back () == &c);
  }

  static void test_push_front ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    SELF_CHECK (items_alive == 0);
    list.push_front (std::make_unique<item_type> ("a"));
    auto &a = list.front ();
    expected = { &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 1);

    list.push_front (std::make_unique<item_type> ("b"));
    auto &b = list.front ();
    expected = { &b, &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 2);

    list.push_front (std::make_unique<item_type> ("c"));
    auto &c = list.front ();
    expected = { &c, &b, &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 3);
  }

  static void test_push_back ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    SELF_CHECK (items_alive == 0);
    list.push_back (std::make_unique<item_type> ("a"));
    auto &a = list.back ();
    expected = { &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 1);

    list.push_back (std::make_unique<item_type> ("b"));
    auto &b = list.back ();
    expected = { &a, &b };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 2);

    list.push_back (std::make_unique<item_type> ("c"));
    auto &c = list.back ();
    expected = { &a, &b, &c };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 3);
  }

  static void test_insert ()
  {
    std::vector<const item_type *> expected;

    {
      /* Insert at beginning.  */
      ListType list;

      auto &a = *list.insert (list.begin (), std::make_unique<item_type> ("a"));
      expected = { &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 1);

      auto &b = *list.insert (list.begin (), std::make_unique<item_type> ("b"));
      expected = { &b, &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 2);

      auto &c = *list.insert (list.begin (), std::make_unique<item_type> ("c"));
      expected = { &c, &b, &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 3);
    }

    {
      /* Insert at end.  */
      ListType list;

      auto &a = *list.insert (list.end (), std::make_unique<item_type> ("a"));
      expected = { &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 1);

      auto &b = *list.insert (list.end (), std::make_unique<item_type> ("b"));
      expected = { &a, &b };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 2);

      auto &c = *list.insert (list.end (), std::make_unique<item_type> ("c"));
      expected = { &a, &b, &c };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 3);
    }

    {
      /* Insert in the middle.  */
      ListType list;

      auto &a = list.emplace_back ("a");
      auto &b = list.emplace_back ("b");

      SELF_CHECK (items_alive == 2);
      auto &c = *list.insert (list.iterator_to (b),
			      std::make_unique<item_type> ("c"));
      expected = { &a, &c, &b };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 3);
    }

    {
      /* Insert in empty list. */
      ListType list;

      SELF_CHECK (items_alive == 0);
      auto &a = *list.insert (list.end (), std::make_unique<item_type> ("a"));
      expected = { &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 1);
    }
  }

  static void test_emplace_front ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    SELF_CHECK (items_alive == 0);
    auto &a = list.emplace_front ("a");
    expected = { &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 1);

    auto &b = list.emplace_front ("b");
    expected = { &b, &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 2);

    auto &c = list.emplace_front ("c");
    expected = { &c, &b, &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 3);
  }

  static void test_emplace_back ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    SELF_CHECK (items_alive == 0);
    auto &a = list.emplace_back ("a");
    expected = { &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 1);

    auto &b = list.emplace_back ("b");
    expected = { &a, &b };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 2);

    auto &c = list.emplace_back ("c");
    expected = { &a, &b, &c };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 3);
  }

  static void test_emplace ()
  {
    std::vector<const item_type *> expected;

    {
      /* Emplace at beginning.  */
      ListType list;

      auto &a = list.emplace (list.begin (), "a");
      expected = { &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 1);

      auto &b = list.emplace (list.begin (), "b");
      expected = { &b, &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 2);

      auto &c = list.emplace (list.begin (), "c");
      expected = { &c, &b, &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 3);
    }

    {
      /* Emplace at end.  */
      ListType list;

      auto &a = list.emplace (list.end (), "a");
      expected = { &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 1);

      auto &b = list.emplace (list.end (), "b");
      expected = { &a, &b };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 2);

      auto &c = list.emplace (list.end (), "c");
      expected = { &a, &b, &c };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 3);
    }

    {
      /* Emplace in the middle.  */
      ListType list;

      auto &a = list.emplace_back ("a");
      auto &b = list.emplace_back ("b");

      SELF_CHECK (items_alive == 2);
      auto &c = list.emplace (list.iterator_to (b), "c");
      expected = { &a, &c, &b };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 3);
    }

    {
      /* Emplace in empty list. */
      ListType list;

      SELF_CHECK (items_alive == 0);
      auto &a = list.emplace (list.end (), "a");
      expected = { &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 1);
    }
  }

  static void test_splice ()
  {
    {
      /* Two non-empty lists.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      auto &b = list1.emplace_back ("b");
      auto &c = list1.emplace_back ("c");

      auto &d = list2.emplace_back ("d");
      auto &e = list2.emplace_back ("e");

      SELF_CHECK (items_alive == 5);
      list1.splice (std::move (list2));
      SELF_CHECK (items_alive == 5);

      expected = { &a, &b, &c, &d, &e };
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Receiving list empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list2.emplace_back ("a");
      auto &b = list2.emplace_back ("b");
      auto &c = list2.emplace_back ("c");

      SELF_CHECK (items_alive == 3);
      list1.splice (std::move (list2));
      SELF_CHECK (items_alive == 3);

      expected = { &a, &b, &c };
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Giving list empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      auto &a = list1.emplace_back ("a");
      auto &b = list1.emplace_back ("b");
      auto &c = list1.emplace_back ("c");

      SELF_CHECK (items_alive == 3);
      list1.splice (std::move (list2));
      SELF_CHECK (items_alive == 3);

      expected = { &a, &b, &c };
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }

    {
      /* Both lists empty.  */
      ListType list1;
      ListType list2;
      std::vector<const item_type *> expected;

      SELF_CHECK (items_alive == 0);
      list1.splice (std::move (list2));
      SELF_CHECK (items_alive == 0);

      expected = {};
      verify_items (list1, expected);

      expected = {};
      verify_items (list2, expected);
    }
  }

  static void test_pop_front ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    list.emplace_back ("a");
    auto &b = list.emplace_back ("b");
    auto &c = list.emplace_back ("c");

    SELF_CHECK (items_alive == 3);
    list.pop_front ();
    expected = { &b, &c };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 2);

    list.pop_front ();
    expected = { &c };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 1);

    list.pop_front ();
    expected = {};
    verify_items (list, expected);
    SELF_CHECK (items_alive == 0);
  }

  static void test_pop_back ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    auto &a = list.emplace_back ("a");
    auto &b = list.emplace_back ("b");
    list.emplace_back ("c");

    SELF_CHECK (items_alive == 3);
    list.pop_back ();
    expected = { &a, &b };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 2);

    list.pop_back ();
    expected = { &a };
    verify_items (list, expected);
    SELF_CHECK (items_alive == 1);

    list.pop_back ();
    expected = {};
    verify_items (list, expected);
    SELF_CHECK (items_alive == 0);
  }

  static void test_release ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    auto &a = list.emplace_back ("a");
    auto &b = list.emplace_back ("b");
    auto &c = list.emplace_back ("c");

    {
      SELF_CHECK (items_alive == 3);
      auto [next_it, released] = list.release (list.iterator_to (b));
      SELF_CHECK (&*next_it == &c);
      expected = { &a, &c };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 3);
      released.reset ();
      SELF_CHECK (items_alive == 2);
    }

    {
      auto [next_it, released] = list.release (list.iterator_to (c));
      SELF_CHECK (next_it == list.end ());
      expected = { &a };
      verify_items (list, expected);
      SELF_CHECK (items_alive == 2);
      released.reset ();
      SELF_CHECK (items_alive == 1);
    }

    {
      auto [next_it, released] = list.release (list.iterator_to (a));
      SELF_CHECK (next_it == list.end ());
      expected = {};
      verify_items (list, expected);
      SELF_CHECK (items_alive == 1);
      released.reset ();
      SELF_CHECK (items_alive == 0);
    }
  }

  static void test_clear ()
  {
    ListType list;
    std::vector<const item_type *> expected;

    list.emplace_back ("a");
    list.emplace_back ("b");
    list.emplace_back ("c");

    SELF_CHECK (items_alive == 3);
    list.clear ();
    expected = {};
    verify_items (list, expected);
    SELF_CHECK (items_alive == 0);

    /* Verify idempotency.  */
    list.clear ();
    expected = {};
    verify_items (list, expected);
    SELF_CHECK (items_alive == 0);
  }

  static void test_empty ()
  {
    ListType list;

    SELF_CHECK (list.empty ());
    auto &a = list.emplace_back ("a");
    SELF_CHECK (!list.empty ());
    list.erase (list.iterator_to (a));
    SELF_CHECK (list.empty ());
  }

  static void test_begin_end ()
  {
    ListType list;
    const ListType &clist = list;

    auto &a = list.emplace_back ("a");
    list.emplace_back ("b");
    auto &c = list.emplace_back ("c");

    SELF_CHECK (&*list.begin () == &a);
    SELF_CHECK (&*list.cbegin () == &a);
    SELF_CHECK (&*clist.begin () == &a);
    SELF_CHECK (&*list.rbegin () == &c);
    SELF_CHECK (&*list.crbegin () == &c);
    SELF_CHECK (&*clist.rbegin () == &c);

    /* At least check that they compile.  */
    list.end ();
    list.cend ();
    clist.end ();
    list.rend ();
    list.crend ();
    clist.end ();
  }
};

template<typename ListType>
static void
test_owning_intrusive_list_1 ()
{
  owning_intrusive_list_test<ListType> tests;

  tests.test_move_constructor ();
  tests.test_move_assignment ();
  tests.test_swap ();
  tests.test_front_back ();
  tests.test_push_front ();
  tests.test_push_back ();
  tests.test_insert ();
  tests.test_emplace_front ();
  tests.test_emplace_back ();
  tests.test_emplace ();
  tests.test_splice ();
  tests.test_pop_front ();
  tests.test_pop_back ();
  tests.test_release ();
  tests.test_clear ();
  tests.test_empty ();
  tests.test_begin_end ();
}

static void
test_node_is_linked ()
{
  {
    item_with_base a ("a");
    item_with_base_list list;

    SELF_CHECK (!a.is_linked ());
    list.push_back (a);
    SELF_CHECK (a.is_linked ());
    list.pop_back ();
    SELF_CHECK (!a.is_linked ());
  }

  {
    item_with_member a ("a");
    item_with_member_list list;

    SELF_CHECK (!a.node.is_linked ());
    list.push_back (a);
    SELF_CHECK (a.node.is_linked ());
    list.pop_back ();
    SELF_CHECK (!a.node.is_linked ());
  }
}

static void
test_intrusive_list ()
{
  test_intrusive_list_1<item_with_base_list> ();
  test_intrusive_list_1<item_with_member_list> ();
  test_owning_intrusive_list_1<item_with_base_owning_list> ();
  test_owning_intrusive_list_1<item_with_member_owning_list> ();
  test_node_is_linked ();
}

void _initialize_intrusive_list_selftests ();

void
_initialize_intrusive_list_selftests ()
{
  selftests::register_test ("intrusive_list", test_intrusive_list);
}
