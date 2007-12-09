// TODO:
//  - Test window focus tracking testing (minimization, mouse focus)
//  - Onquit
//  - LongTextFrame horizontal overhang fix
//  - Recompute size twice a frame?
//  - Boxed text prompt frames
//  - Pinging on text prompt frames.
//  - Investigate ToX utilities, and their place in PrettyString
//  - Figure out system for fonts
//  - Should text prompts really track enter, escape?
//  - Button listener
//  - Tidy up clippedframe, scrollingframe if needed
//  - Fix up general style struct, especially initialization. Should it even exist?
//  - Page up/page down scrolling
//  - Scrolling Window: FocusFrame(WindowFrame(ScrollingFrame(MinSizeFrame()))
//  - Move ScrollingFrame to Base
//  - Tidy up scrollingframe, buttonframe

// Includes
#include "../Glop/source/Base.h"
#include "../Glop/source/BinaryFileManager.h"
#include "../Glop/source/GlopFrame.h"
#include "../Glop/source/GlopWindow.h"
#include "../Glop/source/Image.h"
#include "../Glop/source/Input.h"
#include "../Glop/source/System.h"
#include "../Glop/source/Thread.h"

// Globals
Image *gIcon;

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

void BuildMainMenu() {
  gWindow->SetTitle("Tests menu");
  ColFrame *column = new ColFrame(6, kJustifyLeft);
  column->SetCell(0, new TextFrame("1. DisplayMessage and full-screen modes", kWhite));
  column->SetCell(1, new TextFrame("2. Icon and Title", kWhite));
  column->SetCell(2, new TextFrame("3. Timing", kWhite));
  column->SetCell(3, new TextFrame("4. Input", kWhite));
  column->SetCell(4, new TextFrame("5. Threading", kWhite));
  column->SetCell(5, new TextFrame("6. Quit", kWhite));
  gWindow->AddFrame(column, 0.5f, 0.4f, 0.5f, 0.4f);
/*  gWindow->AddFrame(new ButtonWidget("Test Button Q", 'Q'), 0.5f, 0.4f, 0.5f, 0, 0);
  gWindow->AddFrame(new ButtonWidget("Test Button W", 'W'), 0.5f, 0.6f, 0.5f, 0, 0);
  gWindow->AddFrame(new FocusFrame(new SliderWidget(SliderWidget::Horizontal, 100, 1000, 50)), 0.5f, 0.7f, 0.5f, 0, 0);*/
  
  ColFrame *huge_col = new ColFrame(20, kJustifyLeft);
  for (int j = 0; j < 20; j++) {
    ColFrame *long_col = new ColFrame(52, kJustifyLeft);
    for (int i = 0; i < 26; i++) {
      long_col->SetCell(2*i, new TextFrame(Format("Option #%d:", i+1), kBlue/3));
      ColFrame *temp = new ColFrame(30);
      for (int j = 0; j < 5; j++)
        temp->SetCell(j, new TextFrame(Format("%c %c %c %c %c", i + 'A', i + 'A', i + 'A', i + 'A', i + 'A'), kRed/4)); 
      long_col->SetCell(2*i+1, new ButtonWidget(temp, i + 'A'));
    }
    GlopFrame *ff = new ScrollingFrame(long_col);
    WindowFrame *wnd = new WindowFrame(ff, Format("Scrolling Window #%d", j+1));
    huge_col->SetCell(j, new RecSizeFrame(wnd, 0.5f, 0.5f));
  }
  GlopFrame *outer_frame = new ScrollingFrame(huge_col);
  gWindow->AddFrame(new RecSizeFrame(outer_frame, 0.6f, 0.9f), 0.5f, 0.5f, 0.5f, 0.5f);
  /*StringPromptFrame *s = new StringPromptFrame("Thing #1", 100, kBlack);
  FocusFrame *f1 = new FocusFrame(new SolidBoxFrame(s, kWhite, kRed));
  gWindow->AddFrame(f1, 0.5f, 0.4f, 0.5f, 0, 0);
  StringPromptFrame *s2 = new StringPromptFrame("Thing #2", 100, kBlack);
  FocusFrame *f2 = new FocusFrame(new SolidBoxFrame(s2, kWhite, kRed));
  gWindow->AddFrame(f2, 0.5f, 0.6f, 0.5f, 0, 0);*/
}

int GlopMain(int argc, char **argv) {
  gDefaultStyle = new FrameStyle(gSystem->LoadFontOutline("thames.ttf"));
  gIcon = Image::Load("Icon.bmp", kRed, 1);
  BuildMainMenu();
  gWindow->SetIcon(gIcon);
  ASSERT(gWindow->Create(1024, 768, false));

  while (!input()->WasKeyPressed(27)) {
    int selection = 0;
    if (input()->WasKeyPressed('1')) selection = 1;
    if (input()->WasKeyPressed('2')) selection = 2;
    if (input()->WasKeyPressed('3')) selection = 3;
    if (input()->WasKeyPressed('4')) selection = 4;
    if (input()->WasKeyPressed('5')) selection = 5;
    if (input()->WasKeyPressed('6')) selection = 6;
    if (selection > 0) {
      gWindow->ClearFrames();
      if (selection == 1)
        DisplayMessageTest();
      else if (selection == 2)
        IconTitleTest();
      else if (selection == 3)
        TimeTest();
      else if (selection == 4)
        InputTest();
      else if (selection == 5)
        ThreadTest();
      else if (selection == 6)
        break;
      gWindow->ClearFrames();
      BuildMainMenu();
    }
    gSystem->Think();
  }

  return 0;
}
