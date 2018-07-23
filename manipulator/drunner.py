
import subprocess
import time
import json
import sys
import os
import io
import common


temp_counter = 0


if __name__ == '__main__':
    if len(sys.argv) < 1:
        print('Usage: drunner.py executable')
        sys.exit(1)

    executable = sys.argv[1]

    ps = common.filter_problems(0, 200, 'D')
    ts = common.get_all_good_traces()

    for p in ps:
        pts = [ t for t in ts if t['id'] == p['id'] and t['prefix'] == 'FA']
        if pts == []:
            print('FA%d: no solution' % p['id'])
            continue
        pts.sort(key = lambda pt: pt['energy'])
        best = pts[0]

        trace_base = 'FD%s_%s%d' % (p['id'], best['solver'], best['bots'])
        trace_file = common.traces_dir + trace_base + '.nbt.gz'
        meta_file = common.traces_dir + trace_base + '.meta'
        if os.path.isfile(meta_file):
            print('Trace %s already exists.' % trace_base)
            continue

        print('Producing %s...' % trace_file)
        temp_file = 'temp/tmp%d' % temp_counter
        temp_counter += 1
        print('-in %s, -out %s, -trace %s' % (p['fname'], temp_file, best['fname']))
        proc = subprocess.Popen([executable,
                                 '-in', p['fname'],
                                 '-out', temp_file,
                                 '-trace', best['fname']],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
        out = proc.communicate()[0].decode()
        proc.kill()
        if proc.returncode != 0:
            print('%s: ERROR (return code %d)' % (trace_file, proc.returncode))
        else:
            energy = int(out)
            print('%s: SUCCESS (energy %d)' % (trace_file, energy))
            os.rename(temp_file, trace_file)
            with io.open(meta_file, 'w') as f:
                f.write(json.dumps({'energy': energy}))
