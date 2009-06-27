package itsnotrocketscience;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

public class Model {
	public static final double EARTH_RADIUS = 6.357e6;
	private ArrayList<Snapshot> snapshots = new ArrayList<Snapshot>();
	private int currentTime;
	private ArrayList<Runnable> listeners = new ArrayList<Runnable>();
	private double maxX, maxY;
	
	public Model(String filename) throws IOException {
		BufferedReader reader = new BufferedReader(new FileReader(filename));
		maxX = EARTH_RADIUS; maxY = EARTH_RADIUS;
		for (;;) {
			String line = reader.readLine();
			if (line == null) break;
			Snapshot s = new Snapshot(line);
			maxX = Math.max(maxX, s.getMaxX());
			maxY = Math.max(maxY, s.getMaxY());
			snapshots.add(new Snapshot(line));
		}
		assert snapshots.size() > 0;
		currentTime = 0;
	}
	
	public int getTotalTime() { return snapshots.size(); }
	public int getCurrentTime() { return currentTime; }
	public void setCurrentTime(int t) {
		int old = currentTime;
		currentTime = t;
		if (currentTime != old) fire();
	} 
	
	public Snapshot getSnapshot(int time) { return snapshots.get(time); }
	
	public double getMaxX() { return maxX; }
	public double getMaxY() { return maxY; }
	
	public void addListener(Runnable l) { listeners.add(l); }

	private void fire() {
		for(Runnable r : listeners) r.run();
	}
}
