package itsnotrocketscience;

import javax.swing.UIManager;

public class Visualizer {

	public static void main(String[] args) throws Exception {
		System.out.println("It's Not Rocket Science visualizer 0.2");
		if (args.length != 1) {
			System.out.println("java itsnotrocketscience.Visualizer <coordinates file>");
			return;
		}
		UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		Model model = new Model(args[0]);

		MainWindow window = new MainWindow(model);
		window.start();
	}

}
