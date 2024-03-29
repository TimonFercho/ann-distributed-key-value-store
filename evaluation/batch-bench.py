import json
import os
from datetime import datetime
from tqdm import tqdm
import sys


def read_benchmarks():
    with open(os.path.join('evaluation', 'benchmarks.json')) as f:
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
    cmd += f" --benchmark-samples {params['TEST_N_SAMPLES']}"
    cmd += f" -r {params['reporter']}"
    return cmd


def get_filename(params):
    EXCLUDE_KEYS = ['DEBUG']
    directory = os.path.join('evaluation', 'benchmarks')
    if params['subdir']:
        directory = os.path.join(directory, params['subdir'])
        os.makedirs(directory, exist_ok=True)
    filename = f'{params["catch_tags"]} '
    filename += ' '.join([
        f"{key}={value}" for key, value in sorted(params.items())
        if (key not in EXCLUDE_KEYS and key[0].isupper()) or key[:4] == 'env_'
    ])
    filename += f'.{params["reporter"]}'
    with open('evaluation/parameter_abbreviations.json') as f:
        abbreviations = json.load(f)
    for key, value in abbreviations.items():
        filename = filename.replace(key, value)

    return os.path.join(directory, filename)


if os.path.basename(os.getcwd()) == 'evaluation':
    os.chdir('..')

benchmarks = read_benchmarks()

default_params = benchmarks['defaults']
benchmarks['benchmarks'] = [b for b in benchmarks['benchmarks'] if b['run']]

dry_run = False
if len(sys.argv) > 1:
    dry_run = sys.argv[1] == 'dry'

for benchmark in tqdm(benchmarks['benchmarks']):
    params = default_params.copy()
    params.update(benchmark)
    filename = get_filename(params)
    if dry_run:
        print('=====================')
    print(filename)

    print()
    make_cmd = get_make_command(params)
    print(make_cmd)
    if not dry_run:
        os.system(make_cmd)

    if not dry_run and not os.path.exists(params['executable']):
        print(f"Executable {params['executable']} does not exist.")
        exit(1)

    print()
    envs = get_environment_variables(params)
    benchmark_cmd = get_benchmark_command(params)
    if params['reporter'] == 'xml':
        envs_benchmark_cmd = f'export{envs}; {benchmark_cmd} >> "{filename}"'
    elif params['reporter'] == 'console':
        envs_benchmark_cmd = f'export{envs}; {benchmark_cmd}'
    else:
        raise ValueError(f"Unknown reporter {params['reporter']}")
    print(envs_benchmark_cmd)
    if not dry_run:
        os.system(envs_benchmark_cmd)
