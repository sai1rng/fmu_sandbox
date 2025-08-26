from fmpy import simulate_fmu
from fmpy.util import plot_result
import os
import numpy as np

# --- Configuration ---
# CORRECTED: Path to the wrapper for Amplifier.fmu
WRAPPER_FMU_PATH = 'Amplifier_FaultWrapper.fmu'

# Simulation parameters
start_time = 0.0
stop_time = 10.0
step_size = 0.1

# --- Main Simulation Script ---
def main():
    if not os.path.exists(WRAPPER_FMU_PATH):
        print(f"Error: Wrapper FMU '{WRAPPER_FMU_PATH}' not found.")
        print("Please run 'python create_wrapper_fmu.py' first to build it.")
        return

    print(f"Simulating wrapper FMU: {WRAPPER_FMU_PATH}")

    # CORRECTED: Define inputs and outputs for the GainBlock.fmu
    # The GainBlock model has one input 'u' (vr=0) and one output 'y' (vr=1).
    # We will provide a sine wave as the input signal to see the effects clearly.
    time = np.arange(start_time, stop_time, step_size)
    u_signal = np.sin(time * np.pi)
    input_signal = np.column_stack((time, u_signal))

    # Define which output variables we want to record
    output = ['u', 'y']

    # Use fmpy's high-level simulate_fmu function.
    # It handles the entire simulation lifecycle.
    result = simulate_fmu(
        WRAPPER_FMU_PATH,
        start_time=start_time,
        stop_time=stop_time,
        step_size=step_size,
        input=input_signal, # Provide the time-varying input signal
        output=output,
        # For GainBlock, the parameter 'k' will use its default start value (2.0)
    )

    print("Simulation finished.")

    # Plot results to observe the effect of the faults
    plot_result(result,
                window_title=f"Simulation of {WRAPPER_FMU_PATH}",
                events=True) # Show events in the plot

if __name__ == "__main__":
    main()