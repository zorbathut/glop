// This file is designed as a kind of unit test for Glop. Run it and try the various tests to see
// that Glop is working correctly.

// TODO(darthur):
//  - Trap window-close events, and do not necessarily exit program when it happens
//  - Cease tab grab in some way on slider motion (due to ping?)
//  - Think more about rendering order, perhaps add movetofront to multiparentframe
//  - Add apple command button
//  - Add copy/paste, and shift click in text boxes
//  - Shift-tab to back-tab then releasing shift sucks. Add delay for switching directions. OR make
//    a general change to derived keys?
//  - Make file stuff good
//  - Make a ScrollingInputBoxFrame so that we don't have have extra pixel nastiness with scroll
//    bars
//  - Check gui key change in input
//  - Figure out why camera frame movement is too fast when vsync is off
//  - Directly closing the console can crash?

// Includes
#include <Glop/source/Base.h>
#include <Glop/source/BinaryFileManager.h>
#include <Glop/source/Font.h>
#include <Glop/source/GlopFrame.h>
#include <Glop/source/GlopWindow.h>
#include <Glop/source/Image.h>
#include <Glop/source/Input.h>
#include <Glop/source/OpenGl.h>
#include <Glop/source/System.h>
#include <Glop/source/Thread.h>
#include <Glop/source/glop3d/Camera.h>
#include <Glop/source/glop3d/Mesh.h>

// Constants
const string kTitle = "Glop Tests";

// Globals
Image *gIcon;

// Splash screen
// -------------

void IntroScreen() {
  GlopFrame *info = new FancyTextFrame("\1BUCff8080\1Glop Test Program\1Cffffff/B/U\1\n\n"
                                       "Select tests to verify that Glop performs as expected.");
  GlopFrame *img = new HollowBoxFrame(new ImageFrame("glop.jpg"), kWhite);
	window()->AddFrame(new ColFrame(info, new RecHeightFrame(new EmptyFrame(), 0.1f), img));
  input()->WaitForKeyPress();
  window()->ClearFrames();
}

// 2d rendering test
// -----------------

class GlUtils2dTestFrame: public GlopFrame {
 public:
  void Render() const {
    GlUtils2d::FillRectangle(GetX(), GetY(), GetX2(), GetY2(), kYellow);
    GlUtils2d::DrawRectangle(GetX() + 1, GetY() + 1, GetX2() - 1, GetY2() - 1, kBlack);
    GlUtils2d::DrawLine(GetX2() - 2, GetY2() - 2, GetX() + 2, GetY() + 2, kRed);
    GlUtils2d::DrawLine(GetX2() - 2, GetY() + 2, GetX() + 2, GetY2() - 2, kRed);
    GlUtils2d::DrawLine(GetX() + 2, GetY() + 2, GetX2() - 2, GetY2() - 2, kBlue);
    GlUtils2d::DrawLine(GetX() + 2, GetY2() - 2, GetX2() - 2, GetY() + 2, kBlue);
  }
};
void GlUtils2dTest() {
  window()->AddFrame(new PaddedFrame(new GlUtils2dTestFrame(), 1));
  GlopFrame *info = new FancyTextFrame(
    "You should see a yellow filled box surrounded by a black box, surrounded "
    "by a yellow box. There should be red diagonals in the box (not overlapping "
    "the black part.)\n\n"
    "\1C0000ff\1Press any key to continue", kBlack);
  window()->AddFrame(new RecWidthFrame(info, 0.6f), 0.5f, 0.4f, 0.5f, 0.4f);
  input()->WaitForKeyPress();
}

// Full-screen test
// ----------------

void FullScreenTest() {
  // Create the display mode menu
  vector<pair<int, int> > modes = system()->GetFullScreenModes();
  MenuWidget *menu = new MenuWidget(kJustifyLeft);
  ButtonWidget *done_button = new ButtonWidget("Done", kGuiKeyCancel);
  menu->AddTextItem("Windowed");
  for (int i = 0; i < (int)modes.size(); i++)
    menu->AddTextItem(Format("Full-screen: %d by %d\n", modes[i].first, modes[i].second));
  ColFrame *main_col = new ColFrame(
    new TextFrame("Select a new display mode:"), CellSize::Default(), CellSize::Default(),
    new InputBoxFrame(new ScrollingFrame(menu)), CellSize::Default(), CellSize::Max(),
    done_button, CellSize::Default(), CellSize::Default());
  main_col->SetCellJustify(0, kJustifyLeft);
  main_col->SetPadding(0.02f);
  ScalingPaddedFrame *interior = new ScalingPaddedFrame(main_col, 0.02f);
  window()->AddFrame(new RecSizeFrame(new WindowFrame(interior, "Full-screen test"), 0.8f, 0.8f));

  // Allow the user to switch display mode
  while (!done_button->WasHeldDown()) {
    if (menu->IsConfirmed()) {
      int i = menu->GetSelection() - 1;
      if (i == -1) {
        window()->Create(window()->GetWidth(), window()->GetHeight(), false);
      } else {
        window()->Create(modes[i].first, modes[i].second, true);
      }
      menu->Confirm(false);
    }
    system()->Think();
  }
}

// Icon, title and DisplayMessage test
// -----------------------------------

void IconTitleTest() {
  window()->SetIcon(0);
  window()->SetTitle("Icon and Title Test - Part 1");
  system()->MessageBox("Icon and Title test",
    "Part 1: The title should be \"Icon and Title Test - Part 1\", and the icon "
    "should be the default.");
  window()->SetIcon(gIcon);
  window()->SetTitle("Icon and Title Test - Part 2");
  system()->MessageBox("Icon and Title test",
    "Part 2: The title should be \"Icon and Title Test - Part 2\", and the icon "
    "should be a custom icon with a transparent background.");
  window()->SetTitle(kTitle);
}

// Timing and vertical sync test
// -----------------------------

void TimeTest() {
  TextFrame *prompt = new TextFrame("Trying to run at max speed:", kWhite);
  ColFrame *col = new ColFrame(prompt, new FpsFrame(kCyan));
  window()->AddFrame(col, 0.5f, 0.4f, kJustifyCenter, 0.4f);
  window()->AddFrame(new TextFrame("Press any key to continue...", kYellow),
                     0.5f, 1.0f, kJustifyCenter, kJustifyBottom);

  window()->SetVSync(false);
  input()->WaitForKeyPress();
  window()->SetVSync(true);
  prompt->SetText("Trying to run synced with vertical refresh:");
  input()->WaitForKeyPress();
}

// Input test
// ----------

class EventLog: public KeyListener {
 public:
  EventLog() {}
  vector<string> GetLog() {
    vector<string> result = log_;
    log_.clear();
    return result;
  }
 protected:
  void OnKeyEvent(const KeyEvent &event) {
    // Record key events into a log
    if (event.IsNothing())
      return;
    for (int i = 0; i < (int)event.keys.size(); i++) {
      string text(event.IsDoublePress()? "Double-press": event.IsNonRepeatPress()? "Press" :
                  event.IsRepeatPress()? "Repeat" : "Release");
      text += ": " + event.keys[i].GetName();
      int ascii = (event.IsNonRepeatPress()? input()->GetAsciiValue(event.keys[i]) : 0);
      if (ascii != 0)
        text += Format(" (%d, '%c')", ascii, ascii);
      log_.push_back(text);
    }
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

  window()->AddFrame(main_col, 0, 0, 0, 0);
  while (!input()->WasKeyPressed(kKeyEscape)) {
    int dt = system()->Think();

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

// Threading test
// --------------

class AdderThread: public Thread {
 public:
  AdderThread(int *value, int repeats, Mutex *mutex)
  : value_(value), repeats_(repeats), mutex_(mutex) {}
 protected:
  virtual void Run() {
    // Attempt to add 1 to *value a number of times
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

  TextFrame *result = new TextFrame("", kWhite);
  ColFrame *col = new ColFrame(new TextFrame("Performing thread test...", kWhite), result);
  window()->AddFrame(col, 0.5f, 0.4f, kJustifyCenter, 0.4f);
  system()->Think();

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

  result->SetText("Test " + string(value == kNumThreads * kRepeat? "passed!" : "failed!"));
  window()->AddFrame(new TextFrame("Press any key to continue...", kYellow),
                     0.5f, 1.0f, kJustifyCenter, kJustifyBottom);
  input()->WaitForKeyPress();
}

// Camera frame test
// -----------------

class CubeFrame: public CameraFrame {
 public:
  CubeFrame(): pos_(Vec3(0,0,6)) {
    SetFog(kWhite*0.3f, 5, 8);
    texture_ = Texture::Load("ninja.jpg");
    ASSERT(texture_ != 0);
    mesh_ = StockMeshes::NewCubeMesh(2, kWhite, texture_);
  }

  ~CubeFrame() {
    delete texture_;
  }

  void Render3d() const {
    // Display a rotating cube
    float m[16];
    glPushMatrix();
    pos_.FillTransformationMatrix(m);
    glMultMatrixf(m);
    mesh_->Render();
    glPopMatrix();
  }

  void Think(int dt) {
    pos_.Rotate(Vec3(1,2,3), dt*0.1f);
    Camera c = GetCamera();
    float rx = input()->GetKeyPressAmountFrame(kMouseRight) -
               input()->GetKeyPressAmountFrame(kMouseLeft);
    float ry = input()->GetKeyPressAmountFrame(kMouseDown) -
               input()->GetKeyPressAmountFrame(kMouseUp);
    float strafe = input()->GetKeyPressAmountFrame('d') - input()->GetKeyPressAmountFrame('a');
    float step = input()->GetKeyPressAmountFrame('w') - input()->GetKeyPressAmountFrame('s');
    c.Rotate(kYAxis3, dt*0.2f*rx);
    c.Rotate(c.right(), dt*0.2f*ry);
    c.Translate(step * c.forwards() * 0.1f + strafe * c.right() * 0.1f);
    SetCamera(c);
  }

 private:
  Texture *texture_;
  Mesh *mesh_;
  Viewpoint pos_;
};

void CameraTest() {
  GlopFrame *info = new FancyTextFrame("Rotating Cube with fog\n\n"
                                       "Move the camera with the mouse and with W,A,D,S\n\n\n"
                                       "\1Cffff00\1Press Escape to continue",
                                       kWhite);
  GlopFrame *cube = new HollowBoxFrame(new CubeFrame(), kWhite);
  GlopFrame *content = new ColFrame(
    new PaddedFrame(cube, 10), CellSize::Default(),
    CellSize::Max(), info, CellSize::Default(), CellSize::Default());
  window()->AddFrame(content);
  while (!input()->WasKeyPressed(kKeyEscape))
    system()->Think();
}

// Dialog box test
// ---------------

void DialogTest() {
  string text = "This is a long string of text from \1U\1Ender's Game\1/U\1. It is a good "
                "test for scrolling and for fancy text frames:\1IC000040\1\n\n"
                "But they let go of him. And as soon as they did, Ender kicked out high and hard, "
                "catching Stilson square in the breastbone. He dropped. It took Ender by surprise "
                "-- he hadn't thought to put Stilson on the ground with one kick. It didn't occur "
                "to him that Stilson didn't take a fight like this seriously, that he wasn't "
                "prepared for a truly desperate blow.\n\n"
                "For a moment, the others backed away and Stilson lay motionless. They were all "
                "wondering if he was dead. Ender, however, was trying to figure out a way to "
                "forestall vengeance. To keep them from taking him in a pack tomorrow. I have to "
                "win this now, and for all time, or I'll fight it every day and it will get worse "
                "and worse.\n\n"
                "Ender knew the unspoken rules of manly warfare, even though he was only six. It "
                "was forbidden to strike the opponent who lay helpless on the ground, only an "
                "animal would do that.\n\n"
                "So Ender walked to Stilson's supine body and kicked him again, viciously, in the "
                "ribs. Stilson groaned and rolled away from him. Ender walked around him and "
                "kicked him again, in the crotch. Stilson could not make a sound; he only doubled "
                "up and tears streamed out of his eyes.\n\n"
                "Then Ender looked at the others coldly. \"You might be having some idea of "
                "ganging up on me. You could probably beat me up pretty bad. But just remember "
                "what I do to people who try to hurt me. From then on you'd be wondering when I'd "
                "get you, and how bad it would be.\" He kicked Stilson in the face. Blood from his "
                "nose spattered the ground. \"It wouldn't be this bad,\" Ender said. \"It would be "
                "worse.\"\n\n"
                "He turned and walked away. Nobody followed him. He turned a corner into the "
                "corridor leading to the bus stop. He could hear the boys behind him saying, "
                "\"Geez. Look at him. He's wasted.\" Ender leaned his head against the wall of the "
                "corridor and cried until the bus came. I am just like Peter. Take my monitor "
                "away, and I am just like Peter.";
  string temp;
  DialogWidget::StringPromptOkayCancel("Dialog Frame Test", text, "And this is a text box:",
                                       "No wai!", 100, &temp);
}

// Menu test
// ---------

void MenuTest() {
  // Create the menu
  MenuWidget *menu = new MenuWidget(2, true, kJustifyLeft);
  GlopKey key_prompt_result;  // Dummy addresses for storing results
  int string_select_result;
  string string_prompt_result;
  int integer_prompt_result;
  vector<string> string_select_options;
  string_select_options.push_back("Apple");
  string_select_options.push_back("Banana");
  string_select_options.push_back("Carrot");
  string_select_options.push_back("Domination");
  for (int i = 0; i < 1000; i++) {
    if (i % 4 == 0) {
      menu->AddKeyPromptItem(Format("%d: Key prompt: ", i+1), kKeyEnter, kNoKey, kKeyEscape,
                             &key_prompt_result);
    } else if (i % 4 == 1) {
      menu->AddStringSelectItem(Format("%d: String select: ", i+1), string_select_options, 0,
                                &string_select_result);
    } else if (i % 4 == 2) {
      menu->AddStringPromptItem(Format("%d: String prompt: ", i+1), "A string!", 100,
                                &string_prompt_result);
    } else {
      menu->AddIntegerPromptItem(Format("%d: Integer prompt: ", i+1), 1, 0, 1000,
                                 &integer_prompt_result);
    }
  }
  menu->SetBasicBorderStyle(0, MenuWidget::ExactlyRecSize);
  menu->SetSelectionAndPing(500, true);

  // Create the full displayed frame
  vector<pair<int, int> > modes = system()->GetFullScreenModes();
  ButtonWidget *done_button = new ButtonWidget("Done", kGuiKeyCancel);
  ColFrame *main_col = new ColFrame(
    new InputBoxFrame(new ScrollingFrame(menu)), CellSize::Default(), CellSize::Max(),
    done_button, CellSize::Default(), CellSize::Default());
  main_col->SetPadding(0.02f);
  ScalingPaddedFrame *interior = new ScalingPaddedFrame(main_col, 0.02f);
  window()->AddFrame(new RecSizeFrame(new WindowFrame(interior, "Menu test"), 0.8f, 0.8f));

  // Wait until done
  while (window()->IsCreated() && !done_button->WasPressedFully())
    system()->Think();
}

// Main program
// ------------

// Allow the user to select a test to run
int RunMenu(int selection) {
  window()->ClearFrames();
  MenuWidget *menu = new MenuWidget();
  menu->SetBasicBorderStyle(1, MenuWidget::AtLeastRecSize);
  menu->AddTextItem("1. 2d rendering test");
  menu->AddTextItem("2. Full-screen test");
  menu->AddTextItem("3. Icon and Title");
  menu->AddTextItem("4. Timing");
  menu->AddTextItem("5. Input");
  menu->AddTextItem("6. Threading");
  menu->AddTextItem("7. Camera frame");
  menu->AddTextItem("8. Dialog box");
  menu->AddTextItem("9. Menu");
  int result = menu->AddTextItem("10. Quit");
  menu->SetSelection(selection);
  window()->AddFrame(new RecSizeFrame(new WindowFrame(menu, "Menu"), 0.4f, 0.8f),
                     0.5f, 0.4f, 0.5f, 0.4f);

  while (1) {
    system()->Think();
    if (input()->WasKeyPressed(27))
      break;
    if (menu->IsConfirmed()) {
      result = menu->GetSelection();
      break;
    }
  }

  window()->ClearFrames();
  return result;
}

int main(int argc, char **argv) {
  // Initialize
  Font *font;
  LogToFile("log.txt", true);
  System::Init(); 
  ASSERT((font = GradientFont::Load("thames.ttf", 1.0f, 0.5f, -0.3f, 1.0f)) != 0);
  ASSERT((gIcon = Image::Load("Icon.bmp", kRed, 1)) != 0);
  InitDefaultFrameStyle(font);
  window()->SetTitle(kTitle);
  window()->SetVSync(true);
  window()->SetIcon(gIcon);
  ASSERT(window()->Create(1024, 768, false));

  // Splash screen
  IntroScreen();

  // Execute
  int result = 0;
  while (1) {
    result = RunMenu(result);
    switch (result) {
      case 0:
        GlUtils2dTest();
        break;
      case 1:
        FullScreenTest();
        break;
      case 2:
        IconTitleTest();
        break;
      case 3:
        TimeTest();
        break;
      case 4:
        InputTest();
        break;
      case 5:
        ThreadTest();
        break;
      case 6:
        CameraTest();
        break;
      case 7:
        DialogTest();
        break;
      case 8:
        MenuTest();
        break;
      case 9:
        return 0;
    }
  }
  return 0;
}
