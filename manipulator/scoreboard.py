
import io
import os
import common


if __name__ == '__main__':
    ps = common.get_all_problems()
    ps_a = [ p for p in ps if p['prefix'] == 'FA' ]
    ps_d = [ p for p in ps if p['prefix'] == 'FD' ]
    ps_r = [ p for p in ps if p['prefix'] == 'FR' ]
    ps = ps_a + ps_d + ps_r

    ts = common.get_all_good_traces()

    total_energy = 0

    for p in ps:
        pts = [ t for t in ts if t['id'] == p['id'] and t['prefix'] == p['prefix']]
        if pts == []:
            print('%s%s: no solution' % (p['prefix'], p['id']))
            continue

        pts.sort(key = lambda pt: pt['energy'])
        def print_pt(pt):
            return '%s(%s)' % (pt['solver'], format(pt['energy'], ',d'))
        print('%s%s: %s' % (p['prefix'], p['id'], ', '.join(print_pt(pt) for pt in pts[:4])))

        best = pts[0]
        total_energy += best['energy']

    print()
    print('total energy: %s' % format(total_energy, ',d'))
