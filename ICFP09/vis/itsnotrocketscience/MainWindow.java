package itsnotrocketscience;

import java.awt.BorderLayout;

import javax.swing.JFrame;
import javax.swing.JSlider;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

public class MainWindow {
	private final JFrame frame;
	private final Model model;
	private final JSlider timeSlider;
	private final UniverseView universeView;
	
	public MainWindow(Model m) {
		model = m;
		frame = new JFrame("Visualizer");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		timeSlider = new JSlider(0, model.getTotalTime()-1, 0);
		frame.add(timeSlider, BorderLayout.NORTH);
		timeSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg) {			
				model.setCurrentTime(timeSlider.getValue());
			}
		});
		model.addListener(new Runnable() {
			public void run() {
				timeSlider.setValue(model.getCurrentTime());
			}
		});
		
		universeView = new UniverseView(model);
		frame.add(universeView, BorderLayout.CENTER);
		
		frame.setSize(800, 600);
	}
	
	public void start() {
		frame.setVisible(true);
	}
}
