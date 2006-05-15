package body Pck is

   function Create_Small return Data_Small is
   begin
      return (others => 1);
   end Create_Small;

   function Create_Large return Data_Large is
   begin
      return (others => 2);
   end Create_Large;

end Pck;
