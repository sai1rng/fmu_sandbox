#!/bin/bash

set -e

FMU_NAME="Amplifier_FMI3"
CPP_SOURCE="fmi3_amplifier.cpp"
XML_DESCRIPTION="modelDescription.xml"
HEADER_DIR="." # Assumes fmi3*.h files are in the same directory

echo "--- Starting FMI 3.0 Amplifier FMU Build Process ---"

# 1. Check for required tools
command -v g++ >/dev/null 2>&1 || { echo >&2 "Build failed: 'g++' not found."; exit 1; }
command -v zip >/dev/null 2>&1 || { echo >&2 "Build failed: 'zip' not found."; exit 1; }

# 2. Setup build directory
BUILD_DIR="build_temp_fmi3"
rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}/binaries

# 3. Determine platform and compile the model
PLATFORM_DIR=""
SHARED_LIB_EXT=""
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM_DIR="x86_64-linux"
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
g++ -shared -fPIC -std=c++17 -I"${HEADER_DIR}" "${CPP_SOURCE}" -o "${BUILD_DIR}/binaries/${PLATFORM_DIR}/fmi3_amplifier${SHARED_LIB_EXT}"
echo "Compilation successful."

# 4. Copy modelDescription.xml
cp "${XML_DESCRIPTION}" "${BUILD_DIR}/"

# 5. Create the final FMU zip archive
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
echo "Your FMI 3.0 FMU is ready: ${OUTPUT_FMU}"