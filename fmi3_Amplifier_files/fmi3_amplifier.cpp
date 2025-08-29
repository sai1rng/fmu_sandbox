/**
 * @file fmi3_amplifier.cpp
 * @brief A simple FMI 3.0 Co-Simulation C++ implementation of an amplifier model (y = k * u).
 */

#include "fmi3_amplifier.hpp"
#include <string>
#include <stdexcept>

// --- C++ Class Implementation ---

AmplifierModel::AmplifierModel(fmi3String instanceName, fmi3InstanceEnvironment instanceEnvironment, fmi3LogMessageCallback logger)
    : m_instanceName(instanceName), m_instanceEnvironment(instanceEnvironment), m_logger(logger) {
    // Initialize default values
    m_u = 0.0;
    m_y = 0.0;
    m_k = 2.0; // Default gain
}

AmplifierModel::~AmplifierModel() {}

void AmplifierModel::log(fmi3Status status, const std::string& category, const std::string& message) {
    if (m_logger) {
        m_logger(m_instanceEnvironment, status, category.c_str(), message.c_str());
    }
}

fmi3Status AmplifierModel::getFloat64(const fmi3ValueReference vr[], size_t nvr, fmi3Float64 value[], size_t nValues) {
    for (size_t i = 0; i < nvr; i++) {
        if (vr[i] == VR_U) value[i] = m_u;
        else if (vr[i] == VR_Y) value[i] = m_y;
        else if (vr[i] == VR_K) value[i] = m_k;
    }
    return fmi3OK;
}

fmi3Status AmplifierModel::setFloat64(const fmi3ValueReference vr[], size_t nvr, const fmi3Float64 value[], size_t nValues) {
    for (size_t i = 0; i < nvr; i++) {
        if (vr[i] == VR_U) m_u = value[i];
        else if (vr[i] == VR_K) m_k = value[i];
    }
    return fmi3OK;
}

fmi3Status AmplifierModel::doStep(fmi3Float64 currentCommunicationPoint, fmi3Float64 communicationStepSize) {
    // Core model equation
    m_y = m_k * m_u;
    return fmi3OK;
}

// --- FMI 3.0 C Adapter Layer ---

extern "C" {

static inline AmplifierModel* to_model(fmi3Instance instance) {
    return static_cast<AmplifierModel*>(instance);
}

FMI3_Export fmi3Instance fmi3InstantiateCoSimulation(
    fmi3String instanceName, fmi3String instantiationToken, fmi3String resourcePath,
    fmi3Boolean visible, fmi3Boolean loggingOn, fmi3Boolean eventModeUsed,
    fmi3Boolean earlyReturnAllowed, const fmi3ValueReference requiredIntermediateVariables[],
    size_t nRequiredIntermediateVariables, fmi3InstanceEnvironment instanceEnvironment,
    fmi3LogMessageCallback logMessage, fmi3IntermediateUpdateCallback intermediateUpdate) {

    if (!logMessage) return NULL;
    try {
        return new AmplifierModel(instanceName, instanceEnvironment, logMessage);
    } catch (const std::exception& e) {
        logMessage(instanceEnvironment, fmi3Fatal, "error", e.what());
        return NULL;
    }
}

FMI3_Export void fmi3FreeInstance(fmi3Instance instance) {
    if (!instance) return;
    delete to_model(instance);
}

FMI3_Export fmi3Status fmi3GetFloat64(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Float64 v[], size_t nv) {
    return to_model(c)->getFloat64(vr, nvr, v, nv);
}

FMI3_Export fmi3Status fmi3SetFloat64(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Float64 v[], size_t nv) {
    return to_model(c)->setFloat64(vr, nvr, v, nv);
}

FMI3_Export fmi3Status fmi3DoStep(
    fmi3Instance instance, fmi3Float64 currentCommunicationPoint, fmi3Float64 communicationStepSize,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint, fmi3Boolean* eventHandlingNeeded,
    fmi3Boolean* terminateSimulation, fmi3Boolean* earlyReturn, fmi3Float64* lastSuccessfulTime) {

    // Set output pointers to default values for this simple FMU
    if (eventHandlingNeeded) *eventHandlingNeeded = fmi3False;
    if (terminateSimulation) *terminateSimulation = fmi3False;
    if (earlyReturn) *earlyReturn = fmi3False;
    if (lastSuccessfulTime) *lastSuccessfulTime = currentCommunicationPoint;

    return to_model(instance)->doStep(currentCommunicationPoint, communicationStepSize);
}

// --- Boilerplate/Stub FMI 3.0 Functions ---

FMI3_Export const char* fmi3GetVersion(void) {
    return fmi3Version;
}

FMI3_Export fmi3Status fmi3EnterInitializationMode(fmi3Instance instance, fmi3Boolean toleranceDefined, fmi3Float64 tolerance, fmi3Float64 startTime, fmi3Boolean stopTimeDefined, fmi3Float64 stopTime) {
    return fmi3OK;
}

FMI3_Export fmi3Status fmi3ExitInitializationMode(fmi3Instance instance) {
    return fmi3OK;
}

FMI3_Export fmi3Status fmi3Terminate(fmi3Instance instance) {
    return fmi3OK;
}

FMI3_Export fmi3Status fmi3Reset(fmi3Instance instance) {
    return fmi3Error; // Not implemented
}
// Stubs for other data types
FMI3_Export fmi3Status fmi3GetFloat32(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Float32 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetInt8(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Int8 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetUInt8(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3UInt8 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetInt16(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Int16 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetUInt16(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3UInt16 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetInt32(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Int32 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetUInt32(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3UInt32 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetInt64(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Int64 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetUInt64(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3UInt64 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetBoolean(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Boolean v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetString(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3String v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetBinary(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, size_t s[], fmi3Binary v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetClock(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, fmi3Clock v[]) { return fmi3Error; }

FMI3_Export fmi3Status fmi3SetFloat32(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Float32 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetInt8(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Int8 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetUInt8(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3UInt8 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetInt16(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Int16 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetUInt16(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3UInt16 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetInt32(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Int32 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetUInt32(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3UInt32 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetInt64(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Int64 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetUInt64(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3UInt64 v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetBoolean(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Boolean v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetString(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3String v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetBinary(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const size_t s[], const fmi3Binary v[], size_t nv) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetClock(fmi3Instance c, const fmi3ValueReference vr[], size_t nvr, const fmi3Clock v[]) { return fmi3Error; }

// Stubs for other unused functions
FMI3_Export fmi3Status fmi3SetDebugLogging(fmi3Instance c, fmi3Boolean l, size_t n, const fmi3String cat[]) { return fmi3OK; }
FMI3_Export fmi3Instance fmi3InstantiateModelExchange(fmi3String i, fmi3String t, fmi3String r, fmi3Boolean v, fmi3Boolean l, fmi3InstanceEnvironment e, fmi3LogMessageCallback cl) { return NULL; }
FMI3_Export fmi3Instance fmi3InstantiateScheduledExecution(fmi3String i, fmi3String t, fmi3String r, fmi3Boolean v, fmi3Boolean l, fmi3InstanceEnvironment e, fmi3LogMessageCallback cl, fmi3ClockUpdateCallback cu, fmi3LockPreemptionCallback lpc, fmi3UnlockPreemptionCallback upc) { return NULL; }

FMI3_Export fmi3Status fmi3EnterConfigurationMode(fmi3Instance instance) { return fmi3OK; }
FMI3_Export fmi3Status fmi3ExitConfigurationMode(fmi3Instance instance) { return fmi3OK; }

FMI3_Export fmi3Status fmi3GetFMUState(fmi3Instance instance, fmi3FMUState* FMUState) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetFMUState(fmi3Instance instance, fmi3FMUState FMUState) { return fmi3Error; }
FMI3_Export fmi3Status fmi3FreeFMUState(fmi3Instance instance, fmi3FMUState* FMUState) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SerializedFMUStateSize(fmi3Instance instance, fmi3FMUState FMUState, size_t* size) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SerializeFMUState(fmi3Instance instance, fmi3FMUState FMUState, fmi3Byte serializedState[], size_t size) { return fmi3Error; }
FMI3_Export fmi3Status fmi3DeserializeFMUState(fmi3Instance instance, const fmi3Byte serializedState[], size_t size, fmi3FMUState* FMUState) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetDirectionalDerivative(fmi3Instance instance, const fmi3ValueReference u[], size_t nu, const fmi3ValueReference z[], size_t nz, const fmi3Float64 s[], size_t ns, fmi3Float64 sens[], size_t nsens) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetAdjointDerivative(fmi3Instance instance, const fmi3ValueReference u[], size_t nu, const fmi3ValueReference z[], size_t nz, const fmi3Float64 s[], size_t ns, fmi3Float64 sens[], size_t nsens) { return fmi3Error; }

// Model Exchange
FMI3_Export fmi3Status fmi3EnterEventMode(fmi3Instance instance) { return fmi3Error; }
FMI3_Export fmi3Status fmi3EnterContinuousTimeMode(fmi3Instance instance) { return fmi3Error; }
FMI3_Export fmi3Status fmi3CompletedIntegratorStep(fmi3Instance instance, fmi3Boolean noSetFMUStatePriorToCurrentPoint, fmi3Boolean* enterEventMode, fmi3Boolean* terminateSimulation) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetTime(fmi3Instance instance, fmi3Float64 time) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetContinuousStates(fmi3Instance instance, const fmi3Float64 x[], size_t nx) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetContinuousStateDerivatives(fmi3Instance instance, fmi3Float64 d[], size_t n) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetEventIndicators(fmi3Instance instance, fmi3Float64 ei[], size_t ni) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetContinuousStates(fmi3Instance instance, fmi3Float64 x[], size_t nx) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetNominalsOfContinuousStates(fmi3Instance instance, fmi3Float64 x_nominal[], size_t nx) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetNumberOfEventIndicators(fmi3Instance instance, size_t* ni) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetNumberOfContinuousStates(fmi3Instance instance, size_t* nx) { return fmi3Error; }

// Co-Simulation
FMI3_Export fmi3Status fmi3EnterStepMode(fmi3Instance instance) { return fmi3OK; }
FMI3_Export fmi3Status fmi3GetOutputDerivatives(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, const fmi3Int32 o[], fmi3Float64 v[], size_t nv) { return fmi3Error; }

// Scheduled Execution
FMI3_Export fmi3Status fmi3ActivateModelPartition(fmi3Instance instance, fmi3ValueReference clockReference, fmi3Float64 activationTime) { return fmi3Error; }

// Clock and dependency functions
FMI3_Export fmi3Status fmi3GetNumberOfVariableDependencies(fmi3Instance instance, fmi3ValueReference valueReference, size_t* nDependencies) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetVariableDependencies(fmi3Instance instance, fmi3ValueReference dependent, size_t elementIndicesOfDependent[], fmi3ValueReference independents[], size_t elementIndicesOfIndependents[], fmi3DependencyKind dependencyKinds[], size_t nDependencies) { return fmi3Error; }

// Discrete state functions
FMI3_Export fmi3Status fmi3EvaluateDiscreteStates(fmi3Instance instance) { return fmi3Error; }
FMI3_Export fmi3Status fmi3UpdateDiscreteStates(fmi3Instance instance, fmi3Boolean* discreteStatesNeedUpdate, fmi3Boolean* terminateSimulation, fmi3Boolean* nominalsOfContinuousStatesChanged, fmi3Boolean* valuesOfContinuousStatesChanged, fmi3Boolean* nextEventTimeDefined, fmi3Float64* nextEventTime) { return fmi3Error; }

// Interval related functions
FMI3_Export fmi3Status fmi3GetIntervalDecimal(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, fmi3Float64 intervals[], fmi3IntervalQualifier q[]) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetIntervalFraction(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, fmi3UInt64 counters[], fmi3UInt64 res[], fmi3IntervalQualifier q[]) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetShiftDecimal(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, fmi3Float64 shifts[]) { return fmi3Error; }
FMI3_Export fmi3Status fmi3GetShiftFraction(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, fmi3UInt64 counters[], fmi3UInt64 res[]) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetIntervalDecimal(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, const fmi3Float64 intervals[]) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetIntervalFraction(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, const fmi3UInt64 counters[], const fmi3UInt64 res[]) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetShiftDecimal(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, const fmi3Float64 shifts[]) { return fmi3Error; }
FMI3_Export fmi3Status fmi3SetShiftFraction(fmi3Instance instance, const fmi3ValueReference vrs[], size_t n, const fmi3UInt64 counters[], const fmi3UInt64 res[]) { return fmi3Error; }

} // extern "C"