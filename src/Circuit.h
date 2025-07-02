#ifndef CIRCUIT_H
#define CIRCUIT_H

#include "DataStructures.h"

// The main class to represent and analyze the digital circuit.
// It encapsulates all gates, flip-flops, and nets, along with
// the logic to calculate testability metrics.
class Circuit {
public:
    // Constructor
    Circuit() = default;

    // Main orchestration methods
    bool loadFromVerilog(const std::string& filename);
    void calculateAllScoapMetrics();
    void printDebugInfo(const std::string& outputDir) const;

    // Public accessors
    const std::map<std::string, Net>& getNets() const { return nets; }

    // New methods
    void writeScoapResultsToCSV(const std::string& filepath) const;
    void runKMeansOnScoap(const std::string& outputFile, int k = 3) const;

private:
    // Circuit elements
    std::vector<Gate> gates;
    std::vector<FlipFlop> flipflops;
    std::map<std::string, Net> nets;
    std::vector<std::string> primaryInputs;
    std::vector<std::string> primaryOutputs;

    // Helper methods for internal calculations
    Gate findGateByName(const std::string& name) const;
    void calculateNetLevels();
    void calculateCombinationalControllability();
    void calculateSequentialControllability();
    void calculateCombinationalObservability();
    void calculateSequentialObservability();

    // Helper methods for diagnostics and output
    int detectFeedbackLoops() const;
    void printGatesToFile(const std::string& filepath) const;
    void printNetsToFile(const std::string& filepath) const;
};

#endif // CIRCUIT_H
