library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.keccak_globals.all;

entity nonce_adder is
    port (
        clk         : in std_logic;
        rst_n       : in std_logic;
        nonce       : in std_logic_vector(N - 1 downto 0);
        fsm_ready   : in std_logic;
        -- comp_sig    : in std_logic;
        start       : out std_logic;
        added_nonce : out std_logic_vector(N - 1 downto 0)
    );
end entity;

architecture behaviour of nonce_adder is
    signal n         : unsigned(N - 1 downto 0) := (others => '0');
    signal start_reg : std_logic := '0';
begin

    process(clk, rst_n)
    begin
        if rising_edge(clk) and rst_n = '0' then
            n <= (others => '0');
            start_reg <= '0';
        elsif rising_edge(clk) then
            if fsm_ready = '1' then
                n <= n + 1;
                start_reg <= '1';
            else
                start_reg <= '0';
            end if;
        end if;
    end process;

    added_nonce <= std_logic_vector(unsigned(nonce) + n);
    start <= start_reg;

end behaviour;