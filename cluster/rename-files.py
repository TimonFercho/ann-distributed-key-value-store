# get all files in all subdirectories of the tmp directory
# they have the format SIFT<datasetsize>M_<number>_of_<total>.index
# rename them to SIFT<datasetsize>M_1024_<number>_of_<total>.index

import os
import sys

def rename_files(path):
    for root, dirs, files in os.walk(path):
        for file in files:
            if file.endswith(".index"):
                print(os.path.join(root, file))
                print(file)
                newfile = file.replace('M_', 'M_1024_')
                print(newfile)
                os.rename(os.path.join(root, file), os.path.join(root, newfile))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python rename-files.py <path>")
        sys.exit(1)
    rename_files(sys.argv[1])