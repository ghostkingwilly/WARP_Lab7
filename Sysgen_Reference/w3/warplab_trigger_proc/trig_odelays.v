module trig_odelays (
	input          clk,
	input          ce,
	
    input          trigs_disable,
	input  [3:0]   trigs_in,
	output [3:0]   trigs_out_pins,
	
	input  [4:0]   trig0_dly,
	input  [4:0]   trig1_dly,
	input  [4:0]   trig2_dly,
	input  [4:0]   trig3_dly,

	input          update_delays
);

	trig_odelay trig0_odelay (
		.odelay_clk(clk),
        .odelay_rst(trigs_disable),
		.trig_output(trigs_in[0]),
		.trig_output_delayed(trigs_out_pins[0]),
		.delay_val(trig0_dly),
		.update_delay(update_delays)
	);

	trig_odelay trig1_odelay (
		.odelay_clk(clk),
        .odelay_rst(trigs_disable),
		.trig_output(trigs_in[1]),
		.trig_output_delayed(trigs_out_pins[1]),
		.delay_val(trig1_dly),
		.update_delay(update_delays)
	);

	trig_odelay trig2_odelay (
		.odelay_clk(clk),
        .odelay_rst(trigs_disable),
		.trig_output(trigs_in[2]),
		.trig_output_delayed(trigs_out_pins[2]),
		.delay_val(trig2_dly),
		.update_delay(update_delays)
	);

	trig_odelay trig3_odelay (
		.odelay_clk(clk),
        .odelay_rst(trigs_disable),
		.trig_output(trigs_in[3]),
		.trig_output_delayed(trigs_out_pins[3]),
		.delay_val(trig3_dly),
		.update_delay(update_delays)
	);

endmodule
