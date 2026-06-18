classdef PowerFlow < handle
    % POWERFLOW A library for power flow computations. 
    % It can:
    % - Estimate state in radial and non-radial networks.
    % - Compute gradients in radial networks analytically.
    % - Estimate cable parameters in radial networks.
    % 
    % All non-integer numerical input/output values are complex unless stated otherwise.

    properties (SetAccess = private, Hidden = true)
        % Stores the network handle ID to simplify PowerFlowMex calls, e.g. PowerFlowMex("solve", S, V) 
        % becomes power_flow.solve(S, V).
        networkHandle;
        loaded = false;
    end
    methods
        function this = PowerFlow(networkFilePath, settings)
            % Constructor.
            % Input Arguments:
            % - networkFilePath (string) -
            %   Path to network file.
            % - settings (struct) -
            %   Settings struct. Possible settings include:
            %   - settings.max_iterations_total (integer) - Max number of
            %   iterations for the entire network. Default 10000.
            %   - settings.max_iterations_gauss (integer) - Max number of
            %   iterations for Gauss-Seidel solver. Default 10000.
            %   - settings.gauss_seidel_precision (double) - Max power
            %   mismatch for Gauss-Seidel solver convergence. Default
            %   1e-10.
            %   - settings.max_iterations_bfs (integer) - Max number of
            %   iterations for BFS solver. Defaul 10000.
            %   - settings.bfs_precision (double) - Max power mismatch for
            %   BFS solver convergence. Default 1e-10.
            %   - settings.max_iterations_zbusjacobi - Max number of
            %   iterations for ZBus Jacobi solver. Default 10000.
            %   - settings.zbusjacobi_precision - Max power mismatch for
            %   ZBus Jacobi solver convergence. Default 1e-10.
            %   - settings.compute_gradients (true/false) - Whether to
            %   compute gradients in BFS solver. Default false.
            %   - settings.max_iterations_ols (integer) - Max iterations
            %   for OLS parameter estimation. Default 20.
            %   - settings.ols_precision (double) - Max change in
            %   parameters between iterations for OLS parameter estimation
            %   convergence. Default 1e-3.
            %
            % See also:
            %   PowerFlow

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
            % SOLVE Estimates state in network given power consumptions S and slack voltages V.
            %
            % See also:
            %   PowerFlow

            PowerFlowMex("solve", this.networkHandle, S, V);
        end

        function solveParams(this, keysV, valsV, precision)
            % SOLVEPARAMS NOTE: This method is not recommended, use solveParamsOLS instead. Detects invalid cable parameters in network and estimates them if possible.
            %    
            % Input arguments:
            % - keysV (array) - Array of LOAD node indexes (1 x number of LOAD nodes).
            % - valsV (array) - Array of complex voltages corresponding to LOAD nodes in keysV (1 x number of LOAD nodes).
            % - precision (double) - Maximum change in parameters between iterations for convergence.
            %
            % This method will attempt to detect invalid cable parameters using a majority vote method. 
            % If an invalid cable parameter is detected, a message will be printed, and if possible the
            % function will estimate the true parameter.
            %
            % See also:
            %   PowerFlow, solveParamsOLS

            PowerFlowMex("solveParams", this.networkHandle, keysV, valsV, precision);
        end

        function [Z] = solveParamsOLS(this, keys, voltages, powerInjections, slackVoltages)
            % SOLVEPARAMSOLS Estimates cable parameters in network using OLS regression.
            % Input arguments:
            % - keys (array) - Array of LOAD node indexes (1 x number of LOAD nodes).
            % - voltages (table) - Table of LOAD node voltages (number of LOAD nodes x number of samples).
            % - powerInjections (table) - Table of LOAD powers (number of LOAD nodes x number of samples).
            % - slackVoltages (array) - Array of SLACK voltages (1 x number of samples).
            %
            % Output arguments:
            % - Z (array) - Array of estimated cable parameters (1 x number of branches)
            %
            % See also:
            %   PowerFlow

            Z = PowerFlowMex("solveParamsOLS", this.networkHandle, keys, voltages, powerInjections, slackVoltages);
        end

        function [V] = getLoadVoltages(this)
            % GETLOADVOLTAGES Returns array of LOAD node voltages.
            %
            % See also:
            %   PowerFlow

            V = PowerFlowMex("getLoadVoltages", this.networkHandle);
        end

        function [V] = getAllVoltages(this)
            % GETALLVOLTAGES Returns array of all voltages in the network.
            %
            % See also:
            %   PowerFlow

            V = PowerFlowMex("getAllVoltages", this.networkHandle);
        end

        function [I] = getCurrents(this)
            % GETCURRENTS Returns array of all currents in the network.
            %
            % See also:
            %   PowerFlow

            I = PowerFlowMex("getCurrents", this.networkHandle);
        end

        function [S] = getSlackPowers(this)
            % GETSLACKPOWERS Returns array of SLACK node powers.
            %
            % See also:
            %   PowerFlow

            S = PowerFlowMex("getSlackPowers", this.networkHandle);
        end

        function [Z] = getImpedances(this)
            % GETIMPEDANCES Returns array of all impedances in the network.
            %
            % See also:
            %   PowerFlow

            Z = PowerFlowMex("getImpedances", this.networkHandle);
        end

        function [dvds] = getDvDs(this)
            % GETDVDS Returns table of dV/dS gradients.
            % The voltage of every non-SLACK node with respect to LOAD
            % powers
            %
            % See also:
            %   PowerFlow

            dvds = PowerFlowMex("getDvDs", this.networkHandle);
        end

        function [dids] = getDiDs(this)
            % GETDIDS Returns table of dI/dS gradients.
            % The current of each cable with respect to LOAD powers.
            %
            % See also:
            %   PowerFlow

            dids = PowerFlowMex("getDiDs", this.networkHandle);
        end

        function [dsds] = getDsDs(this)
            % GETDSDS Returns table of dS/dS gradients.
            % The power of every non-LOAD node with respect to LOAD powers.
            %
            % See also:
            %   PowerFlow

            dsds = PowerFlowMex("getDsDs", this.networkHandle);
        end

        function [dslossds] = getDslossDs(this)
            % GETDSLOSSDS Returns table of dSloss/dS gradients.
            % The power loss of each cable with respect to LOAD powers.
            %
            % See also:
            %   PowerFlow

            dslossds = PowerFlowMex("getDslossDs", this.networkHandle);
        end

        function reset(this)
            % RESET Resets the state of the network, setting all voltages to 1, and all powers to 0.
            %
            % See also:
            %   PowerFlow

            PowerFlowMex("reset", this.networkHandle);
        end
    end
end
