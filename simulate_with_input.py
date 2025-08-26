import numpy as np
from fmpy import simulate_fmu
import matplotlib.pyplot as plt

# The name of your compiled FMU
fmu_filename = 'Amplifier.fmu'

# 1. Define the input signal
# Create a time array from 0 to 5 seconds with 501 points
time = np.linspace(0, 5, 501)
# Create a sine wave as the input for 'u'
u_signal = np.sin(time * 2 * np.pi)

# 2. Structure the input for FMPy
# The input must be a structured NumPy array with 'time' and variable names
input_data = np.array(list(zip(time, u_signal)),
                      dtype=[('time', np.double), ('u', np.double)])

# 3. Simulate the FMU with the defined input
# The 'input' argument tells fmpy to use your signal
result = simulate_fmu(fmu_filename,
                      stop_time=5.0,
                      input=input_data,
                      output=['u', 'y']) # We'll record both input and output

# 4. Plot the results
plt.figure(figsize=(10, 6))
plt.plot(result['time'], result['u'], label='Input (u) - Sine Wave')
plt.plot(result['time'], result['y'], label='Output (y) - Gain Applied', linewidth=2)
plt.title('FMU Simulation with Sine Wave Input')
plt.xlabel('Time [s]')
plt.ylabel('Value')
plt.grid(True)
plt.legend()
plt.show()