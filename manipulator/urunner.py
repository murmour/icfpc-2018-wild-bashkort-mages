
import subprocess
import time
import json
import sys
import os
import io
import common


temp_counter = 0


if __name__ == '__main__':
    if len(sys.argv) < 9:
        print('Usage: urunner.py executable solver solver_alias low_index '
              'high_index job_count low_bots high_bots')
        sys.exit(1)

    executable = sys.argv[1]
    solver = sys.argv[2]
    solver_alias = sys.argv[3]
    low_index = int(sys.argv[4])
    high_index = int(sys.argv[5])
    job_count = int(sys.argv[6])
    low_bots = int(sys.argv[7])
    high_bots = int(sys.argv[8])

    def start_solving(p):
        global temp_counter
        job = {}
        trace_base = 'FR%s_%s%d%s%s' % (p['id'],
                                        p['solver'],
                                        p['bots1'],
                                        solver_alias,
                                        p['bots'])
        job['trace_file'] = common.traces_dir + trace_base + '.nbt.gz'
        job['meta_file'] = common.traces_dir + trace_base + '.meta'
        if os.path.isfile(job['meta_file']):
            print('Trace %s already exists.' % trace_base)
            return None
        print('Producing %s...' % job['trace_file'])
        job['temp_file'] = 'temp/tmp%d' % temp_counter
        temp_counter += 1
        job['process'] = subprocess.Popen([executable,
                                           '-in', p['fname'],
                                           '-out', job['temp_file'],
                                           '-trace', p['best_trace'],
                                           '-solver', solver,
                                           '-bots', str(p['bots'])],
                                          stdin=subprocess.PIPE,
                                          stdout=subprocess.PIPE)
        return job

    def finish_solving(job, retcode):
        if retcode != 0:
            print('%s: ERROR (return code %d)' %
                  (job['trace_file'], retcode))
        else:
            out = job['process'].communicate()[0].decode()
            energy = int(out)
            print('%s: SUCCESS (energy %d)' % (job['trace_file'], energy))
            os.rename(job['temp_file'], job['trace_file'])
            with io.open(job['meta_file'], 'w') as f:
                f.write(json.dumps({'energy': energy}))

    ps = common.filter_problems(low_index, high_index, 'U')
    ts = common.get_all_good_traces()
    queue = []
    for p in ps:
        fname = common.problems_dir + 'FR%03d_src.mdl' % p['id']
        pts = [ t for t in ts if t['id'] == p['id'] and t['prefix'] == 'FU']
        if pts == []:
            print('FU%d: no solution' % (p['prefix'], p['id']))
            continue
        pts.sort(key = lambda pt: pt['energy'])
        best = pts[0]
        for bots in range(low_bots, high_bots+1):
            p2 = dict(p)
            p2['fname'] = fname
            p2['prefix'] = 'FR'
            p2['solver'] = best['solver']
            p2['bots1'] = best['bots']
            p2['bots'] = bots
            p2['best_trace'] = best['fname']
            queue.append(p2)


    pool = [None] * job_count
    left = len(queue)
    while left > 0:
        time.sleep(0.01)
        for i in range(len(pool)):
            if pool[i] is None:
                if queue == []:
                    continue
                p = queue[0]
                queue = queue[1:]
                pool[i] = start_solving(p)
                if pool[i] is None:
                    left -= 1
                    continue
                else:
                    break
            job = pool[i]
            retcode = job['process'].poll()
            if retcode is not None:
                left -= 1
                pool[i] = None
                finish_solving(job, retcode)
