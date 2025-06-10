#include "VerilogParser.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace VerilogParser {

// --- Helper functions (local to this file) ---

// Trims leading/trailing whitespace from a string.
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// Splits a comma-separated list into a vector of strings.
static std::vector<std::string> splitCommaList(const std::string& s) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) tokens.push_back(item);
    }
    return tokens;
}

// --- Main Parsing Logic ---

void parseFile(
    const std::string& filename,
    std::vector<Gate>& gates,
    std::vector<FlipFlop>& flipflops,
    std::map<std::string, Net>& nets,
    std::vector<std::string>& primaryInputs,
    std::vector<std::string>& primaryOutputs
) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw ParsingException("Could not open file: " + filename);
    }

    // Helper lambda to ensure a net exists in the map.
    auto ensureNet = [&](const std::string& netName) {
        if (netName.empty()) return;
        if (nets.find(netName) == nets.end()) {
            nets[netName] = {netName, "", {}, {}, -1, INF, INF, INF, INF, INF, INF, false};
        }
    };

    std::string line;
    while (getline(file, line)) {
        // Pre-processing
        size_t comment_pos = line.find("//");
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        line = trim(line);
        if (line.empty() || line.rfind("module", 0) == 0 || line.rfind("endmodule", 0) == 0) {
            continue;
        }

        // Handle declarations (input, output, wire)
        if (line.rfind("input", 0) == 0 || line.rfind("output", 0) == 0 || line.rfind("wire", 0) == 0) {
            std::string declaration_type = line.substr(0, line.find_first_of(" \t"));
            line.erase(0, declaration_type.length());
            
            // Handle multi-line declarations ending with a semicolon
            while (line.find(';') == std::string::npos) {
                std::string next_line;
                if (!getline(file, next_line)) {
                     throw ParsingException("Unterminated declaration line: " + line);
                }
                line += " " + trim(next_line);
            }
            line = trim(line.substr(0, line.find(';')));

            auto names = splitCommaList(line);
            for (const auto& n : names) {
                ensureNet(n);
                if (declaration_type == "input") {
                    nets[n].type = "P";
                    primaryInputs.push_back(n);
                } else if (declaration_type == "output") {
                    nets[n].type = "O";
                    primaryOutputs.push_back(n);
                }
            }
        }
        // Handle gate/module instantiations
        else if (line.find('(') != std::string::npos && line.back() == ';') {
            std::stringstream line_ss(line);
            std::string type, name;
            line_ss >> type >> name;
            
            size_t paren_start = line.find('(');
            size_t paren_end = line.rfind(')');
            if(paren_start == std::string::npos || paren_end == std::string::npos) continue;

            std::string connections_str = line.substr(paren_start + 1, paren_end - paren_start - 1);
            auto connections = splitCommaList(connections_str);

            if (type == "dff" || type == "tff" || type == "jkff" || type == "srff") {
                FlipFlop ff;
                ff.type = type;
                ff.name = name;
                if (type == "dff" && connections.size() >= 3) {
                    ff.clk = connections[0]; ff.q = connections[1]; ff.d = connections[2];
                } else {
                     // Add other FF types here
                }
                
                ensureNet(ff.clk); ensureNet(ff.q); ensureNet(ff.d);
                nets[ff.q].drivenByFlipFlop = true;
                flipflops.push_back(ff);

            } else { // Combinational Gate
                Gate gate;
                gate.type = type;
                gate.name = name;

                if (connections.empty()) continue;
                gate.output = connections[0];
                gate.inputs.assign(connections.begin() + 1, connections.end());
                
                ensureNet(gate.output);
                nets[gate.output].drivers.push_back(gate.name);
                for (const auto& inp : gate.inputs) {
                    ensureNet(inp);
                    nets[inp].loads.push_back(gate.name);
                }
                gates.push_back(gate);
            }
        }
    }
}

} // namespace VerilogParser
