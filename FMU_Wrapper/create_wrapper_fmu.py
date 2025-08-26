import os
import subprocess
import sys

# --- Configuration ---
WRAPPER_SLAVE_SCRIPT = "wrapper_slave.py"

# UPDATED: Use the new FMU name
ORIGINAL_FMU = "Amplifier.fmu"

FAULT_CONFIG = "fault_config.json"

# UPDATED: Output name reflects the new wrapped FMU
OUTPUT_FMU_NAME = "Amplifier_fault_wrapper.fmu"

def build_wrapper():
    """
    Automates the process of building a self-contained wrapper FMU.
    """
    print("--- Starting FMU Wrapper Build Process ---")

    # 1. Check for required files
    required_files = [WRAPPER_SLAVE_SCRIPT, ORIGINAL_FMU, FAULT_CONFIG]
    for f in required_files:
        if not os.path.exists(f):
            print(f"Error: Required file not found: {f}")
            print("Please ensure all source files are in the same directory.")
            sys.exit(1)

    # 2. Define the build command for pythonfmu
    # --- FIX FOR Unrecognized Arguments Error ---
    # This updated syntax is for older versions of pythonfmu. It removes
    # the unsupported '--add-rpath' and '--project-file' arguments and
    # simply lists the resource files to be included.
    build_command = [
        "pythonfmu", "build",
        "-f", WRAPPER_SLAVE_SCRIPT,
        ORIGINAL_FMU,
        FAULT_CONFIG
    ]

    print(f"\nRunning command: {' '.join(build_command)}")

    # 3. Execute the build process
    try:
        result = subprocess.run(build_command, check=True, capture_output=True, text=True)
        print("\n--- STDOUT ---")
        print(result.stdout)
        if result.stderr:
            print("\n--- STDERR ---")
            print(result.stderr)

    except FileNotFoundError:
        print("\n--- Build Failed! ---")
        print("Error: 'pythonfmu' command not found.")
        print("Please ensure you have installed pythonfmu: pip install pythonfmu")
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        print("\n--- Build Failed! ---")
        print("Error building the FMU with pythonfmu.")
        print(f"Return Code: {e.returncode}")
        print("\n--- STDOUT ---")
        print(e.stdout)
        print("\n--- STDERR ---")
        print(e.stderr)
        sys.exit(1)

    # 4. Rename the output file to our desired name
    default_output_name = "FMUWrapper.fmu" 
    if os.path.exists(default_output_name):
        if os.path.exists(OUTPUT_FMU_NAME):
            os.remove(OUTPUT_FMU_NAME)
        os.rename(default_output_name, OUTPUT_FMU_NAME)
        print(f"\nSuccessfully renamed '{default_output_name}' to '{OUTPUT_FMU_NAME}'")
    else:
        print(f"Warning: Expected output file '{default_output_name}' not found.")

    print("\n--- Build Process Finished Successfully! ---")
    print(f"Your wrapper FMU is ready: {OUTPUT_FMU_NAME}")


if __name__ == "__main__":
    build_wrapper()