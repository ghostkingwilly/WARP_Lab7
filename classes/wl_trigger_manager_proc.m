%-------------------------------------------------------------------------
% WARPLab Framework
%
% Copyright 2013, Mango Communications. All rights reserved.
%           Distributed under the WARP license  (http://warpproject.org/license)
%
% Chris Hunter (chunter [at] mangocomm.com)
% Patrick Murphy (murphpo [at] mangocomm.com)
% Erik Welsh (welsh [at] mangocomm.com)
%-------------------------------------------------------------------------

classdef wl_trigger_manager_proc < wl_trigger_manager & handle_light
% Trigger manager object
%     User code should not use this object directly -- the parent wl_node will
%     instantiate the appropriate baseband object for the hardware in use
    properties (SetAccess = public)
        coreVersion;
        input_delayStep_ns;
        input_delayMax_ns;
        output_delayStep_ns;
        output_delayMax_ns;
    end
    
    properties (SetAccess = protected)
        description;                             % Description of this trigger manager object
        triggerInputIDs;
        triggerOutputIDs;
        numInputs; 
        numOutputs;
    end
    
    properties(Hidden = true,Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wl_trigger_manager.h
        GRP                            = 'trigger_manager';

        CMD_ADD_ETHERNET_TRIG          = 1;                % 0x000001
        CMD_DEL_ETHERNET_TRIG          = 2;                % 0x000002
        CMD_CLR_ETHERNET_TRIGS         = 3;                % 0x000003
        CMD_HW_SW_ETHERNET_TRIG        = 4;                % 0x000004

        CMD_INPUT_SEL                  = 16;               % 0x000010
        CMD_OUTPUT_DELAY               = 17;               % 0x000011
        CMD_OUTPUT_HOLD                = 18;               % 0x000012
        CMD_OUTPUT_READ                = 19;               % 0x000013
        CMD_OUTPUT_CLEAR               = 20;               % 0x000014

        CMD_INPUT_ENABLE               = 32;               % 0x000020
        CMD_INPUT_DEBOUNCE             = 33;               % 0x000021
        CMD_INPUT_DELAY                = 34;               % 0x000022
        CMD_IDELAY                     = 35;               % 0x000023
        CMD_ODELAY                     = 36;               % 0x000024

        CMD_ENERGY_BUSY_THRESH         = 48;               % 0x000030
        CMD_ENERGY_AVG_LEN             = 49;               % 0x000031
        CMD_ENERGY_BUSY_MIN_LEN        = 50;               % 0x000032
        CMD_ENERGY_IFC_SELECTION       = 51;               % 0x000033

        CMD_TEST_TRIGGER               = 128;              % 0x000080
        
        TRIGGER_INPUT_FLAG             = hex2dec('80000000');        % Used to check trigger input IDs vs trigger output IDs
    end
    
    methods
        function obj = wl_trigger_manager_proc()
            obj.description = 'WARPLab Trigger Configuration Module';
        end 
        
        function out = setNumInputs(obj, N)
            obj.numInputs          = N;
            obj.triggerInputIDs    = 1:N;
            obj.triggerInputIDs    = obj.triggerInputIDs + obj.TRIGGER_INPUT_FLAG;  % Set "Trigger Input" Flag
            obj.input_delayStep_ns = zeros(1,N);
            obj.input_delayMax_ns  = zeros(1,N);
        end
        
        function out = setNumOutputs(obj, N)
            obj.numOutputs          = N;
            obj.triggerOutputIDs    = 1:N;        
            obj.output_delayStep_ns = zeros(1,N);
            obj.output_delayMax_ns  = zeros(1,N);
        end
        
        function out = isInputID(obj, input_ids)
            out = true;
            
            if (~isempty(input_ids))
                for index = 1:length(input_ids)
                    if (bitand(input_ids(index), obj.TRIGGER_INPUT_FLAG) ~= obj.TRIGGER_INPUT_FLAG)
                        out = false;
                    end
                end
            end
        end
        
        function out = isOutputID(obj, output_ids)
            out = true;
            
            if (~isempty(output_ids))
                for index = 1:length(output_ids)
                    if (bitand(output_ids(index), obj.TRIGGER_INPUT_FLAG) == obj.TRIGGER_INPUT_FLAG)
                        out = false;
                    end
                end
            end
        end
        
        function out = procCmd(obj, nodeInd, node, varargin)
            % wl_trigger_manager procCmd(obj, nodeInd, node, varargin)
            %     obj:       Node object (when called using dot notation)
            %     nodeInd:   Index of the current node, when wl_node is iterating over nodes
            %     node:      Current node object
            %     varargin:
            %         [1]    Command string
            %         [2:N}  Command arguments
            %
            out    = [];
            cmdStr = varargin{1};
            cmdStr = lower(cmdStr);
            
            if(length(varargin) > 1)
               varargin = varargin(2:end);
            else
               varargin = {};
            end
            
            switch(cmdStr)
                %---------------------------------------------------------
                case 'add_ethernet_trigger'
                    % Associates node to a trigger input
                    %
                    % Arguments: (wl_trigger_manager TRIGGER)
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_ADD_ETHERNET_TRIG));
                    
                    triggers  = varargin{1};
                    triggerID = uint32(0);
                    
                    for index = 1:length(triggers)
                        triggerID = bitor(triggerID, triggers(index).ID);
                    end
                    
                    myCmd.addArgs(triggerID);                        
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'delete_ethernet_trigger'
                    % De-associates node to a trigger input
                    %
                    % Arguments: (wl_trigger_manager TRIGGER)
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_DEL_ETHERNET_TRIG));
                    
                    triggers  = varargin{1};
                    triggerID = uint32(0);
                    
                    for index = 1:length(triggers)
                        triggerID = bitor(triggerID, triggers(index).ID);
                    end
                    
                    myCmd.addArgs(triggerID);                        
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'clear_ethernet_triggers'
                    % Clears all trigger associations in the node
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_CLR_ETHERNET_TRIGS));
                    
                    node.sendCmd(myCmd);     

                %---------------------------------------------------------
                case 'set_ethernet_trigger_type'
                    % Set the Ethernet trigger type
                    %
                    %   In WARP v3, the ability was added to the trigger manager to "sniff"
                    % the axi stream between the Ethernet controller and the AXI interface
                    % to the Ethernet controller, such as the AXI DMA or AXI FIFO.  This 
                    % allows for more predictable timing for Ethernet triggers since it does
                    % not depend on any SW interaction.  However, this feature could not be 
                    % added to the WARP v2 trigger manager due to differences in Ethernet 
                    % components.  Therefore, in reference designs since this feature was 
                    % introduced, WARP v3 has used hardware for Ethernet triggers while WARP v2
                    % has used software.
                    % 
                    %   Unfortunately, this introduced a large timing difference between WARP v2
                    % and WARP v3 when asserting the Ethernet trigger within the node in response
                    % to a broadcast Ethernet trigger.  This adds a complication to experiments
                    % involving a mix of WARP v2 and WARP v3 nodes.  Therefore, trigger manager
                    % v1.04.a addressed this by being able to select between using the hardware
                    % capabilities WARP v3 or using software, like WARP v2, for Ethernet triggers.
                    % 
                    %   This command allows users to set whether the WARP v3 node uses hardware
                    % or software for Ethernet triggers.  Since WARP v2 does not support using 
                    % hardware for Ethernet triggers, this command does nothing.
                    %
                    % Arguments: 'hardware' or 'software'
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_HW_SW_ETHERNET_TRIG));
                    
                    % See values in wl_trigger_manager.h for ETH_TRIG_* defined values
                    switch(varargin{1})
                        case 'hardware'
                            trigger_type = uint32(0);

                        case 'software'
                            trigger_type = uint32(1);
                        
                        otherwise
                            error('Unsupported argument ''%s''.  Please use ''hardware'' or ''software''.', varargin{1});
                    end
                    
                    myCmd.addArgs(trigger_type);                        
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'get_ethernet_trigger'
                    % Reads current trigger association from node
                    %
                    % Arguments: node
                    % Returns: (uint32 TRIGGER_ASSOCIATION)
                    %   TRIGGER_ASSOCIATION: bit-wise AND of associated trigger IDs
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_ADD_ETHERNET_TRIG));
                    
                    triggerID = uint32(0);
                    
                    myCmd.addArgs(triggerID);                        
                    resp = node.sendCmd(myCmd);

                    out  = resp.getArgs();
                
                %---------------------------------------------------------
                case 'output_config_input_selection'
                    % Selects which trigger inputs drive the selected outputs
                    % Arguments: (uint32 OUTPUTS), (uint32 OR_INPUTS), [optional] (uint32 AND_INPUTS)
                    % Returns: none
                    %
                    % OUTPUTS:      vector of output trigger IDs, provided by
                    %               wl_getTriggerOutputIDs
                    %
                    % OR_INPUTS:    vector of input trigger IDs, provided by
                    %               wl_getTriggerInputIDs. Any triggers in
                    %               this vector that assert will cause the
                    %               output trigger to assert.
                    %
                    % AND_INPUTS:   vector of input trigger IDs, provided by
                    %               wl_getTriggerInputIDs. Only if all triggers
                    %               in this vector assert will the output 
                    %               trigger assert.   
                    %
                    % NOTE: This command replaces the current input selection on the board for the 
                    %     specified outputs (unspecified outputs are not modified).  The previous 
                    %     state of the output is not saved. 
                    %     
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_INPUT_SEL));
                    
                    if(length(varargin) == 2)
                        OUTPUTS    = varargin{1};
                        OR_INPUTS  = varargin{2};
                        AND_INPUTS = [];
                    elseif(length(varargin) == 3)
                        OUTPUTS    = varargin{1};
                        OR_INPUTS  = varargin{2};
                        AND_INPUTS = varargin{3};
                    end

                    % Check IDs
                    if (~obj.isOutputID(OUTPUTS))
                        error('Output trigger ID invalid')
                    end
                    
                    if (~obj.isInputID(OR_INPUTS))
                        error('OR Input trigger ID invalid')
                    else
                        OR_INPUTS = OR_INPUTS - obj.TRIGGER_INPUT_FLAG;        % Strip flag to pass value to node
                    end
                    
                    if (~obj.isInputID(AND_INPUTS))
                        error('AND Input trigger ID invalid')
                    else
                        AND_INPUTS = AND_INPUTS - obj.TRIGGER_INPUT_FLAG;      % Strip flag to pass value to node
                    end
                    
                    if(~isempty(OUTPUTS))
                        myCmd.addArgs(uint32(length(OUTPUTS)));
                        myCmd.addArgs(uint32(OUTPUTS));
                    else
                        error('Output trigger argument must be non-empty')
                    end
                    
                    myCmd.addArgs(uint32(length(OR_INPUTS)));
                    
                    if(~isempty(OR_INPUTS))
                        myCmd.addArgs(uint32(OR_INPUTS));
                    end
                    
                    myCmd.addArgs(uint32(length(AND_INPUTS)));
                    
                    if(~isempty(AND_INPUTS))
                        myCmd.addArgs(uint32(AND_INPUTS));
                    end
                    
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'output_config_delay'
                    % Configures specified output triggers to be have an
                    % additional delay relative to their inputs
                    %
                    % Arguments: (uint32 OUTPUTS), (double DELAY_NS)
                    % Returns: none
                    %
                    % OUTPUTS:      vector of output trigger IDs, provided by
                    %               wl_getTriggerOutputIDs
                    %
                    % DELAY_NS:     scalar value of the intended delay,
                    %               specified in nanoseconds (1e-9 seconds)
                    %                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_OUTPUT_DELAY));

                    OUTPUTS  = varargin{1};
                    delay_ns = varargin{2};
                    
                    % Check IDs
                    if (~obj.isOutputID(OUTPUTS))
                        error('Output trigger ID invalid')
                    end
                    
                    if(~isempty(OUTPUTS))
                        myCmd.addArgs(uint32(length(OUTPUTS)));
                        myCmd.addArgs(uint32(OUTPUTS));
                    else
                        error('Output trigger argument must be non-empty')
                    end                   
                                                           
                    for k = 1:length(OUTPUTS)
                       index = find(OUTPUTS(k) == obj.triggerOutputIDs); 
                        
                       if k == 1
                          enforced_step = obj.output_delayStep_ns(index);
                          enforced_max  = obj.output_delayMax_ns(index);
                       else
                          enforced_step = max([enforced_step, obj.output_delayStep_ns(index)]);
                          enforced_max  = min([enforced_max, obj.output_delayMax_ns(index)]);
                       end
                    end
                    
                    if (delay_ns < enforced_max) 
                        requested_delay = floor(delay_ns/enforced_step);
                    else
                        requested_delay = floor(enforced_max/enforced_step);
                    end
                    
                    if ( ~(requested_delay == delay_ns/enforced_step) )
                        warning('Node %d with Serial # %d has a delay step of %.3f ns and a maximum delay of %.3f ns.\nRequested output delay of %.3f ns actually set to %.3f ns.', node.ID, node.serialNumber, enforced_step, enforced_max, delay_ns, enforced_step*requested_delay);
                    end
                    
                    myCmd.addArgs(requested_delay);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'output_config_hold_mode'
                    % Configures whether specified output triggers should
                    % hold their outputs once triggered
                    %
                    % Arguments: (uint32 OUTPUTS), (boolean MODE)
                    % Returns: none
                    %
                    % OUTPUTS:      vector of output trigger IDs, provided by
                    %               wl_getTriggerOutputIDs
                    %
                    % MODE:         true enables output hold mode
                    %               false disables output hold mode
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_OUTPUT_HOLD));
                    
                    OUTPUTS = varargin{1};
                    mode    = varargin{2};
                    
                    if (ischar(mode)) 
                        % Print warning that this syntax will be deprecated
                        try
                            temp = evalin('base', 'wl_output_config_hold_mode_did_warn');
                        catch
                            fprintf('WARNING:   This syntax for output_config_hold_mode() is being deprecated.\n');
                            fprintf('WARNING:   Please use:  wl_triggerManagerCmd(node, ''output_config_hold_mode'', OUTPUTS, <true, false>);\n');
                            fprintf('WARNING:   See WARPLab documentation for more information\n\n');
                            
                            assignin('base', 'wl_output_config_hold_mode_did_warn', 1)
                        end
                    
                        if(strcmp(lower(mode),'enable'))
                            mode = 0;
                        elseif(strcmp(lower(mode),'disable'))
                            mode = 1;
                        else
                            error('mode selection must be ''enable'' or ''disable''')
                        end
                    elseif (islogical(mode))
                        if(mode)
                            mode = 0;
                        else
                            mode = 1;
                        end
                    else 
                        error('mode selection must be true or false')
                    end
                    
                    % Check IDs
                    if (~obj.isOutputID(OUTPUTS))
                        error('Output trigger ID invalid')
                    end
                    
                    if(~isempty(OUTPUTS))
                        myCmd.addArgs(uint32(length(OUTPUTS)));
                        myCmd.addArgs(uint32(OUTPUTS));
                    else
                        error('Output trigger argument must be non-empty')
                    end
                   
                    myCmd.addArgs(mode);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'output_state_read'
                    % Reads current state of output triggers. Note: this
                    % command is intended to be used on output triggers
                    % that have enabled their hold mode.
                    %
                    % Arguments: (uint32 OUTPUTS)
                    % Returns: (bool STATES)
                    %
                    % OUTPUTS:      vector of output trigger IDs, provided by
                    %               wl_getTriggerOutputIDs
                    %
                    % STATES:       vector of (true, false) trigger states
                    %               corresponding to state of OUTPUTS vector
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_OUTPUT_READ));
                    
                    OUTPUTS = varargin{1};

                    % Check IDs
                    if (~obj.isOutputID(OUTPUTS))
                        error('Output trigger ID invalid')
                    end
                    
                    if(~isempty(OUTPUTS))
                        myCmd.addArgs(uint32(length(OUTPUTS)));
                        myCmd.addArgs(uint32(OUTPUTS));
                    else
                        error('Output trigger argument must be non-empty')
                    end
                  
                    resp = node.sendCmd(myCmd);
                    out = resp.getArgs();
                    
                %---------------------------------------------------------
                case 'output_state_clear'
                    % Clears current state of output triggers. 
                    %
                    % Arguments: (uint32 OUTPUTS)
                    % Returns: none
                    %
                    % OUTPUTS:      vector of output trigger IDs, provided by
                    %               wl_getTriggerOutputIDs
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_OUTPUT_CLEAR));
                    
                    OUTPUTS = varargin{1};

                    % Check IDs
                    if (~obj.isOutputID(OUTPUTS))
                        error('Output trigger ID invalid')
                    end
                    
                    if(~isempty(OUTPUTS))
                        myCmd.addArgs(uint32(length(OUTPUTS)));
                        myCmd.addArgs(uint32(OUTPUTS));
                    else
                        error('Output trigger argument must be non-empty')
                    end
                  
                    resp = node.sendCmd(myCmd);
                    out = resp.getArgs();
                    
                %---------------------------------------------------------
                case 'input_config_enable_selection'
                    fprintf('In WARPLab 7.6.0, the input_config_enable_selection command is no longer\n');
                    fprintf('supported.  This command is not necessary since you can disable any input\n');
                    fprintf('by disconnecting it from the output.\n');
                    
                %---------------------------------------------------------
                case 'input_config_debounce_mode'
                    % Configures specified input triggers to enable or
                    % disable debounce circuit. Note: debounce circuit adds
                    % delay of 4 cycles, where each cycle is a duration
                    % specified in the input_delayStep_ns property of the
                    % wl_manager_proc.m class.
                    %
                    % Arguments: (uint32 INPUTS), (boolean MODE)
                    % Returns: none
                    %
                    % INPUTS:      vector of output trigger IDs, provided by
                    %              wl_getTriggerInputIDs
                    %
                    % MODE:         true enables input debounce mode
                    %               false disables input debounce mode
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_INPUT_DEBOUNCE));
                    
                    INPUTS    = varargin{1};
                    mode      = varargin{2};
                    input_ids = node.wl_getTriggerInputIDs();

                    if (ischar(mode)) 
                        % Print warning that this syntax will be deprecated
                        try
                            temp = evalin('base', 'wl_input_config_debounce_mode_did_warn');
                        catch
                            fprintf('WARNING:   This syntax for input_config_debounce_mode() is being deprecated.\n');
                            fprintf('WARNING:   Please use:  wl_triggerManagerCmd(node, ''input_config_debounce_mode'', INPUTS, <true, false>);\n');
                            fprintf('WARNING:   See WARPLab documentation for more information\n\n');
                            
                            assignin('base', 'wl_input_config_debounce_mode_did_warn', 1)
                        end
                    
                        if(strcmp(lower(mode),'enable'))
                            mode = 1;
                        elseif(strcmp(lower(mode),'disable'))
                            mode = 0;
                        else
                            error('mode selection must be ''enable'' or ''disable''')
                        end
                    elseif (islogical(mode))
                        if(mode)
                            mode = 1;
                        else
                            mode = 0;
                        end
                    else 
                        error('mode selection must be true or false')
                    end
                    
                    for k = 1:length(INPUTS)
                        if ((INPUTS(k) == input_ids.ETH_A) || (INPUTS(k) == input_ids.ETH_B) || (INPUTS(k) == input_ids.ENERGY_DET) || (INPUTS(k) == input_ids.AGC_DONE) || (INPUTS(k) == input_ids.SW_REG))
                            error('one or more selected inputs do not have debounce circuitry')
                        end
                    end
                    
                    % Check IDs
                    if (~obj.isInputID(INPUTS))
                        error('Input trigger ID invalid')
                    else
                        INPUTS = INPUTS - obj.TRIGGER_INPUT_FLAG;    % Strip flag to pass value to node
                    end
                    
                    if(~isempty(INPUTS))
                        myCmd.addArgs(uint32(length(INPUTS)));
                        myCmd.addArgs(uint32(INPUTS));
                    else
                        error('Input trigger argument must be non-empty')
                    end
                     
                    myCmd.addArgs(mode);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'input_config_delay'
                    % Configures specified input triggers to be have an
                    % additional delay relative to their inputs
                    %
                    % Arguments: (uint32 INPUTS), (double DELAY_NS)
                    % Returns: none
                    %
                    % INPUTS:       vector of input trigger IDs, provided by
                    %               wl_getTriggerInputIDs
                    %
                    % DELAY_NS:     scalar value of the intended delay,
                    %               specified in nanoseconds (1e-9 seconds)
                    %                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_INPUT_DELAY));

                    INPUTS   = varargin{1};
                    delay_ns = varargin{2};
                    
                    % Check IDs
                    if (~obj.isInputID(INPUTS))
                        error('Input trigger ID invalid')
                    else
                        INPUTS = INPUTS - obj.TRIGGER_INPUT_FLAG;    % Strip flag to pass value to node
                    end
                    
                    if(~isempty(INPUTS))
                        myCmd.addArgs(uint32(length(INPUTS)));
                        myCmd.addArgs(uint32(INPUTS));
                    else
                        error('Input trigger argument must be non-empty')
                    end
                    
                    for k = 1:length(INPUTS)
                       % Find the index of the ID
                       %     NOTE:  Add trigger input flag when finding the index because it was stripped off above
                       %
                       index = find((INPUTS(k) + obj.TRIGGER_INPUT_FLAG) == obj.triggerInputIDs); 
                        
                       if k == 1
                          enforced_step = obj.input_delayStep_ns(index);
                          enforced_max  = obj.input_delayMax_ns(index);
                       else
                          enforced_step = max([enforced_step, obj.input_delayStep_ns(index)]);
                          enforced_max  = min([enforced_max, obj.input_delayMax_ns(index)]);  
                       end
                    end
                    
                    if (delay_ns < enforced_max) 
                        requested_delay = floor(delay_ns/enforced_step);
                    else
                        requested_delay = floor(enforced_max/enforced_step);
                    end
                    
                    if (~(requested_delay == delay_ns/enforced_step))
                        warning('Node %d with Serial # %d has a delay step of %.3f ns and a maximum delay of %.3f ns.\nRequested output delay of %.3f ns actually set to %.3f ns.', node.ID, node.serialNumber, enforced_step, enforced_max, delay_ns, enforced_step*requested_delay);
                    end
                    
                    myCmd.addArgs(requested_delay);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'energy_config_busy_threshold'
                    % Configures the threshold above which RSSI is
                    % considered as a "busy" medium.
                    %
                    % Arguments: (uint32 THRESH)
                    % Returns: none
                    %
                    % THRESH:      busy threshold. For the MAX2829-based
                    %              interfaces, WARP uses a 10-bit ADC for
                    %              RSSI (range of [0,1023]).
                    %
                    % Note: RSSI averaging in the core does NOT divide by
                    % the number of samples that are summed together.
                    % Averaging by N cycles means that the maximum possible
                    % RSSI post-averaging is N*1023.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_ENERGY_BUSY_THRESH));
                    
                    thresh_busy = varargin{1};

                    myCmd.addArgs(thresh_busy);
                    node.sendCmd(myCmd);
                
                %---------------------------------------------------------
                case 'energy_config_average_length'
                    % Configures the number of samples over which RSSI is
                    % averaged before it is compared to any threshold.
                    %
                    % Arguments: (uint32 LENGTH)
                    % Returns: none
                    %
                    % LENGTH:      Number of samples over which RSSI is
                    %              averaged.
                    %
                    % Note: For all hardware versions, RSSI is sampled at
                    % 10 MHz. Each sample is, therefore, 100 ns.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_ENERGY_AVG_LEN));
                    
                    avgLen = varargin{1};

                    myCmd.addArgs(avgLen);
                    node.sendCmd(myCmd);
                 
                %---------------------------------------------------------
                case 'energy_config_busy_minlength'
                    % Average RSSI samples must exceed the busy threshold
                    % for a minimum number of samples before the trigger is
                    % activated. This command sets this minimum value.
                    %
                    % Arguments: (uint32 LENGTH)
                    % Returns: none
                    %
                    % LENGTH:      Minimum number of samples that RSSI must
                    %              be busy before trigger is raised.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_ENERGY_BUSY_MIN_LEN));
                    
                    minLen = varargin{1};

                    myCmd.addArgs(minLen);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'energy_config_interface_selection'
                    % Selects the interfaces from which energy detection
                    % should base its decision
                    %
                    % Arguments: (uint32 IFCSELECTION)
                    % Returns: none
                    %
                    % IFCSELECTION:     One or more interfaces that the
                    %                   energy detector system should
                    %                   monitor
                    %
                    % Note: IFCSELECTION is intended to be used with the
                    % return values from the wl_getInterfaceIDs method.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_ENERGY_IFC_SELECTION));
                    
                    ifcSel = varargin{1};

                    myCmd.addArgs(ifcSel);
                    node.sendCmd(myCmd);
                
                %---------------------------------------------------------
                case 'input_config_idelay'
                    % Configures the IDELAY values for specified input triggers. 
                    %
                    % Arguments: (uint32 INPUTS), (string TYPE), (uint32 VALUES)
                    % Returns: none
                    %
                    % INPUTS:      Vector of input trigger IDs, provided by wl_getTriggerInputIDs
                    %
                    % TYPE:        'ext_pin' or 'cm_pll' - Can modify the IDELAY for the path
                    %                  from the external pin independent of the CM-PLL input path
                    %
                    % VALUES:      Scaler value in [0, 31] or Vector of values equal in length
                    %                  to the INPUTS vector
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_IDELAY));
                    
                    % Check arguments
                    if(length(varargin) ~= 3)
                        error('%s: Requires three arguments:  Input Trigger ID(s), Type, Value(s)', cmdStr);
                    end
                    
                    INPUTS    = varargin{1};
                    type      = varargin{2};
                    VALUES    = varargin{3};
                    input_ids = node.wl_getTriggerInputIDs();

                    % Convert type and add it to the command arguments              
                    if(strcmp(lower(type), 'ext_pin'))
                        type = 0;
                    elseif(strcmp(lower(type), 'cm_pll'))
                        type = 1;
                    else
                        error('TYPE selection must be ''ext_pin'' or ''cm_pll''')
                    end

                    myCmd.addArgs(type);

                    % Check inputs                    
                    for k = 1:length(INPUTS)
                        if ((INPUTS(k) == input_ids.ETH_A) || (INPUTS(k) == input_ids.ETH_B) || (INPUTS(k) == input_ids.ENERGY_DET) || (INPUTS(k) == input_ids.AGC_DONE) || (INPUTS(k) == input_ids.SW_REG))
                            error('One or more selected inputs do not have IDELAY circuitry.')
                        end
                    end

                    % Check values
                    if ((length(VALUES) ~= 1) && (length(VALUES) ~= length(INPUTS)))
                        error('VALUES must either be a scalar or a vector of the same length as the input trigger IDs.')
                    end

                    for k = 1:length(VALUES)
                        if ((VALUES(k) < 0) || (VALUES(k) > 31))
                            error('Values must be in [0, 31]')
                        end
                    end
                    
                    % Check IDs
                    if (~obj.isInputID(INPUTS))
                        error('Input trigger ID invalid')
                    else
                        INPUTS = INPUTS - obj.TRIGGER_INPUT_FLAG;    % Strip flag to pass value to node
                    end
                    
                    % Add input trigger IDs to the command                    
                    if(~isempty(INPUTS))
                        myCmd.addArgs(uint32(length(INPUTS)));
                        myCmd.addArgs(uint32(INPUTS));
                    else
                        error('Input trigger ID argument must be non-empty')
                    end

                    % Add values to the command
                    if (length(VALUES) == 1) 
                        for k = 1:length(INPUTS)
                            myCmd.addArgs(uint32(VALUES));
                        end
                    else
                        myCmd.addArgs(uint32(VALUES));
                    end
                    
                    % Send the command to the node
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'output_config_odelay'
                    % Configures the ODELAY values for specified output triggers.
                    %
                    % Arguments: (uint32 OUTPUTS), (string TYPE), (uint32 VALUES)
                    % Returns: none
                    %
                    % OUTPUTS:     Vector of output trigger IDs, provided by wl_getTriggerOutputIDs
                    %
                    % TYPE:        'ext_pin' or 'cm_pll' - Can modify the IDELAY for the path
                    %                  from the external pin independent of the CM-PLL input path
                    %
                    % VALUES:      Scaler value in [0, 31] or Vector of values equal in length
                    %                  to the OUTPUTS vector
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_ODELAY));
                    
                    % Check arguments
                    if(length(varargin) ~= 3)
                        error('%s: Requires three arguments:  Output Trigger ID(s), Type, Value(s)', cmdStr);
                    end
                    
                    OUTPUTS    = varargin{1};
                    type       = varargin{2};
                    VALUES     = varargin{3};
                    output_ids = node.wl_getTriggerOutputIDs();

                    % Convert type and add it to the command arguments              
                    if(strcmp(lower(type), 'ext_pin'))
                        type = 0;
                    elseif(strcmp(lower(type), 'cm_pll'))
                        type = 1;
                    else
                        error('TYPE selection must be ''ext_pin'' or ''cm_pll''')
                    end

                    myCmd.addArgs(type);

                    % Check outputs
                    for k = 1:length(OUTPUTS)
                        if ((OUTPUTS(k) == output_ids.BASEBAND) || (OUTPUTS(k) == output_ids.AGC))
                            error('One or more selected outputs do not have ODELAY circuitry.')
                        end
                    end

                    % Check values
                    if ((length(VALUES) ~= 1) && (length(VALUES) ~= length(OUTPUTS)))
                        error('VALUES must either be a scalar or a vector of the same length as the output trigger IDs.')
                    end

                    for k = 1:length(VALUES)
                        if ((VALUES(k) < 0) || (VALUES(k) > 31))
                            error('Values must be in [0, 31]')
                        end
                    end
                    
                    % Check IDs
                    if (~obj.isOutputID(OUTPUTS))
                        error('Output trigger ID invalid')
                    end
                    
                    % Add output trigger IDs to the command                    
                    if(~isempty(OUTPUTS))
                        myCmd.addArgs(uint32(length(OUTPUTS)));
                        myCmd.addArgs(uint32(OUTPUTS));
                    else
                        error('Output trigger ID argument must be non-empty')
                    end

                    % Add values to the command
                    if (length(VALUES) == 1) 
                        for k = 1:length(OUTPUTS)
                            myCmd.addArgs(uint32(VALUES));
                        end
                    else
                        myCmd.addArgs(uint32(VALUES));
                    end
                    
                    % Send the command to the node
                    node.sendCmd(myCmd);
                
                %---------------------------------------------------------
                case 'test_trigger'
                    % Sends a test trigger
                    %
                    % Arguments: none
                    % Returns: none
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_TEST_TRIGGER));
                    resp = node.sendCmd(myCmd);
                    out = resp.getArgs();
                    
                otherwise
                    error('unknown command ''%s''',cmdStr);
            end

            % Set the outputs            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end
         end
    end
end