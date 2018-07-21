
import re
import os
import json
import io


traces_dir = '../data/tracesF/'


problem_name_rx = re.compile('(?P<prefix>[a-zA-Z]+)(?P<id>[0-9]+)_tgt.mdl$')

def parse_problem_fname(fname):
    m = re.match(problem_name_rx, fname)
    if m is None:
        return None
    else:
        return { 'fname': '../data/problemsL/' + fname,
                 'prefix': m.group('prefix'),
                 'id': int(m.group('id')) }


def filter_problems(lowIndex, highIndex):
    print('Reading problems...')
    ps = [ parse_problem_fname(f) for f in os.listdir("../data/problemsL") ]
    ps = filter(None, ps)
    def is_requested(f):
        return ((f['id'] >= lowIndex) and (f['id'] <= highIndex))
    ps = [ f for f in ps if is_requested(f) ]
    ps.sort(key = lambda f: f['id'])
    print('OK...')
    return ps


def get_all_problems():
    ps = [ parse_problem_fname(f) for f in os.listdir("../data/problemsL") ]
    ps = list(filter(None, ps))
    ps.sort(key = lambda f: f['id'])
    return ps


trace_meta_name_rx = re.compile('(?P<prefix>[a-zA-Z]+)(?P<id>[0-9]+)_(?P<solver>[a-zA-Z0-9_]+).meta$')

def parse_trace_meta_fname(fname):
    m = re.match(trace_meta_name_rx, fname)
    if m is None:
        return None
    else:
        fname = m.group('prefix') + m.group('id') + '_' + m.group('solver')
        return { 'fname': traces_dir + fname + '.nbt',
                 'meta_fname': traces_dir + fname + '.meta',
                 'prefix': m.group('prefix'),
                 'id': int(m.group('id')),
                 'solver': m.group('solver') }


def get_all_good_traces():
    ps = [ parse_trace_meta_fname(f) for f in os.listdir(traces_dir) ]
    ps = list(filter(None, ps))
    ps.sort(key = lambda f: f['id'])
    for p in ps:
        with io.open(p['meta_fname'], 'r') as f:
            meta = json.loads(f.read())
            p['energy'] = meta['energy']
    return ps
