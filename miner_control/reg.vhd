library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.keccak_globals.all;

entity reg is
    port (
        clk         : in std_logic;
        rst_n       : in std_logic;
        enable      : in std_logic;
        din         : in std_logic_vector(N - 1 downto 0);
        dout        : out std_logic_vector(N - 1 downto 0)
    );
end entity;

architecture behaviour of reg is
    signal data : std_logic_vector(N - 1 downto 0) := (others => '0');
begin

    process(clk, rst_n)
    begin
        if rst_n = '0' then
            data <= (others => '0');
        elsif rising_edge(clk) then
            if enable = '1' then
                data <= din;
            end if;
        end if;
    end process;

    dout <= data;

end behaviour;