# Verilog SCOAP Testability Analyzer

This is a C++ tool designed to parse structural Verilog files, build a netlist representation of the circuit, and calculate the **SCOAP** (Sandia Controllability/Observability Analysis Program) testability metrics for each net.

The tool handles both combinational and sequential logic (D-type flip-flops) and provides a detailed analysis of the circuit's testability, which is crucial for DFT (Design for Test) and ATPG (Automatic Test Pattern Generation).

## Features

* **Verilog Parser**: Reads structural Verilog files describing logic gates and flip-flops.
* **Netlist Generation**: Constructs an in-memory graph of the circuit's netlist.
* **Levelization**: Performs a topological sort to determine the level of each net from the primary inputs.
* **SCOAP Calculations**:
    * Combinational Controllability (CC0, CC1)
    * Sequential Controllability (SC0, SC1)
    * Combinational Observability (CO)
    * Sequential Observability (SO)
* **CSV Output**: Exports the final testability metrics to a `scoap_results.csv` file for easy analysis in spreadsheet software.
* **Debug Logs**: Generates detailed logs about the gates and nets for debugging purposes.

## Repository Structure

The project is organized into the following directories:

```
verilog-scoap-analyzer/
│
├── circuits/      # Sample Verilog circuits for testing
├── docs/          # Additional project documentation
├── src/           # All C++ source code (.h, .cpp)
└── output/        # Default location for generated results
```

## Prerequisites

To build and run this project, you will need:
* **CMake**: Version 3.10 or higher.
* **A C++ Compiler**: A modern compiler that supports C++17 (e.g., GCC, Clang, MSVC).

## How to Build

The project uses CMake to generate platform-native build files.

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/your-username/verilog-scoap-analyzer.git](https://github.com/your-username/verilog-scoap-analyzer.git)
    cd verilog-scoap-analyzer
    ```

2.  **Create a build directory:**
    It's good practice to build the project outside the source tree.
    ```bash
    mkdir build
    cd build
    ```

3.  **Run CMake and build the project:**
    CMake will detect your compiler and generate the necessary build files (e.g., Makefiles on Linux, Visual Studio solution on Windows).
    ```bash
    # Generate build files
    cmake ..

    # Compile the project
    cmake --build .
    ```
    On Linux or macOS, you can also just run `make` after `cmake ..`.

## How to Run

After a successful build, the executable (`analyzer` or `analyzer.exe`) will be located in the `build/` directory.

Run the tool from the `build` directory, passing the path to a Verilog file as a command-line argument.

**Example:**
```bash
./analyzer ../circuits/iscas85/c17.v
```

The program will process the circuit and generate its output files in the `output/` directory in the project's root.

## Output Files

The analyzer generates the following files in the `output/` directory:

1.  **`scoap_results.csv`**: The primary output file. It contains the calculated SCOAP values for every net in the design.
2.  **`gates_info.txt`**: A debug file containing detailed information for each gate instance, including its type, level, inputs, and output.
3.  **`nets_info.txt`**: A debug file containing detailed information for each net, including its drivers, loads, and all calculated SCOAP values.

## Future Work: Trojan Detection

A key future goal for this project is to implement hardware trojan detection. The plan is to use the generated SCOAP metrics as features for a machine learning model.

* **Hypothesis**: Maliciously inserted logic (a trojan) will often have controllabilty and observability values that are statistical outliers compared to the rest of the legitimate circuit.
* **Method**: By applying a clustering algorithm like **K-Means** to the SCOAP data, we can automatically identify nets with anomalous testability metrics. These clusters of outlier nets can then be flagged as potential trojan candidates for further, more detailed analysis.

## License

This project is licensed under the terms of the MIT License. See the [LICENSE](LICENSE) file for details.
