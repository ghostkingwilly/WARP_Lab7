%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% user_extension_script.m
%
% In this example, we attach a user_extension_example_class object to a
% node and use it to send custom commands that allow us to write to and
% read from the EEPROM located on the board.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clear;
NUMNODES = 1;
nodes = wl_initNodes(NUMNODES);
wl_setUserExtension(nodes,user_extension_example_class);

eeprom_address = 0; %starting address 

num_characters = 11;
curr_eeprom = wl_userExtCmd(nodes,'eeprom_read_string',eeprom_address,num_characters);

fprintf('\n\nCurrent EEPROM Contents:\n');
for n = 1:NUMNODES
    if(iscell(curr_eeprom))
        fprintf('Node %d: ''%s''\n',n,curr_eeprom{n});
    else
        %In the case of 1 node, the return will not be a cell array,
        %so we can just directly print it.
        fprintf('Node %d: ''%s''\n',n,curr_eeprom);
    end
end

strWrite = 'Hello World';
fprintf('\n\nWriting EEPROM Contents:''%s''\n',strWrite);
wl_userExtCmd(nodes,'eeprom_write_string',eeprom_address, strWrite);