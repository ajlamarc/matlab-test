% Compile script for bdms_mex
script_dir = fileparts(mfilename('fullpath'));

islinux = isunix && ~ismac;

% Define the include paths
include_dir = fullfile(script_dir, 'bdms2-cpp-library', 'include');
include_paths = {script_dir, ...
                     include_dir, ...
                     fullfile(include_dir, 'nlohmann'), ...
                     fullfile(include_dir, 'openssl'), ...
                     fullfile(include_dir, 'crypto')};

% Create the include flags
include_flags = cellfun(@(x) ['-I"' x '"'], include_paths, 'UniformOutput', false);

%define library files
base_library_dir = fullfile(script_dir, 'lib');

if ismac
    library_dir = fullfile(base_library_dir, 'mac');
elseif islinux
    library_dir = fullfile(base_library_dir, 'linux');
elseif ispc
    library_dir = fullfile(base_library_dir, 'win');
else
    error('Platform not supported')
end

% Determine the correct library file names based on the platform
if ispc % Windows
    library_files = {fullfile(library_dir, 'libssl_static.lib'), ...
                         fullfile(library_dir, 'libcrypto_static.lib'), ...
                         fullfile(library_dir, 'zlib.lib')};
else % Unix (Linux and macOS)
    library_files = {fullfile(library_dir, 'libssl.a'), ...
                         fullfile(library_dir, 'libcrypto.a'), ...
                         fullfile(library_dir, 'libz.a')};

    if islinux
        library_files{end + 1} = '-ldl';
    end

end

% Define source files
source_files = {fullfile(script_dir, 'bdms_mex.cpp')};

% Compile the MEX file
if ~islinux
    mex('-R2017b', include_flags{:}, '-output', 'bdms_mex', library_files{:}, source_files{:}, '-largeArrayDims');
else
    mex('GCC="/usr/bin/gcc-4.9"', include_flags{:}, '-output', 'bdms_mex', library_files{:}, source_files{:}, '-largeArrayDims');
end

disp('Compilation completed.');
