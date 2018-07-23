
import io
import os
import common
import sys


if __name__ == '__main__':
    ps = common.filter_problems(0, 200, sys.argv[1])
    ps_a = [ p for p in ps if p['prefix'] == 'FA' ]
    ps_d = [ p for p in ps if p['prefix'] == 'FD' ]
    ps_r = [ p for p in ps if p['prefix'] == 'FR' ]
    ps_u = [ p for p in ps if p['prefix'] == 'FU' ]
    ps = ps_a + ps_d + ps_r + ps_u

    ts = common.get_all_good_traces()

    total_energy = 0

    for p in ps:
        pts = [ t for t in ts if t['id'] == p['id'] and t['prefix'] == p['prefix']]
        if pts == []:
            print('%s%d: no solution' % (p['prefix'], p['id']))
            continue

        pts.sort(key = lambda pt: pt['energy'])
        def print_pt(pt):
            return '%s(%s)' % (pt['solver']+str(pt['bots'])+pt['solver2']+pt['bots2'], format(pt['energy'], ',d'))
        print('%s%s: %s' % (p['prefix'], p['id'], ', '.join(print_pt(pt) for pt in pts[:4])))

        best = pts[0]
        total_energy += best['energy']

    print()
    print('total energy: %s' % format(total_energy, ',d'))
