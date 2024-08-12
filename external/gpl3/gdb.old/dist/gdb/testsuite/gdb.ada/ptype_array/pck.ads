--  Copyright 2019-2023 Free Software Foundation, Inc.
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

with System;

package Pck is
   package W is
      type P is record
          M : String (2 .. 5);
      end record;

      type R is array (1 .. 10, 1 .. 20) of P;

      G : R;
   end W;

   procedure Do_Nothing (A : System.Address);
end Pck;
