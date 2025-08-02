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
    -- limit
    constant N_MAX   : unsigned(N - 1 downto 0) := to_unsigned(1000, N);
    -- this is the counter for the compare logic, will be send back to the ps once the result is found or
    -- n reached the limit. The first bit of n will be the indicator that shows whether the result is found:
    -- first bit = 1 -> result found
    -- first bit = 0 -> not found
    signal n         : unsigned(N - 1 downto 0) := (others => '0');
    signal done      : std_logic := '0';
begin

    process(clk, rst_n)
    begin
        if rising_edge(clk) and rst_n = '0' then
            n <= (others => '0');
            done <= '0';
            comp_sig <= '0';
            output_n <= (others => '0');
        elsif rising_edge(clk) then
            if done = '1' then
                -- Once done, hold status
                comp_sig <= '1';
                output_n <= n;
            elsif fsm_ready = '1' then
                if unsigned(result) < unsigned(target) then
                    -- result found, write '1' to the first bit
                    n(63) <= '1';
                    comp_sig <= '1';
                    output_n <= n;
                    done <= '1';
                elsif n = N_MAX then
                    n(63) <= '0';
                    comp_sig <= '1';
                    output_n <= n;
                    done <= '1';
                else
                    comp_sig <= '0';
                    n <= n + 1;
                end if;
            else
                comp_sig <= '0';
            end if;
        end if;
    end process;

end behaviour;