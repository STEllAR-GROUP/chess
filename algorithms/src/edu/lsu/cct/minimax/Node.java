package edu.lsu.cct.minimax;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

public class Node {
	public final static int MAX=1000;
	final int value;
	int score;
	List<Node> children = new ArrayList<Node>();
	final private static Comparator<? super Node> sorter = new Comparator<Node>() {
		@Override
		public int compare(Node arg0, Node arg1) {
			return arg0.score - arg1.score;
		}
	};
	Node(Random r,int depth,int branching,int sign) {
		int v = sign*r.nextInt(MAX);
		score = 0;
		if(depth > 0) {
			v = 0;
			for(int i=0;i<branching;i++) {
				Node child = new Node(r,depth-1,branching,sign);
				children.add(child);
				v += child.value;
			}
			value = -(int)((1.0*v)/branching+.5);
		} else {
			value = v;
		}
	}
	void clear() {
		score = 0;
		nlower = -MAX;
		nupper = MAX;
		for(Node child : children)
			child.clear();
	}
	
	void minimax() {
		Counter c= new Counter();
		clear();
		minimax(c);
		System.out.println("minimax: score="+score+" in "+c+" steps");
	}
	void minimax(Counter c) {
		c.inc();
		if(children.size()==0) {
			score = value;
			return;
		}
		score = -MAX;
		for(Node child : children) {
			child.minimax(c);
			score = Math.max(score,-child.score);
		}
		Collections.sort(children,sorter);
		if(Game.rand.nextInt(3)==0)
			swap(0,1);
	}
	
	private void swap(int i, int j) {
		Node n = children.get(i);
		children.set(i, children.get(j));
		children.set(j, n);
	}
	public void check(int ans) {
		if(ans != score)
			throw new Error(ans+" != "+score);
	}

	void alphabeta(int ans) {
		Counter c= new Counter();
		clear();
		alphabeta(-MAX,MAX,c);
		check(ans);
		System.out.println("alphabeta: score="+score+" in "+c+" steps");
	}
	void alphabeta(int alpha,int beta,Counter c) {
		c.inc();
		if(children.size()==0) {
			score = value;
			return;
		}
		for(Node child : children) {
			child.alphabeta(-beta,-alpha,c);
			alpha = Math.max(alpha,-child.score);
			if(alpha >= beta)
				break;
		}
		score = alpha;
	}
	
	int nlower = -MAX, nupper = MAX;
	void alphabetamem(int ans) {
		Counter c= new Counter();
		clear();
		alphabetamem(-MAX,MAX,c);
		check(ans);
		System.out.println("alphabetamem: score="+score+" in "+c+" steps");
	}
	void alphabetamem(int alpha,int beta,Counter c) {
		if(beta > nupper)
			beta = nupper;
		if(alpha < nlower)
			alpha = nlower;
		if(alpha >= beta) {
			score = alpha;
			return;
		}
		c.inc();
		if(children.size()==0) {
			nlower = nupper = score = value;
			return;
		}
		int a = alpha;
		for(Node child : children) {
			child.alphabetamem(-beta,-a,c);
			a = Math.max(a,-child.score);
			if(a >= beta)
				break;
		}
		score = a;
		if(a < beta)
			nupper = a;
		if(a > alpha)
			nlower = a;
	}
	
	void negascout(int ans) {
		Counter c= new Counter();
		clear();
		negascout(-MAX,MAX,c);
		check(ans);
		System.out.println("negascout: score="+score+" in "+c+" steps");
	}
	void negascout(int alpha,int beta,Counter c) {
		c.inc();
		if(children.size()==0) {
			score = value;
			return;
		}
		int b = beta;
		for(Node child : children) {
			child.negascout(-b,-alpha,c);
			int a = -child.score;
			if(alpha < a && a < beta && beta > b) {
				child.negascout(-beta, -alpha, c);
				a = -child.score;
			}
			alpha = Math.max(a,alpha);
			if(alpha >= beta) {
				score = alpha;
				return;
			}
			b = alpha+1;
		}
		score = alpha;
	}
	
	void mtdf2(int ans,int f) {
		Counter c= new Counter();
		clear();
		mtdf2(f,c);
		check(ans);
		System.out.println("mtd-f2: score="+score+" in "+c+" steps");
	}
	void mtdf2(int f,Counter c) {
		int g = f;
		int upper = MAX;
		int lower = -MAX;
		while(lower < upper) {
			int beta = g;
			if(g == lower)
				beta = g+1;
			alphabeta(beta-1,beta,c);
			g = score;
			if(g < beta) {
				upper = g;
			}
			if(g > beta-1){
				lower = g;
			}
		}
		score = g;
	}

	
	void mtdf(int ans,int f) {
		Counter c= new Counter();
		clear();
		mtdf(f,c);
		check(ans);
		System.out.println("mtd-f: score="+score+" in "+c+" steps");
	}
	void mtdf(int f,Counter c) {
		int g = f;
		int upper = MAX;
		int lower = -MAX;
		while(lower < upper) {
			int beta = g;
			if(g == lower)
				beta = g+1;
			alphabetamem(beta-1,beta,c);
			g = score;
			if(g < beta) {
				upper = g;
			}
			if(g > beta-1){
				lower = g;
			}
		}
		score = g;
	}
	

	void srb(int ans,int f1,int f2) {
		Counter c= new Counter();
		clear();
		srb(f1,f2,c);
		check(ans);
		System.out.println("srb: score="+score+" in "+c+" steps");
	}
	void srb(int f1,int f2,Counter c) {
		alphabetamem(f1,f2,c);
		int g = score;
		int upper = nupper;
		int lower = nlower;
		while(lower < upper) {
			int beta = g;
			if(g == lower)
				beta = g+1;
			alphabetamem(beta-1,beta,c);
			g = score;
			if(g < beta) {
				upper = g;
			}
			if(g > beta-1){
				lower = g;
			}
		}
		score = g;
	}
	
	void print(Out out) {
		out.println(""+Math.abs(score)+"/"+Math.abs(value));
		out.indent += 2;
		for(Node child : children) {
			child.print(out);
		}
		out.indent -= 2;
	}
}
