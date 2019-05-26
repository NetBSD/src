--  Copyright 2008-2017 Free Software Foundation, Inc.
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 3 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Pck is

   My_Global_Variable : Integer := 1;

   Exported_Capitalized : Integer := 2;
   pragma Export (C, Exported_Capitalized, "Exported_Capitalized");

   Local_Identical_One : Integer := 4;
   Local_Identical_Two : Integer := 8;

   External_Identical_One : Integer := 19;

   package Inner is
      Inside_Variable : Integer := 3;
   end Inner;

   procedure Proc (I : Integer);

   procedure Ambiguous_Func;

end Pck;
