#ifndef VERILOG_PARSER_H
#define VERILOG_PARSER_H

#include "DataStructures.h"
#include <stdexcept>

// A custom exception for parsing errors.
class ParsingException : public std::runtime_error {
public:
    explicit ParsingException(const std::string& message)
        : std::runtime_error(message) {}
};

// Namespace to contain all Verilog parsing logic.
namespace VerilogParser {

    // Main function to parse a Verilog file and populate the circuit data structures.
    void parseFile(
        const std::string& filename,
        std::vector<Gate>& gates,
        std::vector<FlipFlop>& flipflops,
        std::map<std::string, Net>& nets,
        std::vector<std::string>& primaryInputs,
        std::vector<std::string>& primaryOutputs
    );

} // namespace VerilogParser

#endif // VERILOG_PARSER_H
