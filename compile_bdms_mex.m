% Compile script for bdms_mex

% Get the current directory
current_dir = pwd;

% Define the include paths
include_dir = fullfile(current_dir, 'include');
include_paths = {include_dir, ...
                     fullfile(include_dir, 'nlohmann'), ...
                     fullfile(include_dir, 'openssl'), ...
                     fullfile(include_dir, 'crypto')};

% Create the include flags
include_flags = cellfun(@(x) ['-I"' x '"'], include_paths, 'UniformOutput', false);

% Define source files
source_files = {'bdms_mex.c', 'bdms_wrapper.cpp'};

% Compile the MEX file
mex('-R2016b', include_flags{:}, '-output', 'bdms_mex', source_files{:});

disp('Compilation completed.');
