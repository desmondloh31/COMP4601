library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity controller is 
    generic (
        N : integer := 32  -- Assuming 32-bit width, adjust as needed
    );
    port (
        clk     : in std_logic;
        rst_n   : in std_logic;
        nonce   : in std_logic_vector(N-1 downto 0);
        target  : in std_logic_vector(N-1 downto 0);
        init    : in std_logic;
        result  : out std_logic_vector(N-1 downto 0)
    );
end controller;

architecture behavioral of controller is
    
    -- Component declarations
    component core_fsm is
        port (
            clk     : in std_logic;
            rst_n   : in std_logic;
            nonce   : in std_logic_vector(N-1 downto 0);
            finished: out std_logic := '0';
            result  : out std_logic_vector(N-1 downto 0)
        );
    end component;
    
    component nonce_adder is 
        port (
            clk         : in std_logic;
            rst_n       : in std_logic;
            nonce       : in std_logic_vector(N - 1 downto 0);
            fsm_ready   : in std_logic;
            start       : out std_logic;
            added_nonce : out std_logic_vector(N - 1 downto 0)
        );
    end component;

    component result_compare is  
        port (
            clk         : in std_logic;
            rst_n       : in std_logic;
            target      : in std_logic_vector(N - 1 downto 0);
            result      : in std_logic_vector(N - 1 downto 0);
            fsm_ready   : in std_logic;
            comp_sig    : out std_logic;
            output_n    : out unsigned(N - 1 downto 0) := (others => '0')
        );
    end component;
    
    -- Internal signals
    signal current_nonce    : std_logic_vector(N-1 downto 0);
    signal next_nonce       : std_logic_vector(N-1 downto 0);
    signal hash_result      : std_logic_vector(N-1 downto 0);
    signal hash_finished    : std_logic;
    signal hash_start       : std_logic;
    signal comp_sig         : std_logic;
    signal comp_output      : unsigned(N-1 downto 0);
    signal mining_active    : std_logic;
    signal controller_rst   : std_logic;
    
    -- Producer-Consumer coordination signals
    signal hash_valid       : std_logic;
    signal hash_consumed    : std_logic;
    
begin
    
    -- Reset logic: reset when external reset or when solution found
    controller_rst <= rst_n and not comp_sig;
    
    -- Mining control process
    mining_control: process(clk, rst_n)
    begin
        if rst_n = '0' then
            mining_active <= '0';
            current_nonce <= (others => '0');
        elsif rising_edge(clk) then
            if init = '1' then
                mining_active <= '1';
                current_nonce <= nonce;  -- Load initial nonce
            elsif comp_sig = '1' then
                mining_active <= '0';    -- Stop mining when solution found
            elsif hash_finished = '1' and comp_sig = '0' then
                -- Continue mining: load next nonce when hash completes and no solution found
                current_nonce <= next_nonce;
            end if;
        end if;
    end process;
    
    -- Producer-Consumer coordination
    -- Hash is "produced" when hash_finished goes high
    -- Hash is "consumed" when comparator processes it
    hash_valid <= hash_finished;
    
    result <= std_logic_vector(comp_output);
    
    -- Component instantiations
    
    -- Hash core (Producer)
    hash_core: core_fsm
        port map (
            clk      => clk,
            rst_n    => controller_rst,
            nonce    => current_nonce,
            finished => hash_finished,
            result   => hash_result
        );
    
    -- Nonce incrementer
    nonce_inc: nonce_adder
        port map (
            clk         => clk,
            rst_n       => controller_rst,
            nonce       => current_nonce,
            fsm_ready   => hash_finished,  -- Ready to increment when hash finishes
            start       => hash_start,     -- Could be used for hash pipeline control
            added_nonce => next_nonce
        );
    
    -- Result comparator (Consumer)
    comparator: result_compare
        port map (
            clk       => clk,
            rst_n     => controller_rst,
            target    => target,
            result    => hash_result,
            fsm_ready => hash_valid,       -- Process hash when available
            comp_sig  => comp_sig,         -- Signal when solution found
            output_n  => comp_output       -- Output the successful nonce
        );

end behavioral;

