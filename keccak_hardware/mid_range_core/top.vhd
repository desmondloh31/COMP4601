library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity top is
  Port (
    rst_n   : in  std_logic;
    init    : in  std_logic;
    go      : in  std_logic;
    absorb  : in  std_logic;
    squeeze : in  std_logic;
    ready   : out std_logic
  );
end top;

architecture Behavioral of top is

  -- Set this to match your Keccak_globals package!
  constant N : integer := 64;

  -- Keccak component declaration
  component keccak
    Port (
      clk     : in  std_logic;
      rst_n   : in  std_logic;
      init    : in  std_logic;
      go      : in  std_logic;
      absorb  : in  std_logic;
      squeeze : in  std_logic;
      din     : in  std_logic_vector(N-1 downto 0);
      ready   : out std_logic;
      dout    : out std_logic_vector(N-1 downto 0)
    );
  end component;

  -- Dummy test pattern
  signal din_signal  : std_logic_vector(N-1 downto 0) := (others => '0');
  signal dout_signal : std_logic_vector(N-1 downto 0);

  -- PS clock signal coming in from Block Design
  signal ps_clk : std_logic;

begin

  -- Keccak instance using PS clock
  keccak_inst : keccak
    port map (
      clk     => ps_clk,
      rst_n   => rst_n,
      init    => init,
      go      => go,
      absorb  => absorb,
      squeeze => squeeze,
      din     => din_signal,
      ready   => ready,
      dout    => dout_signal
    );

end Behavioral;
