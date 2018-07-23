
import common
from shutil import copyfile


if __name__ == '__main__':
    ps = common.filter_problems(0, 200, 'ADR')
    ts = common.get_all_good_traces()
    for p in ps:
        pts = [ t for t in ts if t['id'] == p['id'] ]
        if pts == []:
            print('%s%s: no solution' % (p['prefix'], p['id']))
            continue
        pts.sort(key = lambda pt: pt['energy'])
        best = pts[0]
        target_file = '../data/panopticum/%s%03d.nbt' % (best['prefix'], best['id'])
        print('copying %s to %s' % (best['fname'], target_file))
        copyfile(best['fname'], target_file)
