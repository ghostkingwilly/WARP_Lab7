
function trig_odelays_config(this_block)

  % Revision History:
  %
  %   22-Oct-2015  (17:35 hours):
  %     Original code was machine generated by Xilinx's System Generator after parsing
  %     C:\work\svn_work\WARP\ResearchApps\PHY\WARPLAB\WARPLab7\Sysgen_Reference\w3\warplab_trigger_proc\trig_odelays.v
  %
  %

  this_block.setTopLevelLanguage('Verilog');

  this_block.setEntityName('trig_odelays');

  % System Generator has to assume that your entity  has a combinational feed through; 
  %   if it  doesn't, then comment out the following line:
  this_block.tagAsCombinational;

  this_block.addSimulinkInport('trigs_disable');
  this_block.addSimulinkInport('trigs_in');
  this_block.addSimulinkInport('trig0_dly');
  this_block.addSimulinkInport('trig1_dly');
  this_block.addSimulinkInport('trig2_dly');
  this_block.addSimulinkInport('trig3_dly');
  this_block.addSimulinkInport('update_delays');

  this_block.addSimulinkOutport('trigs_out_pins');

  trigs_out_pins_port = this_block.port('trigs_out_pins');
  trigs_out_pins_port.setType('UFix_4_0');

  % -----------------------------
  if (this_block.inputTypesKnown)
    % do input type checking, dynamic output type and generic setup in this code block.

    if (this_block.port('trigs_disable').width ~= 1);
      this_block.setError('Input data type for port "trigs_disable" must have width=1.');
    end

    if (this_block.port('trigs_in').width ~= 4);
      this_block.setError('Input data type for port "trigs_in" must have width=4.');
    end

    if (this_block.port('trig0_dly').width ~= 5);
      this_block.setError('Input data type for port "trig0_dly" must have width=5.');
    end

    if (this_block.port('trig1_dly').width ~= 5);
      this_block.setError('Input data type for port "trig1_dly" must have width=5.');
    end

    if (this_block.port('trig2_dly').width ~= 5);
      this_block.setError('Input data type for port "trig2_dly" must have width=5.');
    end

    if (this_block.port('trig3_dly').width ~= 5);
      this_block.setError('Input data type for port "trig3_dly" must have width=5.');
    end

    if (this_block.port('update_delays').width ~= 1);
      this_block.setError('Input data type for port "update_delays" must have width=1.');
    end

    this_block.port('trigs_disable').useHDLVector(false);
    this_block.port('update_delays').useHDLVector(false);

  end  % if(inputTypesKnown)
  % -----------------------------

  % -----------------------------
   if (this_block.inputRatesKnown)
     setup_as_single_rate(this_block,'clk','ce')
   end  % if(inputRatesKnown)
  % -----------------------------

    % (!) Set the inout port rate to be the same as the first input 
    %     rate. Change the following code if this is untrue.
    uniqueInputRates = unique(this_block.getInputRates);


  % Add addtional source files as needed.
  %  |-------------
  %  | Add files in the order in which they should be compiled.
  %  | If two files "a.vhd" and "b.vhd" contain the entities
  %  | entity_a and entity_b, and entity_a contains a
  %  | component of type entity_b, the correct sequence of
  %  | addFile() calls would be:
  %  |    this_block.addFile('b.vhd');
  %  |    this_block.addFile('a.vhd');
  %  |-------------

  %    this_block.addFile('');
  %    this_block.addFile('');
  this_block.addFile('trig_odelay.v');
  this_block.addFile('trig_odelays.v');

return;


% ------------------------------------------------------------

function setup_as_single_rate(block,clkname,cename) 
  inputRates = block.inputRates; 
  uniqueInputRates = unique(inputRates); 
  if (length(uniqueInputRates)==1 & uniqueInputRates(1)==Inf) 
    block.addError('The inputs to this block cannot all be constant.'); 
    return; 
  end 
  if (uniqueInputRates(end) == Inf) 
     hasConstantInput = true; 
     uniqueInputRates = uniqueInputRates(1:end-1); 
  end 
  if (length(uniqueInputRates) ~= 1) 
    block.addError('The inputs to this block must run at a single rate.'); 
    return; 
  end 
  theInputRate = uniqueInputRates(1); 
  for i = 1:block.numSimulinkOutports 
     block.outport(i).setRate(theInputRate); 
  end 
  block.addClkCEPair(clkname,cename,theInputRate); 
  return; 

% ------------------------------------------------------------

