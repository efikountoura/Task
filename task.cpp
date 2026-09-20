//A standalone application that simulates a 5G downlink 
//link-adaptation procedure  for a single User Equipment (UE).

#include <random>
#include <vector>
#include <array>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>


// Constants 

// Simplified assumption for specific SINR-to-CQI thresholds (dB)
constexpr std::array<double, 15> SinrThresholds = {
    -6.0, -4.0, -2.0, 0.0, 2.0, 4.0, 6.0, 8.0,
    10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 22.0
};

// CQI Spectral Efficiency - 3GPP TS 38.214, Table 5.2.2.1-2
constexpr std::array<double, 15> CqiSpectralEfficiency = {
    0.1523, 0.2344, 0.3770, 0.6016, 0.8770, 1.1758, 1.4766, 1.9141,
    2.4063, 2.7305, 3.3223, 3.9023, 4.5234, 5.1152, 5.5547
};

// MCS Spectral Efficiency - 3GPP TS 38.214, Table 5.1.3.1-1
constexpr std::array<double, 29> McsSpectralEfficiency = {
    0.2344, 0.3066, 0.3770, 0.4902, 0.6016, 0.7402, 0.8770, 1.0273,
    1.1758, 1.3262, 1.3281, 1.4766, 1.6953, 1.9141, 2.1602, 2.4063,
    2.5703, 2.5664, 2.7305, 3.0293, 3.3223, 3.6094, 3.9023, 4.2129,
    4.5234, 4.8164, 5.1152, 5.3320, 5.5547
};


// Simulation Environment Constants
constexpr double Bandwidth = 20.0; // Channel bandwidth assumption in MHz
constexpr double OverheadRatio = 0.14; // channel overhead


// Holds the simulation results for one transmission interval 
struct IntervalResults {
    int    t;
    double sinrDb;
    int    cqi;
    int    mcs;
    double throughput;
};


// Holds the aggregated statistics for the entire simulation run
struct Metrics {
    double avgSinrDb;
    double avgCQI;
    double avgMCS;
    double avgThroughput;
    double minThroughput;
    double maxThroughput;
};

// Generates a random SINR value
double generateSINR(double averageSINR, double stdDev, std::mt19937& generator) {
    std::normal_distribution<double> distribution(averageSINR, stdDev);
    return distribution(generator);
}


// SINR -> CQI mapping based on fixed SINR thresholds.
int sinrToCqi(double sinrDb, const std::array<double, 15>& thresholds) {
    size_t i = 0;
    while (i < thresholds.size() && sinrDb >= thresholds[i]) {
        ++i;
    }
    return static_cast<int>(i); 
}

// CQI to MCS mapping
int cqiToMcs(int cqi,
             const std::array<double, 15>& cqiTable,
             const std::array<double, 29>& mcsTable) {
    if (cqi == 0) {
        return -1; // out of range: no safe transmission possible
    }

    // Determine the target Spectral Efficiency based on the reported CQI
    double targetSE = cqiTable[cqi - 1]; 

    // CQI=1 (SE=0.1523) has no MCS with SE<=targetSE
    // the most robust MCS (index 0) is selected 
    int bestMcs = 0;
    for (size_t i = 0; i < mcsTable.size(); ++i) {
        if (mcsTable[i] <= targetSE) {
            bestMcs = static_cast<int>(i);
        }
    }
    return bestMcs;
}

// Throughput estimation in Mbps
double estimateThroughput(int mcs, const std::array<double, 29>& mcsTable) {
    if (mcs < 0) {
        return 0.0; // No transmission occurred
    }
    return mcsTable[mcs] * Bandwidth * (1.0 - OverheadRatio);
}

// Executes the simulation loop over a defined number of intervals.
// Return a vector containing the results of each interval.
std::vector<IntervalResults> runSimulation(int numIntervals,
                                           double averageSinrDb,
                                           double sinrStdDev,
                                           std::mt19937& generator) {
    std::vector<IntervalResults> results;

    for (int t = 0; t < numIntervals; ++t) {
        double sinr = generateSINR(averageSinrDb, sinrStdDev, generator);
        int cqi = sinrToCqi(sinr, SinrThresholds);
        int mcs = cqiToMcs(cqi, CqiSpectralEfficiency, McsSpectralEfficiency);
        double throughput = estimateThroughput(mcs, McsSpectralEfficiency);

        results.push_back(IntervalResults{t, sinr, cqi, mcs, throughput});
    }
    return results;
}

// Computes the end-of-simulation statistics from the per-interval results.
Metrics calculateMetrics(const std::vector<IntervalResults>& results) {
    double sumSinr = 0.0, sumCqi = 0.0, sumMcs = 0.0, sumThroughput = 0.0;
    double minThroughput = std::numeric_limits<double>::max();
    double maxThroughput = std::numeric_limits<double>::lowest();

    for (size_t i = 0; i < results.size(); ++i) {
        sumSinr += results[i].sinrDb;
        sumCqi  += results[i].cqi;
        sumMcs  += (results[i].mcs < 0) ? 0 : results[i].mcs;
        sumThroughput += results[i].throughput;

        minThroughput = std::min(minThroughput, results[i].throughput);
        maxThroughput = std::max(maxThroughput, results[i].throughput);
    }

    // Calculate the average values of each parameter through the intervals
    double n = static_cast<double>(results.size());
    double avgSINR = sumSinr / n;
    double avgCQI = sumCqi / n;
    double avgMCS = sumMcs / n;
    double avgThroughput = sumThroughput / n;

return Metrics{avgSINR, avgCQI, avgMCS, 
    avgThroughput, minThroughput, maxThroughput};
   
}


// Export the results to a CSV file
void resultsRecord(const std::string& path, const std::vector<IntervalResults>& results) {
    std::ofstream out(path);
    out << "interval,sinr_dB,cqi,mcs,throughput_Mbps\n";
    out << std::fixed << std::setprecision(4);
    for (size_t i = 0; i < results.size(); ++i) {
        out << results[i].t << ',' 
            << results[i].sinrDb << ',' 
            << results[i].cqi << ',' 
            << results[i].mcs << ',' 
            << results[i].throughput << '\n';
    }
}

// Prints the final calculated metrics.
void printSummary(int numIntervals, const Metrics& s) {
    std::cout << "SIMULATION RESULTS (" << numIntervals << " intervals)\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Average SINR       : " << s.avgSinrDb << " dB\n";
    std::cout << "Average CQI        : " << s.avgCQI << '\n';
    std::cout << "Average MCS        : " << s.avgMCS << '\n';
    std::cout << "Average Throughput : " << s.avgThroughput << " Mbps\n";
    std::cout << "Minimum Throughput : " << s.minThroughput << " Mbps\n";
    std::cout << "Maximum Throughput : " << s.maxThroughput << " Mbps\n";
}


int main() {
    std::random_device rd;
    std::mt19937 generator(rd());

    constexpr double AverageSinrDb = 10.0;
    constexpr double SinrStdDev    = 4.0;
    constexpr int    NumIntervals  = 1000;
    const std::string csvPath      = "simulation_results.csv";

    std::cout << "Simulating " << NumIntervals << " transmission intervals\n";

    auto results = runSimulation(NumIntervals, AverageSinrDb, SinrStdDev, generator);
    auto summary = calculateMetrics(results);

    resultsRecord(csvPath, results);
    std::cout << "Results written to " << csvPath << "\n\n";

    printSummary(NumIntervals, summary);

    return 0;
}
