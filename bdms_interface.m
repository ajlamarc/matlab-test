%BDMS_INTERFACE bdms MATLAB class wrapper to an underlying C++ class
classdef bdms_interface < handle

    properties (SetAccess = private, Hidden = true)
        objectHandle; % Handle to the underlying C++ class instance
    end

    methods
        %% Constructor - Create a new C++ class instance
        function this = bdms_interface(varargin)
            this.objectHandle = bdms_mex('new', varargin{:});
        end

        %% Destructor - Destroy the C++ class instance
        function delete(this)
            bdms_mex('delete', this.objectHandle);
        end

        %% getArray - a bdms class method call
        function varargout = getArray(this, varargin)
            [varargout{1:nargout}] = bdms_mex('getArray', this.objectHandle, varargin{:});
        end

    end

end
