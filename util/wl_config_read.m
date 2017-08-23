function wl_config_read
    configFile = which('wl_config.ini');
    if(isempty(configFile))
       error('cannot find wl_config.ini. please run wl_setup.m'); 
    end
    
    readKeys = {'network', '', 'host_address', ''};
    IP = inifile(configFile, 'read', readKeys);
    IP = IP{1};
    
    readKeys = {'network', '', 'host_ID', ''};
    host = inifile(configFile, 'read', readKeys);
    host = host{1};
    
    readKeys = {'network', '', 'unicast_starting_port', ''};
    port_unicast = inifile(configFile, 'read', readKeys);
    port_unicast = port_unicast{1}; 
    
    readKeys = {'network', '', 'bcast_port', ''};
    port_bcast = inifile(configFile, 'read', readKeys);
    port_bcast = port_bcast{1}; 
    
    readKeys = {'network', '', 'transport', ''};
    transport = inifile(configFile, 'read', readKeys);
    transport = transport{1}; 
    
    readKeys = {'network', '', 'max_transport_payload_size', ''};
    max_transport_payload_size = inifile(configFile, 'read', readKeys);
    max_transport_payload_size = max_transport_payload_size{1}; 
    
    fprintf('\n\nContents of wl_config.ini:\n');
    fprintf('(located at %s)\n', configFile')
    
    fprintf('--------------------------------------\n')
    fprintf('|       Host IP Address |%12s|\n',IP);
    
    if(ispc)
       [status, tempret] = system('ipconfig /all');
    elseif(ismac||isunix)
       [status, tempret] = system('ifconfig -a');
    end
    tempret = strfind(tempret,IP);

    if(isempty(tempret))
       fprintf('\b *No interface found. Please ensure that your network interface is connected and configured with static IP %s\n',IP);
    end

    fprintf('-----------------------------------------\n')
    fprintf('|          WARPLab Host ID |%12s|\n', host);
    fprintf('-----------------------------------------\n')
    fprintf('|    Unicast Starting Port |%12s|\n', port_unicast);
    fprintf('-----------------------------------------\n')
    fprintf('|           Broadcast Port |%12s|\n', port_bcast);
    fprintf('-----------------------------------------\n')
    fprintf('|                Transport |%12s|\n', transport);
    fprintf('-----------------------------------------\n')
    fprintf('| Max Payload Size (bytes) |%12s|\n', max_transport_payload_size);
    fprintf('-----------------------------------------\n')
    
end