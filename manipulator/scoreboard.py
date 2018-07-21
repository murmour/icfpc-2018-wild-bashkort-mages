
import runner
import re
import io
import os
import json


def get_all_problems():
    ps = [ runner.parse_problem_fname(f) for f in os.listdir("../data/problemsL") ]
    ps = list(filter(None, ps))
    ps.sort(key = lambda f: f['id'])
    return ps


trace_meta_name_rx = re.compile('(?P<prefix>[a-zA-Z]+)(?P<id>[0-9]+)_(?P<solver>[a-zA-Z0-9]+).meta$')

def parse_trace_meta_fname(fname):
    m = re.match(trace_meta_name_rx, fname)
    if m is None:
        return None
    else:
        return { 'fname': '../data/traces/' + fname,
                 'prefix': m.group('prefix'),
                 'id': int(m.group('id')),
                 'solver': m.group('solver') }


def get_all_traces():
    ps = [ parse_trace_meta_fname(f) for f in os.listdir("../data/traces") ]
    ps = list(filter(None, ps))
    ps.sort(key = lambda f: f['id'])
    for p in ps:
        with io.open(p['fname'], 'r') as f:
            meta = json.loads(f.read())
            p['energy'] = meta['energy']
    return ps


if __name__ == '__main__':
    total_energy = 0
    ps = get_all_problems()
    ts = get_all_traces()
    for p in ps:
        pts = [ t for t in ts if t['id'] == p['id'] ]
        if pts == []:
            print('%s%s: no solution' % (p['prefix'], p['id']))
            continue

        best = pts[0]
        best_energy = pts[0]['energy']
        for pt in pts:
            if pt['energy'] < best_energy:
                best = pt
                best_enery = pt['energy']
        print('%s%s: %d (%s)' % (best['prefix'], best['id'], best['energy'], best['solver']))

        total_energy += best_energy

    print()
    print('total energy: %d' % total_energy)
