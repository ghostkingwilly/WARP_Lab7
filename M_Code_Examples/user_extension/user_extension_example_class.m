classdef user_extension_example_class < wl_user_ext
    properties
       description; 
    end
    properties(Hidden = true,Constant = true)
       CMD_EEPROM_WRITE_STRING = 1;
       CMD_EEPROM_READ_STRING = 2;
    end
    methods
        function obj = user_extension_example_class()
            obj@wl_user_ext();
            obj.description = 'This is an example user extension class.';
        end
        
         function out = procCmd(obj, nodeInd, node, varargin)
            %user_extension_example_class procCmd(obj, nodeInd, node, varargin)
            % obj: baseband object (when called using dot notation)
            % nodeInd: index of the current node, when wl_node is iterating over nodes
            % node: current node object (the owner of this baseband)
            % varargin:
            %  Two forms of arguments for commands:
            %   (...,'theCmdString', someArgs) - for commands that affect all buffers
            %   (..., RF_SEL, 'theCmdString', someArgs) - for commands that affect specific RF paths
            out = [];
            
            if(ischar(varargin{1}))
               %No RF paths specified
               cmdStr = varargin{1};    
               if(length(varargin)>1)
                   varargin = varargin(2:end);
               else
                   varargin = {};
               end
            else
               %RF paths specified
               rfSel = (varargin{1});
               cmdStr = varargin{2};

               if(length(varargin)>2)
                   varargin = varargin(3:end);
               else
                   varargin = {};
               end
            end
            
            cmdStr = lower(cmdStr);
            switch(cmdStr)
                case 'eeprom_write_string'
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_EEPROM_WRITE_STRING));
                    eeprom_addr = varargin{1};
                    myCmd.addArgs(eeprom_addr);
                    inputString = varargin{2};
                    inputString = [inputString,0]; %null-terminate
                    padLength = mod(-length(inputString),4);
                    inputByte = [uint8(inputString),zeros(1,padLength)];
                    inputInt = typecast(inputByte,'uint32');
                    myCmd.addArgs(inputInt);                        
                    node.sendCmd(myCmd);
                case 'eeprom_read_string'
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_EEPROM_READ_STRING));
                    eeprom_addr = varargin{1};
                    myCmd.addArgs(eeprom_addr);
                    readLength = varargin{2}+1; %the +1 is for the termination character
                    padLength = mod(-readLength,4);
                    myCmd.addArgs(readLength+padLength);                        
                    resp = node.sendCmd(myCmd);
                    ret = resp.getArgs();
                    outputByte = typecast(ret,'uint8');
                    out = char(outputByte);
                    out = out(1:varargin{2});
            end
         end
        
    end
end