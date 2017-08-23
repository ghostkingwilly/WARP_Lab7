%******************************************************************************
% WARPLab Setup
%
% File   :	wl_setup.m
% Authors:	Chris Hunter (chunter [at] mangocomm.com)
%			Patrick Murphy (murphpo [at] mangocomm.com)
%           Erik Welsh (welsh [at] mangocomm.com)
% License:	Copyright 2013, Mango Communications. All rights reserved.
%			Distributed under the WARP license  (http://warpproject.org/license)
%
%******************************************************************************
% MODIFICATION HISTORY:
%
% Ver   Who  Date     Changes
% ----- ---- -------- -------------------------------------------------------
% 3.00a ejw  8/21/13  Update to use new WARPLab MEX transport;
%                     Added mex/ directory to MATLAB path
%
%******************************************************************************

function wl_setup

REQUIRED_MEX_VERSION = '1.0.4a';


fprintf('Setting up WARPLab Paths...\n');

%------------------------------------------------------------------------------
% Remove any paths from previous WARPLab installations

% NOTE:  We had to add in code involving regular expressions to catch a absolute vs 
%     relative path issue that we found.  For example, if ./util exists in the path, 
%     then the previous version of the code will run forever because it will find
%     the file using 'which' but then the absolute path returned will not be in the
%     actual path.  Now, we use regular expressions to make sure the path exists 
%     before removing it.

% Remove legacy WARPLab v6 paths
%   NOTE:  warplab_defines is not used in WARPLab v7
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('warplab_defines'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);
        
        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
                
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../M_Code_Reference/classes
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('wl_node'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
                
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../M_Code_Reference/util
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('wl_ver'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
                
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../M_Code_Reference/util/inifile
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('inifile'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
                
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../M_Code_Reference/config
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('wl_config.ini'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
                
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../M_Code_Reference/mex
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('wl_mex_udp_transport'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
                
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
        else
            done = true;
        end
    else
        done = true;
    end
end



%------------------------------------------------------------------------------
% Add new paths 

% Add  .../M_Code_Reference/classes
myPath = sprintf('%s%sclasses',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/util
myPath = sprintf('%s%sutil',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/util/inifile
myPath = sprintf('%s%sutil%sinifile',pwd,filesep,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/mex
myPath = sprintf('%s%smex',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/config
configPath = sprintf('%s%sconfig',pwd,filesep);
fprintf('   adding path ''%s''\n',configPath);
addpath(configPath) 

configFile = sprintf('%s%swl_config.ini',configPath,filesep);

% Example of adding a user path to WARPLab setup:
%   In this example, C:\WARP_Repository\ResearchApps\PHY\WARPLAB\WARPLab7\M_Code_Reference\mex is added to the MATLAB path
%
% myPath = sprintf('C:%sWARP_Repository%sResearchApps%sPHY%sWARPLAB%sWARPLab7%sM_Code_Reference%smex',filesep,filesep,filesep,filesep,filesep,filesep,filesep);
% fprintf('   adding path ''%s''\n',myPath);
% addpath(myPath) 

fprintf('   saving path\n\n\n');
savepath

%------------------------------------------------------------------------------
% Read config file

IP                         = '';
host                       = '';
port_unicast               = '';
port_bcast                 = '';
transport                  = '';
max_transport_payload_size = '';

% Read existing config file for defaults
if(exist(configFile))
    fprintf('A wl_config.ini file was found in your path. Values specified in this\n');
    fprintf('configuration file will be used as defaults in the construction of the\n');
    fprintf('new file.\n');
    
    readKeys = {'network', '', 'host_address',''};
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
end

% Sane Defaults
if(isempty(IP))
    IP = '10.0.0.250';
end    
if(isempty(host))
    host = '250';
end    
if(isempty(port_unicast))
    port_unicast = '9000';
end    
if(isempty(port_bcast))
    port_bcast = '10000';
end    
% if(isempty(transport))               % Moved lower in the function
%     transport = 'java';
% end    
if(isempty(max_transport_payload_size))
    max_transport_payload_size = '1470';
end   

inifile(configFile,'new')

writeKeys = {'config_info', '', 'date_created', date};
inifile(configFile, 'write', writeKeys, 'tabbed')

[MAJOR,MINOR,REVISION] = wl_ver();
writeKeys = {'config_info', '', 'wl_ver', sprintf('%d.%d.%d', MAJOR, MINOR, REVISION)};
inifile(configFile, 'write', writeKeys, 'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a WARPLab Ethernet interface address.\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('IP address of: %s\n\n', IP);

% Sanity check input due to errors users were experiencing on the forums
ip_valid = 0;

while( ip_valid == 0 ) 

    temp = input('WARPNet Ethernet Interface Address: ','s');
    if( isempty(temp) )
       temp     = IP; 
       ip_valid = 1;
       fprintf('   defaulting to %s\n',temp);
    else
       if ( regexp( temp, '\d+\.\d+\.\d+\.\d+' ) == 1 ) 
           ip_valid = 1;
           fprintf('   setting to %s\n',temp)
       else
           fprintf('   %s is not a valid IP address.  Please enter a valid IP address.\n',temp); 
       end
    end
end

if(ispc)
   [status, tempret] = system('ipconfig /all');
elseif(ismac||isunix)
   [status, tempret] = system('ifconfig -a');
end
tempret = strfind(tempret,temp);

if(isempty(tempret))
   warning('No interface found. Please ensure that your network interface is connected and configured with static IP %s',temp);
   pause(1)
end

writeKeys = {'network', '', 'host_address', temp};
inifile(configFile, 'write', writeKeys, 'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a WARPLab host ID.\n')
fprintf('Valid host IDs are integers in the range of [200,254]\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('host ID of: %s\n\n', host);

temp = input('WARPLab Host ID: ', 's');
if(isempty(temp))
   temp = host; 
   if(isempty(str2num(temp)))
        error('Host ID must be an integer in the range of [200,254]');
   else
       if((str2num(temp) < 200) || ((str2num(temp) > 254)))
           error('Host ID must be an integer in the range of [200,254]');
       end
   end
   fprintf('   defaulting to %s\n', temp);
else
   if(isempty(str2num(temp)))
        error('Host ID must be an integer in the range of [200,254]');
   else
       if((str2num(temp) < 200) || ((str2num(temp) > 254)))
           error('Host ID must be an integer in the range of [200,254]');
       end
   end
   
   fprintf('   setting to %s\n', temp); 
end

writeKeys = {'network', '', 'host_ID', temp};
inifile(configFile, 'write', writeKeys, 'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a unicast starting port.\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('unicast starting port of: %s\n\n', port_unicast);

temp = input('Unicast Starting Port: ', 's');
if(isempty(temp))
   temp = port_unicast; 
   fprintf('   defaulting to %s\n', temp);
else
   fprintf('   setting to %s\n', temp); 
end

writeKeys = {'network', '', 'unicast_starting_port', temp};
inifile(configFile, 'write', writeKeys, 'tabbed')


fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a broadcast port.\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('broadcast port of: %s\n\n', port_bcast);

temp = input('Broadcast Port: ', 's');
if(isempty(temp))
   temp = port_bcast; 
   fprintf('   defaulting to %s\n', temp);
else
   fprintf('   setting to %s\n', temp); 
end

writeKeys = {'network', '', 'bcast_port', temp};
inifile(configFile, 'write', writeKeys, 'tabbed')


fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
%%%%%%%% Transport Setup %%%%%%%%
I_JAVA       = 1;
I_WL_MEX_UDP = 2;

TRANS_NAME_LONG{I_JAVA}        = 'Java UDP';
TRANS_NAME_SHORT{I_JAVA}       = 'java';
TRANS_NAME_LONG{I_WL_MEX_UDP}  = 'WARPLab Mex UDP';
TRANS_NAME_SHORT{I_WL_MEX_UDP} = 'wl_mex_udp';

TRANS_AVAIL(I_JAVA)        = true;
TRANS_AVAIL(I_WL_MEX_UDP)  = false;

fprintf('Select from the following available transports:\n')

try
    version     = wl_mex_udp_transport('version');
    version     = sscanf(version, '%d.%d.%d%c');
    version_req = sscanf(REQUIRED_MEX_VERSION, '%d.%d.%d%c');

    % Version must match required version    
    if(version(1) == version_req(1) && version(2) == version_req(2) && version(3) == version_req(3) && version(4) >= version_req(4))
        TRANS_AVAIL(I_WL_MEX_UDP) = true;
    else
        [major, minor, rev]  = wl_ver();
        wl_version  = sprintf('v%d.%d.%d', major, minor, rev);
        version     = wl_mex_udp_transport('version');
        version_req = REQUIRED_MEX_VERSION;
        fprintf('\nERROR:  MEX transport version mismatch: \n')
        fprintf('ERROR:  WARPLab %s requires MEX transport version %s.\n', wl_version, version_req )
        fprintf('ERROR:  Current complied version is %s.  \n', version)
        fprintf('ERROR:  Please recompile to use the MEX transport:\n', version)
        fprintf('ERROR:      http://warpproject.org/trac/wiki/WARPLab/MEX \n\n')
    end
    
catch me
    fprintf('   For better transport performance, please setup the WARPLab Mex UDP transport: \n')
    fprintf('      http://warpproject.org/trac/wiki/WARPLab/MEX \n')
    TRANS_AVAIL(I_WL_MEX_UDP) = false;
end

% Set MEX to be the default transport if available
if(isempty(transport))
    if(TRANS_AVAIL(I_WL_MEX_UDP))
        transport = 'wl_mex_udp';
    else
        transport = 'java';
    end
end

% Check if any transports are available
if(any(TRANS_AVAIL)==0)
   error('no supported transports were found installed on your computer'); 
end

defaultSel = 1;
sel        = 1;
for k = 1:length(TRANS_AVAIL)
    if(TRANS_AVAIL(k))
        if(strcmp(TRANS_NAME_SHORT{k}, transport))
            defaultSel = sel;
            fprintf('[%d] (default) %s\n', sel, TRANS_NAME_LONG{k})
        else
            fprintf('[%d]           %s\n', sel, TRANS_NAME_LONG{k})
        end
        selectionToIndex(sel) = k;
        sel = sel + 1;
    end
end

temp = input('Selection: ');
if(isempty(temp))
    temp = defaultSel;
end

if(temp>(sel-1))
   error('invalid selection') 
end

transport = TRANS_NAME_SHORT{selectionToIndex(temp)};

fprintf('   setting to %s\n', transport);

writeKeys = {'network', '', 'transport', transport};
inifile(configFile, 'write', writeKeys, 'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
%%%%%%%% Transport Setup %%%%%%%%
PAYLOAD_SIZE{1} = '1470';
PAYLOAD_SIZE{2} = '5000';
PAYLOAD_SIZE{3} = '8966';

fprintf('Select from the following maximum transport payload size:\n')

sel = 1;
for k = 1:3

    % Check if this exercises jumbo mode
    if (str2num(PAYLOAD_SIZE{k}) <= 1470)
        jumbo = '(non-jumbo)';
    else
        jumbo = '(jumbo)';
    end

    if(strcmp(PAYLOAD_SIZE{k}, max_transport_payload_size))
        defaultSel = sel;
        fprintf('[%d] (default) %s bytes %s\n', sel, PAYLOAD_SIZE{k}, jumbo)
    else
        fprintf('[%d]           %s bytes %s\n', sel, PAYLOAD_SIZE{k}, jumbo)
    end
    selectionToIndex(sel) = k;
    sel = sel + 1;
end

fprintf('\nDepending on your host NIC and network setup you may be able to\n')
fprintf('use larger packets to increase your transport performance.\n\n')

temp = input('Selection: ');

if(isempty(temp))
   temp = defaultSel;
end

if(temp > (sel - 1))
   error('invalid selection') 
end

max_transport_payload_size = PAYLOAD_SIZE{selectionToIndex(temp)};

fprintf('   setting maximum transport payload size to %s bytes\n', max_transport_payload_size);

writeKeys = {'network', '', 'max_transport_payload_size', max_transport_payload_size};
inifile(configFile, 'write', writeKeys, 'tabbed')


fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('\n\nSetup Complete\nwl_ver():\n');
wl_ver()
fprintf('\n')

% Catch 64-bit support issues in Matlab during wl_setup
try
    temp_1 = uint64(0);
    temp_2 = uint64(0);
    result = temp_1 + temp_2;
catch
    warning('Matlab version does not support uint64 arithmetic.  Please use Matlab R2011a or later.');
end

end