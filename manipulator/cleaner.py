
import io
import os
import common
import json


if __name__ == '__main__':
    ts = common.get_all_good_traces()
    groups = {}
    for t in ts:
        key = t['prefix']+str(t['id'])+t['solver']
        if not key in groups:
            groups[key] = [t]
        else:
            groups[key].append(t)

    for g, l in groups.items():
        l.sort(key = lambda t: t['energy'])
        rubbish = l[4:]
        for t in rubbish:
            meta = {}
            with io.open(t['meta_fname'], 'r') as f:
                meta = json.loads(f.read())
            meta['rubbish'] = True
            with io.open(t['meta_fname'], 'w') as f:
                f.write(json.dumps(meta))
            os.remove(t['fname'])
            print('%s was marked as rubbish and removed' % t['meta_fname'])
