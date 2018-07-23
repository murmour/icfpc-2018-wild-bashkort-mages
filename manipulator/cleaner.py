
import io
import os
import common
import json


if __name__ == '__main__':
    ts = common.get_all_traces()
    for t in ts:
        if not os.path.isfile(t['meta_fname']):
            os.remove(t['fname'])
            print('%s was removed' % t['fname'])

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
            os.remove(t['fname'])
            print('%s was removed' % t['fname'])
            if os.path.isfile(t['meta_fname']):
                os.remove(t['meta_fname'])
                print('%s was removed' % t['meta_fname'])
