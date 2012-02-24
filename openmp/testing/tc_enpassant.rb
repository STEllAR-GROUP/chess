require "test/unit"
require "fileutils"
include FileUtils

$chx_exe = "../../build_chx/src/chx"
$success = "8  r n b q k b n r\n7  p p p . p p p .\n6  . . . P . . . p\n5  . . . . . . . .\n4  . . . . . . . .\n3  . . . . . . . .\n2  P P P P . P P P\n1  R N B Q K B N R"

class TestEnPassant < Test::Unit::TestCase


	def setup
		if Dir[$chx_exe].empty?
			puts "Please specify the path to the chx executable in $chx_exe"
			exit
		end
		f = open(".test","w+")
    f.write "e2e4\nh7h6\ne4e5\nd7d5\ne5d6\nd\nquit\n"
    f.close
	end

	def test_enpassant
		val = `#{$chx_exe} < .test`
		assert( val =~ /#{$success}/o )
	end
end