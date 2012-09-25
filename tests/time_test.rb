#!/usr/bin/env ruby
require "open3"
require "fileutils"
include FileUtils

# number of tests
num_tests = 1

# maps revision number to input type (ini or unified) for each 
# svn revision we want to test
svn_revs = Hash[5433 => :ini, 5953 => :unified, 6723 => :unified]

# because I do an out of source build, my executable is in a non standard place
# so put the path to the current chx executable in here (relative to the testing folder)
$chx_exe = "../../build_chx/src/chx"

search_ms = ["minimax","alphabeta","mtdf"]
plies = [2,3,4,5]
boards = [:board1,:board2,:board3,:board4]

#This script assumes that it lives inside of the testing folder
# and that your working directory is already built

def make_sure_dir_exists(svn_rev)
  list = Dir["chx_#{svn_rev}"]
  if list.empty?
    puts "Checking out revision #{svn_rev}"
    `svn checkout https://svn.cct.lsu.edu/repos/projects/parallex/branches/chess/openmp@#{svn_rev} chx_#{svn_rev}`
  end
end

def make(svn_rev)
  make_sure_dir_exists svn_rev
  cd "chx_#{svn_rev}"
  if Dir["Makefile"].empty?
    puts "Running cmake on revision #{svn_rev}"
    puts `cmake .`
  end
  if Dir["src/chx"].empty?
    puts "making revision #{svn_rev}"
    puts `make`
  end
  cd ".."
end

def check_usage
  if Dir[$chx_exe].empty?
    if ARGV.length = 0
      puts "Please specify the current chx executable path either in the script or the command line"
      puts "Usage: #{$0} /path/to/chx"
      exit
    end
    $chx_exe = ARGV[0]
  end
end

def run_test(search_m, board_n, ply, svn_revs)
  svn_revs.each do | svn_rev, input |
    f = open(".test_unified","w")
    f.write "search #{search_m}\neval original\nbench ../inputs/#{board_n} #{ply} 1\n"
    f.close
    t1 = Time.now
    if input == :ini
      f = open(".test","w")
      if search_m == "mtdf" then
        search_m = "mtd-f"
      end
      f.write "[CHX Main]
                search_method = #{search_m}
                eval_method = original

                [Benchmark]
                mode=true
                file=../inputs/#{board_n}
                max_ply=#{ply}
                num_runs=1"
      f.close
      `chx_#{svn_rev}/src/chx -s .test`
      if search_m == "mtd-f" then
        search_m = "mtdf"
      end
    else
      `chx_#{svn_rev}/src/chx < .test_unified`
    end
    t2 = Time.now
    puts "%10s\t%5s\t%3d\t%8s\t%5.3f" % [search_m, board_n, ply, svn_rev, t2-t1]
  end

  t1 = Time.now
  `#{$chx_exe} < .test_unified`
  t2 = Time.now
  puts "%10s\t%5s\t%3d\t%8s\t%5.3f\n\n" % [search_m, board_n, ply, "current", t2-t1]
end

if $0 == __FILE__
  check_usage
  svn_revs.each do | svn_rev, input |
    make(svn_rev)
  end
  puts "    search\t board\tply\trevision\ttime (seconds)"
  puts "-" * 62
  search_ms.each do | search_m |
    boards.each do | board_n |
      plies.each do | ply |
        unless search_m == "minimax" && ply == 5 then
          run_test(search_m, board_n, ply, svn_revs)
        end
      end
    end
  end
end
