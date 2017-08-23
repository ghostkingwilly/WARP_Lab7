module trig_idelays (
	input          clk,
	input          ce,

    input  [3:0]   trigs_disable,	
	input  [3:0]   trigs_in_pin,
	output [3:0]   trigs_in_delayed,
	
	input  [4:0]   trig0_dly,
	input  [4:0]   trig1_dly,
	input  [4:0]   trig2_dly,
	input  [4:0]   trig3_dly,

	input          update_delays
);

	trig_idelay trig0_idelay (
		.idelay_clk(clk),
        .idelay_rst(trigs_disable[0]),
		.input_pin(trigs_in_pin[0]),
		.input_delayed(trigs_in_delayed[0]),
		.delay_val(trig0_dly),
		.update_delay(update_delays)
	);

	trig_idelay trig1_idelay (
		.idelay_clk(clk),
        .idelay_rst(trigs_disable[1]),
		.input_pin(trigs_in_pin[1]),
		.input_delayed(trigs_in_delayed[1]),
		.delay_val(trig1_dly),
		.update_delay(update_delays)
	);

	trig_idelay trig2_idelay (
		.idelay_clk(clk),
        .idelay_rst(trigs_disable[2]),
		.input_pin(trigs_in_pin[2]),
		.input_delayed(trigs_in_delayed[2]),
		.delay_val(trig2_dly),
		.update_delay(update_delays)
	);

	trig_idelay trig3_idelay (
		.idelay_clk(clk),
        .idelay_rst(trigs_disable[3]),
		.input_pin(trigs_in_pin[3]),
		.input_delayed(trigs_in_delayed[3]),
		.delay_val(trig3_dly),
		.update_delay(update_delays)
	);

endmodule
