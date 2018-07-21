
import subprocess
import re
import json
import sys
import os
import io


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


def run_solver(executable, problem_file, trace_file, solver_name, rest_args):
    print('Producing %s...' % trace_file)
    temp_file = 'tmp'
    p = subprocess.Popen([ executable,
                           '-in', problem_file,
                           '-out', temp_file,
                           '-solver', solver_name ] + rest_args,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE)
    out = p.communicate()[0].decode()
    if p.returncode != 0:
        print('Error! (return code %d)' % p.returncode)
        return None
    else:
        energy = int(out)
        print('Success! (energy %d)' % energy)
        os.rename(temp_file, trace_file)
        return energy


if __name__ == '__main__':
    if len(sys.argv) < 5:
        print('Usage: runner.py executable solver lowIndex highIndex')
        sys.exit(1)

    executable = sys.argv[1]
    solver = sys.argv[2]
    low_index = int(sys.argv[3])
    high_index = int(sys.argv[4])

    for p in filter_problems(low_index, high_index):
        trace_base = '../data/traces/%s%s_%s' % (p['prefix'], p['id'], solver)
        trace_file = trace_base + '.nbt'
        meta_file = trace_base + '.meta'
        if os.path.isfile(meta_file):
            print('Trace %s already exists.' % trace_base)
        else:
            energy = run_solver(executable, p['fname'], trace_file, solver, [])
            if energy != None:
                with io.open(meta_file, 'w') as f:
                    f.write(json.dumps({ 'energy': energy }))
