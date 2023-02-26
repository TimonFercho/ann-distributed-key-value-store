import json
import os
from datetime import datetime
from tqdm import tqdm
import sys


def read_benchmarks():
    with open('benchmarks.json') as f:
        benchmarks = json.load(f)
    return benchmarks


def get_make_command(params):
    cmd = params['make_command']
    for key, value in params.items():
        if key[0].isupper():
            cmd += f" {key}={value}"
    return cmd


def get_environment_variables(params):
    envs = ''
    for key, value in params.items():
        if key[:4] == 'env_':
            key = key[4:]
            envs += f" {key}={value}"
    return envs


def get_benchmark_command(params):
    cmd = params['executable']
    cmd += f" {params['catch_tags']}"
    cmd += f" --benchmark-samples {params['benchmark-samples']}"
    cmd += f" -r {params['reporter']}"
    return cmd


def get_filename(params):
    EXCLUDE_KEYS = [
        'DEBUG', 'run', 'make_command', 'executable', 'reporter', 'catch_tags'
    ]
    directory = 'evaluation/benchmarks'
    filename = f'{params["catch_tags"]} '
    filename += ' '.join([
        f"{key}={value}" for key, value in sorted(params.items())
        if key not in EXCLUDE_KEYS
    ])
    filename += ".xml"
    return os.path.join(directory, filename)


benchmarks = read_benchmarks()

if os.path.basename(os.getcwd()) == 'evaluation':
    os.chdir('..')

default_params = benchmarks['defaults']
benchmarks['benchmarks'] = [b for b in benchmarks['benchmarks'] if b['run']]

# read first argument
dry_run = False
if len(sys.argv) > 1:
    dry_run = sys.argv[1] == 'dry'

for benchmark in tqdm(benchmarks['benchmarks']):
    params = default_params.copy()
    params.update(benchmark)
    print("=========================================")
    filename = get_filename(params)
    print(filename)
    make_cmd = get_make_command(params)
    if dry_run:
        print()
        print(make_cmd)
    else:
        os.system(make_cmd)

    envs = get_environment_variables(params)
    benchmark_cmd = get_benchmark_command(params)
    envs_benchmark_cmd = f'export{envs}; {benchmark_cmd} >> "{filename}"'
    if dry_run:
        print()
        print(envs_benchmark_cmd)
    else:
        os.system(envs_benchmark_cmd)
