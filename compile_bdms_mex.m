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

%define library files
library_dir = fullfile(current_dir, 'lib');

% Determine the correct library file names based on the platform
if ispc % Windows
    library_files = {fullfile(library_dir, 'libcrypto_static.lib'), ...
                     fullfile(library_dir, 'libssl_static.lib'), ...
                     fullfile(library_dir, 'zlib.lib')};
else % Unix (Linux and macOS)
    library_files = {fullfile(library_dir, 'libcrypto.a'), ...
                     fullfile(library_dir, 'libssl.a'), ...
                     fullfile(library_dir, 'libz.a')};
end

% Define source files
source_files = {'bdms_mex.cpp', 'bdms_wrapper.hpp'};

% Compile the MEX file
mex('-R2017b', include_flags{:}, '-output', 'bdms_mex', library_files{:}, source_files{:});

disp('Compilation completed.');
