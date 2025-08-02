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


