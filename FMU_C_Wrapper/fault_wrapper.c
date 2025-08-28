#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmi2Functions.h"

// Platform-specific headers and functions for dynamic library loading
#ifdef _WIN32
#include <windows.h>
#define DLL_HANDLE HMODULE
#define LOAD_LIBRARY(path) LoadLibrary(path)
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

// --- Value References for this wrapper FMU ---
// These match the inner FMU for simplicity
#define VR_U 0
#define VR_Y 1
#define VR_K 2

// --- Hardcoded Fault Definition ---
// For simplicity, we hardcode one fault instead of parsing a JSON file.
// Fault: Add an offset of 0.5 to the input 'u' between t=3s and t=7s.
#define FAULT_VR 0 // VR of the variable to apply fault on (u)
#define FAULT_START_TIME 3.0
#define FAULT_END_TIME 7.0
#define FAULT_TYPE_OFFSET
#define FAULT_VALUE 0.5

// --- Inner FMU Function Pointers ---
typedef struct {
    fmi2InstantiateTYPE*            Instantiate;
    fmi2FreeInstanceTYPE*           FreeInstance;
    fmi2SetupExperimentTYPE*        SetupExperiment;
    fmi2EnterInitializationModeTYPE* EnterInitializationMode;
    fmi2ExitInitializationModeTYPE*  ExitInitializationMode;
    fmi2TerminateTYPE*              Terminate;
    fmi2ResetTYPE*                  Reset;
    fmi2GetRealTYPE*                GetReal;
    fmi2SetRealTYPE*                SetReal;
    fmi2DoStepTYPE*                 DoStep;
} InnerFMU;

// --- Model Data Structure ---
typedef struct {
    // Wrapper's own variable values
    double u;
    double y;
    double k;
    double currentTime;

    // Inner FMU handles
    DLL_HANDLE innerFMUHandle;
    fmi2Component innerFMUInstance;
    InnerFMU functions;
    const fmi2CallbackFunctions* callbacks;
    fmi2String instanceName;

} ModelData;

// Helper to load all function pointers from the inner FMU
int loadInnerFmuFunctions(ModelData* model) {
    #define LOAD_FUNC(Name) \
        model->functions.Name = (fmi2##Name##TYPE*)GET_FUNCTION(model->innerFMUHandle, "fmi2" #Name); \
        if (!model->functions.Name) { \
            model->callbacks->logger(NULL, model->instanceName, fmi2Error, "error", "Failed to load function: fmi2" #Name); \
            return 0; \
        }

    LOAD_FUNC(Instantiate);
    LOAD_FUNC(FreeInstance);
    LOAD_FUNC(SetupExperiment);
    LOAD_FUNC(EnterInitializationMode);
    LOAD_FUNC(ExitInitializationMode);
    LOAD_FUNC(Terminate);
    LOAD_FUNC(Reset);
    LOAD_FUNC(GetReal);
    LOAD_FUNC(SetReal);
    LOAD_FUNC(DoStep);

    return 1;
}

// --- FMI API Implementation ---

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID,
                               fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions,
                               fmi2Boolean visible, fmi2Boolean loggingOn) {

    if (!functions || !functions->logger || !functions->allocateMemory) return NULL;

    ModelData* model = (ModelData*)functions->allocateMemory(1, sizeof(ModelData));
    if (!model) return NULL;

    // Store callbacks and instance name
    model->callbacks = functions;
    model->instanceName = instanceName;

    // --- 1. Locate and load the inner FMU's shared library ---
    // The fmuResourceLocation is a URI (e.g., "file:///path/to/resources").
    // Dynamic library loaders (dlopen, LoadLibrary) need a file system path, not a URI.
    const char* resourcePath = fmuResourceLocation;
    const char* scheme = "file://";
    if (strncmp(resourcePath, scheme, strlen(scheme)) == 0) {
        resourcePath += strlen(scheme);
        #ifdef _WIN32
        // On Windows, the path can be /C:/... so we must skip the leading '/'
        if (resourcePath[0] == '/' && resourcePath[2] == ':') {
            resourcePath++;
        }
        #endif
    }

    char innerFmuPath[1024];
    const char* platform = "linux64";
    const char* lib_ext = ".so";
    #ifdef _WIN32
        platform = "win64";
        lib_ext = ".dll";
    #elif __APPLE__
        platform = "darwin64";
        lib_ext = ".dylib";
    #endif

    // Construct the full path to the inner FMU's binary.
    // The inner FMU is unzipped by the build script into "resources/Amplifier".
    snprintf(innerFmuPath, sizeof(innerFmuPath), "%s" SEP "Amplifier" SEP "binaries" SEP "%s" SEP "model%s", resourcePath, platform, lib_ext);

    model->innerFMUHandle = LOAD_LIBRARY(innerFmuPath);
    if (!model->innerFMUHandle) {
        functions->logger(NULL, instanceName, fmi2Fatal, "error", "Could not load inner FMU binary: %s", innerFmuPath);
        functions->freeMemory(model);
        return NULL;
    }

    // --- 2. Load function pointers ---
    if (!loadInnerFmuFunctions(model)) {
        FREE_LIBRARY(model->innerFMUHandle);
        functions->freeMemory(model);
        return NULL;
    }

    // --- 3. Instantiate the inner FMU ---
    // GUID and modelIdentifier are from the *inner* FMU's modelDescription.xml
    // The resource path passed to the inner FMU must be a URI, so we use the original fmuResourceLocation.
    const char* innerGuid = "{8c4e810f-3df3-4a00-8276-176fa3c9f000}";
    char innerResourcePath[1024];
    snprintf(innerResourcePath, sizeof(innerResourcePath), "%s" SEP "Amplifier" SEP "resources", fmuResourceLocation);

    model->innerFMUInstance = model->functions.Instantiate("innerAmplifier", fmuType, innerGuid,
                                                            innerResourcePath, functions, visible, loggingOn);
    if (!model->innerFMUInstance) {
        functions->logger(NULL, instanceName, fmi2Fatal, "error", "Failed to instantiate inner FMU.");
        FREE_LIBRARY(model->innerFMUHandle);
        functions->freeMemory(model);
        return NULL;
    }

    // --- 4. Initialize wrapper state ---
    model->u = 0.0;
    model->y = 0.0;
    model->k = 2.0; // Default gain
    model->currentTime = 0.0;

    return model;
}

void fmi2FreeInstance(fmi2Component c) {
    if (!c) return;
    ModelData* model = (ModelData*)c;
    if (model->innerFMUInstance) {
        model->functions.Terminate(model->innerFMUInstance);
        model->functions.FreeInstance(model->innerFMUInstance);
    }
    if (model->innerFMUHandle) {
        FREE_LIBRARY(model->innerFMUHandle);
    }
    model->callbacks->freeMemory(model);
}

// --- Get/Set for wrapper variables ---
fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    ModelData* model = (ModelData*)c;
    for (size_t i = 0; i < nvr; i++) {
        if (vr[i] == VR_U) model->u = value[i];
        else if (vr[i] == VR_K) model->k = value[i];
        // 'y' is an output, cannot be set
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

// --- Simulation and lifecycle functions (delegating to inner FMU) ---

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                                fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {
    ModelData* model = (ModelData*)c;
    model->currentTime = startTime;
    return model->functions.SetupExperiment(model->innerFMUInstance, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    ModelData* model = (ModelData*)c;
    return model->functions.EnterInitializationMode(model->innerFMUInstance);
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    ModelData* model = (ModelData*)c;
    // Set initial parameters on the inner FMU
    fmi2ValueReference vr_k = VR_K;
    model->functions.SetReal(model->innerFMUInstance, &vr_k, 1, &model->k);
    return model->functions.ExitInitializationMode(model->innerFMUInstance);
}

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint,
                      fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    ModelData* model = (ModelData*)c;
    model->currentTime = currentCommunicationPoint;

    double u_to_set = model->u;

    // --- Fault Injection Logic ---
    if (model->currentTime >= FAULT_START_TIME && model->currentTime < FAULT_END_TIME) {
        #ifdef FAULT_TYPE_OFFSET
        u_to_set += FAULT_VALUE;
        #endif
        // Could add other fault types like "stuckAtValue" here
    }

    // 1. Set (possibly faulty) inputs on the inner FMU
    fmi2ValueReference vr_u = VR_U;
    fmi2Status status = model->functions.SetReal(model->innerFMUInstance, &vr_u, 1, &u_to_set);
    if (status != fmi2OK) return status;

    // 2. Advance the inner FMU
    status = model->functions.DoStep(model->innerFMUInstance, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
    if (status != fmi2OK) return status;

    // 3. Get outputs from the inner FMU
    fmi2ValueReference vr_y = VR_Y;
    status = model->functions.GetReal(model->innerFMUInstance, &vr_y, 1, &model->y);
    return status;
}

fmi2Status fmi2Terminate(fmi2Component c) {
    ModelData* model = (ModelData*)c;
    return model->functions.Terminate(model->innerFMUInstance);
}

fmi2Status fmi2Reset(fmi2Component c) {
    ModelData* model = (ModelData*)c;
    return model->functions.Reset(model->innerFMUInstance);
}

// --- Unused FMI Functions ---
const char* fmi2GetTypesPlatform(void) { return fmi2TypesPlatform; }
const char* fmi2GetVersion(void) { return fmi2Version; }
fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean l, size_t n, const fmi2String cat[]) { return fmi2OK; }
fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) { return fmi2Error; }
fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) { return fmi2Error; }
fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) { return fmi2Error; }
fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) { return fmi2Error; }
fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) { return fmi2Error; }
fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) { return fmi2Error; }
fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* s) { return fmi2Error; }
fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate s) { return fmi2Error; }
fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* s) { return fmi2Error; }
fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate s, size_t* z) { return fmi2Error; }
fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate s, fmi2Byte z[], size_t Z) { return fmi2Error; }
fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte z[], size_t Z, fmi2FMUstate* s) { return fmi2Error; }
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference u[], size_t nu, const fmi2ValueReference z[], size_t nz, const fmi2Real dz[], fmi2Real du[]) { return fmi2Error; }

// --- Missing Co-simulation functions ---
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], const fmi2Real value[]) { return fmi2Error; }
fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], fmi2Real value[]) { return fmi2Error; }
fmi2Status fmi2CancelStep(fmi2Component c) { return fmi2OK; }
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* value) { return fmi2Error; }
fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* value) { return fmi2Error; }
fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value) { return fmi2Error; }
fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value) { return fmi2Error; }
fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String* value) { return fmi2Error; }
