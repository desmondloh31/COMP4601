-- =============================================================================
-- IMPLEMENTATION 1: FINITE STATE MACHINE APPROACH
-- =============================================================================

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity hash_tournament_fsm is
    generic (
        HASH_WIDTH  : integer := 256;  -- SHA-256 hash width
        NONCE_WIDTH : integer := 32    -- Nonce width
    );
    port (
        clk           : in  STD_LOGIC;
        reset         : in  STD_LOGIC;
        
        -- Input interface
        start         : in  STD_LOGIC;
        initial_nonce : in  STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
        target        : in  STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
        
        -- Hash unit interface
        hash_start    : out STD_LOGIC;
        hash_data     : out STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
        hash_result   : in  STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
        hash_valid    : in  STD_LOGIC;
        
        -- Output interface
        solution_found : out STD_LOGIC;
        solution_nonce : out STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
        solution_hash  : out STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
        busy          : out STD_LOGIC
    );
end hash_tournament_fsm;

architecture Behavioral of hash_tournament_fsm is
    
    type state_type is (IDLE, WORK, WAIT_HASH, CMP, ACCEPT);
    signal current_state, next_state : state_type;
    
    signal current_nonce : STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
    signal current_hash  : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    signal target_reg    : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    
begin

    -- State register
    state_reg: process(clk, reset)
    begin
        if reset = '1' then
            current_state <= IDLE;
        elsif rising_edge(clk) then
            current_state <= next_state;
        end if;
    end process;
    
    -- Data registers
    data_reg: process(clk, reset)
    begin
        if reset = '1' then
            current_nonce <= (others => '0');
            current_hash <= (others => '0');
            target_reg <= (others => '0');
        elsif rising_edge(clk) then
            case current_state is
                when IDLE =>
                    if start = '1' then
                        current_nonce <= initial_nonce;
                        target_reg <= target;
                    end if;
                    
                when WAIT_HASH =>
                    if hash_valid = '1' then
                        current_hash <= hash_result;
                    end if;
                    
                when CMP =>
                    -- If hash >= target, use hash as next nonce
                    if unsigned(current_hash) >= unsigned(target_reg) then
                        current_nonce <= current_hash(NONCE_WIDTH-1 downto 0);
                    end if;
                    
                when others =>
                    null;
            end case;
        end if;
    end process;
    
    -- Next state logic
    next_state_logic: process(current_state, start, hash_valid, current_hash, target_reg)
    begin
        next_state <= current_state;
        
        case current_state is
            when IDLE =>
                if start = '1' then
                    next_state <= WORK;
                end if;
                
            when WORK =>
                next_state <= WAIT_HASH;
                
            when WAIT_HASH =>
                if hash_valid = '1' then
                    next_state <= CMP;
                end if;
                
            when CMP =>
                if unsigned(current_hash) < unsigned(target_reg) then
                    next_state <= ACCEPT;
                else
                    next_state <= WORK;  -- Try again with new nonce
                end if;
                
            when ACCEPT =>
                next_state <= IDLE;  -- Wait for next start
                
        end case;
    end process;
    
    -- Output logic
    output_logic: process(current_state, current_nonce, current_hash)
    begin
        -- Default assignments
        hash_start <= '0';
        hash_data <= current_nonce;
        solution_found <= '0';
        solution_nonce <= (others => '0');
        solution_hash <= (others => '0');
        busy <= '1';
        
        case current_state is
            when IDLE =>
                busy <= '0';
                
            when WORK =>
                hash_start <= '1';
                
            when ACCEPT =>
                solution_found <= '1';
                solution_nonce <= current_nonce;
                solution_hash <= current_hash;
                
            when others =>
                null;
        end case;
    end process;

end Behavioral;


-- =============================================================================
-- IMPLEMENTATION 2: SPSC (SINGLE PRODUCER SINGLE CONSUMER) APPROACH
-- =============================================================================

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity hash_tournament_spsc is
    generic (
        HASH_WIDTH   : integer := 256;
        NONCE_WIDTH  : integer := 32;
        QUEUE_DEPTH  : integer := 8     -- Power of 2 for efficient indexing
    );
    port (
        clk           : in  STD_LOGIC;
        reset         : in  STD_LOGIC;
        
        -- Input interface
        start         : in  STD_LOGIC;
        initial_nonce : in  STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
        target        : in  STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
        
        -- Hash unit interface (multiple units possible)
        hash_start    : out STD_LOGIC;
        hash_data     : out STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
        hash_result   : in  STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
        hash_valid    : in  STD_LOGIC;
        
        -- Output interface
        solution_found : out STD_LOGIC;
        solution_nonce : out STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
        solution_hash  : out STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
        busy          : out STD_LOGIC;
        queue_full    : out STD_LOGIC;
        queue_empty   : out STD_LOGIC
    );
end hash_tournament_spsc;

architecture Behavioral of hash_tournament_spsc is
    
    -- Queue structure for SPSC communication
    type queue_entry is record
        nonce : STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
        hash  : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    end record;
    
    type queue_array is array (0 to QUEUE_DEPTH-1) of queue_entry;
    signal queue : queue_array;
    
    -- Queue pointers and control
    signal write_ptr, read_ptr : unsigned(2 downto 0);  -- log2(QUEUE_DEPTH)
    signal queue_count : unsigned(3 downto 0);
    signal queue_full_i, queue_empty_i : STD_LOGIC;
    
    -- Producer (WORK) state
    type producer_state_type is (P_IDLE, P_ACTIVE, P_WAIT_SPACE);
    signal producer_state : producer_state_type;
    signal current_nonce : STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
    
    -- Consumer (CMP) state  
    type consumer_state_type is (C_IDLE, C_ACTIVE, C_WAIT_DATA);
    signal consumer_state : consumer_state_type;
    signal target_reg : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    
    -- Control signals
    signal mining_active : STD_LOGIC;
    signal enqueue, dequeue : STD_LOGIC;
    
begin

    -- Queue management
    queue_control: process(clk, reset)
    begin
        if reset = '1' then
            write_ptr <= (others => '0');
            read_ptr <= (others => '0');
            queue_count <= (others => '0');
        elsif rising_edge(clk) then
            case (enqueue & dequeue) is
                when "10" =>  -- Enqueue only
                    if queue_count < QUEUE_DEPTH then
                        write_ptr <= write_ptr + 1;
                        queue_count <= queue_count + 1;
                    end if;
                    
                when "01" =>  -- Dequeue only
                    if queue_count > 0 then
                        read_ptr <= read_ptr + 1;
                        queue_count <= queue_count - 1;
                    end if;
                    
                when "11" =>  -- Both enqueue and dequeue
                    write_ptr <= write_ptr + 1;
                    read_ptr <= read_ptr + 1;
                    -- queue_count remains the same
                    
                when others =>
                    null;
            end case;
        end if;
    end process;
    
    -- Queue data management
    queue_data: process(clk)
    begin
        if rising_edge(clk) then
            if enqueue = '1' and queue_count < QUEUE_DEPTH then
                queue(to_integer(write_ptr)).nonce <= current_nonce;
                queue(to_integer(write_ptr)).hash <= hash_result;
            end if;
        end if;
    end process;
    
    -- Queue status signals
    queue_full_i <= '1' when queue_count = QUEUE_DEPTH else '0';
    queue_empty_i <= '1' when queue_count = 0 else '0';
    queue_full <= queue_full_i;
    queue_empty <= queue_empty_i;
    
    -- Producer (WORK stage) - generates hashes
    producer: process(clk, reset)
    begin
        if reset = '1' then
            producer_state <= P_IDLE;
            current_nonce <= (others => '0');
        elsif rising_edge(clk) then
            case producer_state is
                when P_IDLE =>
                    if start = '1' then
                        current_nonce <= initial_nonce;
                        producer_state <= P_ACTIVE;
                    end if;
                    
                when P_ACTIVE =>
                    if mining_active = '0' then
                        producer_state <= P_IDLE;
                    elsif queue_full_i = '1' then
                        producer_state <= P_WAIT_SPACE;
                    elsif hash_valid = '1' then
                        -- Continue with next nonce if queue has space
                        if queue_full_i = '0' then
                            current_nonce <= std_logic_vector(unsigned(current_nonce) + 1);
                        end if;
                    end if;
                    
                when P_WAIT_SPACE =>
                    if queue_full_i = '0' then
                        producer_state <= P_ACTIVE;
                    elsif mining_active = '0' then
                        producer_state <= P_IDLE;
                    end if;
                    
            end case;
        end if;
    end process;
    
    -- Consumer (CMP stage) - compares hashes against target
    consumer: process(clk, reset)
    begin
        if reset = '1' then
            consumer_state <= C_IDLE;
            target_reg <= (others => '0');
            mining_active <= '0';
        elsif rising_edge(clk) then
            case consumer_state is
                when C_IDLE =>
                    if start = '1' then
                        target_reg <= target;
                        mining_active <= '1';
                        consumer_state <= C_ACTIVE;
                    end if;
                    
                when C_ACTIVE =>
                    if queue_empty_i = '1' then
                        consumer_state <= C_WAIT_DATA;
                    else
                        -- Check if current hash meets target
                        if unsigned(queue(to_integer(read_ptr)).hash) < unsigned(target_reg) then
                            mining_active <= '0';  -- Solution found
                            consumer_state <= C_IDLE;
                        end if;
                    end if;
                    
                when C_WAIT_DATA =>
                    if queue_empty_i = '0' then
                        consumer_state <= C_ACTIVE;
                    elsif producer_state = P_IDLE then
                        mining_active <= '0';
                        consumer_state <= C_IDLE;
                    end if;
                    
            end case;
        end if;
    end process;
    
    -- Control signal generation
    hash_start <= '1' when (producer_state = P_ACTIVE and queue_full_i = '0') else '0';
    hash_data <= current_nonce;
    
    enqueue <= hash_valid and (not queue_full_i);
    dequeue <= (not queue_empty_i) and 
               ((consumer_state = C_ACTIVE) or (consumer_state = C_WAIT_DATA and queue_empty_i = '0'));
    
    -- Output assignments
    solution_found <= '1' when (consumer_state = C_ACTIVE and queue_empty_i = '0' and 
                               unsigned(queue(to_integer(read_ptr)).hash) < unsigned(target_reg)) else '0';
    
    solution_nonce <= queue(to_integer(read_ptr)).nonce when solution_found = '1' else (others => '0');
    solution_hash <= queue(to_integer(read_ptr)).hash when solution_found = '1' else (others => '0');
    
    busy <= mining_active;

end Behavioral;


-- =============================================================================
-- TESTBENCH FOR COMPARISON
-- =============================================================================

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity tb_hash_tournament_comparison is
end tb_hash_tournament_comparison;

architecture Behavioral of tb_hash_tournament_comparison is
    
    constant HASH_WIDTH : integer := 256;
    constant NONCE_WIDTH : integer := 32;
    constant CLK_PERIOD : time := 10 ns;
    
    -- Common signals
    signal clk : STD_LOGIC := '0';
    signal reset : STD_LOGIC := '0';
    signal start : STD_LOGIC := '0';
    signal initial_nonce : STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
    signal target : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    
    -- Mock hash unit (same for both implementations)
    signal hash_start_fsm, hash_start_spsc : STD_LOGIC;
    signal hash_data_fsm, hash_data_spsc : STD_LOGIC_VECTOR(NONCE_WIDTH-1 downto 0);
    signal hash_result : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    signal hash_valid : STD_LOGIC := '0';
    
    -- FSM outputs
    signal solution_found_fsm, busy_fsm : STD_LOGIC;
    signal solution_nonce_fsm, solution_hash_fsm : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    
    -- SPSC outputs
    signal solution_found_spsc, busy_spsc, queue_full, queue_empty : STD_LOGIC;
    signal solution_nonce_spsc, solution_hash_spsc : STD_LOGIC_VECTOR(HASH_WIDTH-1 downto 0);
    
begin
    
    -- Clock generation
    clk <= not clk after CLK_PERIOD/2;
    
    -- FSM implementation
    fsm_inst: entity work.hash_tournament_fsm
        generic map (
            HASH_WIDTH => HASH_WIDTH,
            NONCE_WIDTH => NONCE_WIDTH
        )
        port map (
            clk => clk,
            reset => reset,
            start => start,
            initial_nonce => initial_nonce,
            target => target,
            hash_start => hash_start_fsm,
            hash_data => hash_data_fsm,
            hash_result => hash_result,
            hash_valid => hash_valid,
            solution_found => solution_found_fsm,
            solution_nonce => solution_nonce_fsm,
            solution_hash => solution_hash_fsm,
            busy => busy_fsm
        );
    
    -- SPSC implementation  
    spsc_inst: entity work.hash_tournament_spsc
        generic map (
            HASH_WIDTH => HASH_WIDTH,
            NONCE_WIDTH => NONCE_WIDTH,
            QUEUE_DEPTH => 8
        )
        port map (
            clk => clk,
            reset => reset,
            start => start,
            initial_nonce => initial_nonce,
            target => target,
            hash_start => hash_start_spsc,
            hash_data => hash_data_spsc,
            hash_result => hash_result,
            hash_valid => hash_valid,
            solution_found => solution_found_spsc,
            solution_nonce => solution_nonce_spsc,  
            solution_hash => solution_hash_spsc,
            busy => busy_spsc,
            queue_full => queue_full,
            queue_empty => queue_empty
        );
    
    -- Simple hash mock (XOR-based for simulation)
    hash_mock: process(clk)
        variable hash_delay : integer := 0;
    begin
        if rising_edge(clk) then
            hash_valid <= '0';
            
            if hash_start_fsm = '1' or hash_start_spsc = '1' then
                hash_delay := 3;  -- 3 cycle hash delay
            elsif hash_delay > 0 then
                hash_delay := hash_delay - 1;
                if hash_delay = 0 then
                    -- Simple hash function for testing
                    hash_result <= std_logic_vector(unsigned(hash_data_fsm) xor 
                                                   unsigned(hash_data_spsc) xor 
                                                   x"DEADBEEF_CAFEBABE_12345678_87654321_ABCDEF01_23456789_FEDCBA98_76543210");
                    hash_valid <= '1';
                end if;
            end if;
        end if;
    end process;
    
    -- Test stimulus
    stimulus: process
    begin
        -- Reset
        reset <= '1';
        wait for 50 ns;
        reset <= '0';
        
        -- Set target (high value so we find solution quickly)
        target <= x"F000000F_F000000F_F000000F_F000000F_F000000F_F000000F_F000000F_F000000F";
        initial_nonce <= x"00000001";
        
        -- Start mining
        wait for 20 ns;
        start <= '1';
        wait for CLK_PERIOD;
        start <= '0';
        
        -- Wait for solutions or timeout
        wait for 1000 ns;
        
        -- End simulation
        wait;
    end process;
    
end Behavioral;
