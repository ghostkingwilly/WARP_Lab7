README

  This Trigger Manager core was taken from the WARP v3 WARPLab 7.5.1 release.  This is straight 
copy of the w3 Trigger manager, renamed for the WARP v2 hardware, except for the following modifications:

  - Updated Model properties to reference w2_warplab_trigger_proc_init.m
  - Removed CM-PLL support (both on debug inputs and outputs)
  - Removed ETH A HW trigger support and all of ETH B trigger support, since it is not supported by the WARP v2 hardware
  - Updated EDK processor block to use PLB bus and removed Ethernet Trigger registers
  - Added pulse extender to the AGC trigger output since AGC runs at a slower clock speed
    - i.e.  The AGC core runs at 80 MHz in WARP v2 due to timing issues vs 160 MHz in WARP v3



