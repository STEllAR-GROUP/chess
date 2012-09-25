Compiling the chess benchmark:
==============================

The Chess benchmark is compiled with cmake.

    cmake ./CMakeLists.txt
    make

The end result is an executable named ./src/chx.
The program can be executed either from the command
line, by supplying an ini file, or by using the
perl test harness.

Running the chess benchmark:
============================

The Perl Test Harness
---------------------

The perl test harness creates ini files and executes
the chess benchmark multiple times, executing the
chess benchmark to a number of different ply depths,
using a variety of algorithms, on a number of different
boards.

The benchmark does not attempt to play a game, instead
it attempts to make a single move by a single side. The
objective of the benchmark is to search to the deepest
ply depth possible in two hours.

Before running the test harness line 93 should be
edited to create an appropriate run command. By default
it reads as follows:

open($fd,"mpiexec -np 1 -env CHX_THREADS_PER_PROC 8 $ENV{PWD}/src/chx < .bench 2>/dev/null|");

This uses MPI to run in a single process using 8
threads per core. Please edit this line to match
the core count and number of processes appropriate
for your platform.

By default three algorithms are run by the benchmark:

1) Minimax - This algorithm is a simple test program
   that provides a deterministic workload.

2) Alpha Beta - This algorithm is actually the Alpha-
   Beta With Memory required by MTD-f. The macro definition
   
   #define PV_ON 1

   can be commented out to disable principle variation
   search.

   The macro definition

   #define TRANSPOSE_ON 1

   can be commented out to disable the use of a transposition
   table.

3) MTD-f - The basic MTD-f algorithm has three parameters
   which modify its behavior. Strictly speaking, MTD-f does
   zero width searches. The code provided with the benchmark
   uses a narrow width instead, one which is permitted to
   grow as the number of searches increases, and which gives
   up after a set number of searches are tried and fail.

   start_width: This is the width in terms of the base
   score. This is the value of the score before the score
   is augmented by the hash value of the board.

   grow_width: This is a constant value that is added to
   the search width after each failure.

   max_tries: This is the maximum number of tries that
   should be made at finding the with MTD-f before giving up
   and doing a full Alpha-Beta search between the remaining
   lower/upper score range.

4) Multistrike - This algorithm divides the possible range
   of scores into subranges and assigns a subrange to each
   available thread/core. Each subrange of Multistrike is
   searched completely by a strict Alpha-Beta search without
   recourse to principle variation. (The code might be improved
   by using MTD-f on each sub-range)

   The Multistrike code with the chess benchmark is not able
   to use MPI. This lack does not represent a limitation of the
   algorithm, but a feature that is not yet implemented.

Interactive Mode
----------------

This mode is useful for actually playing chess. 
To understand how this mode is used, please type "help" at the prompt.

To display the board type "d". To make the first move, type
in a move (e.g. e2e4 to advance white's king pawn two squares).
Type "d" again to see the result. Type "go" to let the chx
program move on behalf of black. After this, the chx program
will automatically move and redisplay the board after each
move by white.

Configuring chx
-----------------

In order to configure chx so that you don't have to use the
interactive mode all the time, you may specify the commands to 
do in a file and then pipe it in using stdin.

For example, to specify the search method to be multistrike and
then run the benchmark, a file might look like:

  $ cat > input
  search multistrike
  bench some_board 3 1
  $ ./src/chx < input

One parameter, the number of threads, is controlled through
an environment variable: CHX_THREADS_PER_PROC.

Limitations of the CHX code
===========================

The current version of the CHX code suffers from a number
of limitations, including:

1) The parallel implementation does not have a proper mechanism
   for aborting a subset of calculations. Because of this,
   the Younger Brother's Wait algorithm must explore moves in
   batches and wait for each batch to complete before moving
   on to search new moves.

2) The MPI implementation is more a proof of concept than an
   optimal implementation. The transposition table is not
   exchanged in the course of MPI searching. Like the threading
   aspect of the parallel implementation, MPI moves must be
   explored in batches.

3) The Multistrike implementation has a number of defects
   that are described above. In short, it needs to make use
   of MTD-f, as well as MPI.
