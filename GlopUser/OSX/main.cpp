#include <Glop/Os.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <stdio.h>
#include <math.h>

int main() {
	Os::Init();
  vector<OsWindowData*> windows;
  windows.push_back(Os::CreateWindow("Victory", 675, 75, 500, 400, false, 0, NULL, true));
  windows.push_back(Os::CreateWindow("Thunder", 75, 75, 500, 400, false, 0, NULL, true));
  windows.push_back(Os::CreateWindow("Domination", 75, 775, 500, 400, false, 0, NULL, true));
  int count = 0;
	while(1) {
		Os::Think();
    printf("Time: %d\n",Os::GetTime());
    for (int i = 0; i < windows.size(); i++, count++) {
      bool is_in_focus;
      bool focus_changed;
//      printf("%s:\t%d %d\n",(i==0?"Thunder":"Victory"), is_in_focus, focus_changed);
//      Os::SetTitle(windows[i], buffer);
      Os::SetCurrentContext(windows[i]);
  		glEnable(GL_DEPTH_TEST);
  		glShadeModel(GL_SMOOTH);    
  		glClearColor(0.0,0.0,0.0,0.0);
  		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glLoadIdentity();   // Reset the current modelview matrix
      gluLookAt(-1,0,0, 0,0,0, 0,1,0);

      glBegin( GL_TRIANGLES );             // Draw a triangle
      glColor3f( ((i+0)%3)/3.0, ((i+1)%3)/3.0, ((i+2)%3)/3.0 );        // Set color to red
      glVertex3f(  0.0f,  cos(clock()/100000.0 + i), sin(clock()/100000.0 + i));    // Top
      glColor3f( 0.0f, 0.0f, 1.0f );        // Set color to green
      glVertex3f(  0.0f, -cos(clock()/100000.0 + i), sin(clock()/100000.0 + i) );    // Bottom left
      glColor3f( 0.0f, 0.0f, 1.0f );        // Set color to blue
      glVertex3f(  0.0f, -sin(clock()/100000.0 + i), cos(clock()/100000.0 + i));    // Bottom right
      glEnd();                             // Done with the triangle
    }

	};
	return 0;
}