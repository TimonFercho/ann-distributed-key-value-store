from lxml import etree as ET
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from cycler import cycler
import matplotlib.ticker as ticker
from os.path import join
import os
import json

N_VECTORS = 1e6
ONE_MB = 2**20
FLOAT_SIZE_B = 4
VECTOR_DIM = 128
VECTOR_SIZE_B = VECTOR_DIM * FLOAT_SIZE_B
VECTOR_SIZE_MB = VECTOR_SIZE_B / ONE_MB
NS_PER_S = 1e9
S_PER_MIN = 60
MS_PER_S = 1e3
US_TO_NS = 1e3


# read benchmark parameters like `n_lists` and `n_probes` which are stored in a `Warning` tag (key := value) preceding the `BenchmarkResults` tag
def extract_benchmark_params(element):
    params = {}
    for sibling in element.itersiblings(preceding=True):
        if sibling.tag == 'Warning':
            parts = list(map(lambda s: s.strip(), sibling.text.split(':=')))
            params[parts[0]] = parts[1]
    return params


def get_abbreviation_to_param_map():
    with open('parameter_abbreviations.json') as f:
        abbreviations = json.load(f)
    return {v: k for k, v in abbreviations.items()}


def get_file_params(filename):
    params = {}
    abbr_to_param = get_abbreviation_to_param_map()
    # remove .xml  extension explicitly
    filename = filename[:-4]
    catch_tags, params_str = filename.split(' ', 1)
    params['catch_tags'] = catch_tags
    for part in params_str.split(' '):
        key, value = part.split('=')
        params[abbr_to_param.get(key, key)] = value
    return params


def parse_xml(subdir, filename, format='throughput'):
    directory = join('benchmarks', subdir) if subdir else 'benchmarks'
    filepath = join(directory, filename)
    tree = ET.parse(filepath)
    root = tree.getroot()
    results = []

    if format == 'throughput':
        for match in root.iter('BenchmarkResults'):
            result = extract_benchmark_params(match)
            result['mean'] = match.find('mean').get('value')
            result['std'] = match.find('standardDeviation').get('value')
            results.append(result)
    elif format == 'latency':
        for match in root.iter('Warning'):
            if not match.text.strip().startswith('latency_99th_std'):
                continue
            result = extract_benchmark_params(match)
            result['latency_99th_std'] = match.text.split(':=')[1].strip()
            results.append(result)
    elif format == 'recall':
        for match in root.iter('Warning'):
            if not match.text.strip().startswith('Recall'):
                continue
            result = extract_benchmark_params(match)
            result['recall@1'] = match.text.split(':=')[1].strip()
            results.append(result)
    else:
        raise ValueError('Unknown format: {}'.format(format))
    return results


def merge_results(subdir, filter_file, format):
    results = []
    for f in os.listdir(join('benchmarks', subdir)):
        if filter_file(f):
            params = get_file_params(f)
            try:
                parsed_xml = parse_xml(subdir, f, format=format)
            except Exception as e:
                print(f'Error parsing {f}: {e}')
                continue
            for result in parsed_xml:
                result.update(params)
                results.append(result)
    return results

def results_to_df(results, mode, dataset='SIFT1M'):
    df = pd.DataFrame(results)

    N_QUERIES = 10 ** 4

    if dataset == 'SIFT1M':
        N_VECTORS = 10 ** 6
    elif dataset == 'SIFT10M':
        N_VECTORS = 10 ** 7
    elif dataset == 'SIFT100M':
        N_VECTORS = 10 ** 8
    elif dataset in ['SIFT1000M', 'SIFT1B']:
        N_VECTORS = 10 ** 9
    else:
        raise ValueError('Unknown dataset: {}'.format(dataset))

    columns = []

    if 'O' in df.columns:
        df['O'] = df['O'].astype(int)
        columns += ['O']

    if 'env_OMP_SCHEDULE' in df.columns:
        df['OMP_SCHEDULE'] = df['env_OMP_SCHEDULE'].astype(str)
        columns += ['OMP_SCHEDULE']

    if 'max_n_threads' in df.columns:
        df['max_n_threads'] = df['max_n_threads'].astype(int)
        columns += ['max_n_threads']

    if 'DYNAMIC_INSERTION' in df.columns:
        # replace 0/1 with dynamic/dynamic

        df['INSERTION_STRATEGY'] = df['DYNAMIC_INSERTION'].replace({'0': 'preallocated', '1': 'dynamic'})

        # if df['DYNAMIC_INSERTION'] == '1':
        #     df['INSERTION_STRATEGY'] = 'dynamic'
        # elif df['DYNAMIC_INSERTION'] == '0':
        #     df['INSERTION_STRATEGY'] = 'preinitialized'
        # df['DYNAMIC_INSERTION'] = (df['DYNAMIC_INSERTION'] == '1').astype(bool)
        columns += ['INSERTION_STRATEGY']

    if 'MAX_BUFFER_SIZE' in df.columns:
        df['MAX_BUFFER_SIZE'] = df['MAX_BUFFER_SIZE'].astype(int)
        columns += ['MAX_BUFFER_SIZE']

    if 'n_lists' in df.columns:
        df['n_lists'] = df['n_lists'].astype(int)
        columns += ['n_lists']

    if 'n_probes' in df.columns:
        df['n_probes'] = df['n_probes'].astype(int)
        columns += ['n_probes']

    if 'n_results' in df.columns:
        df['n_results'] = df['n_results'].astype(int)
        columns += ['n_results']

    if 'mean' in df.columns:
        df['time_s'] = df['mean'].astype(float) / NS_PER_S
        df['s_per_query'] = df['time_s'] / N_QUERIES
        df['vectors_per_s'] = N_VECTORS / df['time_s']
        df['queries_per_s'] = N_QUERIES / df['time_s']
        columns += ['time_s', 's_per_query', 'vectors_per_s', 'queries_per_s']

    if 'std' in df.columns:
        df['time_s_std'] = df['std'].astype(float) / NS_PER_S
        df['s_per_query_std'] = df['time_s_std'] / N_QUERIES
        df['vectors_per_s_std'] = N_VECTORS * df['time_s_std'] / df['time_s']**2
        df['queries_per_s_std'] = N_QUERIES * df['time_s_std'] / df['time_s']**2
        columns += ['time_s_std','s_per_query_std', 'vectors_per_s_std', 'queries_per_s_std']
    
    if 'latency_50th_mean' in df.columns:
        df['latency_50th'] = df['latency_50th_mean'].astype(float) / NS_PER_S
        columns += ['latency_50th']

    if 'latency_50th_std' in df.columns:
        df['latency_50th_std'] = df['latency_50th_std'].astype(float) / NS_PER_S
        columns += ['latency_50th_std']

    if 'latency_95th_mean' in df.columns:
        df['latency_95th'] = df['latency_95th_mean'].astype(float) / NS_PER_S
        columns += ['latency_95th']

    if 'latency_95th_std' in df.columns:
        df['latency_95th_std'] = df['latency_95th_std'].astype(float) / NS_PER_S
        columns += ['latency_95th_std']

    if 'latency_99th_mean' in df.columns:
        df['latency_99th'] = df['latency_99th_mean'].astype(float) / NS_PER_S
        columns += ['latency_99th']

    if 'latency_99th_std' in df.columns:
        df['latency_99th_std'] = df['latency_99th_std'].astype(float) / NS_PER_S
        columns += ['latency_99th_std']

    if 'time_s' in df.columns:
        if mode == 'search_preassigned':
            df['size_per_query_mb'] = (N_VECTORS / df['n_lists']) * df['n_probes'] * VECTOR_SIZE_MB
            df['size_mb'] = df['size_per_query_mb'] * N_QUERIES
            columns += ['size_per_query_mb']
        elif mode == 'preassign_query':
            df['size_per_query_mb'] = df['n_lists'] * VECTOR_SIZE_MB
            df['size_mb'] = df['size_per_query_mb'] * N_QUERIES
            columns += ['size_per_query_mb']
        elif mode == 'bulk_insert_entries':
            df['size_mb'] = N_VECTORS * VECTOR_SIZE_MB
        else:
            raise ValueError('Unknown mode: {}'.format(mode))

        df['mb_per_s'] = df['size_mb'] / df['time_s']
        df['mb_per_s_std'] = df['size_mb'] * df['time_s_std'] / df['time_s']**2
        columns += ['mb_per_s', 'mb_per_s_std', 'size_mb']

    if 'recall@1' in df.columns:
        df['recall@1'] = df['recall@1'].astype(float)
        columns += ['recall@1']

    return df[columns]

def plot(df,
         y_col,
         ylabel,
         title,
         yerr_col=None,
         legend_loc='best',
         yformatter=ticker.ScalarFormatter(),
         yaxis_log=False,
         mode='n_probes',
         xaxis_locator=ticker.LogLocator(base=2),
         xaxis_formatter=ticker.FormatStrFormatter('%d'),
         subtitle='',
         scatter=False,
         yaxis_bottom=None,
         yaxis_top=None,
         show_mean=False,
         filename=None,
         show_legend=True):

    if mode == 'n_probes':
        x_col = 'n_probes'
        series_col = 'n_lists'
        x_label = 'Number of probes'
        legend_title = 'Number of lists'
    elif mode == 'n_lists':
        x_col = 'n_lists'
        series_col = 'n_probes'
        x_label = 'Number of lists'
        legend_title = 'Number of probes'
    elif mode == 'max_n_threads':
        x_col = 'max_n_threads'
        series_col = 'OMP_SCHEDULE'
        x_label = 'Maximum number of threads'
        legend_title = 'OpenMP schedule'
    elif mode == 'n_results':
        x_col = 'n_results'
        series_col = 'n_probes'
        x_label = 'Number of results'
        legend_title = 'Number of probes'
    elif mode == 'MAX_BUFFER_SIZE':
        x_col = 'MAX_BUFFER_SIZE'
        series_col = 'INSERTION_STRATEGY'
        x_label = 'Buffer size'
        legend_title = 'Insertion strategy'
    elif mode == 'dataset_size':
        x_col = 'dataset_size'
        series_col = 'INSERTION_STRATEGY'
        x_label = 'Dataset size'
        legend_title = 'Insertion strategy'
    elif mode == 'recall':
        x_col = 'n_probes'
        series_col = 'n_lists'
        x_label = 'Number of probes'
        legend_title = 'Number of lists'
    elif mode == 'parallel_mode':
        x_col = 'n_probes'
        series_col = 'parallel_mode'
        x_label = 'Number of probes'
        legend_title = 'FAISS parallel mode'
    else:
        raise ValueError(f'Invalid value for plot: {plot}')
    fig, ax = plt.subplots(tight_layout=True)
    ax.set_prop_cycle(marker_cycler)
    title_txt = f'{title}\n{subtitle}' if subtitle else title
    ax.set_title(title_txt)

    # plot
    sorted_uniques = sorted(df[series_col].unique())
    for label in reversed(sorted_uniques):
        params = {'markersize': 7, 'lw': 2, 'label': f'{label}'}
        df_subset = df[df[series_col] == label]
        if yerr_col:
            params = {**params, 'capsize': 7, 'capthick': 1}
            ax.errorbar(x=df_subset[x_col],
                        y=df_subset[y_col],
                        yerr=df_subset[yerr_col],
                        **params)
        elif scatter:
            params.pop('markersize')
            ax.scatter(df_subset[x_col], df_subset[y_col], **params)
        else:
            ax.plot(df_subset[x_col], df_subset[y_col], **params)

    # x axis
    ax.set_xlabel(x_label)
    ax.set_xscale('log')
    ax.xaxis.set_major_locator(xaxis_locator)
    ax.xaxis.set_major_formatter(xaxis_formatter)
    ax.xaxis.set_minor_locator(ticker.NullLocator())

    # y axis
    if yaxis_log:
        ax.set_yscale('log')
        if yaxis_bottom == 0:
            yaxis_bottom = 1e-5
    ax.set_ylabel(ylabel)
    ax.yaxis.set_major_formatter(yformatter)
    ax.set_ylim(top=yaxis_top, bottom=yaxis_bottom)
    if show_mean:
        y_mean = np.mean(df[y_col])
        ax.axhline(y=y_mean, ls='--', c='k', lw=1, label=f'Mean')

    # legend
    if show_legend:
        ax.legend(title=legend_title, loc=legend_loc)

    # grid
    ax.grid(axis='y', linewidth=0.5)

    plt.show()

    # export
    if filename:
        if not os.path.exists('plots'):
            os.makedirs('plots')
        fig.savefig(f'plots/{filename}.pdf', bbox_inches='tight')


marker_cycler = cycler(
    color=[
        '#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b',
        '#e377c2', '#7f7f7f', '#bcbd22', '#17becf'
    ],
    marker=['o', 's', '^', 'd', '*', '+', 'x', 'v', 'p', 'h'])

s_to_ms_formatter = ticker.FuncFormatter(
    lambda x, pos: '{:.2f}'.format(x * 1e3).rstrip('0').rstrip('.'))
s_to_us_formatter = ticker.FuncFormatter(
    lambda x, pos: '{:0.2f}'.format(x * 1e6).rstrip('0').rstrip('.'))
mb_to_gb_formatter = ticker.FuncFormatter(
    lambda x, pos: '{:0.1f}'.format(x / 2**10).rstrip('0').rstrip('.'))
to_k_formatter = ticker.FuncFormatter(
    lambda x, pos: '{:0.1f}'.format(x / 1e3).rstrip('0').rstrip('.') + 'k')
speedup_formatter = ticker.FuncFormatter(
    lambda x, pos: '{:0.1f}'.format(x).rstrip('0').rstrip('.') + 'x')

base10_locator = ticker.LogLocator(base=10)
base10_formatter = ticker.LogFormatterSciNotation(base=10)


def add_speedup(old_df, new_df, column, invert=False, suffix='_speedup'):
    speedup = new_df[column] / old_df[column]
    if invert:
        speedup = 1 / speedup
    speedup_std = speedup * np.sqrt(
        (new_df[column + '_std'] / new_df[column])**2 +
        (old_df[column + '_std'] / old_df[column])**2)
    new_df[column + suffix] = speedup
    new_df[column + suffix + '_std'] = speedup_std
