
import io
import os
import common


if __name__ == '__main__':
    total_energy = 0
    ps = common.get_all_problems()
    ts = common.get_all_traces()
    for p in ps:
        pts = [ t for t in ts if t['id'] == p['id'] ]
        if pts == []:
            print('%s%s: no solution' % (p['prefix'], p['id']))
            continue

        pts.sort(key = lambda pt: pt['energy'])
        def print_pt(pt):
            return '%s(%d)' % (pt['solver'], pt['energy'])
        print('%s%s: %s' % (p['prefix'], p['id'], ', '.join(print_pt(pt) for pt in pts[:4])))

        best = pts[0]
        total_energy += best['energy']

    print()
    print('total energy: %d' % total_energy)
