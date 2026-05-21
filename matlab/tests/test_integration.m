function testIntegration()
    %{
        An integration test to verify that the MATLAB bindings correctly interface
        with the PowerFlowMex implementation and that the reset()
        command properly clears the solver state.
    %} 

    settings = struct();
    settings.max_iterations_total = 1000;
    settings.compute_gradients = true;

    network_path = string(fullfile(pwd, 'examples', 'test_networks', 'test_network.txt'));
    if exist(network_path, 'file') ~= 2
        fprintf('Skipping test: %s not found.\n', network_path);
        return;
    end

    power_flow = PowerFlow(network_path, settings);

    % Define complex power (S) and a reference voltage (V)
    S = [complex(-0.004, -0.002), complex(-0.002, -0.001), complex(-0.005, -0.004)];
    V = [complex(1.0, 0.0)];

    power_flow.solve(S, V);
    verifySolveState(power_flow);
    verifyCableParamsMajority(power_flow);
    verifyCableParamsRegression();
    verifyGradientsTypes(power_flow);

    power_flow.reset();
    verifyResetState(power_flow);
    verifyGradientsReset(power_flow);

    fprintf('MATLAB binding test passed!\n'); 
end  

function checkListTypes(data, name, expected_len) 
    %{ 
        Verify that data is an array of numeric values with the expected length. 
    %} 
    
    % Make sure it is indeed a numeric array.
    assert(isnumeric(data) || iscell(data), '%s should be an array, got %s.', name, class(data));

    % Make sure it is not empty. 
    assert(~isempty(data), '%s list is empty.', name);

    % Make sure our expected length exists and that the list is the expected length.
    if nargin >= 3 && ~isempty(expected_len)
        assert(length(data) == expected_len, '%s length mismatch: expected %d, got %d.', name, expected_len, length(data));
    end

    % Make sure the items in the list are numeric.
    for i = 1:length(data)
        if iscell(data)
            val = data{i};
        else
            val = data(i);
        end
        assert(isnumeric(val), 'Element %d in %s is %s, not a numeric value.', i, name, class(val));
    end
end

function verifySolveState(power_flow)
    %{
        Verify that all the getters return arrays with numbers.
    %}

    fprintf('Verifying solved state types and content...\n');
    checkListTypes(power_flow.getAllVoltages(), 'all_voltages', 10);
    checkListTypes(power_flow.getLoadVoltages(), 'load_voltages', 3);
    checkListTypes(power_flow.getSlackPowers(), 'slack_powers', 3);
    checkListTypes(power_flow.getCurrents(), 'currents', 7);
end

function verifyGradientsTypes(power_flow)
    %{
        Verify that the gradient getters return 3D arrays with [dPre, dQim] pairs.
    %}

    fprintf('Verifying gradient types...\n');

    grads = {power_flow.getDiDs(), 'DiDs'; ...
             power_flow.getDvDs(), 'DvDs'; ...
             power_flow.getDsDs(), 'DsDs'; ...
             power_flow.getDslossDs(), 'DslossDs'};

    for k = 1:size(grads, 1)
        grad_array = grads{k, 1};
        name = grads{k, 2};

        assert(isnumeric(grad_array), '%s should be a numeric array, got %s.', name, class(grad_array));
        
        dims = size(grad_array);
        assert(length(dims) == 3, '%s should be a 3D array, got %d dimensions.', name, length(dims));
        assert(dims(3) == 2, 'Third dimension of %s should be 2, got %d.', name, dims(3));
    end
end

function verifyResetState(power_flow)
    %{
        Verify that all getters are correct after a reset.
    %}

    fprintf('Verifying reset state values...\n');

    voltages = power_flow.getAllVoltages();
    load_voltages = power_flow.getLoadVoltages();
    currents = power_flow.getCurrents();
    slack_powers = power_flow.getSlackPowers();

    % Check all containers again, so they didn't change type.
    checkListTypes(voltages, 'reset_all_voltages', 10);
    checkListTypes(load_voltages, 'reset_load_voltages', 3);
    checkListTypes(currents, 'reset_currents', 7);
    checkListTypes(slack_powers, 'reset_slack_powers', 3);

    % Make sure the voltages are indeed reset. 
    % Concatenate using (:) to ensure column vectors, preventing dimension mismatch.
    all_voltages = [voltages(:); load_voltages(:)];
    for i = 1:length(all_voltages)
        v = all_voltages(i);
        assert(v == complex(1.0, 0.0), 'Voltage %f+%fi did not reset to 1.0+0j.', real(v), imag(v));
    end

    % Make sure the powers and currents are indeed reset. 
    all_currents_powers = [currents(:); slack_powers(:)];
    for i = 1:length(all_currents_powers)
        x = all_currents_powers(i);
        assert(x == complex(0.0, 0.0), 'Value %f+%fi did not reset to 0.0+0j.', real(x), imag(x));
    end
end

function [keys, vals] = getAbsoluteVoltages(power_flow)
    %{ 
        Get the absolute values of the voltages. 
    %} 
    
    voltages = power_flow.getLoadVoltages();
    vals = complex(abs(voltages)); % Ensure it is complex double for MEX
    keys = int32(0:length(voltages)-1); % node_idx_t is int (int32)
end 

function verifyCableParamsMajority(power_flow)
    %{ 
        Verify that the cable parameters are correct after a solve. 
    %} 

    fprintf('Verifying cable parameters majority...\n');
    
    params = power_flow.getImpedances();
    checkListTypes(params, 'impedances', 7);

    impedances_before = power_flow.getImpedances();
    
    [keys, vals] = getAbsoluteVoltages(power_flow);
    power_flow.solveParams(keys, vals, 1e-4);
    
    checkListTypes(power_flow.getImpedances(), 'impedances', 7);
    
    impedances_after = power_flow.getImpedances();
    
    for i = 1:length(impedances_before)
        assert(impedances_before(i) == impedances_after(i), ...
            'Impedances changed after solveParams, when it should not have. %f+%fi != %f+%fi', ...
            real(impedances_before(i)), imag(impedances_before(i)), ...
            real(impedances_after(i)), imag(impedances_after(i)));
    end
end 

function verifyCableParamsRegression()
    %{ 
        Verify that the cable parameters are correct after a solve using regression. 
    %} 

    fprintf('Verifying cable parameters regression...\n');
    
    [keys, U, S_load, slack_V] = regression_data();
    
    settings = struct();
    ref_network_path = string(fullfile(pwd, 'examples', 'net_large_ref.txt'));
    fault_network_path = string(fullfile(pwd, 'examples', 'net_large_test.txt'));
    power_flow_ref = PowerFlow(ref_network_path, settings);
    power_flow_fault = PowerFlow(fault_network_path, settings);
    
    checkListTypes(power_flow_ref.getImpedances(), 'impedances');
    checkListTypes(power_flow_fault.getImpedances(), 'impedances');

    ref_impedances = power_flow_ref.getImpedances();
    
    power_flow_fault.solveParamsReg(keys, U, S_load, slack_V, 3e-4, 20);
    
    checkListTypes(power_flow_fault.getImpedances(), 'impedances');
    
    estimated_impedances = power_flow_fault.getImpedances();
    
    for i = 1:length(ref_impedances)
        err = abs(ref_impedances(i) - estimated_impedances(i)) / abs(ref_impedances(i));
        assert(err <= 1.1e-2, ...
            'Impedance %d is incorrect after parameter regression. Error %f', i, err);
    end
end

function verifyGradientsReset(power_flow)
    %{
        Verify that the gradient getters return zero values after a reset.
    %}

    fprintf('Verifying reset gradient values...\n');

    grads = {power_flow.getDiDs(), 'DiDs'; ...
             power_flow.getDvDs(), 'DvDs'; ...
             power_flow.getDsDs(), 'DsDs'; ...
             power_flow.getDslossDs(), 'DslossDs'};

    for k = 1:size(grads, 1)
        grad_array = grads{k, 1};
        name = grads{k, 2};

        assert(all(grad_array(:) == 0), '%s did not reset to 0.', name);
    end
end

% Main function that will be ran when the MATLAB script is called.
testIntegration();