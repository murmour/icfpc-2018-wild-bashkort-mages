
import subprocess
import time
import json
import sys
import os
import io
import common


temp_counter = 0


if __name__ == '__main__':
    if len(sys.argv) < 5:
        print('Usage: runner.py executable solver solverAlias lowIndex highIndex cpus rest_args...')
        sys.exit(1)

    executable = sys.argv[1]
    solver = sys.argv[2]
    solverAlias = sys.argv[3]
    low_index = int(sys.argv[4])
    high_index = int(sys.argv[5])
    cpus = int(sys.argv[6])
    rest_args = sys.argv[7:]

    def start_solving(p):
        global temp_counter
        worker = {}
        trace_base = common.traces_dir + ('%s%s_%s' % (p['prefix'], p['id'], solverAlias))
        worker['trace_file'] = trace_base + '.nbt.gz'
        worker['meta_file'] = trace_base + '.meta'
        if os.path.isfile(worker['meta_file']):
            print('Trace %s already exists.' % trace_base)
            return None
        print('Producing %s...' % worker['trace_file'])
        worker['temp_file'] = 'temp/tmp%d' % temp_counter
        temp_counter += 1
        worker['process'] = subprocess.Popen([ executable,
                                             '-in', p['fname'],
                                             '-out', worker['temp_file'],
                                             '-solver', solver ] + rest_args,
                                           stdin=subprocess.PIPE,
                                           stdout=subprocess.PIPE)
        return worker

    def finish_solving(worker, retcode):
        if retcode != 0:
            print('%s: ERROR (return code %d)' % (worker['trace_file'], retcode))
        else:
            out = worker['process'].communicate()[0].decode()
            energy = int(out)
            print('%s: SUCCESS (energy %d)' % (worker['trace_file'], energy))
            os.rename(worker['temp_file'], worker['trace_file'])
            with io.open(worker['meta_file'], 'w') as f:
                f.write(json.dumps({ 'energy': energy }))

    pool = [ None ] * cpus
    queue = common.filter_problems(low_index, high_index)
    left = len(queue)

    while left > 0:
        time.sleep(0.1)
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
            worker = pool[i]
            retcode = worker['process'].poll()
            if retcode is not None:
                left -= 1
                pool[i] = None
                finish_solving(worker, retcode)
