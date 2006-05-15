package Pck is

   type Data_Small is array (1 .. 2) of Integer;
   type Data_Large is array (1 .. 4) of Integer;

   function Create_Small return Data_Small;
   function Create_Large return Data_Large;

end Pck;

