// This file is designed as a kind of unit test for Glop. Run it and try the various tests to see
// that Glop is working correctly.

// TODO(darthur):
//  - Clean up this file
//  - Onquit
//  - Boxed text prompt frames
//  - Pinging on text prompt frames.
//  - Should text prompts really track enter, escape?
//  - Rework file stuff
//  - Why is object slightly visible even when deep in the fog?
//  - Add general GlopFrame comments, and formalize render expectations vis a vis gl settings

// Includes
#include "../Glop/include/Base.h"
#include "../Glop/include/BinaryFileManager.h"
#include "../Glop/include/Font.h"
#include "../Glop/include/GlopFrame.h"
#include "../Glop/include/GlopWindow.h"
#include "../Glop/include/Image.h"
#include "../Glop/include/Input.h"
#include "../Glop/include/OpenGl.h"
#include "../Glop/include/System.h"
#include "../Glop/include/Thread.h"
#include "../Glop/include/glop3d/Camera.h"

// Globals
Image *gIcon;

void IntroScreen() {
  GlopFrame *info = new FancyTextFrame("\1bu\1Glop Test Program\1/b/u\1\n\n"
                                       "Select tests to verify that Glop performs as expected.",
                                       kWhite);
  gWindow->AddFrame(info, 0.5f, 0.3f, 0.5f, 0.3f);
  input()->WaitForKeyPress();
  gWindow->ClearFrames();
}

class GlUtils2dTestFrame: public GlopFrame {
 public:
  void Render() const {
    GlUtils2d::FillRectangle(GetX(), GetY(), GetX2(), GetY2(), kYellow);
    GlUtils2d::DrawRectangle(GetX() + 1, GetY() + 1, GetX2() - 1, GetY2() - 1, kBlack);
    GlUtils2d::DrawLine(GetX() + 2, GetY() + 2, GetX2() - 2, GetY2() - 2, kBlue);
    GlUtils2d::DrawLine(GetX() + 2, GetY2() - 2, GetX2() - 2, GetY() + 2, kBlue);
    GlUtils2d::DrawLine(GetX2() - 2, GetY2() - 2, GetX() + 2, GetY() + 2, kRed);
    GlUtils2d::DrawLine(GetX2() - 2, GetY() + 2, GetX() + 2, GetY2() - 2, kRed);
  }
};

void GlUtils2dTest() {
  gWindow->AddFrame(new PaddedFrame(new GlUtils2dTestFrame(), 1));
  GlopFrame *info = new FancyTextFrame(
    "You should see a yellow filled box surrounded by a black box, surrounded "
    "by a yellow box. There should be red diagonals in the box (not overlapping "
    "the black part.)\n\n"
    "\1c0000FF\1Press any key to continue", kBlack);
  gWindow->AddFrame(new RecWidthFrame(info, 0.6f), 0.5f, 0.4f, 0.5f, 0.4f);
  input()->WaitForKeyPress();
}

void DisplayMessageTest() {
  vector<pair<int, int> > modes = gSystem->GetFullScreenModes();
  string message = "Video modes (in lexicographical order):\n\n";
  for (int i = 0; i < (int)modes.size(); i++)
    message += Format("%d by %d\n", modes[i].first, modes[i].second);
  DisplayMessage("Video modes", message);
  gWindow->AddFrame(new TextFrame("The legal full-screen video modes should have been displayed.",
                                   kWhite), 0.5f, 0.4f, kJustifyCenter, 0.4f);
  gWindow->AddFrame(new TextFrame("Press any key to continue...", kYellow),
                     0.5f, 1.0f, kJustifyCenter, kJustifyBottom);
  input()->WaitForKeyPress();
}

void IconTitleTest() {
  gWindow->SetIcon(0);
  gWindow->SetTitle("Icon and Title Test - Part 1");
  TextFrame *text1 = new TextFrame("Part 1: The title should be \"Icon and Title Test - Part 1\",",
                                   kWhite);
  TextFrame *text2 = new TextFrame("and the icon should be the default.",
                                   kWhite);
  ColFrame *col = new ColFrame(text1, text2);
  LightSetId id = gWindow->AddFrame(col, 0.5f, 0.4f, kJustifyCenter, 0.4f);
  gWindow->AddFrame(new TextFrame("Press any key to continue...", kYellow),
                     0.5f, 1.0f, kJustifyCenter, kJustifyBottom);
  input()->WaitForKeyPress();
  gWindow->SetIcon(gIcon);
  gWindow->SetTitle("Icon and Title Test - Part 2");
  text1->SetText("Part 2: The title should be \"Icon and Title Test - Part 2\",");
  text2->SetText("and the icon should be a custom icon with a transparent background.");
  input()->WaitForKeyPress();
}

void TimeTest() {
  TextFrame *prompt = new TextFrame("Trying to run at max speed:", kWhite);
  ColFrame *col = new ColFrame(prompt, new FpsFrame(kCyan));
  gWindow->AddFrame(col, 0.5f, 0.4f, kJustifyCenter, 0.4f);
  gWindow->AddFrame(new TextFrame("Press any key to continue...", kYellow),
                     0.5f, 1.0f, kJustifyCenter, kJustifyBottom);

  gSystem->SetMaxFps(0);
  input()->WaitForKeyPress();
  gSystem->SetMaxFps(100);
  prompt->SetText("Trying to run at 100 fps:");
  input()->WaitForKeyPress();
}

class EventLog: public KeyListener {
 public:
  EventLog() {}
  vector<string> GetLog() {
    vector<string> result = log_;
    log_.clear();
    return result;
  }
 protected:
  void OnKeyEvent(const KeyEvent &event, int dt) {
    if (event.IsNothing())
      return;
    string text(event.IsDoublePress()? "Double-press": event.IsNonRepeatPress()? "Press" :
                event.IsRepeatPress()? "Repeat" : "Release");
    text += ": " + event.key.GetName();
    int ascii = (event.IsNonRepeatPress()? input()->GetAsciiValue(event.key) : 0);
    if (ascii != 0)
      text += Format(" (%d, '%c')", ascii, ascii);
    log_.push_back(text);
  }

 private:
  vector<string> log_;
  DISALLOW_EVIL_CONSTRUCTORS(EventLog);
};

void InputTest() {
  EventLog event_log;
  TextFrame *mouse_pos_label = new TextFrame("Mouse position: ", kYellow);
  TextFrame *mouse_pos_value = new TextFrame("", kWhite);
  RowFrame *mouse_pos = new RowFrame(mouse_pos_label, mouse_pos_value);

  TextFrame *num_joysticks_label = new TextFrame("Num joysticks: ", kYellow);
  TextFrame *num_joysticks_value = new TextFrame("", kWhite);
  RowFrame *num_joysticks = new RowFrame(num_joysticks_label, num_joysticks_value);

  TextFrame *tracker_label = new TextFrame("Analog position tracker: ", kYellow);
  TextFrame *tracker_value = new TextFrame("", kWhite);
  RowFrame *tracker = new RowFrame(tracker_label, tracker_value);
  float tracker_x = 0, tracker_y = 0;

  TextFrame *pressed_keys_header = new TextFrame("Key press history:", kYellow);
  ColFrame *pressed_keys = new ColFrame(0, kJustifyLeft);
  ColFrame *pressed_keys_all = new ColFrame(pressed_keys_header, pressed_keys, kJustifyLeft);
  TextFrame *down_keys_header = new TextFrame("Keys down:", kYellow);
  ColFrame *down_keys = new ColFrame(0, kJustifyLeft);
  ColFrame *down_keys_all = new ColFrame(down_keys_header, down_keys, kJustifyLeft);
  TableauFrame *tableau = new TableauFrame();
  tableau->AddChild(pressed_keys_all, 0, 0, 0, 0);
  tableau->AddChild(down_keys_all, 0.5f, 0, 0, 0);

  ColFrame *main_col = new ColFrame(4, kJustifyLeft);
  main_col->SetCell(0, mouse_pos);
  main_col->SetCell(1, num_joysticks);
  main_col->SetCell(2, tracker);
  main_col->SetCell(3, tableau);

  gWindow->AddFrame(main_col, 0, 0, 0, 0);
  while (!input()->WasKeyPressed(27)) {
    int dt = gSystem->Think();

    // Update the mouse position
    mouse_pos_value->SetText(Format("(%d, %d)", input()->GetMouseX(), input()->GetMouseY()));

    // Update the number of joysticks
    input()->RefreshJoysticks();
    num_joysticks_value->SetText(Format("%d", input()->GetNumJoysticks()));

    // Update the tracker position
    float dx = 0, dy = 0;
    dx += input()->GetKeyPressAmountFrame(kMouseRight) -
          input()->GetKeyPressAmountFrame(kMouseLeft);
    dy += input()->GetKeyPressAmountFrame(kMouseDown) -
          input()->GetKeyPressAmountFrame(kMouseUp);
    for (int i = 0; i < input()->GetNumJoysticks(); i++) {
      dx += input()->GetKeyPressAmountFrame(GetJoystickRight(i)) -
            input()->GetKeyPressAmountFrame(GetJoystickLeft(i));
      dy += input()->GetKeyPressAmountFrame(GetJoystickDown(i)) -
            input()->GetKeyPressAmountFrame(GetJoystickUp(i));
    }
    tracker_x += dx * dt / 1000;
    tracker_y += dy * dt / 1000;
    tracker_value->SetText(Format("(%.3f, %.3f)", tracker_x, tracker_y));

    // Update the key events
    vector<string> events = event_log.GetLog();
    for (int i = 0; i < (int)events.size(); i++) {
      TextFrame *new_frame = new TextFrame(events[i], kWhite);
      pressed_keys->InsertCell(pressed_keys->GetNumCells(), new_frame);
      if (pressed_keys->GetNumCells() > 26)
        pressed_keys->DeleteCell(0);
    }

    // Update the keys down
    vector<GlopKey> keys = input()->GetDownKeysFrame();
    down_keys->Resize((int)keys.size());
    for (int i = 0; i < (int)keys.size(); i++) {
      string text = Format("%s (%f)", keys[i].GetName().c_str(),
                           input()->GetKeyPressAmountFrame(keys[i]));
      TextFrame *new_frame = new TextFrame(text, kWhite);
      down_keys->SetCell(i, new_frame);
    }
  } 
}

class AdderThread: public Thread {
 public:
  AdderThread(int *value, int repeats, Mutex *mutex)
  : value_(value), repeats_(repeats), mutex_(mutex) {}

 protected:
  virtual void Run() {
    for (int i = 0; i < repeats_; i++) {
      MutexLock lock(mutex_);
      (*value_)++;
    }
  }

 private:
  int *value_, repeats_;
  Mutex *mutex_;
};

void ThreadTest() {
  const int kNumThreads = 30, kRepeat = 10000;
  int value = 0;
  Mutex mutex;
  vector<Thread*> threads;
  for (int i = 0; i < kNumThreads; i++) {
    threads.push_back(new AdderThread(&value, kRepeat, &mutex));
    threads[i]->Start();
  }
  for (int i = 0; i < (int)threads.size(); i++) {
    threads[i]->Join();
    delete threads[i];
  }

  string info = "Test " + string(value == kNumThreads * kRepeat? "passed!" : "failed!");
  gWindow->AddFrame(new TextFrame(info, kWhite), 0.5f, 0.4f, kJustifyCenter, 0.4f);
  gWindow->AddFrame(new TextFrame("Press any key to continue...", kYellow),
                     0.5f, 1.0f, kJustifyCenter, kJustifyBottom);
  input()->WaitForKeyPress();
}

class CubeFrame: public CameraFrame {
 public:
  CubeFrame(): pos_(Vec3(0,0,6)) {
    SetFog(kWhite*0.3f, 5, 8);
  }

  void Render3d() const {
    float m[16];
    glPushMatrix();
    pos_.FillTransformationMatrix(m);
    glMultMatrixf(m);

    glBegin(GL_QUADS);
    GlUtils::SetColor(kBlue);
		glVertex3f( 1.0f, 1.0f,-1.0f);
		glVertex3f(-1.0f, 1.0f,-1.0f);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f( 1.0f, 1.0f, 1.0f);
    GlUtils::SetColor(kRed);
		glVertex3f( 1.0f,-1.0f, 1.0f);
		glVertex3f(-1.0f,-1.0f, 1.0f);
		glVertex3f(-1.0f,-1.0f,-1.0f);
		glVertex3f( 1.0f,-1.0f,-1.0f);
    GlUtils::SetColor(kGreen);
		glVertex3f( 1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f,-1.0f, 1.0f);
		glVertex3f( 1.0f,-1.0f, 1.0f);
    GlUtils::SetColor(kYellow);
		glVertex3f( 1.0f,-1.0f,-1.0f);
		glVertex3f(-1.0f,-1.0f,-1.0f);
		glVertex3f(-1.0f, 1.0f,-1.0f);
		glVertex3f( 1.0f, 1.0f,-1.0f);
    GlUtils::SetColor(kWhite);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f,-1.0f);
		glVertex3f(-1.0f,-1.0f,-1.0f);
		glVertex3f(-1.0f,-1.0f, 1.0f);
    GlUtils::SetColor(kPurple);
		glVertex3f( 1.0f, 1.0f,-1.0f);
		glVertex3f( 1.0f, 1.0f, 1.0f);
		glVertex3f( 1.0f,-1.0f, 1.0f);
		glVertex3f( 1.0f,-1.0f,-1.0f);
	  glEnd();	

    glPopMatrix();
    GlUtils::SetColor(kWhite);
  }

  void Think(int dt) {
    pos_.Rotate(Vec3(1,2,3), dt*0.1f);
    Camera c = camera();
    float rx = input()->GetKeyPressAmountFrame(kMouseRight) -
               input()->GetKeyPressAmountFrame(kMouseLeft);
    float ry = input()->GetKeyPressAmountFrame(kMouseDown) -
               input()->GetKeyPressAmountFrame(kMouseUp);
    float strafe = input()->GetKeyPressAmountFrame('D') - input()->GetKeyPressAmountFrame('A');
    float step = input()->GetKeyPressAmountFrame('W') - input()->GetKeyPressAmountFrame('S');
    c.Rotate(kYAxis, dt*0.2f*rx);
    c.Rotate(c.right(), dt*0.2f*ry);
    c.Translate(step * c.forwards() * 0.1f + strafe * c.right() * 0.1f);
    SetCamera(c);
  }

 private:
  Viewpoint pos_;
};

void CameraTest() {
  GlopFrame *info = new FancyTextFrame("Rotating Cube with fog\n\n"
                                       "Move the camera with the mouse and with W,A,D,S\n\n\n"
                                       "\1cFFFF00\1Press Escape to continue",
                                       kWhite);
  GlopFrame *cube = new HollowBoxFrame(new CubeFrame(), kWhite);
  GlopFrame *content = new ColFrame(
    new PaddedFrame(cube, 10), CellSize::Default(),
    CellSize::Max(), info, CellSize::Default(), CellSize::Default());
  gWindow->AddFrame(content);
  while (!input()->WasKeyPressed(27))
    gSystem->Think();
}

void BuildMainMenu() {
  gWindow->SetTitle("Tests menu");
  ColFrame *column = new ColFrame(8, kJustifyLeft);
  column->SetCell(0, new TextFrame("1. 2d rendering test", kWhite));
  column->SetCell(1, new TextFrame("2. DisplayMessage and full-screen modes", kWhite));
  column->SetCell(2, new TextFrame("3. Icon and Title", kWhite));
  column->SetCell(3, new TextFrame("4. Timing", kWhite));
  column->SetCell(4, new TextFrame("5. Input", kWhite));
  column->SetCell(5, new TextFrame("6. Threading", kWhite));
  column->SetCell(6, new TextFrame("7. Camera frame", kWhite));
  column->SetCell(7, new TextFrame("8. Quit", kWhite));
  gWindow->AddFrame(column, 0.5f, 0.4f, 0.5f, 0.4f);
  gSystem->Think();
}

int main(int argc, char **argv) {
  Font *font;
  
  System::Init();
  gSystem->SetMaxFps(0);
  
  ASSERT((font = Font::Load("thames.ttf")) != 0);
  ASSERT((gIcon = Image::Load("Icon.bmp", kRed, 1)) != 0);
  gFrameStyle = new FrameStyle(font);
 
  gWindow->SetIcon(gIcon);
  ASSERT(gWindow->Create(1024, 768, false));
  IntroScreen();
  BuildMainMenu();
  while (!input()->WasKeyPressed(27)) {
    int selection = 0;
    if (input()->WasKeyPressed('1')) selection = 1;
    if (input()->WasKeyPressed('2')) selection = 2;
    if (input()->WasKeyPressed('3')) selection = 3;
    if (input()->WasKeyPressed('4')) selection = 4;
    if (input()->WasKeyPressed('5')) selection = 5;
    if (input()->WasKeyPressed('6')) selection = 6;
    if (input()->WasKeyPressed('7')) selection = 7;
    if (input()->WasKeyPressed('8')) selection = 8;
    if (selection > 0) {
      gWindow->ClearFrames();
      if (selection == 1)
        GlUtils2dTest();
      else if (selection == 2)
        DisplayMessageTest();
      else if (selection == 3)
        IconTitleTest();
      else if (selection == 4)
        TimeTest();
      else if (selection == 5)
        InputTest();
      else if (selection == 6)
        ThreadTest();
      else if (selection == 7)
        CameraTest();
      else if (selection == 8)
        break;
      gWindow->ClearFrames();
      BuildMainMenu();
    }
    gSystem->Think();
  }
  return 0;
}
