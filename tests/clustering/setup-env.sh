#!/bin/bash
PYTHON_DIR="$(pyenv which python)"
CONDA_BIN="$(dirname "$PYTHON_DIR")"
CONDA_LIB="$(dirname "$CONDA_BIN")/lib"
LD_PRELOAD="$CONDA_LIB/libmkl_core.so:$CONDA_LIB/libmkl_sequential.so"
export LD_PRELOAD