#include "Circuit.h"
#include "VerilogParser.h" // For parsing functionality
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <queue>

// Main method to orchestrate the entire SCOAP calculation process.
void Circuit::calculateAllScoapMetrics() {
    std::cout << "Calculating net levels..." << std::endl;
    calculateNetLevels();

    std::cout << "Calculating combinational controllability (CC)..." << std::endl;
    calculateCombinationalControllability();

    std::cout << "Calculating sequential controllability (SC)..." << std::endl;
    calculateSequentialControllability();

    std::cout << "Calculating combinational observability (CO)..." << std::endl;
    // Initialize POs for observability calculation
    for (const auto& poName : primaryOutputs) {
        if (nets.count(poName)) {
            nets.at(poName).co = 0;
        }
    }
    calculateCombinationalObservability();

    std::cout << "Calculating sequential observability (SO)..." << std::endl;
    // Initialize POs for sequential observability
    for (const auto& poName : primaryOutputs) {
        if (nets.count(poName)) {
            nets.at(poName).so = 0; // PO is observable in 0 time steps
        }
    }
    calculateSequentialObservability();
    
    std::cout << "SCOAP calculations complete." << std::endl;
}

// Loads the circuit structure from a Verilog file.
bool Circuit::loadFromVerilog(const std::string& filename) {
    std::cout << "Parsing Verilog file: " << filename << "..." << std::endl;
    try {
        VerilogParser::parseFile(filename, gates, flipflops, nets, primaryInputs, primaryOutputs);
        std::cout << "Parsing complete. Found " << gates.size() << " gates and " << flipflops.size() << " flip-flops." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error during parsing: " << e.what() << std::endl;
        return false;
    }
}

// Assigns a topological level to each net. PIs and FF outputs are level 0.
void Circuit::calculateNetLevels() {
    std::queue<std::string> bfsQueue;
    std::map<std::string, int> gateInputCounts;

    for (const auto& gate : gates) {
        gateInputCounts[gate.name] = gate.inputs.size();
    }

    for (auto& pair : nets) {
        Net& net = pair.second;
        if (net.type == "P" || net.drivenByFlipFlop) {
            net.level = 0;
            bfsQueue.push(net.name);
        }
    }

    while (!bfsQueue.empty()) {
        std::string currentNetName = bfsQueue.front();
        bfsQueue.pop();

        for (const auto& gateName : nets.at(currentNetName).loads) {
            if (--gateInputCounts[gateName] == 0) {
                Gate g = findGateByName(gateName);
                if (g.name.empty()) continue;

                int maxInLevel = 0;
                for (const auto& inpName : g.inputs) {
                    if(nets.count(inpName)) {
                        maxInLevel = std::max(maxInLevel, nets.at(inpName).level);
                    }
                }
                if(nets.count(g.output)) {
                    nets.at(g.output).level = maxInLevel + 1;
                    bfsQueue.push(g.output);
                }
            }
        }
    }
}

// Calculates CC0 and CC1 for all nets.
void Circuit::calculateCombinationalControllability() {
    for (auto& pair : nets) {
        Net& net = pair.second;
        if (net.type == "P" || net.drivenByFlipFlop) {
            net.cc0 = 1;
            net.cc1 = 1;
        }
    }

    std::vector<Gate> topoGates = gates;
    std::sort(topoGates.begin(), topoGates.end(), [&](const Gate& a, const Gate& b) {
        if (!nets.count(a.output) || !nets.count(b.output)) return false;
        return nets.at(a.output).level < nets.at(b.output).level;
    });

    for (const auto& g : topoGates) {
        if (!nets.count(g.output)) continue;
        
        std::vector<int> inCC0, inCC1;
        for (const auto& inp : g.inputs) {
            if(nets.count(inp)) {
                inCC0.push_back(nets.at(inp).cc0);
                inCC1.push_back(nets.at(inp).cc1);
            }
        }

        if (inCC0.empty() || inCC1.empty()) continue; // Skip gates with no connected inputs

        int& out0 = nets.at(g.output).cc0;
        int& out1 = nets.at(g.output).cc1;

        if (g.type == "and") {
            out0 = 1 + *std::min_element(inCC0.begin(), inCC0.end());
            out1 = 1 + std::accumulate(inCC1.begin(), inCC1.end(), 0);
        } else if (g.type == "nand") {
            out0 = 1 + std::accumulate(inCC1.begin(), inCC1.end(), 0);
            out1 = 1 + *std::min_element(inCC0.begin(), inCC0.end());
        } else if (g.type == "or") {
            out0 = 1 + std::accumulate(inCC0.begin(), inCC0.end(), 0);
            out1 = 1 + *std::min_element(inCC1.begin(), inCC1.end());
        } else if (g.type == "nor") {
            out1 = 1 + std::accumulate(inCC0.begin(), inCC0.end(), 0);
            out0 = 1 + *std::min_element(inCC1.begin(), inCC1.end());
        } else if (g.type == "xor") {
            out0 = 1 + std::min(inCC0[0] + inCC0[1], inCC1[0] + inCC1[1]);
            out1 = 1 + std::min(inCC0[0] + inCC1[1], inCC1[0] + inCC0[1]);
        } else if (g.type == "xnor") {
            out1 = 1 + std::min(inCC0[0] + inCC0[1], inCC1[0] + inCC1[1]);
            out0 = 1 + std::min(inCC0[0] + inCC1[1], inCC1[0] + inCC0[1]);
        } else if (g.type == "not") {
            out0 = 1 + inCC1[0];
            out1 = 1 + inCC0[0];
        } else if (g.type == "buf") {
            out0 = 1 + inCC0[0];
            out1 = 1 + inCC1[0];
        }
    }
}

// Calculates SC0 and SC1 for all nets using an iterative approach.
void Circuit::calculateSequentialControllability() {
    // Initialize SC for PIs
    for (auto& pair : nets) {
        if (pair.second.type == "P") {
            pair.second.sc0 = 0;
            pair.second.sc1 = 0;
        }
    }

    bool changed;
    do {
        changed = false;

        // Propagate through combinational logic
        std::vector<Gate> topoGates = gates;
        std::sort(topoGates.begin(), topoGates.end(), [&](const Gate& a, const Gate& b) {
            if (!nets.count(a.output) || !nets.count(b.output)) return false;
            return nets.at(a.output).level < nets.at(b.output).level;
        });

        for (const auto& g : topoGates) {
             if (!nets.count(g.output)) continue;
            
            std::vector<int> inSC0, inSC1;
            for (const auto& inp : g.inputs) {
                if(nets.count(inp)) {
                    inSC0.push_back(nets.at(inp).sc0);
                    inSC1.push_back(nets.at(inp).sc1);
                }
            }
            if (inSC0.empty() || inSC1.empty()) continue;

            int new_sc0 = INF, new_sc1 = INF;
            if (g.type == "and") {
                new_sc0 = *std::min_element(inSC0.begin(), inSC0.end());
                new_sc1 = std::accumulate(inSC1.begin(), inSC1.end(), 0);
            } else if (g.type == "nand") {
                new_sc0 = std::accumulate(inSC1.begin(), inSC1.end(), 0);
                new_sc1 = *std::min_element(inSC0.begin(), inSC0.end());
            } else if (g.type == "or") {
                new_sc0 = std::accumulate(inSC0.begin(), inSC0.end(), 0);
                new_sc1 = *std::min_element(inSC1.begin(), inSC1.end());
            } else if (g.type == "nor") {
                new_sc1 = std::accumulate(inSC0.begin(), inSC0.end(), 0);
                new_sc0 = *std::min_element(inSC1.begin(), inSC1.end());
            } else if (g.type == "not") {
                new_sc0 = inSC1[0];
                new_sc1 = inSC0[0];
            } else if (g.type == "buf") {
                new_sc0 = inSC0[0];
                new_sc1 = inSC1[0];
            }
            
            if (new_sc0 < nets.at(g.output).sc0) {
                nets.at(g.output).sc0 = new_sc0;
                changed = true;
            }
            if (new_sc1 < nets.at(g.output).sc1) {
                nets.at(g.output).sc1 = new_sc1;
                changed = true;
            }
        }
        
        // Propagate through flip-flops
        for (const auto& ff : flipflops) {
            if (!nets.count(ff.q) || !nets.count(ff.d) || !nets.count(ff.clk)) continue;

            if (ff.type == "dff") {
                int d_sc0 = nets.at(ff.d).sc0;
                int d_sc1 = nets.at(ff.d).sc1;
                int clk_sc0 = nets.at(ff.clk).sc0;
                int clk_sc1 = nets.at(ff.clk).sc1;

                int new_q_sc0 = d_sc0 + clk_sc0 + clk_sc1 + 1;
                int new_q_sc1 = d_sc1 + clk_sc0 + clk_sc1 + 1;
                
                if (new_q_sc0 < nets.at(ff.q).sc0) {
                    nets.at(ff.q).sc0 = new_q_sc0;
                    changed = true;
                }
                if (new_q_sc1 < nets.at(ff.q).sc1) {
                    nets.at(ff.q).sc1 = new_q_sc1;
                    changed = true;
                }
            }
            // Add logic for other FF types (T, JK, SR) if needed
        }

    } while (changed);
}

// Calculates CO for all nets.
void Circuit::calculateCombinationalObservability() {
    std::vector<Gate> revGates = gates;
    std::sort(revGates.begin(), revGates.end(), [&](const Gate& a, const Gate& b) {
        if (!nets.count(a.output) || !nets.count(b.output)) return false;
        return nets.at(a.output).level > nets.at(b.output).level;
    });

    for (const auto& g : revGates) {
        if (!nets.count(g.output)) continue;
        int coY = nets.at(g.output).co;
        if (coY == INF) continue;

        for (size_t i = 0; i < g.inputs.size(); ++i) {
            const std::string& inputNetName = g.inputs[i];
            if (!nets.count(inputNetName)) continue;

            int newCO = INF;
            if (g.type == "and" || g.type == "nand") {
                int sumCC1 = 0;
                for (size_t j = 0; j < g.inputs.size(); ++j) {
                    if (i == j) continue;
                    sumCC1 += nets.at(g.inputs[j]).cc1;
                }
                newCO = coY + sumCC1 + 1;
            } else if (g.type == "or" || g.type == "nor") {
                int sumCC0 = 0;
                for (size_t j = 0; j < g.inputs.size(); ++j) {
                    if (i == j) continue;
                    sumCC0 += nets.at(g.inputs[j]).cc0;
                }
                newCO = coY + sumCC0 + 1;
            } else if (g.type == "not" || g.type == "buf") {
                newCO = coY + 1;
            } else if (g.type == "xor" || g.type == "xnor") {
                 int other_idx = (i == 0) ? 1 : 0;
                 if (g.inputs.size() == 2) {
                    int other_cc0 = nets.at(g.inputs[other_idx]).cc0;
                    int other_cc1 = nets.at(g.inputs[other_idx]).cc1;
                    newCO = coY + std::min(other_cc0, other_cc1) + 1;
                 }
            }

            if (newCO < nets.at(inputNetName).co) {
                nets.at(inputNetName).co = newCO;
            }
        }
    }
}

// Calculates SO for all nets.
void Circuit::calculateSequentialObservability() {
    // This is a complex calculation that typically requires iteration, similar to SC.
    // For this refactoring, we will propagate the SO from the FF outputs backward.
    bool changed;
    do {
        changed = false;
        // Propagate SO from FF inputs to their Q outputs
        for (const auto& ff : flipflops) {
            if (!nets.count(ff.q) || !nets.count(ff.d) || !nets.count(ff.clk)) continue;

            int q_so = nets.at(ff.q).so;
            if (q_so == INF) continue;

            if (ff.type == "dff") {
                int clk_sc0 = nets.at(ff.clk).sc0;
                int clk_sc1 = nets.at(ff.clk).sc1;
                
                int new_d_so = q_so + clk_sc0 + clk_sc1 + 1;

                if (new_d_so < nets.at(ff.d).so) {
                    nets.at(ff.d).so = new_d_so;
                    changed = true;
                }
            }
            // Add logic for other FF types if needed
        }

        // Propagate SO backward through combinational logic
        std::vector<Gate> revGates = gates;
        std::sort(revGates.begin(), revGates.end(), [&](const Gate& a, const Gate& b) {
            if (!nets.count(a.output) || !nets.count(b.output)) return false;
            return nets.at(a.output).level > nets.at(b.output).level;
        });

        for (const auto& g : revGates) {
            if (!nets.count(g.output)) continue;
            int soY = nets.at(g.output).so;
            if (soY == INF) continue;

            for (size_t i = 0; i < g.inputs.size(); ++i) {
                const std::string& inputNetName = g.inputs[i];
                if (!nets.count(inputNetName)) continue;
                
                int newSO = INF;
                if (g.type == "and" || g.type == "nand") {
                    int sumSC1 = 0;
                    for (size_t j = 0; j < g.inputs.size(); ++j) {
                        if (i == j) continue;
                        sumSC1 += nets.at(g.inputs[j]).sc1;
                    }
                    newSO = soY + sumSC1;
                } else if (g.type == "or" || g.type == "nor") {
                     int sumSC0 = 0;
                    for (size_t j = 0; j < g.inputs.size(); ++j) {
                        if (i == j) continue;
                        sumSC0 += nets.at(g.inputs[j]).sc0;
                    }
                    newSO = soY + sumSC0;
                } else if (g.type == "not" || g.type == "buf") {
                    newSO = soY;
                }
                
                if (newSO < nets.at(inputNetName).so) {
                    nets.at(inputNetName).so = newSO;
                    changed = true;
                }
            }
        }
    } while(changed);
}

// Finds a gate by its instance name. Returns a dummy gate if not found.
Gate Circuit::findGateByName(const std::string& name) const {
    for (const auto& gate : gates) {
        if (gate.name == name) return gate;
    }
    return Gate(); // Return an empty gate
}

// Generates and prints debug information to files.
void Circuit::printDebugInfo(const std::string& outputDir) const {
    std::cout << "Writing debug files to " << outputDir << "..." << std::endl;
    printGatesToFile(outputDir + "/gates_info.txt");
    printNetsToFile(outputDir + "/nets_info.txt");
    detectFeedbackLoops();
}

// Detects combinational feedback loops.
int Circuit::detectFeedbackLoops() const {
    int feedbackCount = 0;
    for (const auto& g : gates) {
        if (!nets.count(g.output)) continue;
        int outLevel = nets.at(g.output).level;
        if (outLevel == -1) continue;

        for (const auto& inp : g.inputs) {
            if (nets.count(inp) && nets.at(inp).level > outLevel) {
                std::cout << "Feedback detected: Gate " << g.name
                          << ", Input " << inp << " (level " << nets.at(inp).level
                          << ") -> Output " << g.output << " (level " << outLevel << ")" << std::endl;
                feedbackCount++;
                break;
            }
        }
    }
    if (feedbackCount > 0) {
        std::cout << "Total feedback loops detected: " << feedbackCount << std::endl;
    } else {
        std::cout << "No combinational feedback loops detected." << std::endl;
    }
    return feedbackCount;
}

// Writes detailed gate information to a text file.
void Circuit::printGatesToFile(const std::string& filepath) const {
    std::ofstream ofs(filepath);
    if (!ofs) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return;
    }
    ofs << "--- Gates Information ---\n\n";
    for (const auto& g : gates) {
        ofs << "Gate Name: " << g.name << "\n";
        ofs << "Type: " << g.type << "\n";
        if (nets.count(g.output)) {
            ofs << "Level: " << nets.at(g.output).level << "\n";
        }
        ofs << "Output: " << g.output << "\n";
        ofs << "Inputs: ";
        for (const auto& inp : g.inputs) ofs << inp << " ";
        ofs << "\n\n";
    }
    std::cout << "Wrote gate info to " << filepath << std::endl;
}

// Writes detailed net information to a text file.
void Circuit::printNetsToFile(const std::string& filepath) const {
    std::ofstream ofs(filepath);
    if (!ofs) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return;
    }
    ofs << "--- Nets Information ---\n\n";
    for (const auto& pair : nets) {
        const Net& net = pair.second;
        ofs << "Net Name: " << net.name << "\n";
        ofs << "Type: " << (net.type.empty() ? "Wire" : net.type) << "\n";
        ofs << "Level: " << net.level << "\n";
        ofs << "Drivers: ";
        if (net.drivenByFlipFlop) ofs << "(flipflop) ";
        for (const auto& d : net.drivers) ofs << d << " ";
        ofs << "\nLoads: ";
        for (const auto& l : net.loads) ofs << l << " ";
        ofs << "\nSCOAP Values:\n";
        ofs << "  CC0: " << (net.cc0 == INF ? -1 : net.cc0) << ", CC1: " << (net.cc1 == INF ? -1 : net.cc1) << "\n";
        ofs << "  SC0: " << (net.sc0 == INF ? -1 : net.sc0) << ", SC1: " << (net.sc1 == INF ? -1 : net.sc1) << "\n";
        ofs << "  CO: " << (net.co == INF ? -1 : net.co) << ", SO: " << (net.so == INF ? -1 : net.so) << "\n\n";
    }
    std::cout << "Wrote net info to " << filepath << std::endl;
}
