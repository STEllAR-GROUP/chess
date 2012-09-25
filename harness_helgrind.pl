################################################################################
##  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
##
##  Distributed under the Boost Software License, Version 1.0. (See accompanying
##  file BOOST_LICENSE_1_0.rst or copy at http:##www.boost.org#LICENSE_1_0.txt)
################################################################################
use strict;
use FileHandle;

my $fsup = 0;
my $asup = 0;
my $nsup = 0;
my $msup = 1;

my $anskey = {};
my $scoretab = {};
my $speedtab = {};

#Answer key:

#board1
#ply | answer
#====+=======
#  7 |   e2e4
#  6 |   b1c3
#  5 |   b2b4
#  4 |   g1f3
#  3 |   g1f3
#  2 |   d2d4

$anskey->{1}->{2} = "e2e4";
$anskey->{1}->{3} = "e2e4";
$anskey->{1}->{4} = "d2d4";
$anskey->{1}->{5} = "g1f3";

#board2
#ply | answer
#====+=======
#  7 |   d1b3
#  6 |   c1d2
#  5 |   c1g5
#  4 |   c1g5
#  3 |   c1d2
#  2 |   e2e4

$anskey->{2}->{2} = "e2e4";
$anskey->{2}->{3} = "e1d2";
$anskey->{2}->{4} = "c1d2";
$anskey->{2}->{5} = "c1g5";

#board3
#ply | answer
#====+=======
#  7 |   f5h7
#  6 |   f5g7
#  5 |   f5h6
#  4 |   f5g7
#  3 |   f3b3
#  2 |   b2b3

$anskey->{3}->{2} = "f5g7";
$anskey->{3}->{3} = "b2b3";
$anskey->{3}->{4} = "f3h5";
$anskey->{3}->{5} = "g5g6";

#board4
#ply | answer
#====+=======
#  7 |   e5f5
#  6 |   e5e3
#  5 |   h5h6
#  4 |   e5e3
#  3 |   e5b5
#  2 |   e5e3

$anskey->{4}->{2} = "e5a5";
$anskey->{4}->{3} = "g5f7";
$anskey->{4}->{4} = "g5f7";
$anskey->{4}->{5} = "g5f7";

my $bad_score = -6666;
my $tot_time = 0;

for my $sm (("mtdf","alphabeta","minimax")) {
    for(my $b=1;$b<=4;$b++) {
        for(my $ply=2;$ply<=5;$ply++) {
            # It takes too long for minimax above ply 4
            # So I ran it once at 5 to verify the answer
            # and then introduced this next.
            if($sm eq "minimax" and $ply >= 5) {
                next;
            }
            genbench($sm,$b,$ply);
            my $fd = new FileHandle;
            if($sm eq "mtdf") {
                #sleep(1);
            }
            #open($fd,"mpiexec -np 2 -env CHX_THREADS_PER_PROC 2 $ENV{PWD}/src/chx -s .bench.ini 2>/dev/null|");
            #open($fd,"CHX_THREADS_PER_PROC=8; mpiexec -np 1 src/chx -s .bench.ini 2>/dev/null|");
            open($fd,"CHX_THREADS_PER_PROC=8 valgrind --tool=helgrind --log-file=log_helgrind.out src/chx < .bench 2>/dev/null|");

            my $ans = "";
            my $score = $bad_score;

            my $st = "OK ";
            my $tm = 1e-6;
            my $speedup = 1;
            my $doc = "";
            while(<$fd>) {
                $doc .= $_;
                if(/Computer's chess_move: ([a-h][1-8][a-h][1-8])/) {
                    if($ans ne "" and $ans ne $1) {
                        $st = "VARIABLE($ans != $1) ";
                        #last;
                    }
                    $ans = $1;
                }
                if(/SCORE=(-?\d+)/) {
                    if($score != $bad_score and $score != $1) {
                        $st = "VARIABLE($score != $1) ";
                        #last;
                    }
                    $score=$1;
                }
                if(/Average time for run: (\d+) ms/) {
                    $tm = $1*1.0e-3;
                    $tm = 1.0e-3 if($tm < 1.0e-3);
                    $tot_time += $tm;
                }
            }
            if(defined($anskey->{$b}->{$ply})) {
                my $v = $anskey->{$b}->{$ply};
                if($ans ne $v) {
                    $st .= "MOVE($v) ";
                }
            }
            if(defined($scoretab->{$b}->{$ply})) {
                my $s = $scoretab->{$b}->{$ply};
                if($s != $score) {
                    $st .= "SCORE($s) ";
                }
            }
            if($st eq "OK ") {
                $scoretab->{$b}->{$ply} = $score;
            }
            if(defined($speedtab->{$b}->{$ply})) {
                $speedup = $speedtab->{$b}->{$ply}/$tm;
            }
            $speedtab->{$b}->{$ply} = $tm;
            printf("%10s, %4d, %4d, %9d, %s, %s, %.2f\n",$sm,$b,$ply,$score,$ans,$st,$speedup);
            die $doc if($st =~ /VARIABLE/);
            if($sm eq "mtdf" or $sm eq "multistrike") {
                $fsup += 1 if($speedup > 1);
                $fsup += 0.5 if($speedup == 1);
                $asup += $speedup;
                $nsup += 1;
                $msup *= $speedup;
            }
            my $ret=close($fd);
            unless($ret) {
                print $doc;
                die "error code returned r=($ret)";
            }
			open($fd,"log.out");
			while(<$fd>) {
				if(/ERROR SUMMARY: (\d+) errors/) {
					if($1 > 0) {
						die $_;
					}
				}
			}
        }
    }
}

printf("mtdf over alpha speedup: avg=%3.2f geometric mean=%3.2f fraction faster=%3.2f\n",$asup/$nsup,($msup)**(1.0/$nsup),$fsup/$nsup);

printf("total time: %3.2f\n",$tot_time);

sub genbench {
    my $sm = shift;
    my $b = shift;
    my $ply = shift;
    my $fd = new FileHandle;
    my $runs = 1;
    $runs = 1 if($sm eq "minimax");
    $runs = 3 if($ply > 5);
    open($fd,">.bench");
print $fd qq{
search $sm
eval original
bench inputs/board$b $ply $runs
quit

};
    close($fd);
}
