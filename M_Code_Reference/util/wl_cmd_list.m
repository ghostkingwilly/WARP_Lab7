function out = wl_cmd_list(class_name, varargin)
%WL_CMD_LIST WARPLab command lister - lists available commands and their associated
%help text.
% 
% Example:
% Input argument can be:
% String:
% wl_cmd_list('wl_node')
%  Prints all commands/help for 'wl_node' class
%
% Class:
% wl_cmd_list(wl_node)
%  Prints all commands/help for 'wl_node' class
%
% Instance:
% n = wl_node;
% wl_cmd_list(n)
%  Prints all commands/help for 'wl_node' class
%
% wl_cmd_list(n.baseband)
%  Prints all commands/help for the node's baseband object

%Use the first element if the user passes in a cell 
% Handles the case of wl_cmd_list(node.interfaceGroups)
if(iscell(class_name))
    class_name = class_name{1};
end
    
if(ischar(class_name))
    %wl_cmd_list('wl_node')
    wl_file = which(class_name);
elseif(isobject(class_name))
    %wl_cmd_list(wl_node)
    % OR
    %node=wl_node;
    %wl_cmd_list(node)
    %wl_cmd_list(node.baseband)
    wl_file = which(class(class_name));
else
    fprintf('\nError: input argument must be string or object');
end

if(~isempty(varargin) && strcmp(varargin{1}, 'wiki'))
    do_wiki = 1;
else
    do_wiki = 0;
end

if(isempty(wl_file))
    fprintf('\nError: class %s not found\n', class_name);
    return;
end

CR = char(10);
TAB = char(9);
SEP = '--------------------';

procCmd_line = 0;
fid = fopen(wl_file);

lnum = 0;
res = [];

%Find the 'function out=procCmd' section of the class
while (procCmd_line == 0)
    line = fgetl(fid);
    lnum = lnum + 1;
    if(line == -1)
        %EOF - punt
        fprintf('\nError: class %s does not contain a procCmd method\n', class_name);
        return;
    end

    procCmd_ind = regexp(line, '^[ \t]*function.*procCmd', 'once');
    if(~isempty(procCmd_ind))
        %Found the procCmd function
        procCmd_line = lnum;
    end
end

%Check for a case block for the requested command
% This loop is skipped if no procCmd function was found
while (procCmd_line > 0)
    line = fgetl(fid);
    lnum = lnum + 1;

    if(line == -1)
        %EOF - punt
        break;
    end
        
    %Check if we've reached the end of the procCmd switch/case
%    endcase_ind = regexp(line, '^[ \t]*otherwise', 'once');
    endcase_ind = regexp(line, 'error(''unknown command', 'once');
    if(~isempty(endcase_ind))
        %End of procCmd; punt
        break;
    end
        
    %Check if this has a "case 'some_cmd'" string
     match = regexpi(line,'^[ \t]*case[ \t]+''(?<cmd_str>\S+)''','names');
    
    if(~isempty(match))
        if(~do_wiki)
            %Found a command; retrieve all following lines until a non-comment is found
            res = [res CR match.cmd_str ':' CR];
            while 1
                line = fgetl(fid);
                txt_ind = regexp(line, '^[ \t]*%', 'end', 'once');
                if(~isempty(txt_ind))
                    res = [res CR TAB line(txt_ind+1:end)];
                else
                    res = [res CR CR SEP CR];
                    break;
                end
            end
        else
            res = [res CR '=== {{{' match.cmd_str '}}} ==='];
            while 1
                line = fgetl(fid);
                line = strrep(line, '[', '![');
                txt_ind = regexp(line, '^[ \t]*%\s*', 'end', 'once');
                args_ind = regexp(line, '^[ \t]*%\s* Arguments:', 'end', 'once');
                rets_ind = regexp(line, '^[ \t]*%\s* Returns:', 'end', 'once');
                buffsel_ind = regexp(line, '^[ \t]*%\s* Requires BUFF_SEL:', 'end', 'once');
                anytext_ind = regexp(line,'^[ \t]*%\s*\w+', 'end', 'once');
                
                if(~isempty(txt_ind))
                    %Got another documentation line
                    if(~isempty(args_ind))
                        res = [res CR CR '''''''Arguments:''''''' line(args_ind+1:end)];
                    elseif(~isempty(rets_ind))
                        res = [res CR CR '''''''Returns:''''''' line(rets_ind+1:end)];
                    elseif(~isempty(buffsel_ind))
                        res = [res CR CR '''''''Requires BUFF_SEL:''''''' line(buffsel_ind+1:end)];
                    elseif(~isempty(anytext_ind))
                        res = [res CR line(txt_ind+1:end) '[[BR]]'];
                    else
                        res = [res CR line(txt_ind+1:end)];
                    end
                else
                    res = [res CR CR];
                    break;
                end
            end
        end
    end
end

fclose(fid);

out = res;
