require "test/unit"
require "fileutils"
include FileUtils


class TestCastling < Test::Unit::TestCase


	def setup
@chx_exe = "../../build_chx/src/chx"
@input =
"""
a2a3
a7a6
b1c3
b8c6
b2b3
b7b6
c1b2
c8b7
d2d3
d7d6
d1d2
d8d7
e1c1
e8c8
d
quit

"""
@success = 
"""
8  . . k r . b n r
7  . b p q p p p p
6  p p n p . . . .
5  . . . . . . . .
4  . . . . . . . .
3  P P N P . . . .
2  . B P Q P P P P
1  . . K R . B N R

   a b c d e f g h
"""
		if Dir[@chx_exe].empty?
			puts "Please specify the path to the chx executable in $chx_exe"
			exit
		end
		f = open(".test","w+")
    f.write @input
    f.close
	end

	def test_castling
		val = `#{@chx_exe} < .test`
		assert( val =~ /#{@success}/o )
	end
end
