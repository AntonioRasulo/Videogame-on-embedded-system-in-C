module New_Computer_Space_Invaders (
	// Clock pins
	CLOCK_50,
	CLOCK2_50,
	
	// SDRAM
	DRAM_ADDR,
	DRAM_BA,
	DRAM_CAS_N,
	DRAM_CKE,
	DRAM_CLK,
	DRAM_CS_N,
	DRAM_DQ,
	DRAM_LDQM,
	DRAM_RAS_N,
	DRAM_UDQM,
	DRAM_WE_N,

	// Accelerometer
	G_SENSOR_CS_N,
	G_SENSOR_INT,
	G_SENSOR_SCLK,
	G_SENSOR_SDI,
	G_SENSOR_SDO,
	
	

	// Pushbuttons
	KEY,


	// VGA
	VGA_B,
	VGA_G,
	VGA_HS,
	VGA_R,
	VGA_VS
);

//=======================================================
//  PARAMETER declarations
//=======================================================


//=======================================================
//  PORT declarations
//=======================================================

// Clock pins
input						CLOCK_50;
input						CLOCK2_50;

	
// SDRAM
output 		[12: 0]	DRAM_ADDR;
output		[ 1: 0]	DRAM_BA;
output					DRAM_CAS_N;
output					DRAM_CKE;
output					DRAM_CLK;
output					DRAM_CS_N;
inout			[15: 0]	DRAM_DQ;
output					DRAM_LDQM;
output					DRAM_RAS_N;
output					DRAM_UDQM;
output					DRAM_WE_N;

// Accelerometer
output					G_SENSOR_CS_N;
input			[ 2: 1]	G_SENSOR_INT;
output					G_SENSOR_SCLK;
inout						G_SENSOR_SDI;
inout						G_SENSOR_SDO;
	


// Pushbuttons
input			[ 1: 0]	KEY;



// VGA
output		[ 3: 0]	VGA_B;
output		[ 3: 0]	VGA_G;
output					VGA_HS;
output		[ 3: 0]	VGA_R;
output					VGA_VS;


//=======================================================
//  Structural coding
//=======================================================

Computer_System The_System (
	////////////////////////////////////
	// FPGA Side
	////////////////////////////////////

	// Global signals
	.system_pll_ref_clk_clk					(CLOCK_50),
	.system_pll_ref_reset_reset			(1'b0),
	.video_pll_ref_clk_clk			(CLOCK2_50),
	.video_pll_ref_reset_reset		(1'b0),

	

	// Pushbuttons
	.pushbuttons_export						(~KEY[1:0]),

	
	// VGA Subsystem
	.vga_CLK										(),
	.vga_BLANK									(),
	.vga_SYNC									(),
	.vga_HS										(VGA_HS),
	.vga_VS										(VGA_VS),
	.vga_R										(VGA_R),
	.vga_G										(VGA_G),
	.vga_B										(VGA_B),
	
	// SDRAM
	.sdram_clk_clk								(DRAM_CLK),
	.sdram_addr									(DRAM_ADDR),
	.sdram_ba									(DRAM_BA),
	.sdram_cas_n								(DRAM_CAS_N),
	.sdram_cke									(DRAM_CKE),
	.sdram_cs_n									(DRAM_CS_N),
	.sdram_dq									(DRAM_DQ),
	.sdram_dqm									({DRAM_UDQM,DRAM_LDQM}),
	.sdram_ras_n								(DRAM_RAS_N),
	.sdram_we_n									(DRAM_WE_N),
	
	// the Accelerometer
	.accelerometer_I2C_SDAT				(G_SENSOR_SDI),
	.accelerometer_I2C_SCLK				(G_SENSOR_SCLK),
	.accelerometer_G_SENSOR_CS_N		(G_SENSOR_CS_N),
	.accelerometer_G_SENSOR_INT		(G_SENSOR_INT)
);


endmodule
