/**
 * @file fmi3_amplifier.hpp
 * @brief Defines the C++ class that encapsulates the logic for the FMI 3.0 amplifier model.
 */
#ifndef FMI3_AMPLIFIER_HPP
#define FMI3_AMPLIFIER_HPP

#include "fmi3Functions.h"
#include <string>

// Value References for the variables
constexpr fmi3ValueReference VR_U = 1; // Renumbered
constexpr fmi3ValueReference VR_Y = 2; // Renumbered
constexpr fmi3ValueReference VR_K = 3; // Renumbered

/**
 * @class AmplifierModel
 * @brief Encapsulates all state and logic for a single instance of the amplifier FMU.
 */
class AmplifierModel {
public:
    AmplifierModel(fmi3String instanceName, fmi3InstanceEnvironment instanceEnvironment, fmi3LogMessageCallback logger);
    ~AmplifierModel();

    // FMI 3.0 API methods
    fmi3Status getFloat64(const fmi3ValueReference vr[], size_t nvr, fmi3Float64 value[], size_t nValues);
    fmi3Status setFloat64(const fmi3ValueReference vr[], size_t nvr, const fmi3Float64 value[], size_t nValues);
    fmi3Status doStep(fmi3Float64 currentCommunicationPoint, fmi3Float64 communicationStepSize);

private:
    // Model variables
    fmi3Float64 m_u; // input
    fmi3Float64 m_y; // output
    fmi3Float64 m_k; // parameter

    // FMI 3.0 instance information
    std::string m_instanceName;
    fmi3InstanceEnvironment m_instanceEnvironment;
    fmi3LogMessageCallback m_logger;

    // Private helper for logging
    void log(fmi3Status status, const std::string& category, const std::string& message);
};

#endif // FMI3_AMPLIFIER_HPP