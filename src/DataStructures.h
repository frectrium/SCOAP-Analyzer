#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <string>
#include <vector>
#include <limits>
#include <map>
#include <set>

// A constant representing infinity for SCOAP calculations.
constexpr int INF = std::numeric_limits<int>::max() / 2;

// Represents a combinational logic gate.
struct Gate {
    std::string name;
    std::string type;
    std::vector<std::string> inputs;
    std::string output;
    int level = -1;
};

// Represents a sequential element (D, T, JK, or SR flip-flop).
struct FlipFlop {
    std::string type;    // "dff", "tff", "jkff", or "srff"
    std::string name;
    // Port nets (strings of net names). Unused ports remain empty.
    std::string clk, q, d, t, j, k, s, r;
};

// Represents a signal/net in the circuit.
struct Net {
    std::string name;
    std::string type; // "P" for primary input, "O" for primary output, "" for internal wire.
    std::vector<std::string> drivers; // Gates that drive this net.
    std::vector<std::string> loads;   // Gates that this net is an input to.
    int level = -1; // Topological level.

    // SCOAP Metrics
    int cc0 = INF, cc1 = INF; // Combinational Controllability
    int sc0 = INF, sc1 = INF; // Sequential Controllability
    int co = INF;             // Combinational Observability
    int so = INF;             // Sequential Observability

    bool drivenByFlipFlop = false; // True if the net is a flip-flop's Q output.
};

#endif // DATA_STRUCTURES_H
