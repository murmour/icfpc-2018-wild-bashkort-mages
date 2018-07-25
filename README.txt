Hаумыһығыҙ, ауылдаштар!

This transmission is brought to you by WILD BASHKORT MAGES,
the big losers of ICFP Contest 2018.

The losers in question are:
  Damir Akhmetzyanov (linesprower@gmail.com),
  Max Mouratov (mmouratov@gmail.com),
  Artem Ripatti (ripatti@inbox.ru).

We truly are big losers, because we failed to submit a single correct tarball
for the duration of the whole final round, despite having produced a quite
potent solution. We've postponed sending the tarball to the last 30 minutes of
the contest, and somehow failed this mundane ordeal: neither the submit form, nor
the alternative API did take our submission during the last minutes.

Shame on us.

Still, being the big losers that we are, we are proud losers, and we lose with
dignity. Losing is not fun, but we had lots of fun during the contest, and it was
totally worth it. Also, here, in Bashkortostan, we have a national habit of having
fun all the time, and as a result of this habit we've produced a fun video in
memory of the contest. The video is based on our awesome visualizer.

We hope you enjoy it!
https://www.youtube.com/watch?v=lMHMyh5XTIE

The visualizer in question is written in C++, is located in folder "nanovisu",
and should compile on both Linux (run "make"), and Windows (use the Visual
Studio project). It expects the trace files to be gz-compressed (we used
compression to save space and bandwidth).

If you run "nanovisu" with no arguments, it will load models from
../data/problemsF and the associated solutions from ../data/tracesF. You can
also pass it the name of a model file (as a first argument) and, optionally, a
path to the trace (as a second argument), and it will load them.

Example:
./nanovisu FA140_tgt.mdl ../data/tracesF/FA140_cutterpillarxF40.nbt.gz


A few words on the algorithm of our solver:

Harmonics are always Low, which is most profitable.

On first step, 8 bots use GFill to construct a crude carcass of the model. This
step is tricky, as bots sometimes have to dig auxiliary tunnels to reach corners
of the produced blocks. When possible, tunnels are built with GVoid.

On second step, the model is split into similarly-sized clusters, and each of
them is processed by a single bot that uses a search heuristic that minimizes
movement and avoids getting trapped. If a cluster is not grounded, the bot
builds a pillar beneath it, which is erased in the end. The combination of
cutting and building pillars gives this strategy its name: "cutterpillar".

Deconstruction of models is solved by reversing construction. The only
operations that can't be reversed are "GVoid that involves empty voxels" and
"GFill that involves filled voxels", and our solvers don't do this, which is a
deliberate design choice. In a similar fashion, reconstruction is solved by
constructing both the source and the target independently, and then reversing
the first trace and concatenating it with the second. Easy-peasy.

Our solver produced solutions to all of the 487 problems, and the total energy
spent was133'891'156'887'586 (individual results by problem can be found in
best_scores.txt). We have nearly no clue as to how it compares with solutions
from other teams, but we find our result satisfactory in regards to our
self-esteem, and hold no shame to it.

We'd like to thank the organizer team for running an excellent contest. We are
still fascinated with the beautiful task, and will probably play with it for a
while, in the noble purpose of recreation.

Even though we won't be in the scoring table, _we were_ a part of this contest,
and it made us feel great!

Thanks, everyone!
