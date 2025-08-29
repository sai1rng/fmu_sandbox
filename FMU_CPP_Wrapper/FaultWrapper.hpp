/**
 * @file FaultWrapper.hpp
 * @brief Defines the C++ class that encapsulates the logic for the fault-injecting wrapper FMU.
 *
 * This header declares the FaultWrapper class, which manages the lifecycle and state
 * of an inner FMU, intercepts its inputs/outputs, and applies fault logic.
 */
#ifndef FAULT_WRAPPER_HPP
#define FAULT_WRAPPER_HPP

#include <string>
#include <stdexcept>

// FMI standard headers are C headers, so we wrap them in extern "C" for C++ compatibility.
extern "C" {
#include "fmi2Functions.h"
}

// Platform-specific dynamic library loading
#ifdef _WIN32
#include <windows.h>
#define DLL_HANDLE HMODULE
#define LOAD_LIBRARY(path) LoadLibraryA(path)
#define GET_FUNCTION(handle, name) GetProcAddress(handle, name)
#define FREE_LIBRARY(handle) FreeLibrary(handle)
#define SEP "\\"
#else
#include <dlfcn.h>
#define DLL_HANDLE void*
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define GET_FUNCTION(handle, name) dlsym(handle, name)
#define FREE_LIBRARY(handle) dlclose(handle)
#define SEP "/"
#endif

// Value References for the FMU's variables, defined with C++ `constexpr` for compile-time safety.
constexpr fmi2ValueReference VR_U = 0;
constexpr fmi2ValueReference VR_Y = 1;
constexpr fmi2ValueReference VR_K = 2;

// Hardcoded fault definition for demonstration purposes.
constexpr double FAULT_START_TIME = 3.0;
constexpr double FAULT_END_TIME = 7.0;
constexpr double FAULT_VALUE = 0.5;

// A dispatch table to hold function pointers loaded from the inner FMU's shared library.
struct InnerFMU {
    fmi2InstantiateTYPE*            Instantiate = nullptr;
    fmi2FreeInstanceTYPE*           FreeInstance = nullptr;
    fmi2SetupExperimentTYPE*        SetupExperiment = nullptr;
    fmi2EnterInitializationModeTYPE* EnterInitializationMode = nullptr;
    fmi2ExitInitializationModeTYPE*  ExitInitializationMode = nullptr;
    fmi2TerminateTYPE*              Terminate = nullptr;
    fmi2ResetTYPE*                  Reset = nullptr;
    fmi2GetRealTYPE*                GetReal = nullptr;
    fmi2SetRealTYPE*                SetReal = nullptr;
    fmi2DoStepTYPE*                 DoStep = nullptr;
};

/**
 * @class FaultWrapper
 * @brief Encapsulates all state and logic for a single instance of the wrapper FMU.
 *
 * This class uses the RAII (Resource Acquisition Is Initialization) principle.
 * The constructor acquires all necessary resources (loading the inner FMU library,
 * instantiating it), and the destructor ensures all resources are properly released.
 */
class FaultWrapper {
public:
    /**
     * @brief Constructor that initializes the wrapper and the inner FMU.
     * @param instanceName The name of this FMU instance.
     * @param fmuResourceLocation A URI to the resources directory of the unpacked FMU.
     * @param functions A pointer to the FMI callback functions provided by the simulator.
     * @param visible A flag indicating if the simulation is running with a UI.
     * @param loggingOn A flag to enable/disable logging.
     */
    FaultWrapper(fmi2String instanceName, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn);
    
    /** @brief Destructor that cleans up all acquired resources. */
    ~FaultWrapper();

    // FMI API methods implemented as C++ class members.
    fmi2Status setReal(const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]);
    fmi2Status getReal(const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]);
    fmi2Status setupExperiment(fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime);
    fmi2Status enterInitializationMode();
    fmi2Status exitInitializationMode();
    fmi2Status doStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint);
    fmi2Status terminate();

    /** @brief Provides access to the callback functions for the C adapter layer. */
    const fmi2CallbackFunctions* getCallbacks() const { return m_callbacks; }

private:
    // --- Private Member Variables ---
    double m_u = 0.0, m_y = 0.0, m_k = 2.0, m_currentTime = 0.0; // Cached values of the wrapper's variables.
    DLL_HANDLE m_innerFMUHandle = nullptr;                       // Handle to the loaded inner FMU's shared library.
    fmi2Component m_innerFMUInstance = nullptr;                  // The component instance of the inner FMU.
    InnerFMU m_innerFunctions;                                   // Struct containing function pointers to the inner FMU's API.
    const fmi2CallbackFunctions* m_callbacks;                    // Pointer to the simulator's callback functions.
    std::string m_instanceName;                                  // The name of this FMU instance.

    // --- Private Helper Methods ---
    void loadInnerFmuFunctions();                                // Loads all required function pointers from the inner FMU.
    void log(fmi2Status status, const std::string& category, const std::string& message); // A helper for logging messages via the FMI callbacks.
};

#endif // FAULT_WRAPPER_HPP