package itsnotrocketscience;

public class Snapshot {
	private Point[] satellites;
	private double maxX, maxY;
	
	public Snapshot(String line) {
		String[] elems = line.trim().split(" +");
		assert elems.length > 0 && elems.length % 2 == 0;
		int n = elems.length / 2;
		satellites = new Point[n];
		maxX = maxY = Model.EARTH_RADIUS;
		for (int i=0; i<n; ++i) {
			Point p = new Point(
			    Double.parseDouble(elems[2*i]),
			    Double.parseDouble(elems[2*i+1]));
			maxX = Math.max(maxX, Math.abs(p.x));
			maxY = Math.max(maxY, Math.abs(p.y));
			satellites[i] = p;
		}
	}
	
	public double getMaxX() { return maxX; }
	public double getMaxY() { return maxY; }
	
	public int getNumberSatellites() { return satellites.length; }
	public Point getSatellite(int i) { return satellites[i]; }
}
