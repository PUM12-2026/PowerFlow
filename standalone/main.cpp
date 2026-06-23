#include <iostream>
#include <fstream>
#include <vector>

// Include PowerFlow headers.
#include <random>

#include "powerflow/NetworkLoader.hpp"
#include "powerflow/ParameterValidator.hpp"
#include "powerflow/PowerFlowSolver.hpp"
#include "powerflow/logger/CppLogger.hpp"
#include <unordered_map>

int main(int argc, char* argv[])
{
    // Open the network file using an ifstream.
    std::ifstream file("example_network_single_grid.txt");

    if (!file)
    {
        std::cerr << "Could not open network file" << std::endl;
        return -1;
    }

    // Create a NetworkLoader object and provide it with the file input stream.
    // Call the loadNetwork() method to load the network file into a Network
    // struct. In case of errors in the network file, the method will throw a
    // NetworkLoaderError.
    //
    // NOTE: A NetworkLoader object is only meant to be used once. To load
    // additional network files, it is recommended to create new NetworkLoaders
    // for each network file to be loaded.
    // 
    // NOTE: Using NetworkLoader to create a Network struct is not mandatory.
    // A Network struct could be created by some other method or even manually.
    NetworkLoader loader(file);
    std::unique_ptr<Network> net = loader.loadNetwork();

    // Create a PowerFlowSolver object and provide it with the network and a
    // logger. In this case, the CppLogger class is used. You can also
    // implement your own logger if it is needed in your environment.
    // 
    // IMPORTANT: From here on, the PowerFlowSolver owns the Network struct.
    // The Network struct can (should) not be modified by any other code at
    // this point!
    CppLogger logger(std::cout);
    SolverSettings settings{}; // Create a default settings object.
    settings.bfs_precision = 1e-12;
    PowerFlowSolver pfs(std::move(net), settings, &logger);

    // Create S and V vectors for the LOAD and SLACK_EXTERNAL nodes.
    std::vector<complex_t> S = {
        {0.002, 0.001},
        {0.005, 0.004},
        {0.004, 0.002}
    };
    std::vector<complex_t> V = { {1, 0} };

    // Run the solver by calling PowerFlowSolver::solve.
    pfs.solve(S, V);
    
    pfs.solve({
        {0.06, 0.004},
        {0.02, 0.001},
        {0.05, 0.002}
    }, V);

    // Get the resulting voltages at the LOAD nodes.
    std::vector<complex_t> loadVoltages = pfs.getLoadVoltages();

    // Print the calculated voltages to cout.
    for (const complex_t v : loadVoltages)
    {
        std::cout << "(" << v.real() << ", " << v.imag() << ")" << std::endl;
    }
    
    ////////////////////////////////////////
    // Cable parameter estimation example //
    ////////////////////////////////////////

    // Slack voltages as a time series
    std::vector<complex_t> slackVoltages = {{1.0036769293763048,    0.0}, {1.0058613111577857,    0.0}, {1.00440377207962,    0.0}, {1.003026555165954,    0.0}, {0.9981790646891471,    0.0}, {1.0062501835698505,    0.0}, {1.0108950255877984,    0.0}, {1.0031256143289742,    0.0}, {0.9990569103503918,    0.0}, {1.005836571826014,    0.0}, {1.0053401294884434,    0.0}, {1.001712811910133,    0.0}, {1.0052861503602335,    0.0}, {1.000340669354394,    0.0}, {1.0086115801862898,    0.0}, {0.9953299716405853,    0.0}, {0.9980622054454722,    0.0}, {0.9997082934131882,    0.0}, {1.0040601445644408,    0.0}, {1.00412495073671,    0.0}, {1.002752488939893,    0.0}, {1.0003285919687819,    0.0}, {1.008842180405071,    0.0}, {1.0056297024400174,    0.0}, {1.006847942222591,    0.0}, {0.9991148398767512,    0.0}, {1.0067312663434622,    0.0}, {0.9968000032459255,    0.0}, {1.0060844779449554,    0.0}, {0.9987656669056357,    0.0}, {0.9995331954206855,    0.0}, {1.0015539843574646,    0.0}, {1.0035793652213745,    0.0}, {1.0065529554120234,    0.0}, {1.003620196806951,    0.0}, {0.997944701582713,    0.0}, {1.0027969716837835,    0.0}, {1.0092037855749478,    0.0}, {0.9998960581814202,    0.0}, {1.0119201946277026,    0.0}};


    // Load node power consumptions as a time series
    std::vector<std::vector<complex_t>> measuredLoads = {
{{0.017004333276662495, 0.002008467513154429}, {0.01673943999123832, 0.0019982659069257905}, {0.019902505950452388, 0.002006888530830493}, {0.010046452217617605, 0.0020024207091312377}, {0.0011588677254619102, 0.0020017676236975847}, {0.0045926936957432375, 0.0019953579195468563}, {0.007749744735934958, 0.0020000406333829436}, {0.004065879588675499, 0.002000769085670948}, {0.01888565154539172, 0.00200662532002282}, {0.017006135594477598, 0.001994431613603514}, {0.0018794745778566626, 0.001999849901125928}, {0.018916215846292744, 0.0020013966292394977}, {0.004385836051362025, 0.0020063025252172275}, {0.013902181334846001, 0.0020035989771840456}, {0.007125680114009036, 0.0019949762290417334}, {0.002164950561963747, 0.0019981166908602725}, {0.0230177391555053, 0.0020039951212259067}, {0.0010699826144327292, 0.0020032190564315444}, {0.009710648357416932, 0.0019965664961753174}, {0.008299147633401893, 0.002001809627290301}, {0.006281162300499912, 0.001999095392280533}, {0.03148647284683178, 0.0020049691466305403}, {0.003949920059517912, 0.002001152939555944}, {0.012244787354388961, 0.0020092652875978267}, {0.010084826481168997, 0.001993240529909215}, {0.001599815979158592, 0.0019967872872909192}, {0.012541906479421785, 0.0019950532655055074}, {0.0061873365820009595, 0.0020015695337395733}, {0.01862668736170246, 0.0020053487394936756}, {0.03132138294641879, 0.002000284359307871}, {0.009092327714378803, 0.002002396297759153}, {0.01401731858762438, 0.0020042027973092252}, {0.012310475769831944, 0.0019996209411713957}, {0.020387760141149368, 0.0020181823958767353}, {0.025332333138301796, 0.001991017659786524}, {0.00958660267025647, 0.002003100416928351}, {0.005208666860733832, 0.002000621713107038}, {0.016152202403591016, 0.0020024761626867343}, {0.010115092832510168, 0.001998685184185595}, {0.017379714808964365, 0.0020004284728707723}},
{{0.0030942443141238774, 0.0019967104893458387}, {0.025615152590718275, 0.001992724171591509}, {0.005130019211369299, 0.0019912964014494237}, {0.01076657269020514, 0.002000676144610659}, {0.014219680969408354, 0.002004708090896203}, {0.013649341988955267, 0.001992536277909136}, {0.012461611689930513, 0.0020061886496836475}, {0.009319505602060314, 0.0020009738869377932}, {0.0020691261352891715, 0.0019982346480571576}, {0.015276288837480558, 0.0019969991299693094}, {0.024383393268820934, 0.0019934825650410595}, {0.005938776826772771, 0.002004343229781597}, {0.0005536822510101604, 0.001999808202860758}, {0.008982550514498765, 0.0020079804332900236}, {0.0033731699190027928, 0.0019988272556408365}, {0.010856851415301703, 0.001989996164459239}, {0.021660005023638816, 0.0019933239385324128}, {0.020829602911357197, 0.002006371563001798}, {0.004989455732175692, 0.0019979625216648344}, {0.007449053585806159, 0.0020089450129703655}, {0.0019862132968477605, 0.001992516631950851}, {0.018670699304212217, 0.0020003355809607775}, {0.011976186572732134, 0.001991615449549321}, {0.02172450965547089, 0.0020034916067485065}, {0.0001514164525285057, 0.00199921779242891}, {0.0039944045461000974, 0.0019988576913533024}, {0.010075306499874417, 0.0019991848671341524}, {7.546433034481999e-05, 0.0019941008580086016}, {0.004997632121330752, 0.0020032479588372983}, {0.02767244197746932, 0.0019903182426516948}, {0.005039329246713405, 0.001993088395179434}, {0.025948596456042212, 0.0019940364230840286}, {0.009322292129846832, 0.0020011375620864642}, {0.015116751072980652, 0.001999975489542718}, {0.004535301933643985, 0.0020023539227671428}, {0.0017231780164433004, 0.0019946180285390323}, {0.01505189850862246, 0.0019931292497122445}, {0.0002409056622984275, 0.002000909948950439}, {0.019855880388499024, 0.0019958680217364412}, {0.017921044997297443, 0.002003207408298176}},
{{0.0021813922430735117, 0.002005444509322861}, {0.008025788466322105, 0.0019905673016378782}, {0.002402909696609532, 0.0020060388487016575}, {0.008325137959587772, 0.0020124367699464233}, {0.0005796153233567167, 0.0020020745390044647}, {0.0017786143711016333, 0.0020079796053989202}, {0.017135035243811315, 0.0020035479282720157}, {0.00913554484729801, 0.002003318911719075}, {0.013549282369206984, 0.0020028698278322776}, {0.012298602403407391, 0.0019929892678898263}, {0.016900896498697658, 0.0020076564455596383}, {0.0029069920116638684, 0.001993890325761956}, {0.012587032178322091, 0.001995457494289085}, {0.014577899169582546, 0.001995925807997166}, {0.014036103918419127, 0.0019952976262473353}, {0.01052632430746778, 0.0020003870526880247}, {0.011636144281884735, 0.0019993049252500627}, {0.003073504625221337, 0.002002832602739752}, {0.018701923709893882, 0.001998752236564811}, {0.019255211029962305, 0.0020032195986977542}, {0.01729180811614285, 0.001997440124209109}, {0.004322702425991346, 0.0019985366614574543}, {0.014429582638712583, 0.0019945808362842724}, {0.010923193654233894, 0.002003304749677967}, {0.004754363966167518, 0.0020043799703828155}, {0.014513465461036533, 0.0019994671666841777}, {0.003394177564994088, 0.0020055681366189255}, {0.014299558580682326, 0.002000029326805057}, {0.009447728178697838, 0.0020026065095247536}, {0.008800774845591275, 0.0020026421031436445}, {0.0016617505569711793, 0.0020056836625682705}, {0.009613134636611223, 0.001996914570185462}, {0.008656696842419043, 0.001999130339110404}, {0.005067229354220323, 0.001991549187675689}, {0.014745521144218097, 0.0020010297705653746}, {0.013981416954874508, 0.0020081511983664414}, {0.0018699834719546303, 0.002006880349211668}, {0.00458511505246023, 0.0019998465422513736}, {0.014872669147657159, 0.0019962484446927053}, {0.0027856718873189162, 0.001994350828068316}}
};

    // Load node voltages as a time series
    std::vector<std::vector<complex_t>> measuredLoadVoltages = {{}, {}, {}};
    
    // Load network. Nominal impdeances are not considered by this method    
    std::ifstream file2("net_ols_small.txt");
    if (!file2)
    {
        std::cerr << "Could not open network file 2" << std::endl;
        return -1;
    }
    
    NetworkLoader loader2(file2);
    std::unique_ptr<Network> net2 = loader2.loadNetwork();
    
    SolverSettings settings2;
    settings2.max_iterations_ols = 20;
    PowerFlowSolver pfs2 = PowerFlowSolver(std::move(net2), settings2, &logger);
    
    for (int i = 0; i < 40; i++)
    {
        pfs2.solve({
            measuredLoads[0][i], 
            measuredLoads[1][i], 
            measuredLoads[2][i]
        }, {slackVoltages[i]});
        std::vector<complex_t> lv = pfs2.getLoadVoltages();
        for (size_t j = 0; j < lv.size(); j++)
        {
            measuredLoadVoltages[j].push_back(lv[j]);
        }
        pfs2.reset();
    }
    
    const bool addNoise = true;
    if (addNoise)
    {
        // Add normal-distributed error terms to all measurements
        std::default_random_engine generator;
        generator.seed(6789);
        std::normal_distribution<double> voltageNoise(0, 0.0001);
        std::normal_distribution<double> powerNoise(0, 1e-5);
        for (int i = 0; i < 40; i++)
        {
            // Linux doesn't like it when complex_t is omitted here
            slackVoltages[i] += complex_t{voltageNoise(generator), 0};
            for (int j = 0; j < 3; j++)
            {
                measuredLoadVoltages[j][i] += complex_t{voltageNoise(generator), voltageNoise(generator)};
                measuredLoads[j][i] += complex_t{powerNoise(generator), 0};
            }
        }
    }

    // We place each load node time series data into a MeasuredValues struct
    // Map of load node IDs to time series data
    std::unordered_map<node_idx_t, MeasuredValues> measuredValues;
    measuredValues[1] = {measuredLoadVoltages[0], measuredLoads[0]};
    measuredValues[3] = {measuredLoadVoltages[1], measuredLoads[1]};
    measuredValues[4] = {measuredLoadVoltages[2], measuredLoads[2]};
    
    // Cache nominal (reference) impedances to compare to estimated impedances later
    std::vector<complex_t> refImpedances = pfs2.getImpedances();
    
    // Pass in time series data
    std::vector<complex_t> newImpedances = pfs2.solveParamsOLS(measuredValues, slackVoltages);

    std::cout << "Cable parameter estimate error relative to reference:\n";
    // Print out differences relative to nominal values
    for (size_t i = 0; i < refImpedances.size(); i++)
    {
        std::cout << std::abs((newImpedances[i].real() - refImpedances[i].real()) / refImpedances[i].real()) << std::endl;
    }
    
    for (size_t i = 0; i < refImpedances.size(); i++)
    {
        std::cout << "Ref: " << refImpedances[i] << "\tEstimated: " << newImpedances[i] << std::endl;
    }
    
    pfs2.setImpedances(newImpedances);
    
    std::ofstream file3("net_saved.txt");
    pfs2.save(file3);
}
