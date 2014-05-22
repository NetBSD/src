--  Copyright 2007-2013 Free Software Foundation, Inc.
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

package body Pck is

   procedure Same (C : Character) is
   begin
      Procedure_Result := C;
   end Same;

   procedure Next (C : in out Character) is
   begin
      if C = Character'Last then
         C := Character'First;
      else
         C := Character'Succ (C);
      end if;
      Procedure_Result := C;
   end Next;

end Pck;
