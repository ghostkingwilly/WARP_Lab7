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

classdef wl_transport_header < wl_msg_helper
    % Contains data needed by the transport to ensure reliable transmission

    properties(Hidden = true, Constant = true)
        PKTTYPE_TRIGGER     = uint8(0); 
        PKTTYPE_HTON_MSG    = uint8(1); 
        PKTTYPE_NTOH_MSG    = uint8(2);
        ROBUST_FLAG         = uint16(1);         % Value of 0x0001
        NODE_NOT_READY_FLAG = uint16(32768);     % Value of 0x8000
    end
    
    properties(SetAccess = public, Hidden = true)
        seqNum   = uint16(0);                    % Sequence number for outgoing transmissions
        reserved = uint8(0);
    end
    
    properties (SetAccess = public)
        flags     = uint16(0);
        msgLength = uint16(0);
        destID    = uint16(0);                   % Destination ID for outgoing transmissions
        srcID     = uint16(0);                   % Source ID for outgoing transmissions
        pktType   = uint8(0);
    end

    methods
        function obj = wl_transport_header()
            % Sets the default data type of the header to be 'uint32' and initializes sequence number to 0.
            %
            obj.seqNum = 0;
        end
        
        function len = length(obj)
            % Returns the header length in # of uint32s.
            %             
            len = 3;                            % NOTE:  This needs to match the actual u32 length
                                                % NOTE:  We are returning a constant b/c the transport header doesn't change length
        end
        
        function out = sizeof(obj)
            out = (length(obj) * 4);
        end
        
        function reset(obj)
            % Resets the sequence number back to 1.
            %
            obj.seqNum = 1;
        end

        function increment(obj, varargin)
            % Increments the sequence number.
            %
            if( nargin == 1 )
                if(obj.seqNum ~= uint16(realmax))
                    obj.seqNum = obj.seqNum + 1;
                else
                    obj.seqNum = 0;    
                end
            else
                obj.seqNum = mod( (obj.seqNum + varargin{1}), uint16(realmax) );
            end
        end

        function output = deserialize(obj)
            % Un-used for transport_headers
        end
        
        function output = serialize(obj)
            % Serializes the header to a vector of uint32 items
            %     This function serializes the header to a vector for placing into outgoing packets by
            %     wl_transport's children.  The structure of the transport header is:
            %             typedef struct {
            %                 u16 destID;
            %                 u16 srcID;
            %                 u8  reserved;
            %                 u8  pktType;
            %                 u16 length;
            %                 u16 seqNum;
            %                 u16 flags;
            %             } wl_transport_header;
            % NOTE:  Calling cast(x,'uint32') is much slower than uint32(2)
            %        Bitshift/bitor are slower than explicit mults/adds
            %
            w0 = (2^16 * uint32(obj.destID)) + uint32(obj.srcID);
            w1 = (2^24 * uint32(obj.reserved)) + (2^16 * uint32(obj.pktType)) + uint32(obj.msgLength);
            w2 = (2^16 * uint32(obj.seqNum)) + uint32(obj.flags);
            output = [w0 w1 w2];
        end
        
        function match = isReply(obj, input)
            % Checks an input vector to see if it is a valid reply to the last outgoing packet.
            %     This function checks to make sure that the received source address matches 
            %     the previously sent destination addresses and makes sure that the sequence numbers match.
            %
            try
                src_dest    = ((2^16 * obj.srcID) + obj.destID);
                reply_seq   = bitshift(input(3), -16);

                match       = min([(input(1) == src_dest), (reply_seq == obj.seqNum)]);
                
                if(match == 0)
                    reply_dest  = bitand(65535, input(1));
                    reply_src   = bitshift(input(1), -16);
                    
                    warning('wl_transport_header:setType', 'transport_header mismatch: [%d %d] [%d %d] [%d %d]\n',...
                        reply_src, obj.srcID, reply_dest, obj.destID, reply_seq, obj.seqNum);
                end
            catch receiveError
                error('wl_transport_header:isReply',sprintf('Length of header not correct.  Received %d words needed %d', length(input), obj.length()))
            end
        end

        function ready = isNodeReady(obj, input)
            % Checks an input vector to see if the node has set the TRANSPORT_HDR_NODE_NOT_READY_FLAG bit
            %
            if(length(input) == length(obj.serialize))
                ready     = ~(uint16(bitand(uint16(bitand(65535, input(3))), obj.NODE_NOT_READY_FLAG)) == obj.NODE_NOT_READY_FLAG);
            else
                error('wl_transport_header:setType',sprintf('length of header did not match internal length of %d', obj.type))
            end
        end
        
    end %methods
end %classdef
