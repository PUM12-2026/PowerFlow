clear

fprintf('Loading grid data... \n'); 

% Directory where the grid data file is located
grid_directory = '+data/'; 
%grid_directory = '';

% Grid data file to load
%grid_file      = 'S433_Ekangen_Validation';
grid_file      = 'S495_Linghem_Validation'; 



% Load grid data
grid_data = load([grid_directory, grid_file]);

% Open file
fd = fopen("graph.txt", "w");


% Now we have the whole structure and can generate the network:

%put out "grid"

fprintf(fd, "grid\n");

% for pair in grid_data.connectionBuses
    % put out the pair and get the Z_ser[i,j] ( or Y_shy[i.j] ? )

% Get all the bus connections from the grid data
connections = grid_data.connectionBuses; 

% Iterate through all connections in the network
for k = 1:size(connections,1)
    % Hämtar bussindex för anslutningens start- och slutnod
    i = connections(k,1);
    j = connections(k,2);

    % Get the series impedance between bus i and j
    z = grid_data.Z_ser(i,j); 

    % Write connection and impedance to file
    % We have to do - 1 since matlab is 1 indexed :(
    fprintf(fd,"%d %d (%f, %f)\n", i - 1, j - 1, real(z), imag(z));
end
% put %
fprintf(fd, "%%\n");

% get all index where gird_data.busisLoad == 1 
    % put them as load
is_load = grid_data.busIsLoad;

% Write indices of load buses
for k = 1:size(is_load,1)
    if is_load(k)
        % We have to do - 1 since matlab is 1 indexed :(
        fprintf(fd,"%d\n", k-1);
    end
end

    
% put %

fprintf(fd, "%%\n");

% put connections
fprintf(fd, "connections\n");

% put %
fprintf(fd, "%%");
fprintf("Graph converted! \n");

fclose(fd);

fprintf("Gathering variables...\n")

% Open file for eriting system variables
fd = fopen("variables.txt", "w");

% Extract bus voltage and power matrices
U_bus = grid_data.U_bus;
S_bus = grid_data.S_bus;

% Write transformer base quantities
fprintf(fd, "SBase: %f\n", grid_data.TransformerData.S_base);
fprintf(fd, "UBase ( prim, sec): %f %f\n", grid_data.TransformerData.U_prim_base, grid_data.TransformerData.U_sec_base);
fprintf(fd, "ZBase ( prim, sec): %f %f\n", grid_data.TransformerData.Z_prim_base, grid_data.TransformerData.Z_sec_base);
fprintf(fd, "IBase ( prim, sec): %f %f\n", grid_data.TransformerData.I_prim_base, grid_data.TransformerData.I_sec_base);

% Write bus voltage matrix
fprintf(fd, "U_bus\n");
for n = 1:size(U_bus,1)
    for m = 1:size(U_bus,2)
        fprintf(fd,"%f ",U_bus(n,m));
    end
    fprintf(fd,"\n");
end

% Write bus power matrix
fprintf(fd, "S_bus\n");
for n = 1:size(S_bus,1)
    for m = 1:size(S_bus,2)
        fprintf(fd,"%f ",S_bus(n,m));
    end
    fprintf(fd,"\n");
end

fprintf(fd, "Variables gathered!");

fclose(fd);