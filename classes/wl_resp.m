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

classdef wl_resp < wl_msg_helper
   properties(SetAccess = public)
      cmd;
   end
   
   properties(SetAccess = protected)
      cmd_len;
      numArgs;
   end
   
   properties(SetAccess = protected, Hidden=true)
      args; 
   end
   
   methods
       function obj = wl_resp(varargin)
            obj.cmd     = uint32(0); 
            obj.cmd_len = uint16(0); 
            obj.numArgs = uint16(0);  
            obj.args    = uint32([]);
           
            if(length(varargin) == 1)
                obj.deserialize(varargin{1});
            end
       end
       
       function deserialize(obj,vec)
            vec         = uint32(vec);
            obj.cmd     = vec(1);
            obj.numArgs = bitand(65535, vec(2));
            obj.cmd_len = bitshift(vec(2), -16);
            
            if(length(vec) > 2)
                obj.args = {vec(3:(2 + (obj.cmd_len/4)))};
            end
       end
       
       function output = getArgs(obj)
            if(isempty(obj.args)==0)
                output = cat(2,obj.args{:});
            else
                output = [];
            end
       end
       
       function output = len(obj)
           output = (obj.cmd_len / 4) + 2;
       end
       
       function output = sizeof(obj)
           output = length(obj.serialize) * 4;
       end
           
       function output = serialize(obj)
           output(1) = obj.cmd;
           output(2) = bitor(bitshift(uint32(obj.cmd_len),16), uint32(obj.numArgs));
           output = [output, obj.getArgs];
       end
   end
end