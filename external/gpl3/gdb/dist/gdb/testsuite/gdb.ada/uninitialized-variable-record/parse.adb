--  Copyright 2009-2024 Free Software Foundation, Inc.
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

-- Based on gdb.ada/uninitialized_vars/parse.adb.

procedure Parse is

   type Variable_Record (A : Boolean := True) is record
      case A is
         when True =>
            B : Integer;
         when False =>
            C : Float;
            D : Integer;
      end case;
   end record;
   Y2 : Variable_Record := (A => False, C => 1.0, D => 2);

begin
   null;  -- START
end Parse;
