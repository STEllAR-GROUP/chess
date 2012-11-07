package edu.lsu.cct.chess;

public class Piece {
	public final int boardnum;
	final int x;
	final int y;
	final int p;
	public final int time;
	Piece(int t,int b,int x,int y,int p) {
		this.time = t;
		this.boardnum = b;
		this.x = x;
		this.y = y;
		this.p = p;
	}
	public String toString() {
		return ((char)p) + "("+time+","+boardnum+","+x+","+y+")";
	}
}