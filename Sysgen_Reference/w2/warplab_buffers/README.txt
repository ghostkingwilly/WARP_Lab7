README

  This WARPLab Buffers core was taken from the WARP v3 WARPLab 7.5.0 release.  This is straight 
copy of the w3 Buffers core, renamed for the WARP v2 hardware, except for the following modifications:

  - Updated Model properties to reference w2_warplab_buffers_init.m
  - Updated EDK processor block to use PLB bus
  - Modified buffers interface block to use internal 32-bit memories vs external 128-bit memories
  - Updated bus definitions to remove external memories
  - Updated ADC inputs to be Fix_14_13 vs Fix_12_11
  - Updated DAC outputs to be Fix_16_15 vs Fix_12_11
  - Updated Config register to reserve *_WORD_ORDER

NOTE:  Logic exists in this core to support the same extended buffer addressing.  However, the MPD has 
  not been modified to declare the ports as interrupts and the interrupt ports have not been connected 
  to the processor.  Therefore, the Tx / Rx length should never be set greater than the supported 
  number of samples (2^14 for WARPLab 7.5.1) and the Tx / Rx IQ Thesholds should be set to the supported
  number of samples (2^14 for WARPLab 7.5.1).


