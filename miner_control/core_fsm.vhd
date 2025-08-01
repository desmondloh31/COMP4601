library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.keccak_globals.all;

entity core_fsm is

    port (
        clk     : in std_logic;
        rst_n   : in std_logic;
        nonce   : in std_logic_vector(N-1 downto 0);
        result  : out std_logic_vector(N-1 downto 0)
    );

end core_fsm;

architecture behaviour of core_fsm is

component keccak  
port (
    clk     : in  std_logic;
    rst_n   : in  std_logic;
    init : in std_logic;
    go : in std_logic;
    absorb : in std_logic;
    squeeze : in std_logic;
    din     : in  std_logic_vector(N-1 downto 0);    
    ready : out std_logic;
    dout    : out std_logic_vector(N-1 downto 0));
end component;
    ------------------------------------------------
	-- control signals
    ------------------------------------------------
    signal init, go, absorb, ready, squeeze : std_logic := '0';
    signal din, dout    : std_logic_vector(N-1 downto 0);
    signal counter      : integer range 0 to 15 := 0;

    type st_type is (read_first_input, st0, st1, st1a, END_HASH1, END_HASH2);
    signal st : st_type;

begin

    keccak_core : keccak port map (
        clk     => clk,
        rst_n   => rst_n,
        init    => init,
        go      => go,
        absorb  => absorb,
        squeeze => squeeze,
        din     => din,
        ready   => ready,
        dout    => dout
    );

    p_main: process (clk,rst_n)
    variable temp: std_logic_vector(63 downto 0);	
    begin
        if rst_n = '0' then                 -- asynchronous rst_n (active low)
            st <= read_first_input;
            counter <= 0;
            din <= (others=>'0');		
            init <= '0';
            absorb <= '0';
            squeeze <= '0';
            go <= '0';
        elsif clk'event and clk = '1' then
            case st is
                when read_first_input =>
                
                when st0 =>

                when st1 =>

                when st1a =>
                    
                when END_HASH1 =>
                    
                when END_HASH2 =>
                        
            end case;
        end if;
    end process;

end behaviour;