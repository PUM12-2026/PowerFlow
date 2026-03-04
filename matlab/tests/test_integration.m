function testIntegration()
    %{
        An integration test to verify that the MATLAB bindings correctly interface
        with the PowerFlowMex implementation and that the reset()
        command properly clears the solver state.
    %} 

    settings = struct();
    settings.max_iterations_total = 1000;

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

    power_flow.reset();
    verifyResetState(power_flow);

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

% Main function that will be ran when the MATLAB script is called.
testIntegration();