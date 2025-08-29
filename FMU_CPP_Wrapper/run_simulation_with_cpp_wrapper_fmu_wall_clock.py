from fmpy import read_model_description, extract
from fmpy.fmi2 import FMU2Slave
from fmpy.util import plot_result
import os
import numpy as np
import time as wall_clock

# --- Configuration ---
WRAPPER_FMU_PATH = 'Amplifier_CPP_Wrapper.fmu'

# Simulation parameters
start_time = 0.0
stop_time = 100.0
step_size = 0.1

# --- Main Simulation Script ---
def main():
    if not os.path.exists(WRAPPER_FMU_PATH):
        print(f"Error: FMU '{WRAPPER_FMU_PATH}' not found. Please build it first.")
        return

    print(f"Simulating FMU in real-time: {WRAPPER_FMU_PATH}")

    # 1. Read model description and extract the FMU
    model_description = read_model_description(WRAPPER_FMU_PATH)
    unzipdir = extract(WRAPPER_FMU_PATH)

    # Get value references for variables we want to interact with
    vrs = {variable.name: variable.valueReference for variable in model_description.modelVariables}
    vr_u = vrs['u']
    vr_y = vrs['y']

    # 2. Instantiate the FMU slave
    fmu = FMU2Slave(guid=model_description.guid,
                    unzipDirectory=unzipdir,
                    modelIdentifier=model_description.coSimulation.modelIdentifier,
                    instanceName='instance1')

    # 3. Setup and Initialize the FMU
    fmu.instantiate()
    fmu.setupExperiment(startTime=start_time, stopTime=stop_time)
    fmu.enterInitializationMode()
    fmu.exitInitializationMode()

    # 4. Real-time simulation loop
    sim_time = start_time
    results = []

    print(f"Starting real-time simulation for {stop_time} seconds...")
    # Get the real-world start time
    real_world_start_time = wall_clock.perf_counter()

    while sim_time < stop_time:
        # Calculate the real time that should have elapsed since the loop started
        target_real_time = real_world_start_time + sim_time

        # Sleep until the target real time is reached
        sleep_duration = target_real_time - wall_clock.perf_counter()
        if sleep_duration > 0:
            wall_clock.sleep(sleep_duration)

        # Set inputs for the current step
        u_signal_value = np.sin(sim_time * np.pi)
        fmu.setReal([vr_u], [u_signal_value])

        # Advance the simulation by one step
        fmu.doStep(currentCommunicationPoint=sim_time, communicationStepSize=step_size)

        # Get outputs from the FMU
        output_y = fmu.getReal([vr_y])[0]

        # Record results for plotting
        results.append((sim_time, u_signal_value, output_y))

        # Advance simulation time
        sim_time += step_size

    # 5. Terminate and free the FMU instance
    fmu.terminate()
    fmu.freeInstance()

    print("Simulation finished.")

    # Convert results to a structured NumPy array for plotting
    result_array = np.array(results, dtype=np.dtype([('time', np.double), ('u', np.double), ('y', np.double)]))
    plot_result(result_array, window_title=f"Real-Time Simulation of {WRAPPER_FMU_PATH}")

if __name__ == "__main__":
    main()
