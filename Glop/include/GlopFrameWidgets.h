// This file contains a set of useful stand-alone frames. See GlopFrame.h for a high-level overview
// on using GlopFrames.
//
// WARNING: Many of these frames require a valid font, which is not loaded by default. To avoid
//          errors, load a font (see Font.h) and call InitDefaultFrameStyle (see FrameStyle.h).
//
// Conventions: Many of these frames are designed to be easily customizable by user programs. To
//              facilitate changing how frames look, they delegate to View objects for all
//              rendering. These View objects are defined in GlopFrameStyle.h.
//
//              To faciliate changing how frames act, many frames have a dummy version. These have
//              all the same essential features, but their state can only be changed
//              programmatically. Separate classes then provide the desired user interface,
//              delegating to the dummy version as appropriate. The default implementations here
//              also support some limited customization. Where possible, all key detection is
//              restricted to the GUI derived keys in Input. Thus, basic GUI behavior can be
//              changed by remapping those keys.
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
// MenuWidget: A game-style menu (i.e. a menu that is always visible, not a pull-down menu).
//             Supports logic on the menu items themselves so that, for example, an editable
//             (text, value) pair can be easily added to the menu.
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
// similar to Input::KeyState. kAnyKey can be used as a hot key, and it is interpreted as
// anything other than mouse motion or modifiers.
class HotKeyTracker {
 public:
  HotKeyTracker() {}
  ListId AddHotKey(const GlopKey &key) {return hot_keys_.push_back(key);}
  KeyEvent::Type RemoveHotKey(ListId id);

  bool OnKeyEvent(const KeyEvent &event, KeyEvent::Type *result);
  bool OnKeyEvent(const KeyEvent &event) {
    KeyEvent::Type x;
    return OnKeyEvent(event, &x);
  }
  KeyEvent::Type Clear();
  bool IsFocusMagnet(const KeyEvent &event) const;

  void Think() {key_state_.Think();}
  bool IsDownNow() const {return key_state_.IsDownNow();}
  bool IsDownFrame() const {return key_state_.IsDownFrame();}
  bool WasPressed() const {return key_state_.WasPressed();}
  bool WasReleased() const {return key_state_.WasReleased();}
 
 private:
  bool IsMatchingKey(const GlopKey &hot_key, const GlopKey &key) const;

  Input::KeyState key_state_;
  List<GlopKey> hot_keys_, down_hot_keys_;
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
  InputBoxFrame(GlopFrame *inner_frame, const InputBoxView *view = gInputBoxView)
  : SingleParentFrame(new PaddedFrame(inner_frame, 0)), view_(view) {}
  string GetType() const {return "InputBoxFrame";}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height); 
 private:
  const InputBoxView *view_;
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
  ArrowFrame(Direction d, const ArrowView *view)
  : direction_(d), view_(view) {}
  string GetType() const {return "ArrowFrame";}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  Direction direction_;
  const ArrowView *view_;  
  DISALLOW_EVIL_CONSTRUCTORS(ArrowFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Text frames
// ===========
//
// See comment at the top of the file.
class TextFrame: public GlopFrame {
 public:
  TextFrame(const string &text, const GuiTextStyle &style = gGuiTextStyle);
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
//  - Varying style via tags. A character with ASCII value 1 enters and exits tag mode. During tag
//    mode, characters specify formatting options, instead of things to print. For example:
//    "\1bs2\1thunder" would print thunder in bold at twice the normal size. Formatting options:
//
//     Bold: ("B" or "/B") Turns bold on or off.
//     Italics: ("I" or "/I") Turns italics on or off.
//     Underline: ("U" or "/U") Turns underlining on or off.
//     Horz Justify: ("J<num>", e.g. "J0.5") Sets horizontal justification for future text. Takes
//       effect on this line if this line is still empty. Otherwise, takes effect on the next line.
//     Font: ("F<ptr>", e.g. "Fdeadbeef") Sets the active font to the one pointed to by font. The
//       pointer m
//     Size: ("S<size>", e.g. "s2.5") Sets the active font to have size the given multiple of the
//       the original size. Note this is RELATIVE to the base size, unlike other tags.
//     Color: ("C<r><g><b>" or "C<r><g><b><a>", e.g. "Cff0000") Sets the active color.
//
//    Note that capital letters are always reserved for tag names, and lower case 'a'-'f' are
//    reserved for hexadecimal. Tags can also be created using the static Tag methods below.
class FancyTextFrame: public MultiParentFrame {
 public:
  // Constructors. horz_justify is used to align different rows of text.
  FancyTextFrame(const string &text, const GuiTextStyle &style = gGuiTextStyle);
  FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                 const GuiTextStyle &style = gGuiTextStyle);
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
  static const string FTag(const Font *font) {return Format("F%p", font);}
  static const string JTag(float horz_justify) {return Format("J%f", horz_justify);}
  static const string STag(float size_multiplier) {return Format("S%f", size_multiplier);}
    
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
    ListId child_id;
    int x, y;
  };
  vector<vector<TextBlock> > text_blocks_;
  DISALLOW_EVIL_CONSTRUCTORS(FancyTextFrame);
};

class FpsFrame: public SingleParentFrame {
 public:
  FpsFrame(const GuiTextStyle &style = gGuiTextStyle)
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Text prompts
// ============
//
// StringPrompt, IntegerPrompt, etc.

class DummyTextPromptFrame: public SingleParentFrame {
 public:
  DummyTextPromptFrame(const string &text, const TextPromptView *view);
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

  // Given a pixel in local coordinates, these returns the character position it is overlapping.
  // The first one returns a boundary in [0, len]. The second one returns an actual character in
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
  const TextPromptView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummyTextPromptFrame);
};

class BaseTextPromptFrame: public SingleParentFrame {
 public:
  enum FocusGainBehavior {NoChange, CursorToStart, CursorToEnd, SelectAll};
  string GetType() const {return "BaseTextPromptFrame";}
  void SetFocusGainBehavior(FocusGainBehavior behavior) {focus_gain_behavior_ = behavior;}
  bool OnKeyEvent(const KeyEvent &event, bool gained_focus);
  
 protected:
  BaseTextPromptFrame(const string &text, const TextPromptView *view);
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

  void PingSelection();
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

  FocusGainBehavior focus_gain_behavior_;
  bool is_tracking_mouse_;
  int selection_anchor_;
  DISALLOW_EVIL_CONSTRUCTORS(BaseTextPromptFrame);
};

class StringPromptFrame: public BaseTextPromptFrame {
 public:
  StringPromptFrame(const string &start_text, int length_limit,
                    const TextPromptView *view = gTextPromptView);
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
  StringPromptWidget(const string &start_text, int length_limit,
                     float prompt_width = kSizeLimitRec,
                     const TextPromptView *prompt_view = gTextPromptView,
                     const InputBoxView *input_box_view = gInputBoxView);
  string GetType() const {return "StringPromptWidget";}
  const string &Get() const {return prompt_->Get();}
  void Set(const string &value) {prompt_->Set(value);}
 private:
  ExactWidthFrame *sizer_;
  StringPromptFrame *prompt_;
  DISALLOW_EVIL_CONSTRUCTORS(StringPromptWidget);
};

class IntegerPromptFrame: public BaseTextPromptFrame {
 public:
  IntegerPromptFrame(int start_value, int min_value, int max_value,
                     const TextPromptView *view = gTextPromptView);
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
                      const TextPromptView *prompt_view = gTextPromptView,
                      const InputBoxView *input_box_view = gInputBoxView);
  string GetType() const {return "IntegerPromptWidget";}
  int Get() const {return prompt_->Get();}
  void Set(int value) {prompt_->Set(value);}
 private:
  ExactWidthFrame *sizer_;
  IntegerPromptFrame *prompt_;
  DISALLOW_EVIL_CONSTRUCTORS(IntegerPromptWidget);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Window
// ======

class WindowFrame: public SingleParentFrame {
 public:
  WindowFrame(GlopFrame *inner_frame, const string &title,
              const WindowView *view = gWindowView);
  WindowFrame(GlopFrame *inner_frame, const WindowView *view = gWindowView);
  string GetType() const {return "WindowFrame";}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  PaddedFrame *padded_title_frame_, *padded_inner_frame_;
  const WindowView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(WindowFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Button
// ======

class DummyButtonFrame: public SingleParentFrame {
 public:
  DummyButtonFrame(GlopFrame *inner_frame, bool is_down, const ButtonView *view)
  : SingleParentFrame(new PaddedFrame(inner_frame)), is_down_(is_down), view_(view) {}
  string GetType() const {return "DummyButtonFrame";}
  bool IsDown() const {return is_down_;}
  void SetIsDown(bool is_down);

  // Glop overloads
  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  bool is_down_;
  const ButtonView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummyButtonFrame);
};

class ButtonFrame: public SingleParentFrame {
 public:
  ButtonFrame(GlopFrame *inner_frame, const ButtonView *view = gButtonView)
  : SingleParentFrame(new DummyButtonFrame(inner_frame, false, view)),
    ping_on_press_(true), is_confirm_key_down_(false), was_pressed_fully_(false),
    is_mouse_locked_on_(false) {}
  string GetType() const {return "ButtonFrame";}

  // Hot keys
  ListId AddHotKey(const GlopKey &key) {return hot_key_tracker_.AddHotKey(key);}
  void RemoveHotKey(ListId id) {hot_key_tracker_.RemoveHotKey(id);}

  // Returns whether the button is currently in the down state.
  bool IsDown() const {return button()->IsDown();}

  // If the button generated events similar to a key on the keyboard, this returns whether a down
  // event would have been generated this frame. It will be true if a button is just pressed or
  // it will be true periodically while a button is held down.
  bool WasHeldDown() const {return button_state_.WasPressed();}

  // Returns whether a full press and release of the button has completed this frame.
  bool WasPressedFully() const {return was_pressed_fully_;}

  // Glop overloaded functions
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, bool gained_focus);
  bool IsFocusMagnet(const KeyEvent &event) const {return hot_key_tracker_.IsFocusMagnet(event);}
 protected:
  void OnFocusChange();

  // Sets whether we should generate a ping when the button is pressed. Normally this is true, but
  // overloaded classes are allowed to overwrite it if they desire.
  void SetPingOnPress(bool ping_on_press) {ping_on_press_ = ping_on_press;}
 private:
  const DummyButtonFrame *button() const {return (DummyButtonFrame*)GetChild();}
  DummyButtonFrame *button() {return (DummyButtonFrame*)GetChild();}

  enum DownType {Down, UpCancelPress, UpConfirmPress};
  void SetIsDown(DownType down_type);

  bool ping_on_press_;
  bool is_confirm_key_down_;
  HotKeyTracker hot_key_tracker_;
  Input::KeyState button_state_;
  bool was_pressed_fully_;
  bool is_mouse_locked_on_;
  DISALLOW_EVIL_CONSTRUCTORS(ButtonFrame);
};

class ButtonWidget: public FocusFrame {
 public:
  // Basic constructors
  ButtonWidget(GlopFrame *frame, const ButtonView *view = gButtonView)
  : FocusFrame(new ButtonFrame(frame, view)) {}
  ButtonWidget(GlopFrame *frame, const GlopKey &hot_key,
               const ButtonView *view = gButtonView)
  : FocusFrame(new ButtonFrame(frame, view)) {button()->AddHotKey(hot_key);}

  // Convenience constructors for text button frames
  ButtonWidget(const string &text, const GuiTextStyle &text_style = gGuiTextStyle,
               const ButtonView *view = gButtonView)
  : FocusFrame(new ButtonFrame(new TextFrame(text, text_style), view)) {}
  ButtonWidget(const string &text, const GlopKey &hot_key,
               const GuiTextStyle &text_style = gGuiTextStyle,
               const ButtonView *view = gButtonView)
  : FocusFrame(new ButtonFrame(new TextFrame(text, text_style), view)) {
    button()->AddHotKey(hot_key);
  }
  string GetType() const {return "ButtonWidget";}

  // Utilities
  ListId AddHotKey(const GlopKey &key) {return button()->AddHotKey(key);}
  void RemoveHotKey(ListId id) {button()->RemoveHotKey(id);}
  bool IsDown() const {return button()->IsDown();}
  bool WasHeldDown() const {return button()->WasHeldDown();}
  bool WasPressedFully() const {return button()->WasPressedFully();}
 private:
  const ButtonFrame *button() const {return (const ButtonFrame*)GetChild();}
  ButtonFrame *button() {return (ButtonFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(ButtonWidget);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Slider
// ======

class DummySliderFrame: public MultiParentFrame {
 public:
  enum Direction {Horizontal, Vertical};
  DummySliderFrame(Direction direction, int logical_tab_size, int logical_total_size,
                   int logical_tab_position,
                   GlopFrame*(*button_factory)(ArrowFrame::Direction,
                                               const ArrowView *,
                                               const ButtonView *),
                   const SliderView *view);
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
  const SliderView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummySliderFrame);
};

class SliderFrame: public SingleParentFrame {
 public:
  enum Direction {Horizontal, Vertical};
  SliderFrame(Direction direction, int logical_tab_size, int logical_total_size,
              int logical_tab_position, const SliderView *view = gSliderView);
  string GetType() const {return "SliderFrame";}
  ListId AddDecHotKey(const GlopKey &key) {return GetDecButton()->AddHotKey(key);}
  void RemoveDecHotKey(ListId id) {GetDecButton()->RemoveHotKey(id);}
  ListId AddBigDecHotKey(const GlopKey &key) {return big_dec_tracker_.AddHotKey(key);}
  void RemoveBigDecHotKey(ListId id) {big_dec_tracker_.RemoveHotKey(id);}
  ListId AddIncHotKey(const GlopKey &key) {return GetIncButton()->AddHotKey(key);}
  void RemoveIncHotKey(ListId id) {GetIncButton()->RemoveHotKey(id);}
  ListId AddBigIncHotKey(const GlopKey &key) {return big_inc_tracker_.AddHotKey(key);}
  void RemoveBigIncHotKey(ListId id) {big_inc_tracker_.RemoveHotKey(id);}

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
  bool OnKeyEvent(const KeyEvent &event, bool gained_focus);
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
  int tab_grab_position_, last_grabbed_mouse_pos_;
  HotKeyTracker big_dec_tracker_, big_inc_tracker_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderFrame);
};

class SliderWidget: public FocusFrame {
 public:
  enum Direction {Horizontal, Vertical};
  SliderWidget(Direction direction, int logical_tab_size, int logical_total_size,
               int logical_tab_position,
               const SliderView *view = gSliderView)
  : FocusFrame(new SliderFrame((SliderFrame::Direction)direction, logical_tab_size,
                               logical_total_size, logical_tab_position, view)) {}
  string GetType() const {return "SliderWidget";}

  // Utilities
  ListId AddDecHotKey(const GlopKey &key) {return slider()->AddDecHotKey(key);}
  void RemoveDecHotKey(ListId id) {slider()->RemoveDecHotKey(id);}
  ListId AddBigDecHotKey(const GlopKey &key) {return slider()->AddBigDecHotKey(key);}
  void RemoveBigDecHotKey(ListId id) {slider()->RemoveBigDecHotKey(id);}
  ListId AddIncHotKey(const GlopKey &key) {return slider()->AddIncHotKey(key);}
  void RemoveIncHotKey(ListId id) {slider()->RemoveIncHotKey(id);}
  ListId AddBigIncHotKey(const GlopKey &key) {return slider()->AddBigIncHotKey(key);}
  void RemoveBigIncHotKey(ListId id) {slider()->RemoveBigIncHotKey(id);}
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Menu
// ====

class DummyMenuFrame: public MultiParentFrame {
 public:
  DummyMenuFrame(int num_cols, bool is_vertical, float horz_justify, float vert_justify,
                 const MenuView *view = gMenuView);
  string GetType() const {return "DummyMenuFrame";}
  bool IsVertical() const {return is_vertical_;}

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
  void NewItemPing(bool center = false) {AddPing(new ItemPing(this, selection_, center));}
  void NewItemPing(int item, bool center = false) {AddPing(new ItemPing(this, item, center));}

  // Item mutators
  int AddItem(GlopFrame *frame) {return AddItem(frame, GetNumItems());}
  int AddItem(GlopFrame *frame, int index);
  void RemoveItem() {RemoveItem(GetNumItems()-1);}
  void RemoveItem(int index) {delete RemoveItemNoDelete(index);}
  GlopFrame *RemoveItemNoDelete() {return RemoveItemNoDelete(GetNumItems()-1);}
  GlopFrame *RemoveItemNoDelete(int index);
  void SetItem(int item, GlopFrame *frame);
  GlopFrame *SetItemNoDelete(int item, GlopFrame *frame);
  void Clear();

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
  vector<ListId> item_ids_;
  int num_cols_, selection_;
  bool is_vertical_;
  float horz_justify_, vert_justify_;
  int item_lpadding_, item_tpadding_, item_rpadding_, item_bpadding_;
  int col_width_, row_height_;
  const MenuView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(DummyMenuFrame);
};

// Customizable menu items for MenuWidget
// ======================================

class GuiMenuItem {
 public:
  enum Action {Nothing, SelectNoPing, SelectAndPing, SelectAndConfirm, Unconfirm};
  virtual ~GuiMenuItem() {}

  // Accessors. The search key is a string that allows the user to quickly select this item by
  // typing it in. The frame is what is actually added to the menu and displayed. These may be
  // changed at any time by calls to SetFrame and SetSearchKey. Note that once control leaves
  // the GuiMenuItem, the frame returned by GetFrame is owned by the menu and will be deleted
  // automatically.
  const string &GetSearchKey() const {return search_key_;}
  GlopFrame *GetFrame() {return frame_;}

  // Logic functions. These are called whenever the corresponding function on the menu is called.
  // is_selected and is_confirmed specify whether this item is selected, and whether the menu is in
  // the confirmed state. These functions may optionally make the menu take an action:
  //  - Nothing. This may always be done. It does nothing.
  //  - SelectNoPing. If the menu is not confirmed, this switches the selection to this item.
  //  - SelectAndPing. Same as SelectNoPing except it also pings this item.
  //  - SelectAndConfirm. Same as SelectNoPing except it also confirms this item. (This will
  //                      generate a ping automatically.)
  //  - Unconfirm. If the menu is confirmed on THIS item, this Unconfirms.
  // By default *action will always be Nothing.
  virtual void Think(bool is_selected, bool is_confirmed, int dt, Action *action) {}
  virtual bool OnKeyEvent(bool is_selected, bool is_confirmed, const KeyEvent &event,
                          Action *action) {return false;}
  virtual void OnConfirmationChange(bool is_selected, bool is_confirmed, Action *action) {}
  virtual void OnSelectionChange(bool is_selected, Action *action) {}

 protected:
  // Utilities for extensions
  GuiMenuItem(GlopFrame *frame, const string &search_key)
  : frame_(frame), search_key_(search_key) {}
  void SetFrame(GlopFrame *frame) {frame_ = frame;}
  void SetSearchKey(const string &search_key) {search_key_ = search_key;}

 private:
  GlopFrame *frame_;
  string search_key_;
  DISALLOW_EVIL_CONSTRUCTORS(GuiMenuItem);
};

// A basic line of text
class TextMenuItem: public GuiMenuItem {
 public:
  TextMenuItem(const string &text, const GuiTextStyle &style = gGuiTextStyle)
  : GuiMenuItem(new TextFrame(text, style), text) {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(TextMenuItem);
};

// A user-editable key selector
class KeyPromptMenuItem: public GuiMenuItem {
 public:
  // prompt - The text to appear before the chosen key
  // start_value - The initial key selected
  // cancel_key - If this key is pressed, the key selection is reverted to the last choice
  // no_key - If this key is pressed, the key selection is set to kNoKey
  // result_address - Whenever the selected key is changed, the new value is written here
  KeyPromptMenuItem(const string &prompt, const GlopKey &start_value, const GlopKey &cancel_key,
                    const GlopKey &no_key, GlopKey *result_address,
                    const MenuView *view = gMenuView);
  bool OnKeyEvent(bool is_selected, bool is_confirmed, const KeyEvent &event, Action *action);
  void OnConfirmationChange(bool is_selected, bool is_confirmed, Action *action);
 
 protected:
  // Overwrite this function to change which keys are accepted as valid keys. Derived keys are
  // never accepted.
  virtual bool IsValidKey(const GlopKey &key) {return true;}

 private:
  void ResetFrame(bool is_confirmed);
  string prompt_;
  GlopKey cancel_key_, no_key_, value_, *result_address_;
  const MenuView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(KeyPromptMenuItem);
};

// A user-selectable string
class StringSelectMenuItem: public GuiMenuItem {
 public:
  // prompt - The text to appear before the chosen string
  // options - The list of all options
  // start_value - The initial selected option index
  // result_address - Whenever the selected option is changed, the new index is written here
  StringSelectMenuItem(const string &prompt, const vector<string> &options, int start_value,
                       int *result_address, const MenuView *view = gMenuView);
  void OnConfirmationChange(bool is_selected, bool is_confirmed, Action *action);

 private:
  vector<string> options_;
  TextFrame *value_frame_;
  int value_, *result_address_;
  DISALLOW_EVIL_CONSTRUCTORS(StringSelectMenuItem);
};

// A user-editable string
class StringPromptMenuItem: public GuiMenuItem {
 public:
  // prompt - The text to appear before the chosen string
  // start_value - The initial string
  // length_limit - The maximum length of a string the user can enter
  // result_address - Whenever the selected string is changed, the new index is written here
  StringPromptMenuItem(const string &prompt, const string &start_value, int length_limit,
                       string *result_address, const MenuView *view = gMenuView);
  bool OnKeyEvent(bool is_selected, bool is_confirmed, const KeyEvent &event, Action *action);
  void OnConfirmationChange(bool is_selected, bool is_confirmed, Action *action);
 
 private:
  void ResetFrame(bool is_confirmed);
  string prompt_, value_, *result_address_;
  int length_limit_;
  StringPromptFrame *prompt_frame_;
  const MenuView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(StringPromptMenuItem);
};

class IntegerPromptMenuItem: public GuiMenuItem {
 public:
  // prompt - The text to appear before the chosen integer
  // start_value - The initial integer
  // min_value - The minimum integer that can be entered
  // max_value - The maximum integer that can be entered
  // result_address - Whenever the selected integer is changed, the new index is written here
  IntegerPromptMenuItem(const string &prompt, int start_value, int min_value,
                        int max_value, int *result_address, const MenuView *view = gMenuView);
  bool OnKeyEvent(bool is_selected, bool is_confirmed, const KeyEvent &event, Action *action);
  void OnConfirmationChange(bool is_selected, bool is_confirmed, Action *action);
 
 private:
  void ResetFrame(bool is_confirmed);
  string prompt_;
  int value_, min_value_, max_value_, *result_address_;
  IntegerPromptFrame *prompt_frame_;
  const MenuView *view_;
  DISALLOW_EVIL_CONSTRUCTORS(IntegerPromptMenuItem);
};

// MenuFrame and MenuWidget
// ========================

// A menu that is always visible (i.e. not a pulldown menu). Items can be imbued with basic logic,
// intended primarily to let them respond to them being chosen. At all times, a menu has a
// "selection" - the item that is currently highlighted. If the user chooses this selection, the
// menu switches into confirmed state. While in this state, the menu stop responding to input until
// the menu item or the program unconfirms the selection.
class MenuFrame: public SingleParentFrame {
 public:
  enum SelectionStyle {NoMouse, SingleClick, DoubleClick};
  enum ItemBorderSizing {ExactlyRecSize, AtLeastRecSize, AtMostRecSize, IgnoreRecSize};

  // Constructors - a basic one for one-column vertical menus and another for arbitrary menus
  MenuFrame(float horz_justify = kJustifyCenter, SelectionStyle selection_style = SingleClick,
            const MenuView *view = gMenuView)
  : SingleParentFrame(new DummyMenuFrame(1, true, horz_justify, kJustifyCenter, view)),
    item_border_factory_(new BasicItemBorderFactory(0, 0, AtMostRecSize, IgnoreRecSize)),
    selection_style_(selection_style), view_(view), is_confirmed_(false),
    mouse_x_(-1), mouse_y_(-1), search_term_reset_timer_(0) {}
  MenuFrame(int num_cols, bool is_vertical, float horz_justify = kJustifyCenter,
            float vert_justify = kJustifyCenter, SelectionStyle selection_style = SingleClick,
            const MenuView *view = gMenuView)
  : SingleParentFrame(new DummyMenuFrame(num_cols, is_vertical, horz_justify, vert_justify, view)),
    item_border_factory_(new BasicItemBorderFactory(0, 0, is_vertical? AtMostRecSize:IgnoreRecSize,
                                                    is_vertical? IgnoreRecSize:AtLeastRecSize)),
    selection_style_(selection_style), view_(view), is_confirmed_(false),
    mouse_x_(-1), mouse_y_(-1), search_term_reset_timer_(0) {}
  ~MenuFrame() {Clear();}
  string GetType() const {return "MenuFrame";}

  // Meta-accessors. The view accessor is especially important if the client wishes to create a
  // menu item and wishes to choose an appropriate style.
  const MenuView *GetView() const {return view_;}
  bool IsVertical() const {return menu()->IsVertical();}

  // Item border style. The ItemBorderFactory provides a wrapper frame around every menu item. To
  // add padding around items or set their min / max size, the ItemBorderFactory should be changed.
  class ItemBorderFactory {
   public:
    virtual GlopFrame *GetBorderedItem(GlopFrame *item) = 0;
   protected:
    ItemBorderFactory() {}
   private:
    DISALLOW_EVIL_CONSTRUCTORS(ItemBorderFactory);
  };
  void SetBorderStyle(ItemBorderFactory *border_factory);
  void SetBasicBorderStyle(int abs_padding, ItemBorderSizing sizing) {
    SetBorderStyle(new BasicItemBorderFactory(abs_padding, 0, IsVertical()? sizing : IgnoreRecSize,
                   !IsVertical()? sizing : IgnoreRecSize));
  }
  void SetBasicBorderStyle(float rel_padding, ItemBorderSizing sizing) {
    SetBorderStyle(new BasicItemBorderFactory(0, rel_padding, IsVertical()? sizing : IgnoreRecSize,
                   !IsVertical()? sizing : IgnoreRecSize));
  }

  // Selection info
  int GetNumItems() const {return (int)items_.size();}
  int GetSelection() const {return menu()->GetSelection();}
  void SetSelection(int selection);
  void SetSelectionAndPing(int selection, bool center);
  void PingSelection(bool center = false);

  // Key-press emulation
  bool SelectUp(bool ping);
  bool SelectRight(bool ping);
  bool SelectDown(bool ping);
  bool SelectLeft(bool ping);
  bool PageUp(bool ping);
  bool PageRight(bool ping);
  bool PageDown(bool ping);
  bool PageLeft(bool ping);

  // Confirmation. Note that confirming brings the menu to focus and pings the selected menu item.
  void Confirm(bool is_confirmed);
  bool IsConfirmed() const {return is_confirmed_;}

  // Item addition - the menu item becomes owned by the menu
  int AddItem(GuiMenuItem *item) {return AddItem(item, GetNumItems());}
  int AddItem(GuiMenuItem *item, int index);
  int AddTextItem(const string &text) {
    return AddItem(new TextMenuItem(text, view_->GetTextStyle()));
  }
  int AddKeyPromptItem(const string &prompt, const GlopKey &start_value, const GlopKey &cancel_key,
                       const GlopKey &no_key, GlopKey *result_address) {
    return AddItem(new KeyPromptMenuItem(prompt, start_value, cancel_key, no_key, result_address,
                                         view_));
  }
  int AddStringSelectItem(const string &prompt, const vector<string> &options, int start_value,
                          int *result_address) {
    return AddItem(new StringSelectMenuItem(prompt, options, start_value, result_address, view_));
  }
  int AddStringPromptItem(const string &prompt, const string &start_value, int length_limit,
                          string *result_address) {
    return AddItem(new StringPromptMenuItem(prompt, start_value, length_limit, result_address,
                                            view_));
  }
  int AddIntegerPromptItem(const string &prompt, int start_value, int min_value, int max_value,
                           int *result_address) {
    return AddItem(new IntegerPromptMenuItem(prompt, start_value, min_value, max_value,
                                             result_address, view_));
  }

  // Item removal
  void RemoveItem() {RemoveItem(GetNumItems()-1);}
  void RemoveItem(int index);
  void Clear();

  // Overloaded logic functions
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, bool gained_focus);

 private:
  const DummyMenuFrame *menu() const {return (DummyMenuFrame*)GetChild();}
  DummyMenuFrame *menu() {return (DummyMenuFrame*)GetChild();}
  void RecordAction(int item, GuiMenuItem::Action action,
                    vector<pair<int, GuiMenuItem::Action> > *actions);
  void HandleActions(const vector<pair<int, GuiMenuItem::Action> > &actions);

  class BasicItemBorderFactory: public ItemBorderFactory {
   public:
    BasicItemBorderFactory(int abs_padding, float rel_padding, ItemBorderSizing horz_sizing,
                           ItemBorderSizing vert_sizing)
    : abs_padding_(abs_padding), rel_padding_(rel_padding), horz_sizing_(horz_sizing),
      vert_sizing_(vert_sizing) {}
    GlopFrame *GetBorderedItem(GlopFrame *item);
   private:
    int abs_padding_;
    float rel_padding_;
    ItemBorderSizing horz_sizing_, vert_sizing_;
    DISALLOW_EVIL_CONSTRUCTORS(BasicItemBorderFactory);
  };

  struct ItemInfo {
    ItemInfo(): controller(0), parent(0) {}
    ItemInfo(GuiMenuItem *_controller, EditableSingleParentFrame *_parent)
    : controller(_controller), parent(_parent) {}
    GuiMenuItem *controller;
    EditableSingleParentFrame *parent;
  };

  ItemBorderFactory *item_border_factory_;
  vector<ItemInfo> items_;
  SelectionStyle selection_style_;
  const MenuView *view_;
  bool is_confirmed_;
  int mouse_x_, mouse_y_;
  string search_term_;
  int search_term_reset_timer_;
  DISALLOW_EVIL_CONSTRUCTORS(MenuFrame);
};

class MenuWidget: public FocusFrame {
 public:
  enum SelectionStyle {NoMouse, SingleClick, DoubleClick};
  enum ItemBorderSizing {ExactlyRecSize, AtLeastRecSize, AtMostRecSize, IgnoreRecSize};

  // Constructors - a basic one for one-column vertical and another for arbitrary menus
  MenuWidget(float horz_justify = kJustifyCenter, SelectionStyle selection_style = SingleClick,
             const MenuView *view = gMenuView)
  : FocusFrame(new MenuFrame(horz_justify, (MenuFrame::SelectionStyle)selection_style, view)) {}
  MenuWidget(int num_cols, bool is_vertical, float horz_justify = kJustifyCenter,
             float vert_justify = kJustifyCenter, SelectionStyle selection_style = SingleClick,
             const MenuView *view = gMenuView)
  : FocusFrame(new MenuFrame(num_cols, is_vertical, horz_justify, vert_justify,
                             (MenuFrame::SelectionStyle)selection_style, view)) {}
  string GetType() const {return "MenuWidget";}

  // Meta-accessors
  const MenuView *GetView() const {return menu()->GetView();}
  bool IsVertical() const {return menu()->IsVertical();}

  // Item border style - see MenuFrame
  void SetBorderStyle(MenuFrame::ItemBorderFactory *border_factory) {
    menu()->SetBorderStyle(border_factory);
  }
  void SetBasicBorderStyle(int abs_padding, ItemBorderSizing sizing) {
    menu()->SetBasicBorderStyle(abs_padding, (MenuFrame::ItemBorderSizing)sizing);
  }
  void SetBasicBorderStyle(float rel_padding, ItemBorderSizing sizing) {
    menu()->SetBasicBorderStyle(rel_padding, (MenuFrame::ItemBorderSizing)sizing);
  }

  // Selection info
  int GetNumItems() const {return menu()->GetNumItems();}
  int GetSelection() const {return menu()->GetSelection();}
  void SetSelection(int selection) {menu()->SetSelection(selection);}
  void SetSelectionAndPing(int selection, bool center) {
    menu()->SetSelectionAndPing(selection, center);
  }
  void PingSelection(bool center = false) {menu()->PingSelection(center);}

  // Key-press emulation
  bool SelectUp(bool ping) {return menu()->SelectUp(ping);}
  bool SelectRight(bool ping) {return menu()->SelectRight(ping);}
  bool SelectDown(bool ping) {return menu()->SelectDown(ping);}
  bool SelectLeft(bool ping) {return menu()->SelectLeft(ping);}
  bool PageUp(bool ping) {return menu()->PageUp(ping);}
  bool PageRight(bool ping) {return menu()->PageRight(ping);}
  bool PageDown(bool ping) {return menu()->PageDown(ping);}
  bool PageLeft(bool ping) {return menu()->PageLeft(ping);}

  // Confirmation. Note that confirming brings the menu to focus and pings the selected menu item.
  void Confirm(bool is_confirmed) {menu()->Confirm(is_confirmed);}
  bool IsConfirmed() const {return menu()->IsConfirmed();}

  // Item addition - the menu item becomes owned by the menu
  int AddItem(GuiMenuItem *item) {return menu()->AddItem(item);}
  int AddItem(GuiMenuItem *item, int index) {return menu()->AddItem(item, index);}
  int AddTextItem(const string &text) {return menu()->AddTextItem(text);}
  int AddKeyPromptItem(const string &prompt, const GlopKey &start_value, const GlopKey &cancel_key,
                       const GlopKey &no_key, GlopKey *result_address) {
    return menu()->AddKeyPromptItem(prompt, start_value, cancel_key, no_key, result_address);
  }
  int AddStringSelectItem(const string &prompt, const vector<string> &options, int start_value,
                          int *result_address) {
    return menu()->AddStringSelectItem(prompt, options, start_value, result_address);
  }
  int AddStringPromptItem(const string &prompt, const string &start_value, int length_limit,
                          string *result_address) {
    return menu()->AddStringPromptItem(prompt, start_value, length_limit, result_address);
  }
  int AddIntegerPromptItem(const string &prompt, int start_value, int min_value, int max_value,
                           int *result_address) {
    return menu()->AddIntegerPromptItem(prompt, start_value, min_value, max_value, result_address);
  }

  // Item removal
  void RemoveItem() {menu()->RemoveItem();}
  void RemoveItem(int index) {menu()->RemoveItem(index);}
  void Clear() {menu()->Clear();}
 private:
  const MenuFrame *menu() const {return (MenuFrame*)GetChild();}
  MenuFrame *menu() {return (MenuFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(MenuWidget);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Dialog
// ======

class DialogWidget {
 public:
  // Hot key controls
  static ListId AddYesHotKey(const GlopKey &key) {Init(); return yes_keys_.push_back(key);}
  static void RemoveYesHotKey(ListId id) {Init(); yes_keys_.erase(id);}
  static void ClearYesHotKeys() {Init(); yes_keys_.clear();}
  static ListId AddNoHotKey(const GlopKey &key) {Init(); return no_keys_.push_back(key);}
  static void RemoveNoHotKey(ListId id) {Init(); no_keys_.erase(id);}
  static void ClearNoHotKeys() {Init(); no_keys_.clear();}
  static ListId AddOkayHotKey(const GlopKey &key) {Init(); return okay_keys_.push_back(key);}
  static void RemoveOkayHotKey(ListId id) {Init(); okay_keys_.erase(id);}
  static void ClearOkayHotKeys() {Init(); okay_keys_.clear();}
  static ListId AddCancelHotKey(const GlopKey &key) {Init(); return cancel_keys_.push_back(key);}
  static void RemoveCancelHotKey(ListId id) {Init(); cancel_keys_.erase(id);}
  static void ClearCancelHotKeys() {Init(); cancel_keys_.clear();}

  // Text-only dialog boxes.
  enum Result {Yes, No, Okay, Cancel};
  static void TextOkay(const string &title, const string &message,
                       const DialogView *view = gDialogView);
  static Result TextOkayCancel(const string &title, const string &message,
                               const DialogView *view = gDialogView);
  static Result TextYesNo(const string &title, const string &message,
                          const DialogView *view = gDialogView);
  static Result TextYesNoCancel(const string &title, const string &message,
                                const DialogView *view = gDialogView);

  // Dialog boxes with a string prompt.
  static string StringPromptOkay(const string &title, const string &message, const string &prompt,
                                 const string &start_value, int value_length_limit,
                                 const DialogView *view = gDialogView);
  static Result StringPromptOkayCancel(const string &title, const string &message,
                                       const string &prompt, const string &start_value,
                                       int value_length_limit, string *prompt_value,
                                       const DialogView *view = gDialogView);

  // Dialog boxes with an integer prompt.
  static int IntegerPromptOkay(const string &title, const string &message, const string &prompt,
                               int start_value, int min_value, int max_value,
                               const DialogView *view = gDialogView);
  static Result IntegerPromptOkayCancel(const string &title, const string &message,
                                        const string &prompt, int start_value, int min_value,
                                        int max_value, int *prompt_value,
                                        const DialogView *view = gDialogView);
 private:
  static void Init();
  static GlopFrame *Create(const string &title, const string &message, const string &prompt,
                           GlopFrame *extra_frame, bool has_yes_button, bool has_no_button,
                           bool has_okay_button, bool has_cancel_button,
                           const DialogView *view, vector<ButtonWidget*> *buttons,
                           vector<Result> *button_meanings);
  static Result Execute(const vector<ButtonWidget*> &buttons,
                        const vector<Result> &button_meanings);
  static Result DoText(const string &title, const string &message, bool has_yes_button,
                       bool has_no_button, bool has_okay_button, bool has_cancel_button,
                       const DialogView *view);
  static Result DoStringPrompt(const string &title, const string &message, const string &prompt,
                               const string &start_value, int value_length_limit,
                               string *prompt_value, bool has_okay_button, bool has_cancel_button,
                               const DialogView *view);
  static Result DoIntegerPrompt(const string &title, const string &message, const string &prompt,
                                int start_value, int min_value, int max_value, int *prompt_value,
                                bool has_okay_button, bool has_cancel_button,
                                const DialogView *view);
 
  static bool is_initialized_;
  static List<GlopKey> yes_keys_, no_keys_, okay_keys_, cancel_keys_;
  DISALLOW_EVIL_CONSTRUCTORS(DialogWidget);
};

#endif // GLOP_GLOP_FRAME_WIDGETS_H__
