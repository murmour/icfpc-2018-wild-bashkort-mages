
import io
import os
import common
import json


if __name__ == '__main__':
    ts = common.get_all_traces()
    groups = {}
    for t in ts:
        key = t['prefix']+str(t['id'])+t['solver']+t['solver2']
        if not key in groups:
            groups[key] = [t]
        else:
            groups[key].append(t)

    for g, l in groups.items():
        l.sort(key = lambda t: t['energy'])
        rubbish = l[2:]
        for t in rubbish:
            meta = {}
            if os.path.isfile(t['meta_fname']):
                with io.open(t['meta_fname'], 'r') as f:
                    meta = json.loads(f.read())
                if not 'rubbish' in meta:
                    meta['rubbish'] = True
                    with io.open(t['meta_fname'], 'w') as f:
                        f.write(json.dumps(meta))
            if os.path.isfile(t['fname']):
                os.remove(t['fname'])
                print('%s was marked as rubbish and removed' % t['fname'])
