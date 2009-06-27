package itsnotrocketscience;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Rectangle;

import javax.swing.JComponent;

public class UniverseView extends JComponent {
	private static final int SAT_RADIUS = 3;
	private static final long serialVersionUID = 786531234267056768L;
	private final Model model; 
	public UniverseView(Model m) {
		model = m;
		model.addListener(new Runnable() {
			public void run() { repaint(); }
		});
	}
	
	protected void paintComponent(Graphics g) {
		int sizeX = getWidth();
		int sizeY = getHeight();
		g.clearRect(0, 0, sizeX, sizeY);
		double centerx = sizeX / 2.0;
		double centery = sizeY / 2.0;
		double mult = 0.9 * Math.min(
				sizeX / (2.0 * model.getMaxX()),
				sizeY / (2.0 * model.getMaxY()));
		g.setColor(Color.BLUE);
		g.fillOval(
				(int)Math.round(centerx - mult * Model.EARTH_RADIUS),
				(int)Math.round(centery - mult * Model.EARTH_RADIUS),
				(int)Math.round(2*mult*Model.EARTH_RADIUS),
				(int)Math.round(2*mult*Model.EARTH_RADIUS));
		Snapshot s = model.getSnapshot(model.getCurrentTime());
		int n = s.getNumberSatellites();
		for (int i=0; i<n; ++i) {
			Point p = s.getSatellite(i);
			int x = (int)Math.round(centerx + mult * p.x);
			int y = (int)Math.round(centery - mult * p.y);
			g.setColor(i==0 ? Color.GREEN : Color.BLACK);
			g.fillOval(x-SAT_RADIUS, y-SAT_RADIUS, 2*SAT_RADIUS, 2*SAT_RADIUS);
		}
	}

}
