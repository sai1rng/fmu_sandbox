/**
 * @file FaultWrapper.cpp
 * @brief Implements the logic for the FaultWrapper class.
 *
 * This file contains the implementation of the constructor, destructor, and FMI API
 * methods for the C++ wrapper class.
 */
#include "FaultWrapper.hpp"
#include <vector>
#include <cstring> // For strncmp

/**
 * @brief Converts a file URI (e.g., "file:///path/to/file") to a standard filesystem path.
 * @param uri The file URI string.
 * @return A standard filesystem path as a std::string.
 */
std::string uriToPath(const char* uri) {
    std::string path(uri);
    const std::string scheme = "file://";
    if (path.rfind(scheme, 0) == 0) {
        path.erase(0, scheme.length());
#ifdef _WIN32
        if (path.length() > 2 && path[0] == '/' && path[2] == ':') {
            path.erase(0, 1);
        }
#endif
    }
    return path;
}

// The constructor is responsible for all initialization (RAII).
FaultWrapper::FaultWrapper(fmi2String instanceName, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn)
    : m_callbacks(functions), m_instanceName(instanceName) {

    std::string resourcePath = uriToPath(fmuResourceLocation);
    // Determine the correct platform-specific directory and library extension.
    std::string platform, lib_ext;
#if defined(_WIN32)
    platform = "win64"; lib_ext = ".dll";
#elif defined(__APPLE__)
    platform = "darwin64"; lib_ext = ".dylib";
#else
    platform = "linux64"; lib_ext = ".so";
#endif

    // Construct the full path to the inner FMU's shared library.
    std::string innerFmuPath = resourcePath + SEP + "Amplifier" + SEP + "binaries" + SEP + platform + SEP + "model" + lib_ext;

    // 1. Load the inner FMU's shared library.
    m_innerFMUHandle = LOAD_LIBRARY(innerFmuPath.c_str());
    if (!m_innerFMUHandle) {
        log(fmi2Fatal, "error", "Could not load inner FMU binary: " + innerFmuPath);
        throw std::runtime_error("Failed to load inner FMU binary.");
    }

    // 2. Load the function pointers from the library.
    try {
        loadInnerFmuFunctions();
    } catch (...) {
        FREE_LIBRARY(m_innerFMUHandle); // Ensure library is freed on failure.
        throw;
    }

    // 3. Instantiate the inner FMU.
    // The GUID is from the inner FMU's modelDescription.xml.
    const char* innerGuid = "{8c4e810f-3df3-4a00-8276-176fa3c9f000}";
    // The inner FMU needs a URI to its own resources directory.
    std::string innerResourceUri = std::string(fmuResourceLocation) + SEP + "Amplifier" + SEP "resources";

    m_innerFMUInstance = m_innerFunctions.Instantiate("innerAmplifier", fmi2CoSimulation, innerGuid, innerResourceUri.c_str(), m_callbacks, visible, loggingOn);
    if (!m_innerFMUInstance) {
        log(fmi2Fatal, "error", "Failed to instantiate inner FMU.");
        FREE_LIBRARY(m_innerFMUHandle); // Ensure library is freed on failure.
        throw std::runtime_error("Failed to instantiate inner FMU.");
    }
}

// The destructor is responsible for all cleanup (RAII).
FaultWrapper::~FaultWrapper() {
    // Terminate and free the inner FMU instance if it exists.
    if (m_innerFMUInstance) {
        m_innerFunctions.Terminate(m_innerFMUInstance);
        m_innerFunctions.FreeInstance(m_innerFMUInstance);
    }
    // Unload the shared library.
    if (m_innerFMUHandle) {
        FREE_LIBRARY(m_innerFMUHandle);
    }
}

void FaultWrapper::loadInnerFmuFunctions() {
#define LOAD_FUNC(Name) \
    /* Load the function pointer from the shared library by its name. */ \
    m_innerFunctions.Name = (fmi2##Name##TYPE*)GET_FUNCTION(m_innerFMUHandle, "fmi2" #Name); \
    if (!m_innerFunctions.Name) throw std::runtime_error("Failed to load function: fmi2" #Name);

    LOAD_FUNC(Instantiate); LOAD_FUNC(FreeInstance); LOAD_FUNC(SetupExperiment);
    LOAD_FUNC(EnterInitializationMode); LOAD_FUNC(ExitInitializationMode);
    LOAD_FUNC(Terminate); LOAD_FUNC(Reset); LOAD_FUNC(GetReal);
    LOAD_FUNC(SetReal); LOAD_FUNC(DoStep);
#undef LOAD_FUNC
}

// A logging helper that uses the callbacks provided by the simulation environment.
void FaultWrapper::log(fmi2Status status, const std::string& category, const std::string& message) {
    if (m_callbacks && m_callbacks->logger) {
        m_callbacks->logger(m_callbacks->componentEnvironment, m_instanceName.c_str(), status, category.c_str(), message.c_str());
    }
}

// --- FMI API Method Implementations ---
fmi2Status FaultWrapper::setReal(const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    for (size_t i = 0; i < nvr; i++) {
        if (vr[i] == VR_U) m_u = value[i];
        else if (vr[i] == VR_K) m_k = value[i];
    }
    return fmi2OK;
}

fmi2Status FaultWrapper::getReal(const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    for (size_t i = 0; i < nvr; i++) {
        if (vr[i] == VR_U) value[i] = m_u;
        else if (vr[i] == VR_Y) value[i] = m_y;
        else if (vr[i] == VR_K) value[i] = m_k;
    }
    return fmi2OK;
}

fmi2Status FaultWrapper::setupExperiment(fmi2Boolean tolDef, fmi2Real tol, fmi2Real start, fmi2Boolean stopDef, fmi2Real stop) {
    m_currentTime = start;
    return m_innerFunctions.SetupExperiment(m_innerFMUInstance, tolDef, tol, start, stopDef, stop);
}

fmi2Status FaultWrapper::enterInitializationMode() { return m_innerFunctions.EnterInitializationMode(m_innerFMUInstance); }

// At the end of initialization, set the wrapper's parameters on the inner FMU.
fmi2Status FaultWrapper::exitInitializationMode() {
    fmi2ValueReference vr_k = VR_K;
    m_innerFunctions.SetReal(m_innerFMUInstance, &vr_k, 1, &m_k);
    return m_innerFunctions.ExitInitializationMode(m_innerFMUInstance);
}

// This is the core simulation step function.
fmi2Status FaultWrapper::doStep(fmi2Real time, fmi2Real step, fmi2Boolean noSet) {
    m_currentTime = time;
    double u_to_set = m_u;

    // *** FAULT INJECTION LOGIC ***
    // Check if the current time is within the fault window.
    if (m_currentTime >= FAULT_START_TIME && m_currentTime < FAULT_END_TIME) u_to_set += FAULT_VALUE;

    // 1. Set the (potentially faulty) input on the inner FMU.
    fmi2ValueReference vr_u = VR_U;
    m_innerFunctions.SetReal(m_innerFMUInstance, &vr_u, 1, &u_to_set);
    // 2. Tell the inner FMU to perform its calculation for the step.
    m_innerFunctions.DoStep(m_innerFMUInstance, time, step, noSet);
    // 3. Retrieve the result from the inner FMU and cache it.
    fmi2ValueReference vr_y = VR_Y;
    return m_innerFunctions.GetReal(m_innerFMUInstance, &vr_y, 1, &m_y);
}

fmi2Status FaultWrapper::terminate() { return m_innerFunctions.Terminate(m_innerFMUInstance); }