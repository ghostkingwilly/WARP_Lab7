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

classdef wl_cmd < handle_light

    properties(SetAccess = public)
        cmd;
    end
   
    properties(SetAccess = public, Hidden = true)
        len;
        numArgs;
    end
   
    properties(SetAccess = protected, Hidden=true)
        args; 
    end

    properties(SetAccess = public, Hidden=true)
        % These properties correspond to the WARPLab Command Defines in wl_common.h
        CMD_PARAM_WRITE_VAL       = 0;                     % 0x00000000
        CMD_PARAM_READ_VAL        = 1;                     % 0x00000001
        CMD_PARAM_RSVD            = hex2dec('FFFFFFFF');   % 0xFFFFFFFF

        CMD_PARAM_SUCCESS         = 0;                     % 0x00000000
        CMD_PARAM_ERROR           = hex2dec('FF000000');   % 0xFF000000
    end
   
    methods
        function obj = wl_cmd(varargin)
            obj.cmd = uint32(0); 
            obj.len = uint16(0); 
            obj.numArgs = uint16(0);  
           
            if(nargin > 0)
                obj.cmd = uint32(varargin{1});
            end
           
            if(nargin > 1)
                obj.setArgs(varargin{2:end});
            end         
        end
       
        function print(obj,varargin)
            fprintf('WL Command:  0x%8x   length = 0x%x    num_args = 0x%d \n', obj.cmd, obj.len, obj.numArgs );
            tmp_args = obj.getArgs();
            for index = 1:length(tmp_args)
                fprintf('    Arg[%d] = 0x%8x \n', index, tmp_args(index));
            end
        end
       
        function setArgs(obj,varargin)
            % Delete old args
            obj.len = uint16(0);
            obj.numArgs = uint16(0);
            obj.addArgs(varargin{:});
        end
       
        function addArgs(obj,varargin)
            for index = length(varargin):-1:1
                if(min(size(varargin{index}))~=1)
                    error('argument %d must be a scalar or 1-dimensional vector',index)
                end
                newArg = uint32(varargin{index}(:).');
                obj.args{index + (obj.numArgs)} = newArg;
                obj.len = obj.len + (length(newArg)*4);
            end
            obj.numArgs = obj.numArgs + length(varargin);
        end
       
        function output = getArgs(obj)
            if(isempty(obj.args) == 0)
                output = cat(2, obj.args{:});
            else
                output = [];
            end
        end
       
        function output = sizeof(obj)
            output = length(obj.serialize)*4;
        end
           
        function output = serialize(obj)
            % Serializes the command header and all arguments to a vector of uint32 items
            %     The structure of the command header is:
            %             typedef struct {
            %                 u32       cmd;
            %                 u16       length;
            %                 u16       numArgs;
            %             } wl_cmd_hdr;
            %
            % NOTE:  Calling cast(x,'uint32') is much slower than uint32(2)
            %        Bitshift/bitor are slower than explicit mults/adds
            %            
            output(1) = obj.cmd;
            output(2) = (2^16 * uint32(obj.len)) + uint32(obj.numArgs);
            
            if (~isempty(obj.args))
                output = cat(2, output, obj.args{:});
            end
        end
    end
end