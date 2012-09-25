package edu.lsu.cct.minimax;

import java.util.Random;

public class Game {
	final static Random rand = getRandom();
	
	private static Random getRandom() {
		Random rand = new Random();
		long seed = rand.nextLong();
		System.out.println("seed="+seed);
		rand.setSeed(seed);
		return rand;
	}
	
	int depth = 6;
	int branching = 6;
	int count = 0;
	Node root = new Node(rand,depth,branching,1-2*(depth%2));
	
	public static void main(String[] args) {
		Game g = new Game();
		g.root.minimax();
		int ans = g.root.score;
		
		Out out = new Out();
		//g.root.print(out);
		g.root.alphabeta(ans);
		g.root.alphabetamem(ans);
		g.root.negascout(ans);
		int del = 20;
		int off = rand.nextInt(2*del)-del;
		g.root.mtdf(ans,ans+off);
		g.root.mtdf2(ans,ans+off);
		g.root.srb(ans,ans+off-del,ans+off+del);
	}
}
