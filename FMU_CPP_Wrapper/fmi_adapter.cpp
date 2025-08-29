/**
 * @file fmi_adapter.cpp
 * @brief Provides the required C-style FMI 2.0 interface.
 *
 * This file acts as a thin adapter layer that translates the C-style FMI function calls
 * from the simulation environment into method calls on an instance of the C++ FaultWrapper class.
 */
#include "FaultWrapper.hpp"

// The FMI standard requires a C interface, so all functions must be declared `extern "C"`.
extern "C" {

/**
 * @brief A helper to safely cast the opaque fmi2Component pointer back to a FaultWrapper pointer.
 */
static inline FaultWrapper* to_wrapper(fmi2Component c) {
    return static_cast<FaultWrapper*>(c);
}

/**
 * @brief The FMI instantiation function.
 *
 * It allocates memory using the simulator's provided callbacks and uses "placement new"
 * to construct a FaultWrapper object in that memory. This ensures the simulator manages
 * the memory lifecycle.
 */
FMI2_Export fmi2Component fmi2Instantiate(fmi2String i, fmi2Type t, fmi2String g, fmi2String r, const fmi2CallbackFunctions* f, fmi2Boolean v, fmi2Boolean l) {
    if (!f || !f->logger || !f->allocateMemory) return nullptr;
    try {
        // Allocate memory using the simulator's allocator.
        void* mem = f->allocateMemory(1, sizeof(FaultWrapper));
        if (!mem) {
            f->logger(nullptr, i, fmi2Fatal, "error", "Failed to allocate memory for wrapper instance.");
            return nullptr;
        }
        // Construct the object in the allocated memory (placement new).
        return new (mem) FaultWrapper(i, r, f, v, l);
    } catch (const std::exception& e) {
        f->logger(nullptr, i, fmi2Fatal, "error", e.what());
        return nullptr;
    }
}

/**
 * @brief The FMI destruction function.
 *
 * It explicitly calls the destructor of the FaultWrapper object and then uses the
 * simulator's callback to free the memory.
 */
FMI2_Export void fmi2FreeInstance(fmi2Component c) {
    if (!c) return;
    FaultWrapper* wrapper = to_wrapper(c);
    const fmi2CallbackFunctions* callbacks = wrapper->getCallbacks();
    wrapper->~FaultWrapper(); // Explicitly call the destructor.
    callbacks->freeMemory(wrapper);
}

// --- Simple Delegation Functions ---
FMI2_Export fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real v[]) { return to_wrapper(c)->getReal(vr, nvr, v); }
FMI2_Export fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real v[]) { return to_wrapper(c)->setReal(vr, nvr, v); }
FMI2_Export fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean td, fmi2Real t, fmi2Real st, fmi2Boolean spd, fmi2Real sp) { return to_wrapper(c)->setupExperiment(td, t, st, spd, sp); }
FMI2_Export fmi2Status fmi2EnterInitializationMode(fmi2Component c) { return to_wrapper(c)->enterInitializationMode(); }
FMI2_Export fmi2Status fmi2ExitInitializationMode(fmi2Component c) { return to_wrapper(c)->exitInitializationMode(); }
FMI2_Export fmi2Status fmi2DoStep(fmi2Component c, fmi2Real cp, fmi2Real cs, fmi2Boolean ns) { return to_wrapper(c)->doStep(cp, cs, ns); }
FMI2_Export fmi2Status fmi2Terminate(fmi2Component c) { return to_wrapper(c)->terminate(); }

// --- Stub Functions for Unused FMI 2.0 API Calls ---
// These functions are required to be present by the FMI standard, but are not
// needed for this specific wrapper. They simply return an appropriate status
// to indicate that the functionality is not implemented.
FMI2_Export const char* fmi2GetTypesPlatform(void) { return fmi2TypesPlatform; }
FMI2_Export const char* fmi2GetVersion(void) { return fmi2Version; }
FMI2_Export fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean l, size_t n, const fmi2String cat[]) { return fmi2OK; }
FMI2_Export fmi2Status fmi2Reset(fmi2Component c) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* s) { return fmi2Error; }
FMI2_Export fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate s) { return fmi2Error; }
FMI2_Export fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* s) { return fmi2Error; }
FMI2_Export fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate s, size_t* z) { return fmi2Error; }
FMI2_Export fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate s, fmi2Byte z[], size_t Z) { return fmi2Error; }
FMI2_Export fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte z[], size_t Z, fmi2FMUstate* s) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference u[], size_t nu, const fmi2ValueReference z[], size_t nz, const fmi2Real dz[], fmi2Real du[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer o[], const fmi2Real v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer o[], fmi2Real v[]) { return fmi2Error; }
FMI2_Export fmi2Status fmi2CancelStep(fmi2Component c) { return fmi2OK; }
FMI2_Export fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* v) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* v) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* v) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* v) { return fmi2Error; }
FMI2_Export fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String* v) { return fmi2Error; }

} // extern "C"