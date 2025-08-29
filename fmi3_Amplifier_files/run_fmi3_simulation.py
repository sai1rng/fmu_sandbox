import numpy as np
from fmpy import simulate_fmu
from fmpy.util import plot_result
import os

# --- Configuration ---
FMU_PATH = 'Amplifier_FMI3.fmu'

# Simulation parameters
start_time = 0.0
stop_time = 10.0
step_size = 0.1

# --- Main Simulation Script ---
def main():
    if not os.path.exists(FMU_PATH):
        print(f"Error: FMU '{FMU_PATH}' not found.")
        print("Please run './build.sh' first to build it.")
        return

    print(f"Simulating FMI 3.0 FMU: {FMU_PATH}")

    # 1. Define the time vector and input signal values
    time = np.arange(start_time, stop_time, step_size)
    u_signal = np.sin(time * np.pi)

    # 2. Define the data type for the structured input array
    input_dtype = np.dtype([('time', np.double), ('u', np.double)])

    # 3. Create the structured array for the input signal
    input_signal = np.zeros(len(time), dtype=input_dtype)
    input_signal['time'] = time
    input_signal['u'] = u_signal

    # Define which output variables to record
    output = ['u', 'y', 'k']

    result = simulate_fmu(
        FMU_PATH,
        start_time=start_time,
        stop_time=stop_time,
        input=input_signal,
        output=output,
    )

    print("Simulation finished.")
    plot_result(result, window_title=f"Simulation of {FMU_PATH}")

if __name__ == "__main__":
    main()