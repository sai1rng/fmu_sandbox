from fmpy import simulate_fmu
from fmpy.util import plot_result
import os
import numpy as np

# --- Configuration ---
WRAPPER_FMU_PATH = 'Amplifier_CPP_Wrapper.fmu'

# Simulation parameters
start_time = 0.0
stop_time = 10.0
step_size = 0.1

# --- Main Simulation Script ---
def main():
    if not os.path.exists(WRAPPER_FMU_PATH):
        print(f"Error: Wrapper FMU '{WRAPPER_FMU_PATH}' not found.")
        print("Please run 'bash C_Wrapper/build.sh' first to build it.")
        return

    print(f"Simulating C-based wrapper FMU: {WRAPPER_FMU_PATH}")

    # 1. Define the time vector and signal values
    time = np.arange(start_time, stop_time, step_size)
    u_signal = np.sin(time * np.pi)

    # 2. Define the data type for the structured array
    input_dtype = np.dtype([('time', np.double), ('u', np.double)])

    # 3. Create the structured array
    input_signal = np.zeros(len(time), dtype=input_dtype)
    input_signal['time'] = time
    input_signal['u'] = u_signal

    output = ['u', 'y']

    result = simulate_fmu(
        WRAPPER_FMU_PATH,
        start_time=start_time,
        stop_time=stop_time,
        input=input_signal,
        output=output,
    )

    print("Simulation finished.")
    plot_result(result, window_title=f"Simulation of {WRAPPER_FMU_PATH}")

if __name__ == "__main__":
    main()
