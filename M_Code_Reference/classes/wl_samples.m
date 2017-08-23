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

classdef wl_samples < wl_msg_helper   
    properties(SetAccess = public)
        buffSel;
        flags;
        startSamp;
    end

    properties(SetAccess = public, Hidden = true)
        numSamp;
        sample_iq_id; 
        samps;
    end
    
    properties(Hidden = true, Constant = true)
        FLAG_IQ_ERROR      = 1;         % 0x01
        FLAG_IQ_NOT_READY  = 2;         % 0x02
        FLAG_CHKSUM_RESET  = 16;        % 0x10
        FLAG_LAST_WRITE    = 32;        % 0x20
    end
    
    methods
        function obj = wl_samples(varargin)
           obj.buffSel       = uint16(0); 
           obj.flags         = uint8(0); 
           obj.sample_iq_id  = uint8(0); 
           obj.startSamp     = uint32(0);
           obj.numSamp       = uint32(0);
           obj.samps         = [];
           
           if(length(varargin) == 1) 
               obj.deserialize(varargin{1});
           end
        end
        
        function setFlags(obj, flags)
           flags = uint8(flags);
           obj.flags = bitor(obj.flags, flags);
        end
        
        function clearFlags(obj, flags)
           flags = uint8(flags);
           obj.flags = bitand(obj.flags, bitcmp(flags));
        end
        
        function setSamples(obj, samples)
           obj.numSamp = length(samples);
           obj.samps = samples(:).';
        end
        
        function output = getSamples(obj)
           output = obj.samps; 
        end
        
        function output = serialize(obj)
            % Serializes the header and samples to a vector of uint32 items
            %     The structure of the sample header is:
            %             typedef struct {
            %             	  u16 buffSel;
            %                 u8  flags;
            %             	  u8  sample_iq_id;
            %             	  u32 startSamp;
            %             	  u32 numSamp;
            %             } wl_bb_samp_hdr;
            %
            % NOTE:  Calling cast(x,'uint32') is much slower than uint32(2)
            %        Bitshift/bitor are slower than explicit mults/adds
            %
            output(1) = (2^16 * uint32(obj.buffSel)) + (2^8 * uint32(obj.flags)) + uint32(obj.sample_iq_id);
            output(2) = obj.startSamp;
            output(3) = obj.numSamp;
            
            if ( ~isempty(obj.samps))
                output    = cat(2, output, obj.samps);
            end
        end
        
        function deserialize(obj,vec)
           vec           = uint32(vec);
           obj.buffSel   = bitshift(bitand(vec(1), 4294901760), -16);
           obj.flags     = bitshift(bitand(vec(1), 65280), -8);
           obj.startSamp = vec(2);
           obj.numSamp   = vec(3);
           
           if(length(vec) > 3)
              obj.samps = vec(4:end);
           end
           
        end
        
        function output = sizeof(obj)
            persistent wl_samples_length;
            
            if(isempty(wl_samples_length))
                wl_samples_length = length(obj.serialize)*4;
            end
            
            output = wl_samples_length;
        end
    end
end