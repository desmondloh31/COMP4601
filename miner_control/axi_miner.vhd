library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
--use work.keccak_globals.all;

entity axi_miner is
	generic (
        ------------------------------------------------
        -- AXI Lite parameters
        ------------------------------------------------
		-- C_S_AXI_DATA_WIDTH	: integer	:= 32;
		C_S_AXI_DATA_WIDTH	: integer	:= 64;
		C_S_AXI_ADDR_WIDTH	: integer	:= 5
	);
	port (
        ------------------------------------------------
        -- I/O signals (PUT YOUR I/O HERE)
        ------------------------------------------------
    	ps_clk	: in std_logic; -- keccak hashing core clock

    	

        
        ------------------------------------------------
        -- AXI Lite signals
        ------------------------------------------------
		-- Global Clock Signal
		S_AXI_ACLK	: in std_logic;
		-- Global Reset Signal. This Signal is Active LOW
		S_AXI_ARESETN	: in std_logic;
		-- Write address (issued by master, acceped by Slave)
		S_AXI_AWADDR	: in std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
		-- Write channel Protection type. This signal indicates the
    		-- privilege and security level of the transaction, and whether
    		-- the transaction is a data access or an instruction access.
		S_AXI_AWPROT	: in std_logic_vector(2 downto 0);
		-- Write address valid. This signal indicates that the master signaling
    		-- valid write address and control information.
		S_AXI_AWVALID	: in std_logic;
		-- Write address ready. This signal indicates that the slave is ready
    		-- to accept an address and associated control signals.
		S_AXI_AWREADY	: out std_logic;
		-- Write data (issued by master, acceped by Slave) 
		S_AXI_WDATA	: in std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
		-- Write strobes. This signal indicates which byte lanes hold
    		-- valid data. There is one write strobe bit for each eight
    		-- bits of the write data bus.    
		S_AXI_WSTRB	: in std_logic_vector((C_S_AXI_DATA_WIDTH/8)-1 downto 0);
		-- Write valid. This signal indicates that valid write
    		-- data and strobes are available.
		S_AXI_WVALID	: in std_logic;
		-- Write ready. This signal indicates that the slave
    		-- can accept the write data.
		S_AXI_WREADY	: out std_logic;
		-- Write response. This signal indicates the status
    		-- of the write transaction.
		S_AXI_BRESP	: out std_logic_vector(1 downto 0);
		-- Write response valid. This signal indicates that the channel
    		-- is signaling a valid write response.
		S_AXI_BVALID	: out std_logic;
		-- Response ready. This signal indicates that the master
    		-- can accept a write response.
		S_AXI_BREADY	: in std_logic;
		-- Read address (issued by master, acceped by Slave)
		S_AXI_ARADDR	: in std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
		-- Protection type. This signal indicates the privilege
    		-- and security level of the transaction, and whether the
    		-- transaction is a data access or an instruction access.
		S_AXI_ARPROT	: in std_logic_vector(2 downto 0);
		-- Read address valid. This signal indicates that the channel
    		-- is signaling valid read address and control information.
		S_AXI_ARVALID	: in std_logic;
		-- Read address ready. This signal indicates that the slave is
    		-- ready to accept an address and associated control signals.
		S_AXI_ARREADY	: out std_logic;
		-- Read data (issued by slave)
		S_AXI_RDATA	: out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
		-- Read response. This signal indicates the status of the
    		-- read transfer.
		S_AXI_RRESP	: out std_logic_vector(1 downto 0);
		-- Read valid. This signal indicates that the channel is
    		-- signaling the required read data.
		S_AXI_RVALID	: out std_logic;
		-- Read ready. This signal indicates that the master can
    		-- accept the read data and response information.
		S_AXI_RREADY	: in std_logic
	);
end axi_miner;

architecture arch_imp of axi_miner is

	-- -- Hashing Core 
	-- component keccak
	-- 	generic ( N : integer := 64 );
	-- 	port (
	-- 		clk     : in  std_logic;
	-- 		rst_n   : in  std_logic;
	-- 		init    : in  std_logic;
	-- 		go      : in  std_logic;
	-- 		absorb  : in  std_logic;
	-- 		squeeze : in  std_logic;
	-- 		din     : in  std_logic_vector(N-1 downto 0);
	-- 		ready   : out std_logic;
	-- 		dout    : out std_logic_vector(N-1 downto 0)
	-- 	);
	-- end component;

	-- -- Comparator 
	-- component comparator
	-- 	generic (
	-- 	WIDTH : integer := 64
	-- 	);
	-- 	port (
	-- 	hash_val    : in  std_logic_vector(WIDTH-1 downto 0);
	-- 	target_val  : in  std_logic_vector(WIDTH-1 downto 0);
	-- 	match_found : out std_logic
	-- 	);
	-- end component;
    ------------------------------------------------
	-- AXI4LITE signals
    ------------------------------------------------
	signal axi_awaddr	: std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
	signal axi_awready	: std_logic;
	signal axi_wready	: std_logic;
	signal axi_bresp	: std_logic_vector(1 downto 0);
	signal axi_bvalid	: std_logic;
	signal axi_araddr	: std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
	signal axi_arready	: std_logic;
	signal axi_rdata	: std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	signal axi_rresp	: std_logic_vector(1 downto 0);
	signal axi_rvalid	: std_logic;

    ------------------------------------------------
    -- Control bus signals
    ------------------------------------------------
	-- local parameter for addressing 32 bit / 64 bit C_S_AXI_DATA_WIDTH
	-- ADDR_LSB is used for addressing 32/64 bit registers/memories
	-- ADDR_LSB = 2 for 32 bits (n downto 2)
	-- ADDR_LSB = 3 for 64 bits (n downto 3)
	constant ADDR_LSB  : integer := (C_S_AXI_DATA_WIDTH/32)+ 1;
	constant OPT_MEM_ADDR_BITS : integer := 1;
	
	-- AXI handshake and processing intermediate signals
	signal byte_index	: integer;
	signal aw_en	: std_logic;
    signal slv_reg_rden	: std_logic;
    signal slv_reg_wren	: std_logic;
	signal reg_data_out	:std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	
	-----------------------------------------------------------------------------
	-- Signals for user logic register space example (PUT YOUR REGISTERS HERE)

	------------------------------------------------
	-- signal ctrl_reg        : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	-- signal status_reg      : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	-- signal header_0_reg    : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	-- signal header_1_reg    : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	-- signal nonce_start_reg : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	-- signal nonce_end_reg   : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	-- signal valid_nonce_reg : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);

	signal nonce_reg        : std_logic_vector(63 downto 0);
	signal target_reg       : std_logic_vector(63 downto 0);
	signal init_reg         : std_logic;
	signal found_nonce      : std_logic_vector(63 downto 0);

	signal n_reg_din      : std_logic_vector(63 downto 0);
    signal n_reg_dout     : std_logic_vector(63 downto 0);
    signal n_reg_we       : std_logic;

    signal t_reg_din      : std_logic_vector(63 downto 0);
    signal t_reg_dout     : std_logic_vector(63 downto 0);
    signal t_reg_we       : std_logic;
    signal init_reg_we    : std_logic;

    signal ctrl_reg        : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
    signal status_reg      : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
    signal valid_nonce_reg : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
	------------------------------------------------
	-- Control components
	------------------------------------------------
	-- component core_fsm is
	-- 	port (
	-- 		clk     : in std_logic;
	-- 		rst_n   : in std_logic;
	-- 		nonce   : in std_logic_vector(N-1 downto 0);
	-- 		finished: out std_logic := '0';
	-- 		result  : out std_logic_vector(N-1 downto 0)
	-- 	);
	-- end component;

	-- component result_compare is
	-- 	port (
	-- 		clk         : in std_logic;
	-- 		rst_n       : in std_logic; -- use ready signal from PS as reset
	-- 		target      : in std_logic_vector(N - 1 downto 0);
	-- 		result      : in std_logic_vector(N - 1 downto 0);
	-- 		fsm_ready   : in std_logic;
	-- 		comp_sig    : out std_logic; -- serves as reset for other components
	-- 		output_n    : out unsigned(N - 1 downto 0) := (others => '0')
	-- 	);
	-- end component;

	-- component nonce_adder is
	-- 	port (
	-- 		clk         : in std_logic;
	-- 		rst_n       : in std_logic;
	-- 		nonce       : in std_logic_vector(N - 1 downto 0);
	-- 		fsm_ready   : in std_logic;
	-- 		-- comp_sig    : in std_logic;
	-- 		start       : out std_logic;
	-- 		added_nonce : out std_logic_vector(N - 1 downto 0)
	-- 	);
	-- end component;
	component controller is
		generic (
			N : integer := 64
		);
		port (
			clk     : in std_logic;
			rst_n   : in std_logic;
			nonce   : in std_logic_vector(N-1 downto 0);
			target  : in std_logic_vector(N-1 downto 0);
			init    : in std_logic;
			result  : out std_logic_vector(N-1 downto 0)
		);
	end component;

	component reg is 
		generic ( N : integer := 64 );
		port (
			clk         : in std_logic;
			rst_n       : in std_logic;
			enable      : in std_logic;
			din         : in std_logic_vector(N - 1 downto 0);
			dout        : out std_logic_vector(N - 1 downto 0)
		);
	end component;
    
begin
    ------------------------------------------------
	-- I/O Connections assignments
    ------------------------------------------------
	S_AXI_AWREADY	<= axi_awready;
	S_AXI_WREADY	<= axi_wready;
	S_AXI_BRESP	    <= axi_bresp;
	S_AXI_BVALID	<= axi_bvalid;
	S_AXI_ARREADY	<= axi_arready;
	S_AXI_RDATA	    <= axi_rdata;
	S_AXI_RRESP	    <= axi_rresp;
	S_AXI_RVALID	<= axi_rvalid;

	-- Instantiate Keccak core
	-- keccak_inst : keccak
	-- 	generic map ( N => 64 )
	-- 	port map (
	-- 		clk     => ps_clk,
	-- 		rst_n   => rst_n,
	-- 		init    => keccak_init,
	-- 		go      => keccak_go,
	-- 		absorb  => keccak_absorb,
	-- 		squeeze => keccak_squeeze,
	-- 		din     => header_0_reg & header_1_reg(31 downto 0),
	-- 		ready   => keccak_ready,
	-- 		dout    => keccak_dout
	-- 	);
    
    -- nonce register
	n_reg : reg
	port map(
		clk    => S_AXI_ACLK,
		rst_n  => S_AXI_ARESETN,
		enable => n_reg_we,
		din    => n_reg_din,
		dout   => n_reg_dout
	);

	-- target register
	t_reg : reg
	port map (
		clk    => S_AXI_ACLK,
		rst_n  => S_AXI_ARESETN,
		enable => t_reg_we,
		din    => t_reg_din,
		dout   => t_reg_dout
	);
    ---------------------------------------------------------
	-- FSM LOGIC TO CONTROL KECCAK HASHING CORE FOR MINING 
    ---------------------------------------------------------
    -- Assign bit 0 of led output register to the led output 
    -- LED <= LED_output_reg(0);
    -- process (S_AXI_ACLK)
	-- begin
    --     if rising_edge(S_AXI_ACLK) then 
        
    --         -- Set all bits to 0 and then capture the button input to the 
    --         -- bottom bit of sync register 
    --         button_sync_reg <= (others => '0');
    --         button_sync_reg(0) <= BUTTON;
            
    --         -- Set the button input register as the output of button sync register
    --         button_input_reg <= button_sync_reg;
    --     end if;
    -- end process;

    ------------------------------------------------
	-- SPSC controller
    ------------------------------------------------

    -- SEE control.vhd
	controller_inst : controller
	generic map ( N => 64 )
	port map (
		clk    => S_AXI_ACLK,
		rst_n  => S_AXI_ARESETN,
		nonce  => n_reg_dout,
		target => t_reg_dout,
		init   => init_reg,
		result => found_nonce
	);

    ------------------------------------------------
	-- AXI reading/writing
    ------------------------------------------------
    ------------------------------------------------------------------------------------------------
	-- STARTING THE WRITE RESPONSE FROM PS (Master) -> AXI Slave
    ------------------------------------------------------------------------------------------------
    
	
	-- Implement axi_awready generation
	-- axi_awready is asserted for one S_AXI_ACLK clock cycle when both
	-- S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_awready is
	-- de-asserted when reset is low.
	process (S_AXI_ACLK)
	begin
        if rising_edge(S_AXI_ACLK) then 
            if S_AXI_ARESETN = '0' then
                axi_awready <= '0';
                aw_en <= '1';
            else
                if (axi_awready = '0' and S_AXI_AWVALID = '1' and S_AXI_WVALID = '1' and aw_en = '1') then
                    -- slave is ready to accept write address when
                    -- there is a valid write address and write data
                    -- on the write address and data bus. This design 
                    -- expects no outstanding transactions. 
                    axi_awready <= '1';
                    aw_en <= '0';
                    elsif (S_AXI_BREADY = '1' and axi_bvalid = '1') then
                    aw_en <= '1';
                    axi_awready <= '0';
                else
                    axi_awready <= '0';
                end if;
            end if;
        end if;
	end process;

	-- Implement axi_awaddr latching
	-- This process is used to latch the address when both 
	-- S_AXI_AWVALID and S_AXI_WVALID are valid. 
	process (S_AXI_ACLK)
	begin
        if rising_edge(S_AXI_ACLK) then 
            if S_AXI_ARESETN = '0' then
                axi_awaddr <= (others => '0');
            else
                if (axi_awready = '0' and S_AXI_AWVALID = '1' and S_AXI_WVALID = '1' and aw_en = '1') then
                    -- Write Address latching
                    axi_awaddr <= S_AXI_AWADDR;
                end if;
            end if;
        end if;                   
	end process; 

	-- Implement axi_wready generation
	-- axi_wready is asserted for one S_AXI_ACLK clock cycle when both
	-- S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_wready is 
	-- de-asserted when reset is low. 
	process (S_AXI_ACLK)
	begin
        if rising_edge(S_AXI_ACLK) then 
            if S_AXI_ARESETN = '0' then
                axi_wready <= '0';
            else
                if (axi_wready = '0' and S_AXI_WVALID = '1' and S_AXI_AWVALID = '1' and aw_en = '1') then
                    -- slave is ready to accept write data when 
                    -- there is a valid write address and write data
                    -- on the write address and data bus. This design 
                    -- expects no outstanding transactions.           
                    axi_wready <= '1';
                else
                    axi_wready <= '0';
                end if;
            end if;
        end if;
	end process; 

	-- Implement memory mapped register select and write logic generation
	-- The write data is accepted and written to memory mapped registers when
	-- axi_awready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted. Write strobes are used to
	-- select byte enables of slave registers while writing.
	-- These registers are cleared when reset (active low) is applied.
	-- Slave register write enable is asserted when valid address and data are available
	-- and the slave is ready to accept the write address and write data.
	slv_reg_wren <= axi_wready and S_AXI_WVALID and axi_awready and S_AXI_AWVALID;

	-- memory maps - data from PS (AXI controller in PL)
	-- TODO debug
	process (S_AXI_ACLK)
	variable loc_addr :std_logic_vector(OPT_MEM_ADDR_BITS downto 0); 
	begin
		if rising_edge(S_AXI_ACLK) then 
			if S_AXI_ARESETN = '0' then
				n_reg_we    <= '0';
				n_reg_din   <= (others => '0');
				t_reg_we    <= '0';
				t_reg_din   <= (others => '0');
				init_reg_we <= '0';
				init_reg    <= '0';
			else
				n_reg_we    <= '0';
				t_reg_we    <= '0';
				init_reg_we <= '0';
				loc_addr := axi_awaddr(ADDR_LSB + OPT_MEM_ADDR_BITS downto ADDR_LSB);
				if (slv_reg_wren = '1') then
					case loc_addr is
						when "00" =>  -- nonce
							n_reg_din <= S_AXI_WDATA;
							n_reg_we  <= '1';
						when "01" =>  -- target
							t_reg_din <= S_AXI_WDATA;
							t_reg_we  <= '1';
						when "10" =>  -- init/start
							init_reg  <= S_AXI_WDATA(0);
							init_reg_we <= '1';
						when others =>
							null;
					end case;
				end if;
			end if;
		end if;             
	end process;

	-- This block acknowledges a successful write from master (PS) to AXI Slave
	-- Implement write response logic generation
	-- The write response and response valid signals are asserted by the slave 
	-- when axi_wready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted.  
	-- This marks the acceptance of address and indicates the status of 
	-- write transaction.
	process (S_AXI_ACLK)
	begin
        if rising_edge(S_AXI_ACLK) then 
            if S_AXI_ARESETN = '0' then
                axi_bvalid  <= '0';
                axi_bresp   <= "00"; --need to work more on the responses
            else
                if (axi_awready = '1' and S_AXI_AWVALID = '1' and axi_wready = '1' and S_AXI_WVALID = '1' and axi_bvalid = '0'  ) then
                    axi_bvalid <= '1';
                    axi_bresp  <= "00"; 
                elsif (S_AXI_BREADY = '1' and axi_bvalid = '1') then   --check if bready is asserted while bvalid is high)
                    axi_bvalid <= '0';                                 -- (there is a possibility that bready is always asserted high)
                end if;
            end if;
        end if;                   
	end process; 
	
    
    -----------------------------------------------------------------------------------------
	-- STARTING THE READ RESPONSE FROM AXI Slave -> PS Master
    -----------------------------------------------------------------------------------------

	-- This checks if there's a valid red request from PS and latches the read address
	-- Implement axi_arready generation
	-- axi_arready is asserted for one S_AXI_ACLK clock cycle when
	-- S_AXI_ARVALID is asserted. axi_awready is 
	-- de-asserted when reset (active low) is asserted. 
	-- The read address is also latched when S_AXI_ARVALID is 
	-- asserted. axi_araddr is reset to zero on reset assertion.
	process (S_AXI_ACLK)
	begin
        if rising_edge(S_AXI_ACLK) then 
            if S_AXI_ARESETN = '0' then
                axi_arready <= '0';
                axi_araddr  <= (others => '1');
            else
                if (axi_arready = '0' and S_AXI_ARVALID = '1') then
                    -- indicates that the slave has accepted the valid read address
                    axi_arready <= '1';
                    -- Read Address latching 
                    axi_araddr  <= S_AXI_ARADDR;           
                else
                    axi_arready <= '0';
                end if;
            end if;
        end if;                   
	end process; 

	-- This block generates valid read data, making it ready to be consumed by the master (PS)
	-- Implement axi_arvalid generation
	-- axi_rvalid is asserted for one S_AXI_ACLK clock cycle when both 
	-- S_AXI_ARVALID and axi_arready are asserted. The slave registers 
	-- data are available on the axi_rdata bus at this instance. The 
	-- assertion of axi_rvalid marks the validity of read data on the 
	-- bus and axi_rresp indicates the status of read transaction.axi_rvalid 
	-- is deasserted on reset (active low). axi_rresp and axi_rdata are 
	-- cleared to zero on reset (active low).  
	process (S_AXI_ACLK)
	begin
        if rising_edge(S_AXI_ACLK) then
            if S_AXI_ARESETN = '0' then
                axi_rvalid <= '0';
                axi_rresp  <= "00";
            else
                if (axi_arready = '1' and S_AXI_ARVALID = '1' and axi_rvalid = '0') then
                    -- Valid read data is available at the read data bus
                    axi_rvalid <= '1';
                    axi_rresp  <= "00"; -- 'OKAY' response
                elsif (axi_rvalid = '1' and S_AXI_RREADY = '1') then
                    -- Read data is accepted by the master
                    axi_rvalid <= '0';
                end if;            
            end if;
        end if;
	end process;
	
	-- This block will read correct register values when requested by PS (master)
	-- Implement memory mapped register select and read logic generation
	-- Slave register read enable is asserted when valid address is available
	-- and the slave is ready to accept the read address.

	-- TODO debug
	slv_reg_rden <= axi_arready and S_AXI_ARVALID and (not axi_rvalid);

	process (ctrl_reg, status_reg, valid_nonce_reg, axi_araddr, S_AXI_ARESETN, slv_reg_rden)
		variable loc_addr : std_logic_vector(OPT_MEM_ADDR_BITS downto 0);
	begin
		-- Address decoding for reading registers
		loc_addr := axi_araddr(ADDR_LSB + OPT_MEM_ADDR_BITS downto ADDR_LSB);
		case loc_addr is
			when "00" =>  -- status
				reg_data_out <= status_reg;
			when "01" =>  -- reserved
				reg_data_out <= (others => '0');
			when "10" =>  -- reserved
				reg_data_out <= (others => '0');
			when "11" =>  -- found nonce
				reg_data_out <= found_nonce;
			when others =>
				reg_data_out <= (others => '0');
		end case;
	end process;


	-- This block loads the read data into the AXI output channel to be received by the PS
	-- Output register or memory read data
	process( S_AXI_ACLK ) is
	begin
        if (rising_edge (S_AXI_ACLK)) then
            if ( S_AXI_ARESETN = '0' ) then
                axi_rdata  <= (others => '0');
            else
                if (slv_reg_rden = '1') then
                    -- When there is a valid read address (S_AXI_ARVALID) with 
                    -- acceptance of read address by the slave (axi_arready), 
                    -- output the read dada 
                    -- Read address mux
                    axi_rdata <= reg_data_out;     -- register read data
                end if;
            end if;
        end if;
	end process;
end arch_imp;
