// This file contains a set of useful stand-alone frames.
// WARNING: Many of these frames require a valid font, which is not loaded by default. To avoid
//          errors, load a font (see Font.h) and call InitDefaultFrameStyle (see FrameStyle.h).
//
// Conventions: Many of these frames are designed to be easily customizable by user programs. To
//              facilitate changing how frames look, they delegate to View objects for all
//              rendering. These View objects are defined in GlopFrameStyle.
//
//              To faciliate changing how frames act, many frames have a Dummy version. These have
//              all the same essential features but their state can only be changed
//              programmatically. These are then overloaded to give the desired functionality. These
//              overloads further support some key rebindings. As much as possible, they depend only
//              on the Gui derived keys in Input. Thus, their behavior can be changed by remapping
//              those keys.
//
//              Finally, recall that a frame only receives input events if it is wrapped in a
//              FocusFrame. By convention, all the major interactive frames here have a convenience
//              Widget that is the frame wrapped inside a FocusFrame.
//
//              See, for example, ButtonView, DummyButtonFrame, ButtonFrame, ButtonWidget.
//
// Decorative Frames
// =================
//
// EmptyFrame: A convenience frame that takes max size and renders nothing.
// SolidBoxFrame, HollowBoxFrame: Solid or hollow boxes, possibly sized to fit around an existing
//                                frame.
// InputBoxFrame: Similar to SolidBoxFrame, but based on an InputBoxView. Used as background for
//                text boxes, etc.
// ImageFrame: Renders an image, magnified as much as possible.
// ArrowFrame: Renders an arrow in some direction. Used for slider buttons.
// WindowFrame: A decorative, unmovable window, optionally with a title.
// TextFrame, FancyTextFrame: Text output. TextFrame is faster but requires a uniform text style
//                            with no new lines. FancyTextFrame can handle new lines and changing
//                            style within the text.
// FpsFrame: Text output, always giving the FPS at which Glop is running.
//
//
// Interactive GUI Widgets
// =======================
//
// StringPromptWidget: A text box that accepts strings.
// IntegerPromptWidget: A text box that accepts integers.
// ButtonWidget: A basic push-button.
// SliderWidget: A horizontal or vertical scroll-bar, although with no scrolling properties. It can
//               be used to select any integer value.
// DialogWidget: A modal dialog box. It displays a message and waits for the user to press a
//               button. It may also allow the user to interact with a single other widget inside
//               (e.g. a StringPromptWidget).
//
// See also GlopFrameBase.h.

#ifndef GLOP_GLOP_FRAME_WIDGETS_H__
#define GLOP_GLOP_FRAME_WIDGETS_H__

// Includes
#include "BinaryFileManager.h"
#include "Color.h"
#include "Font.h"
#include "GlopFrameBase.h"
#include "GlopFrameStyle.h"
#include "Input.h"

// Class declarations
class Image;
class TextRenderer;

// HotKeyTracker
// =============
//
// A utility class for tracking one or more interchangeable "hot keys" for a frame. Interface is
// similar to Input::KeyTracker. kAnyKey can be used as a hot key, and it is interpreted as
// anything other than mouse motion or modifiers.
class HotKeyTracker {
 public:
  HotKeyTracker() {}
  LightSetId AddHotKey(const GlopKey &key) {return hot_keys_.InsertItem(key);}
  KeyEvent::Type RemoveHotKey(LightSetId id);

  bool OnKeyEvent(const KeyEvent &event, int dt, KeyEvent::Type *result);
  bool OnKeyEvent(const KeyEvent &event, int dt) {
    KeyEvent::Type x;
    return OnKeyEvent(event, dt, &x);
  }
  KeyEvent::Type Clear();
  bool IsFocusMagnet(const KeyEvent &event) const;

  void Think() {tracker_.Think();}
  bool IsDownNow() const {return tracker_.IsDownNow();}
  bool IsDownFrame() const {return tracker_.IsDownFrame();}
  bool WasPressed() const {return tracker_.WasPressed();}
  bool WasReleased() const {return tracker_.WasReleased();}
 
 private:
  bool IsMatchingKey(const GlopKey &hot_key, const GlopKey &key) const;

  Input::KeyTracker tracker_;
  LightSet<GlopKey> hot_keys_, down_hot_keys_;
  DISALLOW_EVIL_CONSTRUCTORS(HotKeyTracker);
};

// Basic decorative frames
// =======================
//
// See comment at the top of the file.
class EmptyFrame: public GlopFrame {
 public:
  EmptyFrame(): GlopFrame() {}
  string GetType() const {return "EmptyFrame";}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EmptyFrame);
};

class SolidBoxFrame: public SingleParentFrame {
 public:
  SolidBoxFrame(GlopFrame *frame, const Color &inner_color, const Color &outer_color)
  : SingleParentFrame(frame), has_outer_part_(true), inner_color_(inner_color),
    outer_color_(outer_color) {}
  SolidBoxFrame(const Color &inner_color, const Color &outer_color)
  : SingleParentFrame(0), has_outer_part_(true), inner_color_(inner_color),
    outer_color_(outer_color) {}
  SolidBoxFrame(GlopFrame *frame, const Color &inner_color)
  : SingleParentFrame(frame), has_outer_part_(false), inner_color_(inner_color) {}
  SolidBoxFrame(const Color &inner_color)
  : SingleParentFrame(0), has_outer_part_(false), inner_color_(inner_color) {}
  string GetType() const {return "SolidBoxFrame";}

  void Render() const;
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  bool has_outer_part_;
  Color inner_color_, outer_color_;
  DISALLOW_EVIL_CONSTRUCTORS(SolidBoxFrame);
};

class HollowBoxFrame: public SingleParentFrame {
 public:
  HollowBoxFrame(GlopFrame *frame, const Color &color): SingleParentFrame(frame), color_(color) {}
  HollowBoxFrame(const Color &color): SingleParentFrame(0), color_(color) {}

  void Render() const;
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(HollowBoxFrame);
};

class InputBoxFrame: public SingleParentFrame {
 public:
  InputBoxFrame(GlopFrame *inner_frame, const InputBoxViewFactory *factory = gInputBoxViewFactory)
  : SingleParentFrame(new PaddedFrame(inner_frame, 0)), view_(factory->Create()) {}
  ~InputBoxFrame() {delete view_;}
  string GetType() const {return "InputBoxFrame";}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height); 
 private:
  InputBoxView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(InputBoxFrame);
};

class ImageFrame: public GlopFrame {
 public:
  // Constructors - Either an image, a texture or a file can be specified. Any objects
  // loaded/created here are also deleted here.
  ImageFrame(BinaryFileReader reader, const Color &bg_color, int bg_tolerance,
             const Color &color = kWhite);
  ImageFrame(BinaryFileReader reader, const Color &color = kWhite);
  ImageFrame(const Image *image, const Color &color = kWhite);
  ImageFrame(const Texture *texture, const Color &color = kWhite);
  ~ImageFrame();
  string GetType() const {return "ImageFrame";}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  void Init(const Texture *texture, bool is_texture_owned, const Color &color);
  bool is_texture_owned_;
  const Texture *texture_;
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(ImageFrame);
};

class ArrowFrame: public GlopFrame {
 public:
  enum Direction {Up, Right, Down, Left};
  ArrowFrame(Direction d, const ArrowViewFactory *factory)
  : direction_(d), view_(factory->Create()) {}
  ~ArrowFrame() {delete view_;}
  string GetType() const {return "ArrowFrame";}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  Direction direction_;
  ArrowView *view_;  
  DISALLOW_EVIL_CONSTRUCTORS(ArrowFrame);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Text frames
// ===========
//
// See comment at the top of the file.
class TextFrame: public GlopFrame {
 public:
  TextFrame(const string &text, const GuiTextStyle &style = *gGuiTextStyle);
  virtual ~TextFrame();
  string GetType() const {return "TextFrame";}
  
  // Returns the pixel height we would choose for our text given the relative height as specified
  // to TextFrame. This is provided so that an external class can get font information about a
  // potential TextFrame before instantiating the TextFrame.
  static int GetFontPixelHeight(float height);

  // Returns the renderer that is currently being used for rendering this text. Can be used to
  // get font metrics if needed.
  const TextRenderer *GetRenderer() const {return renderer_;}

  // Accessors and mutators
  const string &GetText() const {return text_;}
  void SetText(const string &text) {
    if (text_ != text) {
      text_ = text;
      DirtySize();
    }
  }
  const GuiTextStyle &GetStyle() const {return text_style_;}
  void SetStyle(const GuiTextStyle &style) {
    text_style_ = style;
    DirtySize();
  }

  // Standard GlopFrame functionality
  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  string text_;
  GuiTextStyle text_style_;
  TextRenderer *renderer_;
  DISALLOW_EVIL_CONSTRUCTORS(TextFrame);
};

// A TextFrame with additional formatting options. In particular, it supports:
//  - New lines. These may either be added explicitly with "\n", or they can be added automatically
//    as soft returns so that the FancyTextFrame will not exceed its recommended width. The latter
//    feature can be disabled by setting add_soft_returns to false.
//  - Horizontal justification. This is used to align different lines of different sizes.
//  - Varying style via tags. The text for a FancyTextFrame can contain tags delimited by ASCII
//    value 1 (thus, to turn a section bold, add the text "\1b\1"). These change the style of all
//    future text.
//
//     Bold/Italics/Underline: (concatenation of "b", "/b", "i", "/i", "u", "/u") Turns bold,
//       italics, or underlining on or off.
//     Italics: ("i" or "nu") Turns italics on or off.
//     Underline: ("u" or "nu") Turns underlining on or off.
//     Horz Justify: ("j<num>", e.g. "j0.5") Sets horizontal justification for future text. Takes
//       effect on this line if this line is still empty. Otherwise, takes effect on the next line.
//     Font: ("f<ptr>", e.g. "fDEADBEEF") Sets the active font to the one pointed to by font.
//     Size: ("s<size>", e.g. "s2.5") Sets the active font to have size the given multiple of the
//       the original size. Note this is RELATIVE to the base size, unlike other tags.
//     Color: ("c<r><g><b>" or "c<r><g><b><a>", e.g. "cFF0000") Sets the active color.
//
//    Tags can also be created using the static methods, _Tag, below.
class FancyTextFrame: public MultiParentFrame {
 public:
  // Constructors. horz_justify is used to align different rows of text.
  FancyTextFrame(const string &text, const GuiTextStyle &style = *gGuiTextStyle);
  FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                 const GuiTextStyle &style = *gGuiTextStyle);
  string GetType() const {return "FancyTextFrame";}

  // Tags. These can be used as follows: string("Test: " + CTag(kRed) + " red");
  // Note that, they return strings, not char points, so if they are to be used with a call to
  // Format, .c_str() is needed.
  static const string BiuTag(int flags) {
    return string("\1") + ((flags & kFontBold) > 0? "b" : "/b") +
      ((flags & kFontItalics) > 0? "i" : "/i") + ((flags & kFontUnderline) > 0? "u" : "/u") + "\1";
  }
  static const string CTag(const Color &color) {
    return Format("\1c%02X%02X%02X%02X\1",
      min(max(int(color[0] * 255), 0), 255), min(max(int(color[1] * 255), 0), 255),
      min(max(int(color[2] * 255), 0), 255), min(max(int(color[3] * 255), 0), 255));
  }
  static const string FTag(const Font *font) {return Format("\1f%p\1", font);}
  static const string JTag(float horz_justify) {return Format("\1j%f\1", horz_justify);}
  static const string STag(float size_multiplier) {return Format("\1s%f\1", size_multiplier);}
    
  // Accessors and mutators
  const string &GetText() const {return text_;}
  void SetText(const string &text) {
    if (text_ != text) {
      text_ = text;
      DirtySize();
    }
  }
  const GuiTextStyle &GetStyle() const {return text_style_;}
  void SetStyle(const GuiTextStyle &style) {
    text_style_ = style;
    DirtySize();
  }

  // Standard GlopFrame functionality
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  // Parsing utilities
  struct ParseStatus {
    int pos;
    float horz_justify;
    GuiTextStyle style;
    TextRenderer *renderer;
  };
  ParseStatus CreateParseStatus();
  void StartParsing(ParseStatus *status, vector<ParseStatus> *active_parsers);
  void StopParsing(vector<ParseStatus> *active_parsers);
  enum ParseResult {Normal, NewRenderer, Error};
  ParseResult ParseNextCharacter(const string &s, ParseStatus *status,
                                 vector<ParseStatus> *active_parsers, char *ch);

  // Data
  string text_;
  float base_horz_justify_;
  GuiTextStyle text_style_;
  bool add_soft_returns_;
  struct TextBlock {
    LightSetId child_id;
    int x, y;
  };
  vector<vector<TextBlock> > text_blocks_;
  DISALLOW_EVIL_CONSTRUCTORS(FancyTextFrame);
};

class FpsFrame: public SingleParentFrame {
 public:
  FpsFrame(const GuiTextStyle &style = *gGuiTextStyle)
  : SingleParentFrame(new TextFrame("", style)) {}
  string GetType() const {return "FpsFrame";}
  const GuiTextStyle &GetStyle() const {return text()->GetStyle();}
  void SetStyle(const GuiTextStyle &style) {text()->SetStyle(style);}

  void Think(int dt);
 private:
  const TextFrame *text() const {return (TextFrame*)GetChild();}
  TextFrame *text() {return (TextFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(FpsFrame);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Text prompts
// ============
//
// StringPrompt, IntegerPrompt, etc.

class DummyTextPromptFrame: public SingleParentFrame {
 public:
  DummyTextPromptFrame(const string &text, const TextPromptViewFactory *view_factory);
  ~DummyTextPromptFrame() {delete view_;}
  string GetType() const {return "DummyTextPromptFrame";}

  // Basic accessors and mutators. SetText automatically moves the cursor to the end of the prompt
  // if the text changed.
  const string &GetText() const {return text()->GetText();}
  void SetText(const string &new_text);
  int GetCursorPos() const {return cursor_pos_;}
  void SetCursorPos(int pos);
  bool IsSelectionActive() const {return selection_start_ != selection_end_;}
  void GetSelection(int *start, int *end) {
    *start = selection_start_;
    *end = selection_end_;
  }
  void SetSelection(int start, int end);

  // Given a pixel in local coordinates, these returns the character position it is overlapping. The
  // first one returns a boundary in [0, len]. The second one returns an actual character in
  // [0, len-1].
  int PixelToBoundaryPosition(int x) const;
  int PixelToCharacterPosition(int x) const;
  void GetCursorExtents(int pos, int *x1, int *x2) const;
  void GetCharacterExtents(int pos, int *x1, int *x2) const;

  // Overloaded Glop functions
  void Render() const;
  void Think(int dt);
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
  void OnFocusChange();

 private:
  const TextFrame *text() const {return (const TextFrame*)GetChild();}
  TextFrame *text() {return (TextFrame*)GetChild();}
  int cursor_pos_, cursor_time_;
  int selection_start_, selection_end_;
  int left_padding_, top_padding_, right_padding_;
  TextPromptView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummyTextPromptFrame);
};

class BaseTextPromptFrame: public SingleParentFrame {
 public:
  string GetType() const {return "BaseTextPromptFrame";}
  bool OnKeyEvent(const KeyEvent &event, int dt);

 protected:
  BaseTextPromptFrame(const string &text, const TextPromptViewFactory *view_factory);
  void OnFocusChange();

  // Text accessors, mutators. SetText ensures the cursor information is valid but does not
  // reform the text automatically.
  void SetText(const string &text);
  const string &GetText() const {return prompt()->GetText();}
  int GetCursorPos() const {return prompt()->GetCursorPos();}

  // The two functions that any BasicTextPromptFrame must implement:
  // CanInsertCharacter: If in_theory is true, is it ever possible to add ASCII value ch to the
  //                     text? If false, is it possible to add the value at the current cursor
  //                     location? If so, the subclass can still control how it is added via
  //                     ReformText below.
  // ReformText: Given the prompt right after an edit, this changes the text to valid text. For
  //             example, this might convert lower-case letters to upper-case, or it might clip a
  //             number to certain upper or lower bounds.
  virtual bool CanInsertCharacter(char ch, bool in_theory) const = 0;
  virtual void ReformText() = 0;

 private:
  const DummyTextPromptFrame *prompt() const {return (const DummyTextPromptFrame*)GetChild();}
  DummyTextPromptFrame *prompt() {return (DummyTextPromptFrame*)GetChild();}

  int GetPrevWordBoundary(int pos) const;
  int GetNextWordBoundary(int pos) const;
  void DeleteSelection();
  void DeleteCharacter(bool is_next_character);
  void InsertCharacter(char ch);
  void SetCursorPos(int pos, bool also_set_anchor);
  class CharacterPing: public Ping {
   public:
    CharacterPing(GlopFrame *frame, int i): Ping(frame, false), i_(i) {}
    void GetCoords(int *x1, int *y1, int *x2, int *y2);
   private:
    int i_;
    DISALLOW_EVIL_CONSTRUCTORS(CharacterPing);
  };

  bool is_tracking_mouse_;
  int selection_anchor_;
  DISALLOW_EVIL_CONSTRUCTORS(BaseTextPromptFrame);
};

class StringPromptFrame: public BaseTextPromptFrame {
 public:
  StringPromptFrame(const string &start_text, int length_limit,
                    const TextPromptViewFactory *view_factory = gTextPromptViewFactory);
  string GetType() const {return "StringPromptFrame";}
  const string &Get() const {return GetText();}
  void Set(const string &value);

 protected:
  bool CanInsertCharacter(char ch, bool in_theory) const;
  void ReformText() {}
 private:
  int length_limit_;
  DISALLOW_EVIL_CONSTRUCTORS(StringPromptFrame);
};

class StringPromptWidget: public FocusFrame {
 public:
  StringPromptWidget(const string &start_text, int length_limit, float prompt_width = kSizeLimitRec,
                     const TextPromptViewFactory *prompt_view_factory = gTextPromptViewFactory,
                     const InputBoxViewFactory *input_box_view_factory = gInputBoxViewFactory);
  string GetType() const {return "StringPromptWidget";}
  const string &Get() const {return prompt_->Get();}
  void Set(const string &value) {prompt_->Set(value);}
 private:
  StringPromptFrame *prompt_;
  DISALLOW_EVIL_CONSTRUCTORS(StringPromptWidget);
};

class IntegerPromptFrame: public BaseTextPromptFrame {
 public:
  IntegerPromptFrame(int start_value, int min_value, int max_value,
                     const TextPromptViewFactory *view_factory = gTextPromptViewFactory);
  string GetType() const {return "IntegerPromptFrame";}
  int Get() const {return atoi(GetText().c_str());}
  void Set(int value);

 protected:
  bool CanInsertCharacter(char ch, bool in_theory) const;
  void ReformText();
 private:
  int min_value_, max_value_;
  DISALLOW_EVIL_CONSTRUCTORS(IntegerPromptFrame);
};

class IntegerPromptWidget: public FocusFrame {
 public:
  IntegerPromptWidget(int start_value, int min_value, int max_value,
                      float prompt_width = kSizeLimitRec,
                      const TextPromptViewFactory *prompt_view_factory = gTextPromptViewFactory,
                      const InputBoxViewFactory *input_box_view_factory = gInputBoxViewFactory);
  string GetType() const {return "IntegerPromptWidget";}
  int Get() const {return prompt_->Get();}
  void Set(int value) {prompt_->Set(value);}
 private:
  IntegerPromptFrame *prompt_;
  DISALLOW_EVIL_CONSTRUCTORS(IntegerPromptWidget);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Window
// ======

class WindowFrame: public SingleParentFrame {
 public:
  WindowFrame(GlopFrame *inner_frame, const string &title,
              const WindowViewFactory *view_factory = gWindowViewFactory);
  WindowFrame(GlopFrame *inner_frame, const WindowViewFactory *view_factory = gWindowViewFactory);
  ~WindowFrame() {delete view_;}
  string GetType() const {return "WindowFrame";}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  PaddedFrame *padded_title_frame_, *padded_inner_frame_;
  WindowView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(WindowFrame);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Button
// ======

class DummyButtonFrame: public SingleParentFrame {
 public:
  DummyButtonFrame(GlopFrame *inner_frame, bool is_down, const ButtonViewFactory *view_factory)
  : SingleParentFrame(new PaddedFrame(inner_frame)), is_down_(is_down),
    view_(view_factory->Create()) {}
  ~DummyButtonFrame() {delete view_;}
  string GetType() const {return "DummyButtonFrame";}
  bool IsDown() const {return is_down_;}
  void SetIsDown(bool is_down);

  // Glop overloads
  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  bool is_down_;
  ButtonView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummyButtonFrame);
};

class ButtonFrame: public SingleParentFrame {
 public:
  ButtonFrame(GlopFrame *inner_frame, const ButtonViewFactory *view_factory = gButtonViewFactory)
  : SingleParentFrame(new DummyButtonFrame(inner_frame, false, view_factory)),
    ping_on_press_(true), is_confirm_key_down_(false), was_pressed_fully_(false),
    is_mouse_locked_on_(false) {}
  string GetType() const {return "ButtonFrame";}

  // Hot keys
  LightSetId AddHotKey(const GlopKey &key) {return hot_key_tracker_.AddHotKey(key);}
  void RemoveHotKey(LightSetId id) {hot_key_tracker_.RemoveHotKey(id);}

  // Returns whether the button is currently in the down state.
  bool IsDown() const {return button()->IsDown();}

  // If the button generated events similar to a key on the keyboard, this returns whether a down
  // event would have been generated this frame. It will be true if a button is just pressed or
  // it will be true periodically while a button is held down.
  bool WasHeldDown() const {return button_tracker_.WasPressed();}

  // Returns whether a full press and release of the button has completed this frame.
  bool WasPressedFully() const {return was_pressed_fully_;}

  // Glop overloaded functions
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, int dt);
  bool IsFocusMagnet(const KeyEvent &event) const {return hot_key_tracker_.IsFocusMagnet(event);}
 protected:
  void OnFocusChange();

  // Sets whether we should generate a ping when the button is pressed. Normally this is true, but
  // overloaded classes are allowed to overwrite it if they desire.
  void SetPingOnPress(bool ping_on_press) {ping_on_press_ = ping_on_press;}
 private:
  const DummyButtonFrame *button() const {return (DummyButtonFrame*)GetChild();}
  DummyButtonFrame *button() {return (DummyButtonFrame*)GetChild();}

  enum DownType {Down, DownRepeatSoon, UpCancelPress, UpConfirmPress};
  void SetIsDown(DownType down_type);

  bool ping_on_press_;
  bool is_confirm_key_down_;
  HotKeyTracker hot_key_tracker_;
  Input::KeyTracker button_tracker_;
  bool was_pressed_fully_;
  bool is_mouse_locked_on_;
  DISALLOW_EVIL_CONSTRUCTORS(ButtonFrame);
};

class ButtonWidget: public FocusFrame {
 public:
  // Basic constructors
  ButtonWidget(GlopFrame *frame, const ButtonViewFactory *factory = gButtonViewFactory)
  : FocusFrame(new ButtonFrame(frame, factory)) {}
  ButtonWidget(GlopFrame *frame, const GlopKey &hot_key,
               const ButtonViewFactory *factory = gButtonViewFactory)
  : FocusFrame(new ButtonFrame(frame, factory)) {button()->AddHotKey(hot_key);}

  // Convenience constructors for text button frames
  ButtonWidget(const string &text, const GuiTextStyle &text_style = *gGuiTextStyle,
               const ButtonViewFactory *factory = gButtonViewFactory)
  : FocusFrame(new ButtonFrame(new TextFrame(text, text_style), factory)) {}
  ButtonWidget(const string &text, const GlopKey &hot_key,
               const GuiTextStyle &text_style = *gGuiTextStyle,
               const ButtonViewFactory *factory = gButtonViewFactory)
  : FocusFrame(new ButtonFrame(new TextFrame(text, text_style), factory)) {
    button()->AddHotKey(hot_key);
  }
  string GetType() const {return "ButtonWidget";}

  // Utilities
  LightSetId AddHotKey(const GlopKey &key) {return button()->AddHotKey(key);}
  void RemoveHotKey(LightSetId id) {button()->RemoveHotKey(id);}
  bool IsDown() const {return button()->IsDown();}
  bool WasHeldDown() const {return button()->WasHeldDown();}
  bool WasPressedFully() const {return button()->WasPressedFully();}
 private:
  const ButtonFrame *button() const {return (const ButtonFrame*)GetChild();}
  ButtonFrame *button() {return (ButtonFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(ButtonWidget);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Slider
// ======

class DummySliderFrame: public MultiParentFrame {
 public:
  enum Direction {Horizontal, Vertical};
  DummySliderFrame(Direction direction, int logical_tab_size, int logical_total_size,
                   int logical_tab_position,
                   GlopFrame*(*button_factory)(ArrowFrame::Direction,
                                               const ArrowViewFactory *,
                                               const ButtonViewFactory *),
                   const SliderViewFactory *factory);
  ~DummySliderFrame() {delete view_;}
  string GetType() const {return "DummySliderFrame";}
  const GlopFrame *GetDecButton() const {return dec_button_;}
  GlopFrame *GetDecButton() {return dec_button_;}
  const GlopFrame *GetIncButton() const {return inc_button_;}
  GlopFrame *GetIncButton() {return inc_button_;}

  // Logical state accessors/mutators
  int GetTabPosition() const {return logical_tab_position_;}
  int GetTabSize() const {return logical_tab_size_;}
  int GetTotalSize() const {return logical_total_size_;}
  void SetTabPosition(int position);
  void SetTabSize(int size);
  void SetTotalSize(int size);

  // Pixel accessors - Note that all pixel coordinates are relative to this frame.
  void GetTabCoordinates(int *x1, int *y1, int *x2, int *y2) const;
  int GetMaxPixelLocation() const {return bar_pixel_length_ - 1;}
  int PixelToPixelLocation(int x, int y) const;
  int LogicalPositionToFirstPixelLocation(int logical_position) const;
  int PixelLocationToLogicalPosition(int pixel_location) const;

  // Glop overloads
  void Render() const;
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  void RecomputeTabScreenPosition();

  Direction direction_;
  GlopFrame *dec_button_, *inc_button_;
  int logical_tab_size_, logical_total_size_, logical_tab_position_;
  int tab_x1_, tab_y1_, tab_x2_, tab_y2_;
  int tab_pixel_length_, bar_pixel_length_;
  SliderView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummySliderFrame);
};

class SliderFrame: public SingleParentFrame {
 public:
  enum Direction {Horizontal, Vertical};  // Must match DummySliderFrame::Direction
  SliderFrame(Direction direction, int logical_tab_size, int logical_total_size,
              int logical_tab_position, const SliderViewFactory *factory = gSliderViewFactory);
  string GetType() const {return "SliderFrame";}
  LightSetId AddDecHotKey(const GlopKey &key) {return GetDecButton()->AddHotKey(key);}
  void RemoveDecHotKey(LightSetId id) {GetDecButton()->RemoveHotKey(id);}
  LightSetId AddBigDecHotKey(const GlopKey &key) {return big_dec_tracker_.AddHotKey(key);}
  void RemoveBigDecHotKey(LightSetId id) {big_dec_tracker_.RemoveHotKey(id);}
  LightSetId AddIncHotKey(const GlopKey &key) {return GetIncButton()->AddHotKey(key);}
  void RemoveIncHotKey(LightSetId id) {GetIncButton()->RemoveHotKey(id);}
  LightSetId AddBigIncHotKey(const GlopKey &key) {return big_inc_tracker_.AddHotKey(key);}
  void RemoveBigIncHotKey(LightSetId id) {big_inc_tracker_.RemoveHotKey(id);}

  // State accessors/mutators
  int GetTabPosition() const {return slider()->GetTabPosition();}
  void SetTabPosition(int position) {slider()->SetTabPosition(position);}
  void SmallDec() {SetTabPosition(GetTabPosition() - GetStepSize());}
  void SmallInc() {SetTabPosition(GetTabPosition() + GetStepSize());}
  void BigDec() {SetTabPosition(GetTabPosition() - (GetTabSize()*9+9)/10);}
  void BigInc() {SetTabPosition(GetTabPosition() + (GetTabSize()*9+9)/10);}
  int GetTabSize() const {return slider()->GetTabSize();}
  void SetTabSize(int size) {slider()->SetTabSize(size);}
  int GetTotalSize() const {return slider()->GetTotalSize();}
  void SetTotalSize(int size) {slider()->SetTotalSize(size);}

  // Overloaded functions
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, int dt);
 protected:
  void OnFocusChange();

 private:
  int GetStepSize() const {return (GetTabSize() + 9) / 10;}
  const DummySliderFrame *slider() const {return (const DummySliderFrame*)GetChild();}
  DummySliderFrame *slider() {return (DummySliderFrame*)GetChild();}
  const ButtonFrame *GetDecButton() const {return (const ButtonFrame*)slider()->GetDecButton();}
  ButtonFrame *GetDecButton() {return (ButtonFrame*)slider()->GetDecButton();}
  const ButtonFrame *GetIncButton() const {return (const ButtonFrame*)slider()->GetIncButton();}
  ButtonFrame *GetIncButton() {return (ButtonFrame*)slider()->GetIncButton();}

  enum MouseLockMode {None, Bar, Tab};
  int step_size_;
  MouseLockMode mouse_lock_mode_;
  int tab_grab_position_;
  HotKeyTracker big_dec_tracker_, big_inc_tracker_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderFrame);
};

class SliderWidget: public FocusFrame {
 public:
  enum Direction {Horizontal, Vertical};  // Must match DummySliderFrame::Direction
  SliderWidget(Direction direction, int logical_tab_size, int logical_total_size,
               int logical_tab_position,
               const SliderViewFactory *factory = gSliderViewFactory)
  : FocusFrame(new SliderFrame((SliderFrame::Direction)direction, logical_tab_size,
                               logical_total_size, logical_tab_position, factory)) {}
  string GetType() const {return "SliderWidget";}

  // Utilities
  LightSetId AddDecHotKey(const GlopKey &key) {return slider()->AddDecHotKey(key);}
  void RemoveDecHotKey(LightSetId id) {slider()->RemoveDecHotKey(id);}
  LightSetId AddBigDecHotKey(const GlopKey &key) {return slider()->AddBigDecHotKey(key);}
  void RemoveBigDecHotKey(LightSetId id) {slider()->RemoveBigDecHotKey(id);}
  LightSetId AddIncHotKey(const GlopKey &key) {return slider()->AddIncHotKey(key);}
  void RemoveIncHotKey(LightSetId id) {slider()->RemoveIncHotKey(id);}
  LightSetId AddBigIncHotKey(const GlopKey &key) {return slider()->AddBigIncHotKey(key);}
  void RemoveBigIncHotKey(LightSetId id) {slider()->RemoveBigIncHotKey(id);}
  int GetTabPosition() const {return slider()->GetTabPosition();}
  void SetTabPosition(int position) {slider()->SetTabPosition(position);}
  void SmallDec() {slider()->SmallDec();}
  void SmallInc() {slider()->SmallInc();}
  void BigDec() {slider()->BigDec();}
  void BigInc() {slider()->BigInc();}
  int GetTabSize() const {return slider()->GetTabSize();}
  int GetTotalSize() const {return slider()->GetTotalSize();}

 private:
  const SliderFrame *slider() const {return (const SliderFrame*)GetChild();}
  SliderFrame *slider() {return (SliderFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(SliderWidget);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Menu
// ====

class DummyMenuFrame: public MultiParentFrame {
 public:
  DummyMenuFrame(int num_cols, bool is_vertical, float horz_justify, float vert_justify,
                 const MenuViewFactory *factory = gMenuViewFactory);
  ~DummyMenuFrame() {delete view_;}

  // Item layout accessors.
  int GetNumItems() const {return (int)item_ids_.size();}
  int GetNumRows() const {return is_vertical_? GetNumItems() / num_cols_ : num_cols_;}
  int GetNumCols() const {return is_vertical_? num_cols_ : GetNumItems() / num_cols_;}
  int GetRow(int item) const {return is_vertical_? item / num_cols_ : item % num_cols_;}
  int GetCol(int item) const {return is_vertical_? item % num_cols_ : item / num_cols_;}
  int GetItemIndex(int row, int col) const {
    return is_vertical_? row * num_cols_ + col : col * num_cols_ + row;
  }

  // Item accessors. Note that all coordinates are relative to this frame.
  int GetSelection() const {return selection_;}
  void SetSelection(int selection); 
  const GlopFrame *GetItem(int item) const {return GetChild(item_ids_[item]);}
  GlopFrame *GetItem(int item) {return GetChild(item_ids_[item]);}
  void GetItemCoords(int item, int *x1, int *y1, int *x2, int *y2) const;
  int GetItemByCoords(int x, int y) const {return GetItemIndex(y / row_height_, x / col_width_);}
  void NewItemPing(int item, bool center = false) {AddPing(new ItemPing(this, item, center));}

  // Item mutators
  int AddItem(GlopFrame *frame);
  void DeleteItem();
  void SetItem(int item, GlopFrame *frame);
  GlopFrame *SetItemNoDelete(int item, GlopFrame *frame);

  // Overloaded functions
  void Render() const;
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  class ItemPing: public Ping {
   public:
    ItemPing(DummyMenuFrame *frame, int item, bool center)
    : Ping(frame, center), item_(item) {}
    void GetCoords(int *x1, int *y1, int *x2, int *y2) {
      ((DummyMenuFrame*)GetFrame())->GetItemCoords(item_, x1, y1, x2, y2);
    }
   private:
    int item_;
    DISALLOW_EVIL_CONSTRUCTORS(ItemPing);
  };
  vector<LightSetId> item_ids_;
  int num_cols_, selection_;
  bool is_vertical_;
  float horz_justify_, vert_justify_;
  int item_lpadding_, item_tpadding_, item_rpadding_, item_bpadding_;
  int col_width_, row_height_;
  MenuView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummyMenuFrame);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Dialog
// ======

class DialogWidget {
 public:
  // Hot key controls
  static LightSetId AddYesHotKey(const GlopKey &key) {Init(); return yes_keys_.GetFirstId();}
  static void RemoveYesHotKey(LightSetId id) {Init(); yes_keys_.RemoveItem(id);}
  static void ClearYesHotKeys() {Init(); yes_keys_.Clear();}
  static LightSetId AddNoHotKey(const GlopKey &key) {Init(); return no_keys_.GetFirstId();}
  static void RemoveNoHotKey(LightSetId id) {Init(); no_keys_.RemoveItem(id);}
  static void ClearNoHotKeys() {Init(); no_keys_.Clear();}
  static LightSetId AddOkayHotKey(const GlopKey &key) {Init(); return okay_keys_.GetFirstId();}
  static void RemoveOkayHotKey(LightSetId id) {Init(); okay_keys_.RemoveItem(id);}
  static void ClearOkayHotKeys() {Init(); okay_keys_.Clear();}
  static LightSetId AddCancelHotKey(const GlopKey &key) {Init(); return cancel_keys_.GetFirstId();}
  static void RemoveCancelHotKey(LightSetId id) {Init(); cancel_keys_.RemoveItem(id);}
  static void ClearCancelHotKeys() {Init(); cancel_keys_.Clear();}

  // Text-only dialog boxes.
  enum Result {Yes, No, Okay, Cancel};
  static void TextOkay(const string &title, const string &message,
                       const DialogViewFactory *factory = gDialogViewFactory);
  static Result TextOkayCancel(const string &title, const string &message,
                               const DialogViewFactory *factory = gDialogViewFactory);
  static Result TextYesNo(const string &title, const string &message,
                          const DialogViewFactory *factory = gDialogViewFactory);
  static Result TextYesNoCancel(const string &title, const string &message,
                                const DialogViewFactory *factory = gDialogViewFactory);

  // Dialog boxes with a string prompt.
  static string StringPromptOkay(const string &title, const string &message, const string &prompt,
                                 const string &start_value, int value_length_limit,
                                 const DialogViewFactory *factory = gDialogViewFactory);
  static Result StringPromptOkayCancel(const string &title, const string &message,
                                       const string &prompt, const string &start_value,
                                       int value_length_limit, string *prompt_value,
                                       const DialogViewFactory *factory = gDialogViewFactory);

  // Dialog boxes with an integer prompt.
  static int IntegerPromptOkay(const string &title, const string &message, const string &prompt,
                               int start_value, int min_value, int max_value,
                               const DialogViewFactory *factory = gDialogViewFactory);
  static Result IntegerPromptOkayCancel(const string &title, const string &message,
                                        const string &prompt, int start_value, int min_value,
                                        int max_value, int *prompt_value,
                                        const DialogViewFactory *factory = gDialogViewFactory);
 private:
  static void Init();
  static GlopFrame *Create(const string &title, const string &message, const string &prompt,
                           GlopFrame *extra_frame, bool has_yes_button, bool has_no_button,
                           bool has_okay_button, bool has_cancel_button,
                           const DialogViewFactory *factory, vector<ButtonWidget*> *buttons,
                           vector<Result> *button_meanings);
  static Result Execute(const vector<ButtonWidget*> &buttons,
                        const vector<Result> &button_meanings);
  static Result DoText(const string &title, const string &message, bool has_yes_button,
                       bool has_no_button, bool has_okay_button, bool has_cancel_button,
                       const DialogViewFactory *factory);
  static Result DoStringPrompt(const string &title, const string &message, const string &prompt,
                               const string &start_value, int value_length_limit,
                               string *prompt_value, bool has_okay_button, bool has_cancel_button,
                               const DialogViewFactory *factory);
  static Result DoIntegerPrompt(const string &title, const string &message, const string &prompt,
                                int start_value, int min_value, int max_value, int *prompt_value,
                                bool has_okay_button, bool has_cancel_button,
                                const DialogViewFactory *factory);
 
  static bool is_initialized_;
  static LightSet<GlopKey> yes_keys_, no_keys_, okay_keys_, cancel_keys_;
  DISALLOW_EVIL_CONSTRUCTORS(DialogWidget);
};

#endif // GLOP_GLOP_FRAME_WIDGETS_H__
