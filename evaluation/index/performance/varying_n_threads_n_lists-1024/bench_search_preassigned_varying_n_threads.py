# Script that executes the search preassigned benchmark varying the number of threads

import os
import subprocess

OUTPUT_DIR = "evaluation/index/performance/varying_n_threads"

for n_threads in [16, 32, 64, 128, 256, 512]:
    output_filename = os.path.join(
        OUTPUT_DIR,
        f"search_preassigned_simd_omp_pmode2_{n_threads}threads.xml")
    with open(output_filename, "a") as xml_file:
        os.environ["OMP_NUM_THREADS"] = str(n_threads)
        subprocess.call([
            "./out/testmain", "[search_preassigned][SIFT1M][benchmark]",
            "--benchmark-samples", "3", "-r", "xml"
        ],
                        env=os.environ,
                        stdout=xml_file)
