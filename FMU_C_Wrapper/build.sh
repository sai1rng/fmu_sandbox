#!/bin/bash

set -e

FMU_NAME="Amplifier_C_Wrapper"
WRAPPER_C_SOURCE="fault_wrapper.c"
WRAPPER_XML="modelDescription.xml"
ORIGINAL_FMU="../Amplifier.fmu"

echo "--- Starting C Wrapper FMU Build Process ---"

# 1. Check for required tools
command -v gcc >/dev/null 2>&1 || { echo >&2 "Build failed: 'gcc' not found."; exit 1; }
command -v zip >/dev/null 2>&1 || { echo >&2 "Build failed: 'zip' not found."; exit 1; }
command -v unzip >/dev/null 2>&1 || { echo >&2 "Build failed: 'unzip' not found."; exit 1; }

# 2. Setup build directory
BUILD_DIR="build_temp"
rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}/binaries
mkdir -p ${BUILD_DIR}/resources

# 3. Determine platform and compile wrapper
PLATFORM_DIR=""
SHARED_LIB_EXT=""
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM_DIR="linux64"
    SHARED_LIB_EXT=".so"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM_DIR="darwin64"
    SHARED_LIB_EXT=".dylib"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    PLATFORM_DIR="win64"
    SHARED_LIB_EXT=".dll"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

mkdir -p "${BUILD_DIR}/binaries/${PLATFORM_DIR}"

echo "Compiling for platform: ${PLATFORM_DIR}"
gcc -shared -fPIC -I"../Amplifier_files/headers" "${WRAPPER_C_SOURCE}" -o "${BUILD_DIR}/binaries/${PLATFORM_DIR}/fault_wrapper${SHARED_LIB_EXT}"
echo "Compilation successful."

# 4. Copy wrapper modelDescription.xml
cp "${WRAPPER_XML}" "${BUILD_DIR}/"

# 5. Unpack the original FMU into the resources directory
echo "Unpacking original FMU into resources..."
unzip -q "${ORIGINAL_FMU}" -d "${BUILD_DIR}/resources/Amplifier"

# 6. Create the final FMU zip archive
OUTPUT_FMU="../${FMU_NAME}.fmu"
if [ -f "${OUTPUT_FMU}" ]; then
    rm "${OUTPUT_FMU}"
fi

echo "Creating FMU archive: ${OUTPUT_FMU}"
cd ${BUILD_DIR}
zip -r "${OUTPUT_FMU}" .
cd ..

rm -rf ${BUILD_DIR}

echo "--- Build Process Finished Successfully! ---"
echo "Your wrapper FMU is ready: ${OUTPUT_FMU}"

