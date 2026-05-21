classdef PowerFlow < handle
    properties (SetAccess = private, Hidden = true)
        % Stores the network handle ID to simplify PowerFlowMex calls, e.g. PowerFlowMex("solve", S, V) 
        % becomes power_flow.solve(S, V).
        networkHandle;
        loaded = false;
    end
    methods
        function this = PowerFlow(networkFilePath, settings)
            if isempty(settings)
                this.networkHandle = PowerFlowMex("load", networkFilePath);
            else
                this.networkHandle = PowerFlowMex("load", networkFilePath, settings);
            end
            this.loaded = true;
        end

        function delete(this)
            if this.loaded
                PowerFlowMex("unload", this.networkHandle);
                this.loaded = false;
            end
        end

        function solve(this, S, V)
            PowerFlowMex("solve", this.networkHandle, S, V);
        end

        function solveParams(this, keysV, valsV, precision)
            PowerFlowMex("solveParams", this.networkHandle, keysV, valsV, precision);
        end

        function solveParamsReg(this, keys, voltages, powerInjections, slackVoltages, convergenceThreshold, maxIterations)
            PowerFlowMex("solveParamsReg", this.networkHandle, keys, voltages, powerInjections, slackVoltages, convergenceThreshold, maxIterations);
        end

        function [V] = getLoadVoltages(this)
            V = PowerFlowMex("getLoadVoltages", this.networkHandle);
        end

        function [V] = getAllVoltages(this)
            V = PowerFlowMex("getAllVoltages", this.networkHandle);
        end

        function [I] = getCurrents(this)
            I = PowerFlowMex("getCurrents", this.networkHandle);
        end

        function [S] = getSlackPowers(this)
            S = PowerFlowMex("getSlackPowers", this.networkHandle);
        end

        function [Z] = getImpedances(this)
            Z = PowerFlowMex("getImpedances", this.networkHandle);
        end

        function [dvds] = getDvDs(this)
            dvds = PowerFlowMex("getDvDs", this.networkHandle);
        end

        function [dids] = getDiDs(this)
            dids = PowerFlowMex("getDiDs", this.networkHandle);
        end

        function [dsds] = getDsDs(this)
            dsds = PowerFlowMex("getDsDs", this.networkHandle);
        end

        function [dslossds] = getDslossDs(this)
            dslossds = PowerFlowMex("getDslossDs", this.networkHandle);
        end

        function reset(this)
            PowerFlowMex("reset", this.networkHandle);
        end
    end
end
