module trig_idelay (
	input          idelay_clk,
    input          idelay_rst,
    
	input          input_pin,
	output reg     input_delayed,
	
	input [4:0]    delay_val,
	input          update_delay
);

	wire trig_idelay_out;

    initial begin
        input_delayed = 0;
    end
	
    IODELAYE1 #(
      .CINVCTRL_SEL("FALSE"),          // Enable dynamic clock inversion ("TRUE"/"FALSE") 
      .DELAY_SRC("I"),                 // Delay input ("I", "CLKIN", "DATAIN", "IO", "O")
      .HIGH_PERFORMANCE_MODE("TRUE"),  // Reduced jitter ("TRUE"), Reduced power ("FALSE")
      .IDELAY_TYPE("VAR_LOADABLE"),    // "DEFAULT", "FIXED", "VARIABLE", or "VAR_LOADABLE" 
      .IDELAY_VALUE(0),                // Input delay tap setting (0-32)
      .REFCLK_FREQUENCY(200.0),        // IDELAYCTRL clock input frequency in MHz
      .SIGNAL_PATTERN("DATA")          // "DATA" or "CLOCK" input signal
    )
    IODELAYE1_inst (
      .C(idelay_clk),                  // 1-bit input - Clock input
      
      .RST(update_delay),              // 1-bit input - Active high, synchronous reset, resets delay chain to IDELAY_VALUE/
      
      .CE(1'b0),                       // 1-bit input - Active high enable increment/decrement function
      
      .CINVCTRL(1'b0),                 // 1-bit input - Dynamically inverts the Clock (C) polarity
      .CNTVALUEIN(delay_val),          // 5-bit input - Counter value for loadable counter application

      .IDATAIN(input_pin),             // 1-bit input - Delay data input

      .DATAOUT(trig_idelay_out)        // 1-bit output - Delayed data output
    );
   
	//synthesis attribute IOB of input_delayed is true;
	always @(posedge idelay_clk) begin
        if (idelay_rst)
            input_delayed <= 0;
        else
            input_delayed <= trig_idelay_out;
    end
endmodule
