README

  This AGC core originated from the 802.11 reference design and was modified for WARPLab
as part of the WARPLab 7.5.0 release.  This is straight copy of the w3 AGC, renamed for the
WARP v2 hardware, except for the following modifications:

  - Updated Model properties to reference w3_warplab_agc_init.m
  - Updated EDK processor block to use PLB bus
  - Updated ADC inputs to be Fix_14_13 vs Fix_12_11
  - Updated pipeline for new 14 bit inputs
    - DCO correlator increased bit widths from 17 to 19
    - NOTE:  The IQ Magnitude calculations are the same as before because the extra precision does not matter
  - Changed sysgen clock from 160 MHz to 80 MHz
    - Updated IQ valid generation

  

