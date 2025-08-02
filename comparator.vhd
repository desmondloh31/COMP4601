library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity comparator is
  generic (
    WIDTH : integer := 64 
  );
  port (
    hash_val    : in  std_logic_vector(WIDTH-1 downto 0);
    target_val  : in  std_logic_vector(WIDTH-1 downto 0);
    match_found : out std_logic
  );
end entity comparator;

architecture rtl of comparator is
begin
  process(hash_val, target_val)
  begin
    if unsigned(hash_val) < unsigned(target_val) then
      match_found <= '1';
    else
      match_found <= '0';
    end if;
  end process;
end architecture rtl;
