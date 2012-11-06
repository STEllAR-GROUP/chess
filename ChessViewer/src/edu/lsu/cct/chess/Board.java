package edu.lsu.cct.chess;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.RectF;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;

import com.example.chessviewer.R;


public class Board extends View {
	final static String INPUTS = "prnbkqPRNBKQ.";
	
	final int nbx = 2;
	final int nby = 3;
	final int nboards = nbx*nby;
	final int size = (int)(27*1.75);
	public int tstep = 1;
	public int now;
	public final int[] current = new int[nboards];
	public final int[] old = new int[nboards];
	
	public List<Piece> pieces = new ArrayList<Piece>();
	Bitmap[] pieceBitmaps = new Bitmap[255];

	public Board(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
		new readTask().execute();
		
	}

	void init() {
		pieceBitmaps['p'] = BitmapFactory.decodeResource(getResources(), R.drawable.black_pawn);
		pieceBitmaps['r'] = BitmapFactory.decodeResource(getResources(), R.drawable.black_rook);
		pieceBitmaps['b'] = BitmapFactory.decodeResource(getResources(), R.drawable.black_bishop);
		pieceBitmaps['n'] = BitmapFactory.decodeResource(getResources(), R.drawable.black_knight);
		pieceBitmaps['k'] = BitmapFactory.decodeResource(getResources(), R.drawable.black_king);
		pieceBitmaps['q'] = BitmapFactory.decodeResource(getResources(), R.drawable.black_queen);
		
		pieceBitmaps['P'] = BitmapFactory.decodeResource(getResources(), R.drawable.white_pawn);
		pieceBitmaps['R'] = BitmapFactory.decodeResource(getResources(), R.drawable.white_rook);
		pieceBitmaps['B'] = BitmapFactory.decodeResource(getResources(), R.drawable.white_bishop);
		pieceBitmaps['N'] = BitmapFactory.decodeResource(getResources(), R.drawable.white_knight);
		pieceBitmaps['K'] = BitmapFactory.decodeResource(getResources(), R.drawable.white_king);
		pieceBitmaps['Q'] = BitmapFactory.decodeResource(getResources(), R.drawable.white_queen);
		
	}
	@Override
	public void draw(Canvas canvas) {
		Log.i("Board-Draw", "Draw called");
		int width = getWidth();
		int height = getHeight();
		Paint black = new Paint();
		black.setColor(0xFFCC0000);
		Paint background_black=new Paint(0x00000000);
		Paint white = new Paint();
		white.setColor(0xFFEEEEEE);
		Point window_size;
		//WindowManager wm=(WindowManager)Context.getSystemService(Context.WINDOW_SERVICE);
		//getWindowManager().getDefaultDisplay().getSize(window_size);
		canvas.drawRect(0,0,10000,10000,background_black);
		for(int boardnum=0;boardnum<6;boardnum++) {
			for(int i=0;i<8;i++) {
				for(int j=0;j<8;j++) {
					int offx = (width-nbx*8*size)/(nbx+1);
					int offy = (height-nby*8*size)/(nby+1);
					int bx = boardnum % nbx;
					int by = (boardnum / nbx) % nby;
					int x = (bx+1)*offx+size*i+bx*8*size;
					int y = (by+1)*offy+size*j+by*8*size;
					RectF r = new RectF(x,y,x+size,y+size);
					canvas.drawRect(r, (i+j)%2==0 ? white : black);
				}
			}
		}
		for(Piece p : pieces) {
			Bitmap bitm = pieceBitmaps[p.p];
			if(bitm != null) {
				if(p.time != current[p.boardnum])
					continue;
				int boardnum = (p.boardnum)%(nbx*nby);
				int i = p.x;
				int j = p.y;
				int offx = (width-nbx*8*size)/(nbx+1);
				int offy = (height-nby*8*size)/(nby+1);
				int bx = boardnum % nbx;
				int by = (boardnum / nbx) % nby;
				int x = (bx+1)*offx+size*i+bx*8*size;
				int y = (by+1)*offy+size*j+by*8*size;
				Paint trans = new Paint();
//				trans.setAlpha(0x50);
				canvas.drawBitmap(bitm, x, y, trans );
			}
		}
	}
	
	/* 
	 * This task will read in the file in the background, then set the List we create
	 * here to the list that will be rendered and used by draw and animate to avoid
	 * ConcurrentModificationException
	 */
	private class readTask extends AsyncTask<Void, Void, Void> {
		private List<Piece> _pieces = new ArrayList<Piece>();
		
		@Override
		protected Void doInBackground(Void... params) {
			

			try {
				URL url = new URL("http://stevenrbrandt.com/board.txt");
				InputStream in = url.openStream();
				int x = 0, y = 0, boardnum = 0;
				_pieces.clear();
				outer: while (true) {
					int c = in.read();
					if (c < 0)
						break;

					// parse comment
					if (c == '#') {
						while (true) {
							c = in.read();
							if (c < 0)
								break outer;
							if (c == '\n')
								continue outer;
						}
					}

					if (INPUTS.indexOf(c)>=0) {
						_pieces.add(new Piece(tstep,boardnum, x, y, c));
						x++;
						if(x == 8) {
							y++;
							x=0;
						}
						if(y == 8) {
							x = y = 0;
						}
					} else if(c >= '0' && c <='9') {
						boardnum = c - '0';
						tstep++;
					}
				}
			} catch (MalformedURLException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}
			//size = Math.min(getWidth()/nbx, getHeight()/nby);
			for(int i=0;i<pieceBitmaps.length;i++) {
				Bitmap bitm = pieceBitmaps[i];
				if(bitm != null) {
					pieceBitmaps[i] = Bitmap.createScaledBitmap(bitm, size, size, false);
				}
			}
			return null;
		}
		
		@Override
		protected void onPostExecute(Void result) {
			super.onPostExecute(result);
			pieces = _pieces; //Set our list to the list being rendered.
			invalidate();
		}
		
	}
}
