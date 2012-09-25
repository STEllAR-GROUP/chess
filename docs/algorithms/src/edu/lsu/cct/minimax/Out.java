package edu.lsu.cct.minimax;

public class Out {
	int indent = 0;
	void println(String s) {
		for(int i=0;i<indent;i++)
			System.out.print(' ');
		System.out.println(s);
	}
}
