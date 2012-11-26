package com.example.chessviewer;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import edu.lsu.cct.chess.Board;
import edu.lsu.cct.chess.Piece;

public class ChessActivity extends Activity {
	
	boolean running=true;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        System.out.println("start");
        setContentView(R.layout.activity_chess);
        Timer t = new Timer();
        final Board board = (Board)findViewById(R.id.board1);
        final Animate animate = new Animate(board);
        TimerTask task = new TimerTask() {
        	public void run() {
        		System.out.println("run");
        		if(running){
        			runOnUiThread(animate);
        		}
        	}
        };
        t.scheduleAtFixedRate(task, 0, 50);
    }
	
	static class Animate implements Runnable {
		Board board;
		Animate(Board board) {
			this.board = board;
		}
		int ts = 0;
		
		//@Override
		public void run() {
			System.out.println("animate");
			for(Piece p : board.pieces) {
				int t = ts % board.tstep;
				if(t == p.time) {
					board.old[p.boardnum] = board.current[p.boardnum];
					board.current[p.boardnum] = t;
					board.now = t;
				}
			}
			board.invalidate();
			ts++;
		}
	}

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.mainmenu, menu);
        return true;
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item){
    	switch(item.getItemId()){
    	case R.id.Stop:
    		running=!running;
    		break;
    	}
    	return true;
    }
}
