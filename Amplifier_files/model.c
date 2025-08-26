#include <stdlib.h>
#include "./fmi2Functions.h" // The official FMI header

// --- Model Data Structure ---
typedef struct {
    double u; // Input
    double y; // Output
    double k; // Parameter (the gain)
} ModelData;

// --- Value References ---
#define VR_U 0
#define VR_Y 1
#define VR_K 2

// --- FMI API Implementation ---

// "Constructor"
fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID,
                               fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions,
                               fmi2Boolean visible, fmi2Boolean loggingOn) {
    ModelData* model = (ModelData*)malloc(sizeof(ModelData));
    if (!model) return NULL;
    model->u = 0.0;
    model->y = 0.0;
    model->k = 2.0; // Default gain value
    return model;
}

// "Destructor"
void fmi2FreeInstance(fmi2Component c) {
    free(c);
}

// --- Get/Set for ALL data types ---
fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    ModelData* model = (ModelData*)c;
    for (size_t i = 0; i < nvr; i++) {
        if (vr[i] == VR_U) model->u = value[i];
        else if (vr[i] == VR_K) model->k = value[i];
    }
    return fmi2OK;
}

fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    ModelData* model = (ModelData*)c;
    for (size_t i = 0; i < nvr; i++) {
        if (vr[i] == VR_U) value[i] = model->u;
        else if (vr[i] == VR_Y) value[i] = model->y;
        else if (vr[i] == VR_K) value[i] = model->k;
    }
    return fmi2OK;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) { return fmi2Error; }
fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) { return fmi2Error; }
fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) { return fmi2Error; }
fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) { return fmi2Error; }
fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) { return fmi2Error; }
fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) { return fmi2Error; }


// --- Core simulation and lifecycle functions ---
fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint,
                      fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    ModelData* model = (ModelData*)c;
    model->y = model->k * model->u; // The actual model equation
    return fmi2OK;
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                                fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) { return fmi2OK; }
fmi2Status fmi2EnterInitializationMode(fmi2Component c) { return fmi2OK; }
fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    fmi2DoStep(c, 0, 0, fmi2True);
    return fmi2OK;
}
fmi2Status fmi2Terminate(fmi2Component c) { return fmi2OK; }
fmi2Status fmi2Reset(fmi2Component c) { return fmi2OK; }


// --- Complete FMI 2.0 API Stub Implementation ---

// These two functions have special signatures (no fmi2Component argument)
const char* fmi2GetTypesPlatform(void) { return fmi2TypesPlatform; }
const char* fmi2GetVersion(void) { return fmi2Version; }

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn,
                               size_t nCategories, const fmi2String categories[]) { return fmi2OK; }

// State management and serialization
fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) { return fmi2Error; }
fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate FMUstate) { return fmi2Error; }
fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) { return fmi2Error; }
fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t* size) { return fmi2Error; } // <-- ADDED THIS FUNCTION
fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size) { return fmi2Error; }
fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) { return fmi2Error; }

// Other advanced features
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[], size_t nKnown,
                                        const fmi2Real dvKnown[], fmi2Real dvUnknown[]) { return fmi2Error; }
fmi2Status fmi2CancelStep(fmi2Component c) { return fmi2Error; }
fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                        const fmi2Integer order[], fmi2Real value[]) { return fmi2Error; }
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                       const fmi2Integer order[], const fmi2Real value[]) { return fmi2Error; }
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* value) { return fmi2Error; }
fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* value) { return fmi2Error; }
fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value) { return fmi2Error; }
fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value) { return fmi2Error; }
fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String* value) { return fmi2Error; }