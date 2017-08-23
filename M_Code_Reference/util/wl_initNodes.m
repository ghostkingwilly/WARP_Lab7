%==============================================================================
% Function wl_initNodes()
%
% Input:  
%     - Single number
%     - Array of struct (derived from wl_networkSetup())
%     - Array of wl_nodes; Either optional array of numbers, or array of struct 
%
% Output:
%     - Array of wl_nodes
% 
%==============================================================================

function nodes = wl_initNodes(varargin)
    
    if nargin == 0
        error('Not enough arguments are provided');
    elseif nargin == 1
        switch( class(varargin{1}) ) 
            case 'wl_node'
                nodes    = varargin{1};
                numNodes = length(nodes);
                nodeIDs  = 0:(numNodes-1);            % Node IDs default:  0 to (number of nodes in the array)
            case 'struct'
                numNodes        = length(varargin{1}); 
                nodes(numNodes) = wl_node;
                nodeIDs         = varargin{1};         % Node IDs are specified in the input structure
            case 'double'
                numNodes        = varargin{1}; 
                nodes(numNodes) = wl_node;
                nodeIDs         = 0:(numNodes-1);     % Node IDs default:  0 to (number of nodes specified)
            otherwise
                error('Unknown argument.  Argument is of type "%s", need "wl_node", "struct", or "double"', class(varargin{1}));
        end        
    elseif nargin == 2

        switch( class(varargin{1}) ) 
            case 'wl_node'
                nodes    = varargin{1};
                numNodes = length(nodes);
            case 'double'
                numNodes        = varargin{1}; 
                nodes(numNodes) = wl_node;
            otherwise
                error('Unknown argument.  Argument is of type "%s", need "wl_node" or "double"', class(varargin{1}));
        end        

        switch( class(varargin{2}) ) 
            case 'struct'
                nodeIDs         = varargin{2};         % Node IDs are specified in the input structure
            case 'double'
                nodeIDs         = varargin{2};        % Node IDs default:  0 to (number of nodes specified)
            otherwise
                error('Unknown argument.  Argument is of type "%s", need "struct", or "double"', class(varargin{2}));
        end        
        
        if(length(nodeIDs)~=numNodes)
            error('Number of nodes does not match ID vector');
        end     
    else
        error('Too many arguments provided');
    end

    
    gen_error       = [];
    gen_error_index = 0;
    
    for n = numNodes:-1:1
        currNode = nodes(n);

        % If we are doing a network setup of the node based on the input structure then create the broadcast packet and send it 
        % Note: 
        %   - Node must be configured with dip switches 0xF
        %     - Node will initialize itself to IP address of 10.0.0.0 and wait for a broadcast message to configure itself
        %     - Node will check against the serial number (only last 5 digits; "W3-a-" is not stored in EEPROM)
        if( strcmp( class( nodeIDs ), 'struct') )
        
            configFile = which('wl_config.ini');
            
            if(isempty(configFile))
               error('cannot find wl_config.ini. please run wl_setup.m'); 
            end
            
            readKeys = {'network','','transport',''};
            transport = inifile(configFile,'read',readKeys);
            transport = transport{1};

            switch(transport)
                case 'java'
                    transport_bcast   = wl_transport_eth_udp_java_bcast;
                    transport_unicast = wl_transport_eth_udp_java;
                case 'wl_mex_udp'
                    transport_bcast   = wl_transport_eth_udp_mex_bcast;
                    transport_unicast = wl_transport_eth_udp_mex;
            end

            % Open a broadcast transport to send the configure command to the node
            transport_bcast.open();
            tempNode      = wl_node;

            % Send serial number, node ID, and IP address
            myCmd         = wl_cmd(tempNode.calcCmd(tempNode.GRP, tempNode.CMD_NODE_CONFIG_SETUP));
            myCmd.addArgs(uint32(sscanf(nodeIDs(n).serialNumber, 'W3-a-%d')));
            myCmd.addArgs(nodeIDs(n).IDUint32);
            myCmd.addArgs(nodeIDs(n).ipAddressUint32);
            transport_bcast.send('message',myCmd.serialize());
        end

        % Error check to make sure that no node in the network has the same
        % ID as this host.
        configFile = which('wl_config.ini');
        
        if(isempty(configFile))
           error('cannot find wl_config.ini. please run wl_setup.m'); 
        end
        
        readKeys = {'network','','host_ID',''};
        hostID = inifile(configFile,'read',readKeys);
        hostID = hostID{1};
        hostID = sscanf(hostID,'%d');

        % Error check to make sure that no node in the network has the same ID as this host.
        %
        nodeID = 0;
        switch(class( nodeIDs(n) ))
            case 'double'
                nodeID = nodeIDs(n);
                if( hostID == nodeID )
                    error('Host ID is set to %d and must be unique. No node in the network can share this ID',hostID); 
                end
                
            case 'struct'
                nodeID = nodeIDs(n).IDUint32;
                if( hostID == nodeID )
                    error('Host ID is set to %d and must be unique. No node in the network can share this ID',hostID); 
                end
        end
        
        % Now that the node has a valid IP address, we can apply the configuration
        %
        try 
            currNode.applyConfiguration(nodeIDs(n));
        catch ME
            fprintf('\n');
            fprintf('Error in node %d with ID = %d: ', n, nodeID );
            ME
            fprintf('Error message follows:\n%s\n', ME.message);
            gen_error_index              = gen_error_index + 1;
            gen_error( gen_error_index ) = nodeID;
            % error('Node with ID = %d is not responding. Please ensure that the node has been configured with the WLAN Exp bitstream.',nodeID)
            fprintf('\n');
        end
    end

    % Output an error if there was one
    if( gen_error_index ~= 0 ) 

        error_nodes = sprintf('[');
        for n = 1:gen_error_index      
            error_nodes = sprintf('%s %d', error_nodes, gen_error(n));
        end
        error_nodes = sprintf('%s ]', error_nodes);
        
        error('The following nodes with IDs = %s are not responding. Please ensure that the nodes have been configured with the WARPLab bitstream.', error_nodes)
    end
    
    %Send a test broadcast trigger command
    if(strcmp(class(nodes(1).trigger_manager),'wl_trigger_manager_proc'))
        
        configFile = which('wl_config.ini');
        
        if(isempty(configFile))
           error('cannot find wl_config.ini. please run wl_setup.m'); 
        end
        
        readKeys = {'network','','transport',''};
        transport = inifile(configFile,'read',readKeys);
        transport = transport{1};

        switch(transport)
            case 'java'
                transport_bcast = wl_transport_eth_udp_java_bcast;
            case 'wl_mex_udp'
                transport_bcast = wl_transport_eth_udp_mex_bcast;
        end

        transport_bcast.open();
        tempNode = wl_node;
        myCmd = wl_cmd(tempNode.calcCmd(wl_trigger_manager_proc.GRP,wl_trigger_manager_proc.CMD_TEST_TRIGGER));
        bcastTestFlag = uint32(round(2^32 * rand));
        myCmd.addArgs(bcastTestFlag); %Signals to the board that this is writing the received trigger test variable
        transport_bcast.send('message',myCmd.serialize());

        resp = wl_triggerManagerCmd(nodes,'test_trigger');

        I = find(resp~=bcastTestFlag);
        
        for nodeIndex = I
           warning('Node %d with Serial # %d failed trigger test',nodes(nodeIndex).ID,nodes(nodeIndex).serialNumber) 
        end

        if(any(resp~=bcastTestFlag))
           error('Broadcast triggers are not working. Please verify your ARP table has an entry for the broadcast address on your WARPLab subnet') 
        end
    end

    nodes.wl_nodeCmd('initialize');
end