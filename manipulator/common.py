
import re
import os
import json
import io


traces_dir = '../data/tracesF/'
problems_dir = '../data/problemsF/'


problem_name_rx = re.compile('(?P<prefix>[a-zA-Z]+)(?P<id>[0-9]+)_(?P<suffix>[a-z]+).mdl$')

def parse_problem_fname(fname):
    m = re.match(problem_name_rx, fname)
    if m is None:
        return None
    else:
        return { 'fname': problems_dir + fname,
                 'prefix': m.group('prefix'),
                 'suffix': m.group('suffix'),
                 'id': int(m.group('id')) }


def get_all_problems():
    ps = [ parse_problem_fname(f) for f in os.listdir(problems_dir) ]
    def is_valid(p):
        return not ((p is None) or (p['prefix'] == 'FR' and p['suffix'] == 'tgt'))
    ps = [ p for p in ps if is_valid(p) ]
    ps.sort(key = lambda f: f['id'])
    return ps


def filter_problems(lowIndex, highIndex, kinds):
    ps = get_all_problems()
    def is_requested(p):
        return ((p['id'] >= lowIndex) and (p['id'] <= highIndex) and (p['prefix'][1] in kinds))
    ps = [ p for p in ps if is_requested(p) ]
    return ps


trace_meta_name_rx = re.compile('(?P<prefix>[a-zA-Z]+)(?P<id>[0-9]+)_(?P<solver>[a-zA-Z_]+)(?P<bots>[0-9]+)(?P<solver2>[a-zA-Z_]*)(?P<bots2>[0-9]*).meta$')

def parse_trace_meta_fname(fname):
    m = re.match(trace_meta_name_rx, fname)
    if m is None:
        return None
    fname = m.group('prefix') + m.group('id') + '_' + m.group('solver') + m.group('bots') + m.group('solver2') + m.group('bots2')
    return { 'fname': traces_dir + fname + '.nbt.gz',
             'meta_fname': traces_dir + fname + '.meta',
             'prefix': m.group('prefix'),
             'id': int(m.group('id')),
             'solver': m.group('solver'),
             'solver2': m.group('solver2'),
             'bots': int(m.group('bots')),
             'bots2': m.group('bots2')}


def get_all_good_traces():
    ts = [ parse_trace_meta_fname(f) for f in os.listdir(traces_dir) ]
    ts = list(filter(None, ts))
    ts.sort(key = lambda t: t['id'])
    for t in ts:
        with io.open(t['meta_fname'], 'r') as f:
            meta = json.loads(f.read())
            t['energy'] = meta['energy']
    return ts


trace_name_rx = re.compile('(?P<prefix>[a-zA-Z]+)(?P<id>[0-9]+)_'
                           '(?P<solver>[a-zA-Z_]+)(?P<bots>[0-9]+)'
                           '(?P<solver2>[a-zA-Z_]*)(?P<bots2>[0-9]*).nbt.gz$')

def parse_trace_fname(fname):
    m = re.match(trace_name_rx, fname)
    if m is None:
        return None
    fname = m.group('prefix') + m.group('id') + '_' + m.group('solver') + m.group('bots') + m.group('solver2') + m.group('bots2')
    return { 'fname': traces_dir + fname + '.nbt.gz',
             'meta_fname': traces_dir + fname + '.meta',
             'prefix': m.group('prefix'),
             'id': int(m.group('id')),
             'solver': m.group('solver'),
             'solver2': m.group('solver2'),
             'bots': int(m.group('bots')),
             'bots2': m.group('bots2')}


def get_all_traces():
    ts = [ parse_trace_fname(f) for f in os.listdir(traces_dir) ]
    ts = list(filter(None, ts))
    ts.sort(key = lambda t: t['id'])
    for t in ts:
        if os.path.isfile(t['meta_fname']):
            with io.open(t['meta_fname'], 'r') as f:
                meta = json.loads(f.read())
                t['energy'] = meta['energy']
    return ts
