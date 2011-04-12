use strict;
use FileHandle;

my $anskey = {};

$anskey->{1}->{2} = "e2e4";
$anskey->{1}->{3} = "e2e4";
$anskey->{1}->{4} = "g1f3";

$anskey->{2}->{2} = "e2e4";
$anskey->{2}->{3} = "c1d2";
$anskey->{2}->{4} = "c1g5";

$anskey->{3}->{2} = "b2b3";
$anskey->{3}->{3} = "f3b3";
$anskey->{3}->{4} = "f5g7";

$anskey->{4}->{2} = "e5e1";
$anskey->{4}->{3} = "g5f7";
$anskey->{4}->{4} = "e5e1";

printf("%10s,%5s, %4s, %5s, %s, %s\n","method","board","ply","score","ansr","status");
for my $sm (("minimax","alphabeta","mtd-f")) {
    for(my $b=1;$b<=4;$b++) {
        for(my $ply=2;$ply<=4;$ply++) {
            genbench($sm,$b,$ply);
            my $fd = new FileHandle;
            open($fd,"./src/chx -s .bench.ini|");
            my $ans = "";
            my $score = -666;
            while(<$fd>) {
                if(/Computer's move: ([a-h][1-8][a-h][1-8])/) {
                    $ans = $1;
                }
                if(/SCORE=(-?\d+)/) {
                    $score=$1;
                }
            }
            my $st = "OK";
            if(defined($anskey->{$b}->{$ply})) {
                my $v = $anskey->{$b}->{$ply};
                if($ans ne $v) {
                    $st = "ERROR($v)";
                }
            }
            printf("%10s, %4d, %4d, %5d, %s, %s\n",$sm,$b,$ply,$score,$ans,$st);
            close($fd);
        }
    }
}

sub genbench {
    my $sm = shift;
    my $b = shift;
    my $ply = shift;
    my $fd = new FileHandle;
    open($fd,">.bench.ini");
print $fd qq{
; settings.ini
; Configuration file for CHX

[CHX Main]

; Valid arguments to search_method:
;  minimax (default)
; alphabeta
;search_method = mtd-f
search_method = $sm

; Valid arguments to eval_method:
;  original (default)
;  simple
eval_method = original

; Valid arguments to num_threads: (note that this only works when OpenMP is enabled, otherwise ignored)
;  [1-9][0-9]* (default 3)
num_threads = 2

; Valid arguments to output:
;  true (default)
;  false
output = true

; Iterative deepening can be tuned to only go to a certain number of plys before just calling the
; search function
; Valid arguments to iter_depth:
; [1-9][0-9]* (default 5)
iter_depth = 5

[Depth]

; This sets the ply-depth of the white player
; Valid arguments to white:
; [1-9][0-9]* (default 3)
white = 7

; Depth for black player, same arguments
black = 7

[Benchmark]
; This section is for using the built in benchmark

; If mode is set to true, then it will read in the file
; specified by file, run the benchmark and then quit (useful for automating the bench runs)
mode=true

; If a relative path is given, it will be to the relative path in which CHX was called from
file=inputs/board$b

max_ply=$ply

num_runs=1
};
    close($fd);
}
