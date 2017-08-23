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

classdef wl_node < handle_light
    % The WARPLab node class represents one node in a WARPLab network
    % wl_node is the primary interface for interacting with WARPLab nodes.
    % This class provides methods for sending commands, exchanging samples
    % and checking the status of WARPLab nodes.
    % wl_node wraps the six underlying components that comprise a WARPLab
    % node implementation: node, baseband, interface, transport, trigger
    % setup and user extensions.
    
    properties (SetAccess = protected)
        ID;                    % Unique identification for this node
        description;           % String description of this node (auto-generated)
        serialNumber;          % Node's serial number, read from EEPROM on hardware
        eth_MAC_addr;          % Node's MAC address (for ETH_A on WARP v3)
        fpgaDNA;               % Node's FPGA's unique identification (on select hardware)
        hwVer;                 % WARP hardware version of this node
        wlVer_major;           % WARPLab version running on this node
        wlVer_minor;
        wlVer_revision;
        num_interfacesGroups;  % # of interface groups attached to node
        num_interfaces;        % Vector of length num_interfaceGroups with
                               %     # of interfaces per group
        interfaceGroups;       % Node's interface group object(s)
        interfaceIDs;          % Vector of interface identifiers
        baseband;              % Node's baseband object
        trigger_manager;       % Node's trigger configuration object
        transport;             % Node's transport object
        user;                  % Node's user extension object
    end
    
    properties (SetAccess = public)
        name;                  % User specified name for node; user scripts supply this
    end
    
    properties(Hidden = true,Constant = true)
        % Command Groups - Most Significant Byte of Command ID
        GRPID_NODE                     = hex2dec('00');
        GRPID_TRANS                    = hex2dec('10');
        GRPID_IFC                      = hex2dec('20');
        GRPID_BB                       = hex2dec('30');
        GRPID_TRIGMNGR                 = hex2dec('40');
        GRPID_USER                     = hex2dec('50');
    end
    
    properties(Hidden = true,Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wl_node.h
        GRP                            = 'node';
        CMD_INITIALIZE                 = 1;                % 0x000001
        CMD_INFO                       = 2;                % 0x000002
        CMD_IDENTIFY                   = 3;                % 0x000003
        CMD_TEMPERATURE                = 4;                % 0x000004
        CMD_NODE_CONFIG_SETUP          = 5;                % 0x000005
        CMD_NODE_CONFIG_RESET          = 6;                % 0x000006
        
        CMD_MEM_RW                     = 16;               % 0x000010
    end
    
    methods
        function obj = wl_node()
            % The constructor is intentionally blank for wl_node objects.
            % Instead, the objects are configured via the separate applyConfiguration method.
        end
        
        
        function applyConfiguration(objVec, IDVec)
            % Set all the node object parameters
        
            % Apply Configuration only operates on one object at a time
            if ((length(objVec) > 1) || (length(IDVec) > 1)) 
                error('applyConfiguration only operates on a single object with a single ID.  Provided parameters with lengths: %d and %d', length(objVec), length(IDVec) ); 
            end

            % Apply configuration will finish setting properties for a specific hardware node
            obj    = objVec(1);
            currID = IDVec(1);
            
            % currID can be either a structure containing node information or a number
            switch (class(currID))
                case 'struct'
                    if ( ~strcmp( currID.serialNumber, '' ) )
                        obj.serialNumber = sscanf( currID.serialNumber, 'W3-a-%d' );  % Only store the last 5 digits ( "W3-a-" is not stored )
                    else
                        error('Unknown argument.  Serial Number provided is blank');
                    end

                    if ( ~strcmp( currID.ID, '' ) )
                        obj.ID           = sscanf( currID.ID, '%d');
                    else
                        error('Unknown argument.  Node ID provided is blank');
                    end
                
                    if ( ~strcmp( currID.name, '' ) ) % Name is an optional parameter in the structure
                        obj.name         = currID.name;
                    end

                case 'double'
                    obj.ID = currID;                  % The node ID must match the DIP switch on the WARP board

                otherwise
                    error('Unknown argument.  IDVec is of type "%s", need "struct", or "double"', class(currID));
            end        
            

            % Get ini configuration file 
            configFile = which('wl_config.ini');
            if(isempty(configFile))
                error('cannot find wl_config.ini. please run wl_setup.m'); 
            end
                
            % Use Ethernet/UDP transport for all wl_nodes. The specific type of transport
            % is specified in the user's wl_config.ini file that is created via wl_setup.m
            readKeys      = {'network','','transport',''};
            transportType = inifile(configFile,'read',readKeys);
            transportType = transportType{1};

            switch(transportType)
                case 'java'
                    obj.transport = wl_transport_eth_udp_java;
                case 'wl_mex_udp'
                    obj.transport = wl_transport_eth_udp_mex;
            end
                
            if(isempty(obj.trigger_manager))
                obj.wl_setTriggerManager('wl_trigger_manager_proc');
            end

            readKeys = {'network','','host_address',''};
            IP = inifile(configFile,'read',readKeys);
            IP = IP{1};
            IP = sscanf(IP,'%d.%d.%d.%d');

            readKeys = {'network','','host_ID',''};
            hostID = inifile(configFile,'read',readKeys);
            hostID = hostID{1};
            hostID = sscanf(hostID,'%d');
                
            readKeys = {'network','','unicast_starting_port',''};
            unicast_starting_port = inifile(configFile,'read',readKeys);
            unicast_starting_port = unicast_starting_port{1};
            unicast_starting_port = sscanf(unicast_starting_port,'%d');
                
            % Configure transport with settings associated with provided ID
            obj.transport.hdr.srcID  = hostID;
            obj.transport.hdr.destID = obj.ID;

            % Determine IP address based on the input parameter
            switch( class(currID) ) 
                case 'struct'
                    if ( ~strcmp( currID.ipAddress, '' ) )
                        obj.transport.setAddress(currID.ipAddress);
                    else
                        error('Unknown argument.  IP Address provided is blank');
                    end
                case 'double'
                    obj.transport.setAddress(sprintf('%d.%d.%d.%d', IP(1), IP(2), IP(3), (obj.ID + 1)));
            end        
                            
            obj.transport.setPort(unicast_starting_port);
               
            obj.transport.open();
            obj.transport.hdr.srcID  = hostID;   % ????? Redundant ????? - TBD
            obj.transport.hdr.destID = obj.ID;   % ????? Redundant ????? - TBD

            obj.wl_nodeCmd('initialize');
            obj.wl_transportCmd('ping');              % Confirm the WARP node is online
            obj.wl_transportCmd('payload_size_test'); % Perform a test to figure out max payload size
            
            obj.interfaceIDs = [];

            if(isempty(obj.baseband))
                % Instantiate baseband object
                obj.wl_setBaseband('wl_baseband_buffers');
            end
                
            % Read details from the hardware (serial number, etc) and save to local properties
            obj.wl_nodeCmd('get_hardware_info');
                
            % Instantiate interfaces group.
            if(isempty(obj.interfaceGroups))
                obj.interfaceGroups{1} = wl_interface_group_X245(1:obj.num_interfaces, 'w3');
            end

            % Extract the interface IDs from the interface group. These IDs
            % will be supplied to user scripts to identify individual
            % interfaces for interface and baseband commands
            for ifcGroupIndex = 1:length(obj.interfaceGroups)
                obj.interfaceIDs = [obj.interfaceIDs, obj.interfaceGroups{1}.ID(:).'];
            end
                
            % Populate the description property with a human-readable
            % description of the node
            obj.description = sprintf('WARP v%d Node - ID %d', obj.hwVer, obj.ID);
       end
       
       
       function wl_setUserExtension(obj,module)
           % Sets the User Extension module to a user-provided object or
           % string of that object's class name.
           %
           makeComparison = 1;
           
           if(module == 0)                                 % No module attached
              module = 'double(0)';
              makeComparison = 0;
           elseif(~ischar(module))                         % Input is an actual instance of an object
              module = class(module);               
           end
           
           if(makeComparison && ~any(strcmp(superclasses(module),'wl_user_ext')))
              error('Module is not a wl_user_ext type');
           end
           
           for n = 1:length(obj)
              obj(n).user = eval(module); 
           end
       end 
       
       
       function wl_setBaseband(obj,module)
           % Sets the Baseband module to a user-provided object or
           % string of that object's class name.
           %
           makeComparison = 1;

           if(module == 0)                                 % No module attached
              module = 'double(0)';
              makeComparison = 0;
           elseif(~ischar(module))                         % Input is an actual instance of an object
              module = class(module);               
           end
           
           if(makeComparison && ~any(strcmp(superclasses(module),'wl_baseband')))
              error('Module is not a wl_baseband type');
           end
           
           for n = 1:length(obj)
              obj(n).baseband = eval(module); 
           end
       end 
       
       
       function wl_setTriggerManager(obj,module)
           % Sets the Trigger Manager module to a user-provided object or
           % string of that object's class name.
           %
           makeComparison = 1;
           
           if(module == 0)                                 % No module attached
              module = 'double(0)';
              makeComparison = 0;
           elseif(~ischar(module))                         % Input is an actual instance of an object
              module = class(module);               
           end
           
           if(makeComparison && ~any(strcmp(superclasses(module),'wl_trigger_manager')))
              error('Module is not a wl_trigger_manager type');
           end
           
           for n = 1:length(obj)
              obj(n).trigger_manager = eval(module); 
           end
       end 
       
       
       function out = wl_basebandCmd(obj, varargin)
            % Sends commands to the baseband object.
            % This method is safe to call with multiple wl_node objects as
            % its first argument. For example, let node0 and node1 be
            % wl_node objects:
            %
            %   wl_basebandCmd(node0, args )
            %   wl_basebandCmd([node0, node1], args )
            %   node0.wl_basebandCmd( args )
            %
            % are all valid ways to call this method. Note, when calling
            % this method for multiple node objects, the interpretation of
            % other vectored arguments is left up to the underlying
            % components.
            %
            nodes    = obj;
            numNodes = numel(nodes);
            
            for n = numNodes:-1:1
                currNode = nodes(n);
                if(any(strcmp(superclasses(currNode.baseband),'wl_baseband')))
                    out(n) = currNode.baseband.procCmd(n, currNode, varargin{:});
                else
                    error('Node %d does not have an attached baseband module',currNode.ID);
                end
            end

            if((length(out) == 1) && iscell(out))
               out = out{1};                               % Strip away the cell if it's just a single element. 
            end
        end
        
        
        function out = wl_nodeCmd(obj, varargin)
            %Sends commands to the node object.
            % This method is safe to call with multiple wl_node objects as
            % its first argument. For example, let node0 and node1 be
            % wl_node objects:
            %
            %   wl_nodeCmd(node0, args )
            %   wl_nodeCmd([node0, node1], args )
            %   node0.wl_nodeCmd( args )
            %
            % are all valid ways to call this method. Note, when calling
            % this method for multiple node objects, the interpretation of
            % other vectored arguments is left up to the underlying
            % components.
            %
            nodes    = obj;
            numNodes = numel(nodes);
            
            for n = numNodes:-1:1
                currNode = nodes(n);
                out(n) = currNode.procCmd(n, currNode, varargin{:});
            end
            
            if((length(out) == 1) && iscell(out))
               out = out{1};                               % Strip away the cell if it's just a single element. 
            end            
        end
        
        
        function out = wl_transportCmd(obj, varargin)
            % Sends commands to the transport object.
            % This method is safe to call with multiple wl_node objects as
            % its first argument. For example, let node0 and node1 be
            % wl_node objects:
            %
            %   wl_transportCmd(node0, args )
            %   wl_transportCmd([node0, node1], args )
            %   node0.wl_transportCmd( args )
            %
            % are all valid ways to call this method. Note, when calling
            % this method for multiple node objects, the interpretation of
            % other vectored arguments is left up to the underlying
            % components.
            %
            nodes    = obj;
            numNodes = numel(nodes);
            
            for n = numNodes:-1:1
                currNode = nodes(n);
                if(any(strcmp(superclasses(currNode.transport),'wl_transport')))
                    out(n) = currNode.transport.procCmd(n, currNode, varargin{:});
                else
                    error('Node %d does not have an attached transport module',currNode.ID);
                end
                
            end
            
            if((length(out) == 1) && iscell(out))
               out = out{1};                               % Strip away the cell if it's just a single element. 
            end           
        end
        
        
        function out = wl_userExtCmd(obj, varargin)
            % Sends commands to the user extension object.
            % This method is safe to call with multiple wl_node objects as
            % its first argument. For example, let node0 and node1 be
            % wl_node objects:
            %
            %   wl_userExtCmd(node0, args )
            %   wl_userExtCmd([node0, node1], args )
            %   node0.wl_userExtCmd( args )
            %
            % are all valid ways to call this method. Note, when calling
            % this method for multiple node objects, the interpretation of
            % other vectored arguments is left up to the underlying
            % components.
            %
            nodes    = obj;
            numNodes = numel(nodes);
            
            for n = numNodes:-1:1
                currNode = nodes(n);
                if(any(strcmp(superclasses(currNode.user),'wl_user_ext')))
                     out(n) = {currNode.user.procCmd(n, currNode, varargin{:})};
                else
                    error('Node %d does not have an attached user extension module',currNode.ID);
                end
            end
            
            if((length(out) == 1) && iscell(out))
               out = out{1};                               % Strip away the cell if it's just a single element. 
            end
        end
        
        
        function out = wl_triggerManagerCmd(obj, varargin)
            % Sends commands to the trigger manager object.
            % This method is safe to call with multiple wl_node objects as
            % its first argument. For example, let node0 and node1 be
            % wl_node objects:
            %
            %   wl_triggerManagerCmd(node0, args )
            %   wl_triggerManagerCmd([node0, node1], args )
            %   node0.wl_triggerManagerCmd( args )
            %
            % are all valid ways to call this method. Note, when calling
            % this method for multiple node objects, the interpretation of
            % other vectored arguments is left up to the underlying
            % components.
            %
            nodes    = obj;
            numNodes = numel(nodes);
            
            for n = numNodes:-1:1
                currNode = nodes(n);
                if(any(strcmp(superclasses(currNode.trigger_manager),'wl_trigger_manager')))
                    out(n) = currNode.trigger_manager.procCmd(n, currNode, varargin{:});
                else
                    error('Node %d does not have an attached trigger manager module',currNode.ID);
                end
            end
            
            if((length(out) == 1) && iscell(out))
               out = out{1};                               % Strip away the cell if it's just a single element. 
            end            
        end
        
        
        function out = wl_interfaceCmd(obj, rfSel, varargin)
            % Sends commands to the interface object.
            % This method must be called with RF selection masks as an
            % argument. These masks are returned by the wl_getInterfaceIDs
            % method. The RFMASKS argument must be a scaler or vector of 
            % bit-OR'd interface IDs from a single interface group
            %
            % This method is safe to call with multiple wl_node objects as
            % its first argument. For example, let node0 and node1 be
            % wl_node objects:
            %
            %   wl_interfaceCmd(node0, RFMASKS, args )
            %   wl_interfaceCmd([node0, node1], RFMASKS, args )
            %   node0.wl_trigConfCmd(RFMASKS, args )
            %
            % are all valid ways to call this method. Note, when calling
            % this method for multiple node objects, the interpretation of
            % other vectored arguments is left up to the underlying
            % components.
            %
            nodes    = obj;
            numNodes = numel(nodes);
            
            ifcIndex = 1;
            for nodeIndex = numNodes:-1:1
                currNode = nodes(nodeIndex);  
                if(any(strcmp(superclasses(currNode.interfaceGroups{ifcIndex}), 'wl_interface_group')))
                    out(nodeIndex) = currNode.interfaceGroups{ifcIndex}.procCmd(nodeIndex, currNode, rfSel, varargin{:});
                else
                    error('Node %d does not have an attached interface group module',currNode.ID);
                end
            end    

            if((length(out) == 1) && iscell(out))
               out = out{1};                               % Strip away the cell if it's just a single element. 
            end
        end
        
        
        function varargout = wl_getInterfaceIDs(obj)
            % Returns the interfaces IDs that can be used as inputs to all interface commands, some 
            % baseband commands and possibly some user extension commands.
            %
            % The interface IDs are returned in a structure that contains fields for individual 
            % interfaces and combinations of interfaces.  When a node only supports 2 interfaces,
            % the fields for RFC and RFD (ie the fields specific to a 4 interface node) are not
            % present in the structure.
            %
            % The fields in the structure are:
            %     - Scalar fields:
            %       - RF_A
            %       - RF_B
            %       - RF_C
            %       - RF_D
            %       - RF_ON_BOARD      NOTE: RF_ON_BOARD      = RF_A + RF_B
            %       - RF_FMC           NOTE: RF_FMC           = RF_C + RF_D
            %       - RF_ALL           NOTE: 2RF:  RF_ALL     = RF_A + RF_B
            %                                4RF:  RF_ALL     = RF_A + RF_B + RF_C + RF_D
            %     - Vector fields:
            %       - RF_ON_BOARD_VEC  NOTE: RF_ON_BOARD_VEC  = [RF_A, RF_B]
            %       - RF_FMC_VEC       NOTE: RF_FMC_VEC       = [RF_C, RF_D]
            %       - RF_ALL_VEC       NOTE: 2RF:  RF_ALL_VEC = [RF_A, RF_B]
            %                                4RF:  RF_ALL_VEC = [RF_A, RF_B, RF_C, RF_D]
            %
            % NOTE:  Due to Matlab behavior, the scalar fields for RF_A, RF_B, RF_C, and RF_D can be 
            %     used as vectors and therefore do not need separate vector fields in the structure
            %
            % Examples:
            %     Get the interface ID structure (let node be a wl_node object):
            %         ifc_ids = wl_getInterfaceIDs(node);
            %         ifc_ids = node.wl_getInterfaceIDs();
            %
            %     Use the interface ID structure:
            %         1) Enable RF_A for transmit:  
            %                wl_interfaceCmd(node, ifc_ids.RF_A, 'tx_en');
            %
            %         2) Get 1000 samples of Read IQ data from all interfaces:
            %                rx_IQ = wl_basebandCmd(node, ifc_ids.RF_ALL_VEC, 'read_IQ', 0, 1000);
            % 
            
            if (nargout > 1) 
                % Legacy code for compatibility
                %
                % Returns a vector of interface IDs that can be used as inputs to all interface 
                % commands and some baseband or user extension commands.
                %
                % Let node0 be a wl_node object:
                %   [RFA, RFB] = wl_getInterfaceIDs(node0)
                %   [RFA, RFB] = node0.wl_getInterfaceIDs(node0)
                %
                % Issues a warning that this syntax will be deprecated in future releases.
                %
                if(nargout > obj.num_interfaces)
                   error('Node %d has only %d interfaces. User has requested %d interface IDs', obj.ID, obj.num_interfaces, nargout); 
                end
                
                varargout = num2cell(obj.interfaceIDs(1:nargout));  
                
                % Print warning that this syntax will be deprecated
                try
                    temp = evalin('base', 'wl_get_interface_ids_did_warn');
                catch
                    fprintf('WARNING:   This syntax for wl_getInterfaceIDs() is being deprecated.\n');
                    fprintf('WARNING:   Please use:  ifc_ids = wl_getInterfaceIDs(node);\n');
                    fprintf('WARNING:       where ifc_ids is a structure that contains the interface IDs\n');
                    fprintf('WARNING:   See WARPLab documentation for more information\n\n');
                    
                    assignin('base', 'wl_get_interface_ids_did_warn', 1)
                end
                
            else
                ifc_ids = struct();
                
                switch(obj.num_interfaces)
                
                    %---------------------------------------------------------
                    case 2
                        % Structure contains:
                        %     Scalar variables:  RF_A, RF_B, RF_ON_BOARD, RF_ALL
                        %     Vector variables:  RF_ON_BOARD_VEC, RF_ALL_VEC
                        %
                        
                        % Scalar variables
                        ifc_ids(1).RF_A               = obj.interfaceIDs(1);
                        ifc_ids(1).RF_B               = obj.interfaceIDs(2);
                        
                        ifc_ids(1).RF_ON_BOARD        = obj.interfaceIDs(1) + obj.interfaceIDs(2);
                        
                        ifc_ids(1).RF_ALL             = obj.interfaceIDs(1) + obj.interfaceIDs(2);
                        
                        % Vector variables
                        ifc_ids(1).RF_ON_BOARD_VEC    = [obj.interfaceIDs(1), obj.interfaceIDs(2)];
                        
                        ifc_ids(1).RF_ALL_VEC         = [obj.interfaceIDs(1), obj.interfaceIDs(2)];
                        
                    %---------------------------------------------------------
                    case 4
                        % Structure contains:
                        %     Scalar variables:  RF_A, RF_B, RF_C, RF_D, RF_ON_BOARD, RF_FMC, RF_ALL
                        %     Vector variables:  RF_ON_BOARD_VEC, RF_FMC_VEC, RF_ALL_VEC
                        %
                
                        % Scalar variables
                        ifc_ids(1).RF_A               = obj.interfaceIDs(1);
                        ifc_ids(1).RF_B               = obj.interfaceIDs(2);
                        ifc_ids(1).RF_C               = obj.interfaceIDs(3);
                        ifc_ids(1).RF_D               = obj.interfaceIDs(4);
                        
                        ifc_ids(1).RF_ON_BOARD        = obj.interfaceIDs(1) + obj.interfaceIDs(2);
                        ifc_ids(1).RF_FMC             = obj.interfaceIDs(3) + obj.interfaceIDs(4);
                        
                        ifc_ids(1).RF_ALL             = obj.interfaceIDs(1) + obj.interfaceIDs(2) + obj.interfaceIDs(3) + obj.interfaceIDs(4);
                        
                        % Vector variables
                        ifc_ids(1).RF_ON_BOARD_VEC    = [obj.interfaceIDs(1), obj.interfaceIDs(2)];
                        ifc_ids(1).RF_FMC_VEC         = [obj.interfaceIDs(3), obj.interfaceIDs(4)];
                        
                        ifc_ids(1).RF_ALL_VEC         = [obj.interfaceIDs(1), obj.interfaceIDs(2), obj.interfaceIDs(3), obj.interfaceIDs(4)];
                        
                    %---------------------------------------------------------
                    otherwise
                        error( 'Number of interfaces not supported.  Node reported %d interfaces, should be in [2, 4].', obj.num_interfaces);
                end
                
                varargout = {ifc_ids};
            end
        end
        
        
        function varargout = wl_getTriggerInputIDs(obj)
            % Returns the trigger input IDs that can be used as inputs to trigger manager commands
            %
            % The trigger input IDs are returned in a structure that contains fields for individual 
            % triggers.  
            %
            % The fields in the structure are:
            %     - Scalar fields:
            %       - ETH_A                 - Ethernet Port A 
            %       - ETH_B                 - Ethernet Port B
            %       - ENERGY_DET            - Energy detection (See 'energy_config_*' commands in the Trigger Manager documentation)
            %       - AGC_DONE              - Automatic Gain Controller complete
            %       - SW_REG                - Software register (ie Memory mapped registers that can be triggered by a CPU write)
            %       - EXT_IN_P0             - External Input Pin 0
            %       - EXT_IN_P1             - External Input Pin 1
            %       - EXT_IN_P2             - External Input Pin 2
            %       - EXT_IN_P3             - External Input Pin 3
            %
            % Examples:
            %     Get the trigger input ID structure (let node be a wl_node object):
            %         trig_in_ids = wl_getTriggerInputIDs(node);
            %         trig_in_ids = node.wl_getTriggerInputIDs();
            %
            %     Use the trigger input ID structure:
            %         1) Enable baseband and agc output triggers to be triggered on the Ethernet A input trigger:  
            %                wl_triggerManagerCmd(nodes, 'output_config_input_selection', [trig_out_ids.BASEBAND, trig_out_ids.AGC], [trig_in_ids.ETH_A]);
            % 
            
            if (nargout > 1) 
                % Legacy code for compatibility
                %
                % Returns a vector of trigger input IDs that can be used as inputs to trigger manager commands
                %
                % Let node0 be a wl_node object
                %   [T_IN_ETH_A, T_IN_ENERGY, T_IN_AGCDONE, T_IN_REG, T_IN_D0, T_IN_D1, T_IN_D2, T_IN_D3, T_IN_ETH_B] = wl_getTriggerInputIDs(node0)
                %   [T_IN_ETH_A, T_IN_ENERGY, T_IN_AGCDONE, T_IN_REG, T_IN_D0, T_IN_D1, T_IN_D2, T_IN_D3, T_IN_ETH_B] = node0.wl_getTriggerInputIDs(node0)
                %
                if(nargout > length(obj.trigger_manager.triggerInputIDs))
                   error('Node %d has only %d trigger inputs. User has requested %d trigger input IDs',obj.ID,length(obj.trigger_manager.triggerInputIDs),nargout); 
                end
                
                varargout = num2cell(obj.trigger_manager.triggerInputIDs(1:nargout));
            
                % Print warning that this syntax will be deprecated
                try
                    temp = evalin('base', 'wl_get_trigger_input_ids_did_warn');
                catch
                    fprintf('WARNING:   This syntax for wl_getTriggerInputIDs() is being deprecated.\n');
                    fprintf('WARNING:   Please use:  trig_in_ids = wl_getTriggerInputIDs(node);\n');
                    fprintf('WARNING:       where trig_in_ids is a structure that contains the trigger input IDs\n');
                    fprintf('WARNING:   See WARPLab documentation for more information\n\n');
                    
                    assignin('base', 'wl_get_trigger_input_ids_did_warn', 1)
                end
                
            else
                % Structure contains:
                %     Scalar variables:  ETH_A, ETH_B, ENERGY, AGC_DONE, SW_REG, DEBUG_0, DEBUG_1, DEBUG_2, DEBUG_3
                %
                trig_in_ids = struct();
                
                if(length(obj.trigger_manager.triggerInputIDs) ~= 9)
                   error('Node %d has %d trigger inputs. Expected 9.', obj.ID, length(obj.trigger_manager.triggerInputIDs)); 
                end
                
                % Scalar variables
                trig_in_ids(1).ETH_A        = obj.trigger_manager.triggerInputIDs(1);
                trig_in_ids(1).ENERGY_DET   = obj.trigger_manager.triggerInputIDs(2);
                trig_in_ids(1).AGC_DONE     = obj.trigger_manager.triggerInputIDs(3);
                trig_in_ids(1).SW_REG       = obj.trigger_manager.triggerInputIDs(4);
                trig_in_ids(1).EXT_IN_P0    = obj.trigger_manager.triggerInputIDs(5);
                trig_in_ids(1).EXT_IN_P1    = obj.trigger_manager.triggerInputIDs(6);
                trig_in_ids(1).EXT_IN_P2    = obj.trigger_manager.triggerInputIDs(7);
                trig_in_ids(1).EXT_IN_P3    = obj.trigger_manager.triggerInputIDs(8);
                trig_in_ids(1).ETH_B        = obj.trigger_manager.triggerInputIDs(9);
        
                varargout = {trig_in_ids};
            end
        end
        
        
        function varargout = wl_getTriggerOutputIDs(obj)
            % Returns the trigger output IDs that can be used as inputs to trigger manager commands
            %
            % The trigger output IDs are returned in a structure that contains fields for individual 
            % triggers.  
            %
            % The fields in the structure are:
            %     - Scalar fields:
            %       - BASEBAND              - Baseband buffers module
            %       - AGC                   - Automatic Gain Controller module
            %       - EXT_OUT_P0            - External Output Pin 0
            %       - EXT_OUT_P1            - External Output Pin 1
            %       - EXT_OUT_P2            - External Output Pin 2
            %       - EXT_OUT_P3            - External Output Pin 3
            %
            % Examples:
            %     Get the trigger output ID structure (let node be a wl_node object):
            %         trig_out_ids = wl_getTriggerInputIDs(node);
            %         trig_out_ids = node.wl_getTriggerInputIDs();
            %
            %     Use the trigger input ID structure:
            %         1) Enable baseband and agc output triggers to be triggered on the Ethernet A input trigger:  
            %                wl_triggerManagerCmd(node, 'output_config_input_selection', [trig_out_ids.BASEBAND, trig_out_ids.AGC], [trig_in_ids.ETH_A]);
            %
            %         2) Configure output delay for the baseband:
            %                node.wl_triggerManagerCmd('output_config_delay', [trig_out_ids.BASEBAND], 0);
            % 
            
            if (nargout > 1) 
                % Legacy code for compatibility
                %
                % Returns a vector of trigger output IDs that can be used as inputs to trigger manager commands
                %
                % Let node0 be a wl_node object:
                %   [T_OUT_BASEBAND, T_OUT_AGC, T_OUT_D0, T_OUT_D1, T_OUT_D2, T_OUT_D3] = wl_getTriggerOutputIDs(node0)
                %   [T_OUT_BASEBAND, T_OUT_AGC, T_OUT_D0, T_OUT_D1, T_OUT_D2, T_OUT_D3] = node0.wl_getTriggerOutputIDs(node0)
                %
                if(nargout > length(obj.trigger_manager.triggerOutputIDs))
                   error('Node %d has only %d trigger outputs. User has requested %d trigger output IDs',obj.ID,length(obj.trigger_manager.triggerOutputIDs),nargout); 
                end
                
                varargout = num2cell(obj.trigger_manager.triggerOutputIDs(1:nargout));  
            
                % Print warning that this syntax will be deprecated
                try
                    temp = evalin('base', 'wl_get_trigger_output_ids_did_warn');
                catch
                    fprintf('WARNING:   This syntax for wl_getTriggerOutputIDs() is being deprecated.\n');
                    fprintf('WARNING:   Please use:  trig_out_ids = wl_getTriggerOutputIDs(node);\n');
                    fprintf('WARNING:       where trig_out_ids is a structure that contains the trigger output IDs\n');
                    fprintf('WARNING:   See WARPLab documentation for more information\n\n');
                    
                    assignin('base', 'wl_get_trigger_output_ids_did_warn', 1)
                end
                
            else
                % Structure contains:
                %     Scalar variables:  BASEBAND, AGC, DEBUG_0, DEBUG_1, DEBUG_2, DEBUG_3
                %
                trig_out_ids = struct();
                
                if(length(obj.trigger_manager.triggerOutputIDs) ~= 6)
                   error('Node %d has %d trigger outputs. Expected 6.', obj.ID, length(obj.trigger_manager.triggerOutputIDs)); 
                end
                
                % Scalar variables
                trig_out_ids(1).BASEBAND    = obj.trigger_manager.triggerOutputIDs(1);
                trig_out_ids(1).AGC         = obj.trigger_manager.triggerOutputIDs(2);
                trig_out_ids(1).EXT_OUT_P0  = obj.trigger_manager.triggerOutputIDs(3);
                trig_out_ids(1).EXT_OUT_P1  = obj.trigger_manager.triggerOutputIDs(4);
                trig_out_ids(1).EXT_OUT_P2  = obj.trigger_manager.triggerOutputIDs(5);
                trig_out_ids(1).EXT_OUT_P3  = obj.trigger_manager.triggerOutputIDs(6);
        
                varargout = {trig_out_ids};
            end
        end
        
        
        function verify_writeIQ_checksum(obj, checksum)
            % This is a callback to verify the WriteIQ checksum in the case that WriteIQ is called by a broadcast
            % transport.
            if ( numel(checksum) > 1 )
                error('Cannot verify more than one checksum at a time.')
            end

            % Get the current checksum on the node
            node_checksum = obj.wl_basebandCmd('write_iq_checksum');

            if ( node_checksum ~= checksum )
                warning('Checksums do not match on node %d:  %d != %d', obj.ID, node_checksum, checksum)
            end
        end
        
        
        function delete(obj)
            % Clears the transport object to close any open socket
            % connections in the event that the node object is deleted.
            if(~isempty(obj.transport))
                obj.transport.delete();
                obj.transport = [];
            end
        end
    end
    
   
    methods(Static=true,Hidden=true)   
        function command_out = calcCmd_helper(group,command)
            % Performs the actual command calculation for calcCmd
            command_out = uint32(bitor(bitshift(group,24),command));
        end
    end
    
    
    methods(Hidden=true)
        % These methods are hidden because users are not intended to call
        % them directly from their WARPLab scripts.     
        
        function out = calcCmd(obj, grp, cmdID)
            % Takes a group ID and a cmd ID to form a single
            % uint32 command. Every WARPLab module calls this method
            % to construct their outgoing commands.
            switch(lower(grp))
                case 'node'
                    out = obj.calcCmd_helper(obj.GRPID_NODE, cmdID);
                case 'interface'
                    out = obj.calcCmd_helper(obj.GRPID_IFC, cmdID);
                case 'baseband'
                    out = obj.calcCmd_helper(obj.GRPID_BB, cmdID);
                case 'trigger_manager'
                    out = obj.calcCmd_helper(obj.GRPID_TRIGMNGR, cmdID);
                case 'transport'
                    out = obj.calcCmd_helper(obj.GRPID_TRANS, cmdID);
                case 'user_extension'
                    out = obj.calcCmd_helper(obj.GRPID_USER, cmdID);
            end
        end
        
        
        function out = procCmd(obj, nodeInd, node, cmdStr, varargin)
            % wl_node procCmd(obj, nodeInd, node, varargin)
            %     obj:       Node object (when called using dot notation)
            %     nodeInd:   Index of the current node, when wl_node is iterating over nodes
            %     node:      Current node object
            %     cmdStr:    Command string of the interface command
            %     varargin:
            %         [1:N}  Command arguments
            %
            out    = [];

            cmdStr = lower(cmdStr);            
            switch(cmdStr)
            
                %---------------------------------------------------------
                case 'get_hardware_info'
                    % Reads details from the WARP hardware and updates node object parameters
                    %
                    % Arguments: none
                    % Returns: none (access updated node parameters if needed)
                    %
                    % Hardware support:
                    %     All:
                    %         WARPLab design version
                    %         Hardware version
                    %         Ethernet MAC Address
                    %         Number of Interface Groups
                    %         Number of Interfaces
                    %
                    %     WARP v3:
                    %         Serial number
                    %         Virtex-6 FPGA DNA
                    %
                    [MAJOR, MINOR, REVISION, XTRA] = wl_ver();
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_INFO));
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    % Response payload (all u32):
                    %      1:   Serial number
                    %      2:   FPGA DNA MSB
                    %      3:   FPGA DNA LSB
                    %      4:   MAC address bytes 5:4
                    %      5:   MAC address bytes 3:0
                    %      6:   [hw_version wl_ver_major wl_ver_minor wl_ver_rev]
                    %      7:   Current txIQ Buffer Length
                    %      8:   Current rxIQ Buffer Length
                    %      9:   Maximum txIQ Buffer Length
                    %     10:   Maximum rxIQ Buffer Length
                    %     11:   [trigger manager coreID, trigger manager numOutputs, trigger manager numInputs]
                    %     12:   number of interface groups
                    %     13:N: interface group descriptions (one u32 per group)

                    % If the serial number was provided via the network setup, then check the serial number against the HW
                    if ( ~isempty( obj.serialNumber ) ) 
                        if ( ~eq( obj.serialNumber, resp(1) ) )
                            error('Serial Number provided in config, W3-a-%d, does not match HW serial number W3-a-%d ', obj.serialNumber, resp(1))         
                        end
                    else 
                        obj.serialNumber = resp(1);
                    end
                    
                    obj.fpgaDNA = bitshift(resp(2), 32) + resp(3);
                    
                    obj.eth_MAC_addr = 2^32*double(bitand(resp(4),2^16-1)) + double(resp(5));
                    
                    obj.hwVer          = double(bitand(bitshift(resp(6), -24), 255));
                    obj.wlVer_major    = double(bitand(bitshift(resp(6), -16), 255));
                    obj.wlVer_minor    = double(bitand(bitshift(resp(6), -8), 255));
                    obj.wlVer_revision = double(bitand(resp(6), 255));
                    
                    if((obj.wlVer_major ~= MAJOR) || (obj.wlVer_minor ~= MINOR))
                        myErrorMsg = sprintf('Node %d reports WARPLab version %d.%d.%d while this PC is configured with %d.%d.%d', ...
                            obj.ID, obj.wlVer_major, obj.wlVer_minor, obj.wlVer_revision, MAJOR, MINOR, REVISION);
                        error(myErrorMsg);
                    end

                    if(obj.wlVer_revision ~= REVISION)
                        myWarningMsg = sprintf('Node %d reports WARPLab version %d.%d.%d while this PC is configured with %d.%d.%d', ...
                            obj.ID, obj.wlVer_major, obj.wlVer_minor, obj.wlVer_revision, MAJOR, MINOR, REVISION);
                        warning(myWarningMsg);
                    end
                    
                    % Get the maximum supported IQ lengths                   
                    % obj.baseband.max_txIQLen   = double(resp(7));
                    % obj.baseband.max_rxIQLen   = double(resp(8));
                    % obj.baseband.max_rxRSSILen = (obj.baseband.max_rxIQLen) / 4;    % RSSI is sampled at 1/4 the speed of IQ 

                    % Get the current supported IQ lengths                    
                    obj.baseband.txIQLen       = double(resp(9));
                    obj.baseband.rxIQLen       = double(resp(10));
                    obj.baseband.rxRSSILen     = double(resp(10))/4;
                    
                    % Trigger Manager -- core runs at different speed depending on HW version.
                    obj.trigger_manager.setNumInputs(double(bitand(resp(11),255)));
                    obj.trigger_manager.setNumOutputs(double(bitand(bitshift(resp(11),-8),255)));
                    obj.trigger_manager.coreVersion = double(bitand(bitshift(resp(11),-16),255));

                    switch(obj.hwVer)
                        %TODO: These parameters should be passed up from
                        %the board
                        case 1
                            error('WARP v1 Hardware is not supported by WARPLab 7');
                        case {2, 3}
                            % Clock frequency of the sysgen core in the design
                            %   NOTE:  In WARPLab 7.5.1, the WARP v2 sysgen clock was increased to 160 MHz
                            %
                            clock_freq   = 160e6;
                            trig_in_ids  = obj.wl_getTriggerInputIDs();
                            trig_out_ids = obj.wl_getTriggerOutputIDs();
                            
                            % Trigger output delays
                            for k = length(obj.trigger_manager.triggerOutputIDs):-1:1
                                % With Trigger Processor v1.07.a, all delays were increased to 65535                                
                                obj.trigger_manager.output_delayStep_ns(k) = (1/(clock_freq))*1e9;
                                obj.trigger_manager.output_delayMax_ns(k)  = 65535 * obj.trigger_manager.output_delayStep_ns(k);
                            end
                            
                            % Trigger input delays
                            for k = length(obj.trigger_manager.triggerInputIDs):-1:1
                                obj.trigger_manager.input_delayStep_ns(k) = (1/(clock_freq))*1e9;
                                obj.trigger_manager.input_delayMax_ns(k)  = 31 * obj.trigger_manager.input_delayStep_ns(k);
                            end
                    end
                    
                    obj.num_interfacesGroups = resp(12);
                    obj.num_interfaces = resp(13);
                    
                    %% TODO - parse each interface descriptor and create interface group objects
                    
                %---------------------------------------------------------
                case 'get_fpga_temperature'
                    % Reads the temperature (in Celsius) from the FPGA
                    %
                    % Arguments: none
                    % Returns: (double currTemp), (double minTemp), (double maxTemp)
                    %   currTemp - current temperature of FPGA in degrees Celsius
                    %   minTemp - minimum recorded temperature of FPGA in degrees Celsius
                    %   maxTemp - maximum recorded temperature temperature of FPGA in degrees Celsius
                    %                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TEMPERATURE));
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    % Convert from raw temperature to Celsius
                    out = ((double(resp)/65536.0)/0.00198421639) - 273.15;

                %---------------------------------------------------------
                case 'initialize'
                    % Initializes the node; this must be called at least once per power cycle of the node
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_INITIALIZE));
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'identify'
                    % Blinks the user LEDs on the WARP node, to help identify MATLAB node-to-hardware node mapping
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_IDENTIFY));
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'node_config_reset'
                    % Resets the HW state of the node to accept a new node configuration
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_NODE_CONFIG_RESET));                    
                    myCmd.addArgs(uint32(node.serialNumber));  
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                % 'node_config_setup' is only implemented in wl_initNodes.m 
                %

                %---------------------------------------------------------
                case 'node_mem_read'
                    % Read memory addresses in the node
                    %
                    % Arguments:   (uint32 ADDRESS), (uint32 LENGTH)
                    % Returns:     Vector of uint32 values read from the node
                    %
                    % ADDRESS:     Address to start the read
                    %
                    % LENGTH:      Number of uint32 words to read from the node
                    %
                    % NOTE:  The node enforces a maximum number of words that can be transferred for a 
                    %     given read.  This is typically on the order of 350 words.
                    %
                    % NOTE:  Please use the C code files, such as xparameters.h and other header files,
                    %     to understand the addresses of various registers in the system and the meaning
                    %     of the bits within those registers.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_MEM_RW));
                    
                    % Check arguments
                    if(length(varargin) ~= 2)
                        error('%s: Requires two arguments:  Address, Length (in uint32 words of the read)', cmdStr);
                    end
                    
                    address     = varargin{1};
                    read_length = varargin{2};

                    myCmd.addArgs(myCmd.CMD_PARAM_READ_VAL);
                    
                    % Check arguments
                    if ((address < 0) || (address > hex2dec('FFFFFFFF')))
                        error('%s: Address must be in [0, 0xFFFFFFFF].\n', cmdStr);
                    end

                    if ((read_length < 0) || (read_length > 350))
                        error('%s: Length must be in [0, 350] words.\n', cmdStr);
                    end
                    
                    % Send command to the node
                    myCmd.addArgs(address);
                    myCmd.addArgs(read_length);
                    
                    resp = node.sendCmd(myCmd);
                    
                    % Process response from the node.  Return arguments:
                    %         [1] - Status
                    %         [2] - Length
                    %     [ 3: N] - Values
                    %
                    for i = 1:numel(resp)                   % Needed for unicast node_group support
                        ret   = resp(i).getArgs();

                        if (ret(1) == myCmd.CMD_PARAM_SUCCESS)
                            if ((ret(2) + 2) == length(ret))
                                if (i == 1) 
                                    out = ret(3:end);
                                else
                                    out(:,i) = ret(3:end);
                                end
                            else
                                msg = sprintf('%s: Memory read length mismatch error in node %d:  %d != %d\n', cmdStr, nodeInd, (ret(2) + 2), length(ret));
                                error(msg);
                            end
                        else 
                            msg = sprintf('%s: Memory read error in node %d.\n', cmdStr, nodeInd);
                            error(msg);
                        end
                    end
                    
                %---------------------------------------------------------
                case 'node_mem_write'
                    % Write memory addresses in the node
                    %
                    % Arguments: (uint32 ADDRESS), (uint32 VALUES)
                    % Returns: none
                    %
                    % ADDRESS:     Address to start the write
                    %
                    % VALUES:      Vector of uint32 words to write to the node
                    %
                    % NOTE:  The node enforces a maximum number of words that can be transferred for a 
                    %     given write.  This is typically on the order of 350 words.
                    %
                    % NOTE:  Please use the C code files, such as xparameters.h and other header files,
                    %     to understand the addresses of various registers in the system and the meaning
                    %     of the bits within those registers.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_MEM_RW));
                    
                    % Check arguments
                    if(length(varargin) ~= 2)
                        error('%s: Requires two arguments:  Address, Vector of uint32 words to write', cmdStr);
                    end
                    
                    address     = varargin{1};
                    values      = varargin{2};

                    myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                    
                    % Check arguments
                    if ((address < 0) || (address > hex2dec('FFFFFFFF')))
                        error('%s: Address must be in [0, 0xFFFFFFFF].\n', cmdStr);
                    end

                    if ((length(values) < 0) || (length(values) > 350))
                        error('%s: Length of VALUES vector must be in [0, 350] words.\n', cmdStr);
                    end
                    
                    % Send command to the node
                    if(~isempty(values))
                        myCmd.addArgs(address);
                        myCmd.addArgs(length(values));
                        myCmd.addArgs(values);
                        
                        resp = node.sendCmd(myCmd);

                        % Process response from the node.  Return arguments:
                        %         [1] - Status
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (ret(1) ~= myCmd.CMD_PARAM_SUCCESS)
                                msg = sprintf('%s: Memory write error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        end
                    end
                    
                %---------------------------------------------------------
                otherwise
                    error( 'unknown node command %s', cmdStr);
            end
            
            if((iscell(out) == 0) && (numel(out) ~= 1))
                out = {out}; 
            end     
        end
        
        
        function out = sendCmd(obj, cmd)
            % This method is responsible for serializing the command
            % objects provided by each of the components of WARPLab into a
            % vector of values that the transport object can send. This
            % method is used when the board must at least provide a
            % transport-level acknowledgement. If the response has actual
            % payload that needs to be provided back to the calling method,
            % that response is passed as an output of this method.
            %
            resp = obj.transport.send(cmd.serialize(), true);
            
            out = wl_resp(resp);
        end
        
        
        function sendCmd_noresp(obj, cmd)
            % This method is responsible for serializing the command
            % objects provided by each of the components of WARPLab into a
            % vector of values that the transport object can send. This 
            % method is used when a board should not send an immediate
            % response. The transport object is non-blocking -- it will send
            % the command and then immediately return.
            %
            obj.transport.send(cmd.serialize(), false);
        end 
        
        
        function out = receiveResp(obj)
            % This method will return a vector of responses that are
            % sitting in the host's receive queue. It will empty the queue
            % and return them all to the calling method.
            %
            resp = obj.transport.receive();
            
            if(~isempty(resp))
                % Create vector of response objects if received string of bytes is a concatenation of many responses
                done       = false;
                index      = 1;
                resp_index = 1;
                while ~done
                    out(index)  = wl_resp(resp(resp_index:end));
                    currRespLen = out(index).len();
                    
                    resp_index  = resp_index + currRespLen;
                    index       = index + 1;
                    
                    if(resp_index >= length(resp))
                        done = true;
                    end
                end
            else
                out = [];
            end
        end
        
        
        function out = repr(obj)
            % Return a string representation of the wl_node
            out = cell(1,length(obj));
            
            for n = 1:length(obj)
                currObj = obj(n); 
                
                if(isempty(currObj.ID))
                    out(n) =  {'Node has not been initialized'};
                else
                    ID     = sprintf('%d', currObj.ID);
                    
                    if(currObj.hwVer == 3)
                        SN = sprintf('W3-a-%05d',currObj.serialNumber);
                    else
                        SN = 'N/A';
                    end
                    
                    out(n) = {sprintf('%12s (ID = %3s)', SN, ID)};
                end
            end
            
            if (length(obj) == 1) 
                out = out{1};
            end
        end
        
        
        function disp(obj)
            % This is a "pretty print" method to summarize details about wl_node
            % objects and print them in a table on Matlab's command line.
            %
            hasUserExt = 0;
            strLen     = 0;
            
            for n = 1:length(obj)
               currObj = obj(n);
               hasUserExt = hasUserExt || ~isempty(currObj.user); 
               
               if(~isempty(currObj.user))
                  strLen = max(strLen,length(class(currObj.user)) + 3);        % +3 is for surrounding spaces and the | 
               end
            end
            
            extraTitle = '';
            extraLine = '';
            extraArgs = '';
            
            if(hasUserExt)
                extraTitle = repmat(' ', 1, strLen-1);
                extraLine  = repmat('-', 1, strLen-1);
                extraTitle = [extraTitle, '|'];
                extraLine  = [extraLine, '-'];
                
                newTitle = 'User Ext.';
                extraTitle((strLen - length(newTitle) - 1):(end - 2)) = newTitle;
            end
                    fprintf('Displaying properties of %d wl_node objects:\n',length(obj));
                    fprintf('|  ID |  WLVER |  HWVER |    Serial # |  Ethernet MAC Addr |          Address | %s\n', extraTitle)
                    fprintf('-------------------------------------------------------------------------------%s\n', extraLine)
            for n = 1:length(obj)
                currObj = obj(n); 
                
                if(~isempty(currObj.user))
                    myFormat = sprintf('%%%ds |',strLen-2);
                    extraArgs = sprintf(myFormat,class(currObj.user));
                elseif(hasUserExt)
                    extraArgs = extraLine;
                    extraArgs(end) = '|';
                end
                
                if(isempty(currObj.ID))
                    fprintf('|     N/A     Node object has not been initialized                            |%s\n', extraArgs)
                else
                    ID    = sprintf('%d', currObj.ID);
                    WLVER = sprintf('%d.%d.%d', currObj.wlVer_major, currObj.wlVer_minor, currObj.wlVer_revision);
                    HWVER = sprintf('%d', currObj.hwVer);
                    
                    if(currObj.hwVer == 3)
                        SN = sprintf('W3-a-%05d',currObj.serialNumber);
                    else
                        SN = 'N/A';
                    end
                    
                    temp    = dec2hex(uint64(currObj.eth_MAC_addr),12);                    
                    MACADDR = sprintf('%2s-%2s-%2s-%2s-%2s-%2s',...
                        temp(1:2),temp(3:4),temp(5:6),temp(7:8),temp(9:10),temp(11:12));

                    if ( ~isempty( currObj.transport ) & ~isempty( currObj.transport.getAddress() ) )
                        ADDR = currObj.transport.getAddress();
                    else
                        ADDR = '';
                    end                     
                        
                    fprintf('|%4s |%7s |%7s |%12s |%19s |%17s |%s\n', ID, WLVER, HWVER, SN, MACADDR, ADDR, extraArgs);
                    
                end
                    fprintf('-------------------------------------------------------------------------------%s\n', extraLine)
            end
        end        
    end % end methods(Hidden)
end % end classdef
