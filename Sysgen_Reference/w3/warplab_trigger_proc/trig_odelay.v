module trig_odelay (
	input          odelay_clk,
	input          odelay_rst,
    
	input          trig_output,
	output         trig_output_delayed,
	
	input [4:0]    delay_val,
	input          update_delay	
);

	reg trig_output_reg;
	
    initial begin
        trig_output_reg = 0;
    end
    
    IODELAYE1 #(
      .CINVCTRL_SEL("FALSE"),          // Enable dynamic clock inversion ("TRUE"/"FALSE") 
      .DELAY_SRC("O"),                 // Delay input ("I", "CLKIN", "DATAIN", "IO", "O")
      .HIGH_PERFORMANCE_MODE("TRUE"),  // Reduced jitter ("TRUE"), Reduced power ("FALSE")
      .ODELAY_TYPE("VAR_LOADABLE"),    // "DEFAULT", "FIXED", "VARIABLE", or "VAR_LOADABLE" 
      .ODELAY_VALUE(0),                // Input delay tap setting (0-32)
      .REFCLK_FREQUENCY(200.0),        // IDELAYCTRL clock input frequency in MHz
      .SIGNAL_PATTERN("DATA")          // "DATA" or "CLOCK" input signal
    )
    IODELAYE1_inst (
      .C(odelay_clk),                  // 1-bit input - Clock input
      .T(1'b0),                        // 1-bit input - 3-state input control port (1 - IDELAY, 0 - ODELAY)
      .RST(update_delay),              // 1-bit input - Active high, synchronous reset, resets delay chain to IDELAY_VALUE/
      
      .CE(1'b0),                       // 1-bit input - Active high enable increment/decrement function
      
      .CINVCTRL(1'b0),                 // 1-bit input - Dynamically inverts the Clock (C) polarity
      .CNTVALUEIN(delay_val),          // 5-bit input - Counter value for loadable counter application
      
      .ODATAIN(trig_output_reg),       // 1-bit input - Data input for the output datapath from the device
      
      .DATAOUT(trig_output_delayed)    // 1-bit output - Delayed data output
    );
   
	// synthesis attribute IOB of trig_output_reg is true;
	always @(posedge odelay_clk) begin
        if (odelay_rst)
            trig_output_reg <= 0;
        else
            trig_output_reg <= trig_output;
    end
endmodule
