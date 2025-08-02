library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.keccak_globals.all;

entity result_compare is
    port (
        clk         : in std_logic;
        rst_n       : in std_logic; -- use ready signal from PS as reset
        target      : in std_logic_vector(N - 1 downto 0);
        result      : in std_logic_vector(N - 1 downto 0);
        fsm_ready   : in std_logic;
        comp_sig    : out std_logic; -- serves as reset for other components
        output_n    : out unsigned(N - 1 downto 0) := (others => '0')
    );
end entity;

architecture behaviour of result_compare is
    signal n         : unsigned(N - 1 downto 0) := (others => '0');
begin

    process(clk, rst_n)
    begin
        if rising_edge(clk) and rst_n = '0' then
            n <= (others => '0');
        elsif rising_edge(clk) then
            if fsm_ready = '1' then
                if unsigned(result) < unsigned(target) then
                    comp_sig <= '1';
                    output_n <= n;
                else
                    comp_sig <= '0';
                end if;
                n <= n + 1;
            else
                comp_sig <= '0';
            end if;
        end if;
    end process;

end behaviour;