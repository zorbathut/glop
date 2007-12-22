// Here we provide a set of useful stand-alone frames.
//
// Decorative Frames
// =================
//
// EmptyFrame: A convenience frame that takes max size and renders nothing.
// SolidBoxFrame, HollowBoxFrame: Solid or hollow boxes, possibly sized to fit around an existing
//                                frame.
// ArrowImageFrame: Renders an arrow in some direction. Used for slider buttons for example.
// WindowFrame: A decorative, unmovable window, optionally with a title.
//  Helper classes: WindowRenderer, DefaultWindowRenderer
// TextFrame, FancyTextFrame: Text output. TextFrame is faster but requires a uniform text style
//                            with no new lines. FancyTextFrame can handle new lines and changing
//                            style within the text.
//
// Interactive GUI Widgets
// =======================
//
// ButtonWidget: A basic push-button. Can override rendering via ButtonRenderer. Can override
//               functionality by extending AbstractButtonFrame.
//  Helper classes: ButtonRenderer, DefaultButtonRenderer, AbstractButtonFrame, DefaultButtonFrame
//
// SliderWidget: A horizontal or vertical scroll-bar, although with no scrolling properties. It can
//               be used to select any integer value. Can override rendering via SliderRenderer.
//  Helper classes: SliderRenderer, DefaultSliderRenderer, InnerSliderFrame, SliderFrame
//
//
// TextPrompt overview: A TextPrompt is any frame containing a flashing cursor and a text value
//   that responds to user input. For example, the text could be a user-entered string or it could
//   be the last key pressed by the user. The hierarchy is:
//     AbstractTextPromptFrame: The absolute base class
//     BasicTextPromptFrame: The base class for a TextPrompt that is edited one character at a
//                           time (e.g. string or integer prompts)
//     XXXPromptFrame: 
//
//
// See GlopFrameBase.h.

#ifndef GLOP_GLOP_FRAME_WIDGETS_H__
#define GLOP_GLOP_FRAME_WIDGETS_H__

// Includes
#include "Color.h"
#include "Font.h"
#include "GlopFrameBase.h"
#include "GlopFrameStyle.h"
#include "Input.h"

// Class declarations
class TextRenderer;

// EmptyFrame
// ==========
//
// This is the same as a regular GlopFrame - it fills the recommended size with empty space.
class EmptyFrame: public GlopFrame {
 public:
  EmptyFrame(): GlopFrame() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EmptyFrame);
};

// SolidBoxFrame
// =============
//
// This draws a filled box at the recommended size. If a frame is specified, the box is instead
// drawn behind the frame, and resized to match the frame size. See also HollowBoxFrame.
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

  virtual void Render() const;
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  bool has_outer_part_;
  Color inner_color_, outer_color_;
  DISALLOW_EVIL_CONSTRUCTORS(SolidBoxFrame);
};

// HollowBoxFrame
// ==============
//
// This is similar to SolidBoxFrame except that the box has no inner color.
class HollowBoxFrame: public SingleParentFrame {
 public:
  HollowBoxFrame(GlopFrame *frame, const Color &color): SingleParentFrame(frame), color_(color) {}
  HollowBoxFrame(const Color &color): SingleParentFrame(0), color_(color) {}

  virtual void Render() const ;
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 
 private:
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(HollowBoxFrame);
};

// ArrowImageFrame
// ===============
//
// Helper class that renders a black arrow covering about 80% of the frame. This is often on
// buttons - for example sliders.
class ArrowImageFrame: public GlopFrame {
 public:
  enum Direction {Up, Right, Down, Left};
  ArrowImageFrame(Direction d, const ArrowViewFactory *factory)
  : direction_(d), view_(factory->Create()) {}
  ~ArrowImageFrame() {delete view_;}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  ArrowView *view_;  
  Direction direction_;
  DISALLOW_EVIL_CONSTRUCTORS(ArrowImageFrame);
};

// TextFrame
// =========
//
// This renders a string text in a single TextStyle. Hard returns, soft returns, and multiple
// styles are not supported. See FancyTextFrame.
class TextFrame: public GlopFrame {
 public:
  TextFrame(const string &text, const TextStyle &style = gFrameStyle->text_style);
  virtual ~TextFrame();
  
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
  const TextStyle &GetStyle() const {return text_style_;}
  void SetStyle(const TextStyle &style) {
    text_style_ = style;
    DirtySize();
  }

  // Standard GlopFrame functionality
  virtual void Render() const;
 protected:
  virtual void RecomputeSize(int rec_width, int rec_height);

 private:
  string text_;
  TextStyle text_style_;
  TextRenderer *renderer_;
  DISALLOW_EVIL_CONSTRUCTORS(TextFrame);
};

// FpsFrame
// ========
//
// A TextFrame that always displays the current frame rate.
class FpsFrame: public SingleParentFrame {
 public:
  FpsFrame(const TextStyle &style = gFrameStyle->text_style)
  : SingleParentFrame(new TextFrame("", style)) {}
  const TextStyle &GetStyle() const {return text()->GetStyle();}
  void SetStyle(const TextStyle &style) {text()->SetStyle(style);}

  virtual void Think(int dt);
 private:
  const TextFrame *text() const {return (TextFrame*)GetChild();}
  TextFrame *text() {return (TextFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(FpsFrame);
};

// FancyTextFrame
// ==============
//
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
  FancyTextFrame(const string &text, const TextStyle &style = gFrameStyle->text_style);
  FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                 const TextStyle &style = gFrameStyle->text_style);

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
  const TextStyle &GetStyle() const {return text_style_;}
  void SetStyle(const TextStyle &style) {
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
    TextStyle style;
    TextRenderer *renderer;
  };
  ParseStatus CreateParseStatus();
  void StartParsing(ParseStatus *status);
  void StopParsing(ParseStatus *status);
  enum ParseResult {Normal, NewRenderer, Error};
  ParseResult ParseNextCharacter(const string &s, ParseStatus *status, char *ch);

  // Data
  string text_;
  float base_horz_justify_;
  TextStyle text_style_;
  bool add_soft_returns_;
  struct TextBlock {
    LightSetId child_id;
    int x, y;
  };
  vector<vector<TextBlock> > text_blocks_;
  DISALLOW_EVIL_CONSTRUCTORS(FancyTextFrame);
};

// AbstractTextPromptFrame
// =======================
//
// This is an abstract base class for a user-editable string of text. This text could be a generic
// string, an integer, a GlopKey, or virtually anything else that can be built on top of a
// TextFrame with a flashing cursor. Note that an AbstractTextPromptFrame cannot be instantiated
// directly - a subclass must be used instead.
class AbstractTextPromptFrame: public SingleParentFrame {
 public:
  // Overloaded functions for rendering the flashing cursor
  void Render() const;
  void Think(int dt);

  // Saves or unsaves the value of the text prompt. This is virtual so that overloaded frames do
  // not need to use the TextFrame as their data storage.
  virtual void StoreValue() = 0;
  virtual void RestoreValue() = 0;

  // Returns whether the text frame thinks the user tried to confirm or cancel his selection.
  // Updated when Think is called.
  bool WasConfirmed() const {return was_confirmed_;}
  bool WasCanceled() const {return was_canceled_;}

 protected:
  AbstractTextPromptFrame(const string &text, const TextStyle &style)
  : SingleParentFrame(new TextFrame(text, style)),
    cursor_timer_(0), cursor_pos_((int)text.size()),
    was_confirmed_(false), was_canceled_(false),
    was_confirm_queued_(false), was_cancel_queued_(false) {}

  // Access to the underlying text. Note that the only legal mutation is setting the text, and this
  // must go through us.
  const TextFrame *text() const {return (TextFrame*)GetChild();}
  virtual void SetText(const string &text) {
    ((TextFrame*)GetChild())->SetText(text);
    if ((int)text.size() < cursor_pos_)
      SetCursorPos((int)text.size());
  }
    
  // Compute and return the x-coordinate of each character (including the dummy last character) in
  // our text frame. Also, ensure that we have a little room at the end for our cursor.
  void RecomputeSize(int rec_width, int rec_height);
  const vector<int> &GetCharX() const {return char_x_;}

  // Make the cursor visible if we just appeared
  void OnFocusChange() {
    cursor_timer_ = 0;
    SingleParentFrame::OnFocusChange();
  }

  // Utilities for subclasses
  int GetCursorPos() const {return cursor_pos_;}
  void SetCursorPos(int pos, bool reset_timer = true) {
    if (cursor_pos_ != pos) {
      cursor_pos_ = min(max(pos, 0), (int)text()->GetText().size());
      if (reset_timer)
        cursor_timer_ = 0;
    }
  }
  void Confirm() {was_confirm_queued_ = true;}
  void Cancel() {was_cancel_queued_ = true;}

 private:
  vector<int> char_x_;
  int cursor_timer_, cursor_pos_;
  bool was_confirmed_, was_canceled_;
  bool was_confirm_queued_, was_cancel_queued_;
  DISALLOW_EVIL_CONSTRUCTORS(AbstractTextPromptFrame);
};

// BasicTextPromptFrame
// ====================
//
// This is a subclass of AbstractTextPromptFrame that emulates a standard text prompt with a
// cursor that can be moved with arrow keys and the mouse, as well as typing in any location. It
// is still an abstract class, and it cannot be instantiated directly. See StringPromptFrame below
// for the most basic implementation.
class BasicTextPromptFrame: public AbstractTextPromptFrame {
 public:
  // GlopFrame overloads
  void Render() const;
  bool OnKeyEvent(const KeyEvent &event, int dt);
  bool IsFocusKeeper(const KeyEvent &event) const;

  // AbstractTextPromptFrame overloads
  void StoreValue() {stored_value_ = text()->GetText();}
  void RestoreValue() {SetText(stored_value_);}

 protected:
  BasicTextPromptFrame(const string &text, const FrameStyle *style);
  BasicTextPromptFrame(const string &text, const TextStyle &text_style,
                       const Color &selection_color);
  void OnFocusChange();
  void SetCursorPos(int pos, bool move_anchor);
  void SetText(const string &text) {
    AbstractTextPromptFrame::SetText(text);
    selection_anchor_ = GetCursorPos();
  }

  // The two functions that any BasicTextPromptFrame must implement:
  // CanInsertCharacter: If in_theory is true, is it ever possible to add ASCII value ch to the
  //                     text? If false, is it possible to add the value at the current cursor
  //                     location? If so, the subclass can still control how it is added via
  //                     ReformText below.
  // ReformText: Given the prompt right after the user confirmed (if is_confirmed) or after a
  //             normal edit (if !is_confirmed), this changes the text to valid text. For example,
  //             this might convert lower-case letters to upper-case, or it might clip a number to
  //             certain upper or lower bounds.
  virtual bool CanInsertCharacter(char ch, bool in_theory) const = 0;
  virtual void ReformText(bool is_confirmed) = 0;

 private:
  void DeleteSelection();
  void DeleteCharacter(bool is_next_character);
  void InsertCharacter(char ch);
  void ReformTextAndCursor(bool is_confirmed);

  string stored_value_;
  bool is_tracking_mouse_;
  int selection_anchor_;
  Color selection_color_;
  DISALLOW_EVIL_CONSTRUCTORS(BasicTextPromptFrame);
};

// AbstractTextPromptFrame implementations
// =======================================
//
// None of these are automatically wrapped in a FocusFrame.

// GlopKeyPromptFrame - reads a GlopKey. Can override IsValidSelection to restrict to a subset of
// keys.
class GlopKeyPromptFrame: public AbstractTextPromptFrame {
 public:
  GlopKeyPromptFrame(const GlopKey &start_value,
                     const TextStyle &style = gFrameStyle->text_style)
  : AbstractTextPromptFrame("", style),
    stored_value_(start_value) {
    RestoreValue();
  }
  bool OnKeyEvent(const KeyEvent &event, int dt);
  bool IsFocusKeeper(const KeyEvent &event) const {
    return event.IsNonRepeatPress() && (IsValidSelection(event.key) || event.key == 27);
  }

  // Accessor and mutator
  void StoreValue() {stored_value_ = Get();}
  void RestoreValue() {Set(stored_value_);}
  const GlopKey &Get() const {return key_;}
  void Set(const GlopKey &key) {
    key_ = key;
    SetText(key.GetName());
    SetCursorPos((int)text()->GetText().size());
  }

 protected:
  virtual bool IsValidSelection(const GlopKey &key) const {
    return key != 27 && key != kKeyPause && key != kMouseUp && key != kMouseRight &&
           key != kMouseDown && key != kMouseLeft;
  }

 private:
  GlopKey key_, stored_value_;
  DISALLOW_EVIL_CONSTRUCTORS(GlopKeyPromptFrame);
};

// StringPromptFrame - A standard text prompt that limits the input length to be at most
// length_limit characters long unless length_limit == 0;
class StringPromptFrame: public BasicTextPromptFrame {
 public:
  StringPromptFrame(const string &start_value, int length_limit,
                    const FrameStyle *style = gFrameStyle)
  : BasicTextPromptFrame("", style), length_limit_(length_limit) {
    Set(start_value);
  }
  StringPromptFrame(const string &start_value, int length_limit,
                    const TextStyle &text_style = gFrameStyle->text_style,
                    const Color &selection_color = gFrameStyle->prompt_highlight_color)
  : BasicTextPromptFrame("", text_style, selection_color),
    length_limit_(length_limit) {
    Set(start_value);
  }

  // Accessor and mutator
  const string &Get() const {return text()->GetText();}
  void Set(const string &value);

 protected:
  bool CanInsertCharacter(char ch, bool in_theory) const;
  void ReformText(bool is_confirmed) {}

 private:
  int length_limit_;
  DISALLOW_EVIL_CONSTRUCTORS(StringPromptFrame);
};

// IntegerPromptFrame - A text prompt for integers in a certain range.
class IntegerPromptFrame: public BasicTextPromptFrame {
 public:
  IntegerPromptFrame(int start_value, int min_value, int max_value,
                     const FrameStyle *style = gFrameStyle)
  : BasicTextPromptFrame("", style), min_value_(min_value), max_value_(max_value) {
    Set(start_value);
  }
  IntegerPromptFrame(int start_value, int min_value, int max_value,
                     const TextStyle &text_style = gFrameStyle->text_style,
                     const Color &selection_color = gFrameStyle->prompt_highlight_color)
  : BasicTextPromptFrame("", text_style, selection_color),
    min_value_(min_value), max_value_(max_value) {
    Set(start_value);
  }

  // Accessor and mutator
  int Get() const {return atoi(text()->GetText().c_str());}
  void Set(int value) {SetText(Format("%d", value));}

 protected:
  bool CanInsertCharacter(char ch, bool in_theory) const;
  void ReformText(bool is_confirmed);

 private:
  int min_value_, max_value_;
  DISALLOW_EVIL_CONSTRUCTORS(IntegerPromptFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// WindowFrame
// ===========

class WindowFrame: public SingleParentFrame {
 public:
  WindowFrame(GlopFrame *inner_frame, const string &title,
              const WindowViewFactory *view_factory = gFrameStyle->window_view_factory);
  WindowFrame(GlopFrame *inner_frame,
              const WindowViewFactory *view_factory = gFrameStyle->window_view_factory);
  ~WindowFrame() {delete view_;}

  void Render() const;
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  WindowView *view_;
  PaddedFrame *padded_title_frame_, *padded_inner_frame_;
  DISALLOW_EVIL_CONSTRUCTORS(WindowFrame);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ButtonWidget
// ============

// Unfocusable version of a button with no pushing logic
class AbstractButtonFrame: public SingleParentFrame {
 public:
  AbstractButtonFrame(GlopFrame *inner_frame, const ButtonViewFactory *view_factory)
  : SingleParentFrame(new PaddedFrame(inner_frame)), view_(view_factory->Create()),
    is_down_(false), full_press_queued_(false), held_down_queued_(false),
    was_held_down_(false), was_pressed_fully_(false) {}
  ~AbstractButtonFrame() {delete view_;}

  // Overloaded Glop functions
  void Render() const;
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, int dt);

  // Returns whether the button is currently in the down state.
  bool IsDown() const {return is_down_;}

  // If the button generated events similar to a key on the keyboard, this returns whether a down
  // event would have been generated this frame. It will be true if a button is just pressed or
  // it will be true periodically while a button is held down.
  bool WasHeldDown() const {return was_held_down_;}

  // Returns whether a full press and release of the button was completed this frame.
  bool WasPressedFully() const {return was_pressed_fully_;}

 protected:
  void RecomputeSize(int rec_width, int rec_height);

  // Handles a state change on the button. The AbstractButtonFrame will never change whether the
  // button is down by itself. This must be done by an extending class using this function.
  //  UpNoFullRelease: The button is now in the not-pressed state. Do not mark it as having been
  //                   pressed fully this farme.
  //  UpFullRelease: The button is now in the not-pressed state. If it was just in the pressed
  //                 state, mark it as having been pressed fully this frame.
  //  Down: The button is now in the pressed state. If it was not pressed before, queue a
  //        WasHeldDown repeat event with a long delay (similar to a key just being pressed).
  //  DownRepeatSoon: The button is now in the pressed state. If it was not pressed before, queue a
  //                  WasHeldDown repeat event with a short delay (similar to a key being held
  //                  down).
  enum DownType {UpNoFullRelease, UpFullRelease, Down, DownRepeatSoon};
  void SetIsDown(DownType down_type);

 private:
  ButtonView *view_;
  bool is_down_,                // Is the button currently down? Set by extending class.
       full_press_queued_,      // The next value for was_pressed_fully_. Set in OnKeyEvent.
       held_down_queued_,       // The next value for was_held_down_. Set in OnKeyEvent.
       was_held_down_,          // Was a down event generated this frame? (similar to key down)
       was_pressed_fully_;      // Was a full press and release completed this frame?
  int held_down_repeat_delay_;  // The time in ms before was_held_down_ is set to true
  DISALLOW_EVIL_CONSTRUCTORS(AbstractButtonFrame);
};

class DefaultButtonFrame: public AbstractButtonFrame {
 public:
  DefaultButtonFrame(GlopFrame *inner_frame,
                     const ButtonViewFactory *factory = gFrameStyle->button_view_factory)
  : AbstractButtonFrame(inner_frame, factory),
    is_mouse_locked_on_(false) {}

  // Overloaded Glop functions
  bool OnKeyEvent(const KeyEvent &event, int dt);
  bool IsFocusMagnet(const KeyEvent &event) const {return hot_keys_.Find(event.key) != 0;}

  // Button functions
  LightSetId AddHotKey(const GlopKey &key) {return hot_keys_.InsertItem(key);}
  void RemoveHotKey(LightSetId id) {
    SetIsHotKeyDown(hot_keys_[id], false);
    hot_keys_.RemoveItem(id);
  }

 protected:
  virtual void OnFocusChange();

 private:
  void SetIsHotKeyDown(const GlopKey &key, bool is_down);

  LightSet<GlopKey> down_hot_keys_;
  LightSet<GlopKey> hot_keys_;
  bool is_mouse_locked_on_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultButtonFrame);
};

class ButtonWidget: public FocusFrame {
 public:
  // Basic constructors
  ButtonWidget(GlopFrame *frame,
               const ButtonViewFactory *factory = gFrameStyle->button_view_factory)
  : FocusFrame(button_ = new DefaultButtonFrame(frame, factory)) {}
  ButtonWidget(GlopFrame *frame, const GlopKey &hot_key,
               const ButtonViewFactory *factory = gFrameStyle->button_view_factory)
  : FocusFrame(button_ = new DefaultButtonFrame(frame, factory)) {
    button_->AddHotKey(hot_key);
  }
  ButtonWidget(GlopFrame *frame, const GlopKey &hot_key1, const GlopKey &hot_key2,
               const ButtonViewFactory *factory = gFrameStyle->button_view_factory)
  : FocusFrame(button_ = new DefaultButtonFrame(frame, factory)) {
    button_->AddHotKey(hot_key1);
    button_->AddHotKey(hot_key2);
  }

  // Convenience constructors for text button frames
  ButtonWidget(const string &text, const FrameStyle *style = gFrameStyle)
  : FocusFrame(button_ = new DefaultButtonFrame(new TextFrame(text, style->text_style),
                                                style->button_view_factory)) {}
  ButtonWidget(const string &text, const GlopKey &hot_key, const FrameStyle *style = gFrameStyle)
  : FocusFrame(button_ = new DefaultButtonFrame(new TextFrame(text, style->text_style),
                                                style->button_view_factory)) {
    button_->AddHotKey(hot_key);
  }
  ButtonWidget(const string &text, const GlopKey &hot_key1, const GlopKey &hot_key2,
               const FrameStyle *style = gFrameStyle)
  : FocusFrame(button_ = new DefaultButtonFrame(new TextFrame(text, style->text_style),
                                                style->button_view_factory)) {
    button_->AddHotKey(hot_key1);
    button_->AddHotKey(hot_key2);
  }

  // Utilities
  bool IsDown() const {return button_->IsDown();}
  bool WasHeldDown() const {return button_->WasHeldDown();}
  bool WasPressedFully() const {return button_->WasPressedFully();}
  
  LightSetId AddHotKey(const GlopKey &key) {return button_->AddHotKey(key);}
  void RemoveHotKey(LightSetId id) {button_->RemoveHotKey(id);}

 private:
  DefaultButtonFrame *button_;
  DISALLOW_EVIL_CONSTRUCTORS(ButtonWidget);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SliderWidget
// ============

class SliderFrame: public MultiParentFrame {
 public:
  enum Direction {Horizontal, Vertical};
  
  // Constructor - See SliderWidget
  SliderFrame(Direction direction, int tab_size, int total_size, int position,
              bool has_arrow_hot_keys = true, int step_size = -1,
              const SliderViewFactory *factory = gFrameStyle->slider_view_factory);
  ~SliderFrame() {delete view_;}

  // Hot keys
  LightSetId AddLargeDecHotKey(const GlopKey &key) {return large_dec_keys_.InsertItem(key);}
  LightSetId AddLargeIncHotKey(const GlopKey &key) {return large_inc_keys_.InsertItem(key);}
  LightSetId AddDecHotKey(const GlopKey &key) {return dec_button_->AddHotKey(key);}
  LightSetId AddIncHotKey(const GlopKey &key) {return inc_button_->AddHotKey(key);}
  void RemoveLargeDecHotKey(LightSetId id) {large_dec_keys_.RemoveItem(id);}
  void RemoveLargeIncHotKey(LightSetId id) {large_inc_keys_.RemoveItem(id);}
  void RemoveDecHotKey(LightSetId id) {dec_button_->RemoveHotKey(id);}
  void RemoveIncHotKey(LightSetId id) {inc_button_->RemoveHotKey(id);}

  // State accessors/mutators
  int GetTabPosition() const {return tab_logical_position_;}
  void SetTabPosition(int position);
  void SmallDec() {SetTabPosition(tab_logical_position_ - step_size_);}
  void SmallInc() {SetTabPosition(tab_logical_position_ + step_size_);}
  void LargeDec() {SetTabPosition(tab_logical_position_ - (tab_logical_size_*9+9)/10);}
  void LargeInc() {SetTabPosition(tab_logical_position_ + (tab_logical_size_*9+9)/10);}
  int GetTabSize() const {return tab_logical_size_;}
  int GetTotalSize() const {return total_logical_size_;}

  // Overloaded functions
  void Render() const;
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, int dt);
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
  void OnChildPing(GlopFrame *child, int x1, int y1, int x2, int y2, bool center);
  void OnFocusChange();

 private:
  int GetLocationPixelStart(int pos) const;
  int GetLocationByPixel(int pixel_location);
  void RecomputeTabScreenPosition();

  Direction direction_;
  LightSet<GlopKey> large_dec_keys_, large_inc_keys_;
  DefaultButtonFrame *dec_button_, *inc_button_;
  SliderView *view_;

  enum MouseLockMode {None, Bar, Tab};
  int tab_logical_position_, tab_logical_size_, total_logical_size_;
  int step_size_;
  MouseLockMode mouse_lock_mode_;
  int bar_pixel_length_, inner_bar_x_, inner_bar_y_;
  int tab_pixel_length_, tab_x1_, tab_y1_, tab_x2_, tab_y2_;
  int tab_grab_position_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderFrame);
};

class SliderWidget: public FocusFrame {
 public:
  enum Direction {Horizontal, Vertical};  // Should match SliderFrame::Direction

  // Constructor.
  //  direction: Is this a horizontal slider or a vertical slider?
  //  tab_size: The logical length of the tab (at least 1)
  //  total_size: The logical length of the entire slider bar
  //  position: The logical position where the tab start
  //  has_arrow_hot_keys: Whether the slider should respond to arrow keys
  //  step_size: How much pushing a button will move the slider (-1 sets a default value)
  //  renderer: The function uses for rendering the slider
  SliderWidget(Direction direction, int tab_size, int total_size, int position,
               bool has_arrow_hot_keys = true, int step_size = -1,
               const SliderViewFactory *factory = gFrameStyle->slider_view_factory)
  : FocusFrame(slider_ = new SliderFrame((SliderFrame::Direction)direction, tab_size, total_size,
                                         position, has_arrow_hot_keys, step_size, factory)) {}

  // Hot keys
  LightSetId AddLargeDecHotKey(const GlopKey &key) {return slider_->AddLargeDecHotKey(key);}
  LightSetId AddLargeIncHotKey(const GlopKey &key) {return slider_->AddLargeIncHotKey(key);}
  LightSetId AddDecHotKey(const GlopKey &key) {return slider_->AddDecHotKey(key);}
  LightSetId AddIncHotKey(const GlopKey &key) {return slider_->AddIncHotKey(key);}
  void RemoveLargeDecHotKey(LightSetId id) {slider_->RemoveLargeDecHotKey(id);}
  void RemoveLargeIncHotKey(LightSetId id) {slider_->RemoveLargeIncHotKey(id);}
  void RemoveDecHotKey(LightSetId id) {slider_->RemoveDecHotKey(id);}
  void RemoveIncHotKey(LightSetId id) {slider_->RemoveIncHotKey(id);}

  // State accessors/mutators
  int GetTabPosition() const {return slider_->GetTabPosition();}
  void SetTabPosition(int position) {slider_->SetTabPosition(position);}
  int GetTabSize() const {return slider_->GetTabSize();}
  int GetTotalSize() const {return slider_->GetTotalSize();}

 private:
  SliderFrame *slider_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderWidget);
};

#endif // GLOP_GLOP_FRAME_WIDGETS_H__
