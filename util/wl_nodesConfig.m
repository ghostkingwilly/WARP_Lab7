%==============================================================================
% Function:  wl_nodesConfig
%
% Inputs:
%     'Command'       - Command for the script to perform
%     'Filename'      - File to either read, write, or validate
%     NodeArray       - Optional argument when trying to write the nodes configuration
%
% Commands:
%     'validate'  - Will validate the input file and return 1 if valid or generate an error message
%     'read'      - Will read the input file and return the structure of nodes
%     'write'     - Will take the array of nodes and write to the specified file and return 1 if successful
%
% File Format:
%    serialNumber,ID,ipAddress,(opt 1),(opt 2), ...,(opt N)
%
% NOTE:  The ',' and '\t' are reserved characters and will be interpreted as the next field;
%        All spaces, ' ', are valid and are not removed from the field
%        If you add in a optional field, then you have to put the correct number of delimiters, ie ',',
%            to indicate fields are not used
%        There should be no spaces in the field names in the header row
%
% Example:
%  serialNumber,    ID,    ipAddress,    name,    opt_1
%  W3-a-00006,    0,    10.0.0.0,    Node 0,    primary
%  W3-a-00007,    0,    10.0.0.1,
%  W3-a-00008,    1,    10.0.0.200,    Node X
%
% This is ok due to the fact that the opt_1 field is used in a row that has the correct number of delimiters
%
% In this example, the opt_1 field is not used and the node names are "Node 0" and "primary":
%
%  serialNumber,    ID,    ipAddress,    name,    opt_1
%  W3-a-00006,    0,    10.0.0.0,    Node 0
%  W3-a-00007,    0,    10.0.0.1,    primary                  <-- Issue:  Not enough delimiters to use opt_1 field
% 
% To use the opt_1 field, you would need to add another delimiter:
%
%  serialNumber,    ID,    ipAddress,    name,    opt_1
%  W3-a-00006,    0,    10.0.0.0,    Node 0
%  W3-a-00007,    0,    10.0.0.1,,    primary
%
% in order to use the opt_1 field and leave a blank name for the node in the second row.
%
%
%==============================================================================

function nodesInfo = wl_nodesConfig(varargin)

    % Validate input parameters

    if ( (nargin == 0) | (nargin == 1) )
        error('Not enough arguments are provided to function call');
    else
        % Check that the command is valid
        if ( ~strcmp(varargin{1}, 'validate') & ~strcmp(varargin{1}, 'read') & ~strcmp(varargin{1}, 'write') )
            error('Command "%s" is not valid.  Please use "validate", "read", or "write"', varargin{1}) 
        end
    
        % Check that the filename is a character string
        if ( ~ischar( varargin{2} ) )
            error('Filename is not valid') 
        end

        % Check the filename exists if the command is "read" or "validate"
        if ( strcmp(varargin{1}, 'validate') | strcmp(varargin{1}, 'read') )
            if ( ~exist( varargin{2} ) )
                error('Filename "%s" does not exist', varargin{2}) 
            end
        end

        % Check that "write" command only has three arguments
        if ( (nargin == 2) & strcmp(varargin{1}, 'write') )
            error('Not enough arguments are provided to "write" function call (need 3, provided 2)');
        end
        
        % Check for optional array of nodes
        if ( (nargin == 3) )         
            if ( ~strcmp(varargin{1}, 'write') )
                error('Too many arguments are provided to "read" or "validate" function call');    
            end
            
            if ( ~strcmp(class(varargin{3}),'wl_node') )
                error('Argument must be an array of "wl_node".  Provided "%s"', class(varargin{3}));
            end
        end
        
        % Default for other arguments
        if ( (nargin > 3) )
            error('Too many arguments are provided to function call');    
        end
    end

    % Process the command

    fileID = fopen(varargin{2});

    switch( varargin{1} ) 
        case 'validate'
            wl_nodesReadAndValidate( fileID );
            nodesInfo = 1;
        case 'read'
            nodesInfo = wl_nodesReadAndValidate( fileID );
        case 'write'
            error('TODO:  The "write" command for wl_nodesConfig() has not been implemented yet.');    
        otherwise
            error('unknown command ''%s''',cmdStr);
    end
 
end    
    

function nodeInfo = wl_nodesReadAndValidate( fid )

    % Parse the input file    
    inputParsing    = textscan(fid,'%s', 'CollectOutput', 1, 'Delimiter', '\n\r', 'MultipleDelimsAsOne', 1);
    inputArrayTemp  = inputParsing{:};

    % Remove any commented lines
    input_index = 0;
    inputArray  = cell(1, 1);
    for n = 1:size(inputArrayTemp)
        if ( isempty( regexp(inputArrayTemp{n}, '#', 'start' ) ) )
            input_index = input_index + 1;
            inputArray{input_index, 1} = inputArrayTemp{n};
        end
    end
    
    % Convert cells to character arrays and break apart on ',' or '\t'
    removeTabs  = regexprep( inputArray, '[\t]', '' );
    fieldsArray = regexp( removeTabs, '[,]', 'split' );

    % Remove spaces from the header row    
    headerRow   = regexprep( fieldsArray{1}, '[ ]', '' );
    
    % Determine size of the matrices
    headerSize = size(headerRow);        % Number of Columns
    fieldSize  = size(fieldsArray);      % Number of Rows (Nodes)

    % Pad columns in cell array 
    % Create structure from cell array
    for n = 1:(fieldSize(1) - 1)
        rowSize = size( fieldsArray{n+1} );
        if ( rowSize(2) > headerSize(2) )
            error('Node %d has too many columns.  Header specifies %d, provided %d', n, headerSize(2), rowSize(2))  
        end
        if ( rowSize(2) < headerSize(2) )
            for m = (rowSize(2) + 1):headerSize(2)
                fieldsArray{n+1}{m} = '';
            end
        end
        nodesStruct(n) = cell2struct( fieldsArray{n + 1}, headerRow, 2);
    end
    
    % Determine if array has correct required fields
    if ( ~isfield(nodesStruct, 'serialNumber') | ~isfield(nodesStruct, 'ID') | ~isfield(nodesStruct, 'ipAddress') )
        error('Does not have required fields: "serialNumber", "ID", and "ipAddress" ') 
    end         

    % Convert to unit32 for serialNumber, ID, and ipAddress
    nodeSize = size(nodesStruct);
        
    for n = 1:(nodeSize(2))
        inputString = [nodesStruct(n).serialNumber,0]; %null-terminate
        padLength = mod(-length(inputString),4);
        inputByte = [uint8(inputString),zeros(1,padLength)];
        nodesStruct(n).serialNumberUint32 = typecast(inputByte,'uint32');

        nodesStruct(n).IDUint32 = uint32( str2num( nodesStruct(n).ID ) );

        addrChars = sscanf(nodesStruct(n).ipAddress,'%d.%d.%d.%d');

        if ( ~eq( length( addrChars ), 4 ) )
            error('IP address of node must have form "w.x.y.z".  Provided "%s" ', nodesStruct(n).ipAddress)         
        end

        if ( (addrChars(4) == 0) | (addrChars(4) == 255) )
            error('IP address of node %d cannot end in .0 or .255', n)         
        end
        nodesStruct(n).ipAddressUint32 =  uint32( 2^0 * addrChars(4) + 2^8 * addrChars(3) + 2^16 * addrChars(2) + 2^24 * addrChars(1) );
    end    
    
    % Determine if each input row is valid
    %   - All serial numbers must be unique
    %   - All UIDs must be unique
    %   - All IP Addresses must be unique
    
    for n = 1:(nodeSize(2) - 1)
        for m = n+1:(nodeSize(2))
            if ( strcmp( nodesStruct(n).serialNumber, nodesStruct(m).serialNumber ) )
                error('Serial Number must be unique.  Nodes %d and %d have the same serial number %s', n, m, nodesStruct(n).serialNumber)
            end
            if ( strcmp( nodesStruct(n).ID, nodesStruct(m).IDUint32 ) )
                error('Node ID must be unique.  Nodes %d and %d have the same node ID %d', n, m, nodesStruct(n).ID)
            end
            if (nodesStruct(n).ipAddressUint32 == nodesStruct(m).ipAddressUint32)
                error('IP Address must be unique.  Nodes %d and %d have the same IP Address %s', n, m, nodesStruct(n).ipAddress)
            end    
        end
    end

    % Uncomment Section to Display final node contents       
    % for n = 1:(nodeSize(2))
    %     disp(nodesStruct(n))
    % end
    
    nodeInfo = nodesStruct;    
end






