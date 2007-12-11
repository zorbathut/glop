// Here we provide a set of useful stand-alone frames.
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
// WindowFrame: A decorative, unmovable window, optionally with a title.
//  Helper classes: WindowRenderer, DefaultWindowRenderer
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
// See GlopFrameBase.h.

#ifndef GLOP_FRAME_WIDGETS_H__
#define GLOP_FRAME_WIDGETS_H__

// Includes
#include "Color.h"
#include "GlopFrameBase.h"
#include "Input.h"

// Style constants
const float kDefaultTextHeight(0.025f);

const Color kDefaultWindowBorderHighlightColor(0.9f, 0.9f, 0.95f);
const Color kDefaultWindowBorderLowlightColor(0.6f, 0.6f, 0.7f);
const Color kDefaultWindowInnerColor(0.8f, 0.8f, 0.83f);
const Color kDefaultWindowTitleColor(0, 0, 0);

const float kDefaultButtonBorderSize = 0.003f;
const Color kDefaultButtonSelectionColor(0, 0, 1.0f);
const Color kDefaultButtonBorderColor(0.2f, 0.2f, 0.2f);
const Color kDefaultButtonHighlightColor(0.95f, 0.95f, 0.95f);
const Color kDefaultButtonLowlightColor(0.5f, 0.5f, 0.5f);
const Color kDefaultButtonUnpressedInnerColor(0.85f, 0.85f, 0.88f);
const Color kDefaultButtonPressedInnerColor(0.75f, 0.75f, 0.77f);

const float kDefaultSliderWidth = 0.03f;
const Color kDefaultSliderBackgroundColor(0.7f, 0.7f, 0.7f);
const Color kDefaultSliderBorderColor(0.2f, 0.2f, 0.2f);

// Class declarations
class InnerSliderFrame;

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
class SolidBoxFrame: public PaddedFrame {
 public:
  SolidBoxFrame(GlopFrame *frame, const Color &inner_color, const Color &outer_color)
  : PaddedFrame(frame, 1), has_outer_part_(true), inner_color_(inner_color),
    outer_color_(outer_color) {}
  SolidBoxFrame(const Color &inner_color, const Color &outer_color)
  : PaddedFrame(0, 1), has_outer_part_(true), inner_color_(inner_color),
    outer_color_(outer_color) {}
  SolidBoxFrame(GlopFrame *frame, const Color &inner_color)
  : PaddedFrame(frame, 0), has_outer_part_(false), inner_color_(inner_color) {}
  SolidBoxFrame(const Color &inner_color)
  : PaddedFrame(0, 0), has_outer_part_(false), inner_color_(inner_color) {}
  virtual void Render();

 private:
  bool has_outer_part_;
  Color inner_color_, outer_color_;
  DISALLOW_EVIL_CONSTRUCTORS(SolidBoxFrame);
};

// HollowBoxFrame
// ==============
//
// This is similar to SolidBoxFrame except that the box has no inner color.
class HollowBoxFrame: public PaddedFrame {
 public:
  HollowBoxFrame(GlopFrame *frame, const Color &color): PaddedFrame(frame, 1), color_(color) {}
  HollowBoxFrame(const Color &color): PaddedFrame(0, 1), color_(color) {}
  virtual void Render();

 private:
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(HollowBoxFrame);
};

// TextFrame
// =========
//
// This renders a single line of text in a single style. The following customization is supported:
//  FontOutlineId, Color: Font and color.
//  Height: The text will automatically rescale to be this fraction of the window height.
//  IsFullHeight: If true, the frame height will be set so that it completely covers the line of
//                text. Otherwise the size will not include overhang below the baseline. For
//                example, the frame size will include the bottom part of a 'p' only if
//                IsFullHeight==true. Usually, this would be true, but in a paragraph of text, it
//                is natural to use IsFullHeight == false to pack the text closer together.
class TextFrame: public GlopFrame {
 public:
  TextFrame(const string &text, const FrameStyle *style = gDefaultStyle,
            bool is_full_height = true);
  TextFrame(const string &text, const Color &color, float text_height = gDefaultStyle->text_height,
            LightSetId font_outline_id = gDefaultStyle->font_outline_id,
            bool is_full_height = true);
  virtual ~TextFrame();
  
  // A utility to return the pixel height we would choose for a font given the relative height as
  // specified to TextFrame. This is provided so that an external class can get font information
  // about a potential TextFrame before instantiating the TextFrame.
  static int GetFontPixelHeight(float height);

  // Accessors and mutators
  const string &GetText() const {return text_;}
  void SetText(const string &text) {
    if (text_ != text) {
      text_ = text;
      DirtySize();
    }
  }
  LightSetId GetFontId() const {return font_id_;}
  const Color &GetColor() const {return color_;}
  void SetColor(const Color &color) {color_ = color;}
  float GetFontHeight() const {return text_height_;}
  void SetFontHeight(float text_height) {
    if (text_height_ != text_height) {
      text_height_ = text_height;
      DirtySize();
    }
  }
  bool IsFullHeight() const {return is_full_height_;}

  // Standard overloaded functionality
  virtual void Render();
 protected:
  virtual void RecomputeSize(int rec_width, int rec_height);

 private:
  string text_;
  LightSetId font_outline_id_, font_id_;
  Color color_;
  float text_height_;
  bool is_full_height_;
  DISALLOW_EVIL_CONSTRUCTORS(TextFrame);
};

// FpsFrame
// ========
//
// This is a TextFrame that always gives the current frame rate.
class FpsFrame: public SingleParentFrame {
 public:
  FpsFrame(const FrameStyle *style = gDefaultStyle, bool is_full_height = true)
  : SingleParentFrame(new TextFrame("", style, is_full_height)) {}
  FpsFrame(const Color &color, float text_height = gDefaultStyle->text_height,
           LightSetId font_outline_id = gDefaultStyle->font_outline_id, bool is_full_height = true)
  : SingleParentFrame(new TextFrame("", color, text_height, font_outline_id, is_full_height)) {}

  // Text properties
  const Color &GetColor() const {return text()->GetColor();}
  void SetColor(const Color &color) {text()->SetColor(color);}
  float GetFontHeight() const {return text()->GetFontHeight();}
  void SetFontHeight(float text_height) {text()->SetFontHeight(text_height);}
  bool IsFullHeight() const {return text()->IsFullHeight();}

  // Logic
  virtual void Think(int dt);

 private:
  const TextFrame *text() const {return (TextFrame*)GetChild();}
  TextFrame *text() {return (TextFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(FpsFrame);
};

// FancyTextFrame
// ==============
//
// This is a TextFrame with additional formatting options. In particular, it supports:
//  - New lines. These may either be used explicitly with "\n", or they can be added automatically
//    as soft returns so that the FancyTextFrame will not exceed its recommended width. The latter
//    feature can be disabled by setting add_soft_returns to false.
//  - Colors. If a CTag is added into a string, all following characters are printed with the given
//    CTag.color.
//  - Sizes. If an STag is added into a string, all following characters are printed with size
//    STag.size * base_text_height.
// All tags are implemented with ASCII value 1 delimiters.
class FancyTextFrame: public SingleParentFrame {
 public:
  // Constructors. horz_justify is used to align different rows of text.
  FancyTextFrame(const string &text, const FrameStyle *style = gDefaultStyle);
  FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                 const FrameStyle *style = gDefaultStyle);

  FancyTextFrame(const string &text, const Color &start_color,
                 float base_text_height = gDefaultStyle->text_height,
                 LightSetId font_outline_id = gDefaultStyle->font_outline_id);
  FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                 const Color &start_color, float base_text_height = gDefaultStyle->text_height,
                 LightSetId font_outline_id = gDefaultStyle->font_outline_id);

  // Tags. These can be added directly to any string.
  //   Example: CTag(kRed) + "red text " + CTag(kWhite) + " white text";
  // If this string is given to a FancyTextFrame, it will print the string accordingly.
  static const string CTag(const Color &color) {
    return Format("\1c%02X%02X%02X%02X\1",
      min(max(int(color[0] * 255), 0), 255), min(max(int(color[1] * 255), 0), 255),
      min(max(int(color[2] * 255), 0), 255), min(max(int(color[3] * 255), 0), 255));
  }
  static const string STag(float size_multiplier) {return Format("\1s%f\1", size_multiplier);}
    
  // Accessors and mutators
  const string &GetText() const {return text_;}
  void SetText(const string &text) {
    if (text_ != text) {
      text_ = text;
      DirtySize();
    }
  }
  LightSetId GetFontOutlineId() const {return font_outline_id_;}
  void SetFontOutlineId(LightSetId font_outline_id) {
    if (font_outline_id_ != font_outline_id) {
      font_outline_id_ = font_outline_id;
      DirtySize();
    }
  }
  const Color &GetStartColor() const {return start_color_;}
  void SetStartColor(const Color &color) {
    if (start_color_ != color) {
      start_color_ = color;
      DirtySize();
    }
  }
  float GetBaseFontHeight() const {return base_text_height_;}
  void SetBaseFontHeight(float base_text_height) {
    if (base_text_height_ != base_text_height) {
      base_text_height_ = base_text_height;
      DirtySize();
    }
  }
  float GetHorzJustify() const {return column()->GetDefaultHorzJustify();}
  void SetHorzJustify(float horz_justify) {column()->SetDefaultHorzJustify(horz_justify);}

 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  const ColFrame *column() const {return (ColFrame*)GetChild();}
  ColFrame *column() {return (ColFrame*)GetChild();}

  // Parsing utilities
  struct ParseStatus {
    int pos;
    Color color;
    float text_height;
    LightSetId font_id;
  };
  ParseStatus CreateParseStatus();
  void StartParsing(ParseStatus *status);
  void StopParsing(ParseStatus *status);
  bool ParseNextCharacter(const string &s, ParseStatus *status, char *ch);

  string text_;
  LightSetId font_outline_id_;
  bool add_soft_returns_;
  Color start_color_;
  float base_text_height_;
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
  void Render();
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
  AbstractTextPromptFrame(const string &text, const FrameStyle *style)
  : SingleParentFrame(new TextFrame(text, style)),
    cursor_timer_(0), cursor_pos_((int)text.size()),
    was_confirmed_(false), was_canceled_(false),
    was_confirm_queued_(false), was_cancel_queued_(false) {}
  AbstractTextPromptFrame(const string &text, const Color &color, float text_height,
                          LightSetId font_outline_id)
  : SingleParentFrame(new TextFrame(text, color, text_height, font_outline_id)),
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
  void Render();
  bool OnKeyEvent(const KeyEvent &event, int dt);
  bool IsFocusKeeper(const KeyEvent &event) const;

  // AbstractTextPromptFrame overloads
  void StoreValue() {stored_value_ = text()->GetText();}
  void RestoreValue() {SetText(stored_value_);}

 protected:
  BasicTextPromptFrame(const string &text, const FrameStyle *style);
  BasicTextPromptFrame(const string &text, const Color &color, float text_height,
                       LightSetId font_outline_id, const Color &selection_color);
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
  GlopKeyPromptFrame(const GlopKey &start_value, const FrameStyle *style = gDefaultStyle)
  : AbstractTextPromptFrame("", style), stored_value_(start_value) {
    RestoreValue();
  }

  GlopKeyPromptFrame(const GlopKey &start_value, const Color &color,
                     float text_height = gDefaultStyle->text_height,
                     LightSetId font_outline_id = gDefaultStyle->font_outline_id)
  : AbstractTextPromptFrame("", color, text_height, font_outline_id),
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
                    const FrameStyle *style = gDefaultStyle)
  : BasicTextPromptFrame("", style), length_limit_(length_limit) {
    Set(start_value);
  }
  StringPromptFrame(const string &start_value, int length_limit, const Color &color,
                    float text_height = gDefaultStyle->text_height,
                    LightSetId font_outline_id = gDefaultStyle->font_outline_id,
                    const Color &selection_color = gDefaultStyle->prompt_highlight_color)
  : BasicTextPromptFrame("", color, text_height, font_outline_id, selection_color),
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
                     const FrameStyle *style = gDefaultStyle)
  : BasicTextPromptFrame("", style), min_value_(min_value), max_value_(max_value) {
    Set(start_value);
  }
  IntegerPromptFrame(int start_value, int min_value, int max_value, const Color &color,
                     float text_height = gDefaultStyle->text_height,
                     LightSetId font_outline_id = gDefaultStyle->font_outline_id,
                     const Color &selection_color = gDefaultStyle->prompt_highlight_color)
  : BasicTextPromptFrame("", color, text_height, font_outline_id, selection_color),
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

// Overloadable renderer
class WindowRenderer {
 public:
  WindowRenderer() {}

  // Creates a frame for the title bar. Some (or all) of the rendering can be done here within
  // Render. However, this frame is, at the very least, required to return an appropriate size for
  // the title bar.
  virtual GlopFrame *CreateTitleFrame(const string &title) const = 0;

  // Calculates the padding between the inner frame and the edge of the window (or of the title
  // bar if has_title == true).
  virtual void GetInnerFramePadding(bool has_title, int *lp, int *tp, int *rp, int *bp) const = 0;

  // Renders a window with the given coordinates and inner frames. Note that title_frame may be
  // 0 if the window has no title.
  virtual void Render(int x1, int y1, int x2, int y2, GlopFrame *title_frame,
                      GlopFrame *inner_frame) const = 0;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(WindowRenderer);
};

class DefaultWindowRenderer: public WindowRenderer {
 public:
  // Constructors
  DefaultWindowRenderer(LightSetId font_id = 0)
  : border_highlight_color_(kDefaultWindowBorderHighlightColor),
    border_lowlight_color_(kDefaultWindowBorderLowlightColor),
    inner_color_(kDefaultWindowInnerColor), title_color_(kDefaultWindowTitleColor),
    title_font_id_(font_id), title_height_(kDefaultTextHeight) {}
  DefaultWindowRenderer(const Color &border_highlight_color, const Color &border_lowlight_color,
                        const Color &inner_color, const Color &title_color,
                        LightSetId title_font_id, float title_height)
  : border_highlight_color_(border_highlight_color), border_lowlight_color_(border_lowlight_color),
    inner_color_(inner_color), title_color_(title_color), title_font_id_(title_font_id),
    title_height_(title_height) {}

  // Mutators
  void SetBorderHighlightColor(const Color &c) {border_highlight_color_ = c;}
  void SetBorderLowlightColor(const Color &c) {border_lowlight_color_ = c;}
  void SetInnerColor(const Color &c) {inner_color_ = c;}
  void SetTitleColor(const Color &c) {title_color_ = c;}
  void SetTitleFontId(LightSetId id) {title_font_id_ = id;}
  void SetTitleHeight(float height) {title_height_ = height;}

  // Operations
  GlopFrame *CreateTitleFrame(const string &title) const;
  void GetInnerFramePadding(bool has_title, int *lp, int *tp, int *rp, int *bp) const;
  void Render(int x1, int y1, int x2, int y2, GlopFrame *title_frame,
              GlopFrame *inner_frame) const;

 private:
  Color border_highlight_color_, border_lowlight_color_, inner_color_, title_color_;
  LightSetId title_font_id_;
  float title_height_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultWindowRenderer);
};

class WindowFrame: public SingleParentFrame {
 public:
  WindowFrame(GlopFrame *inner_frame, const string &title,
              const WindowRenderer *renderer = gDefaultStyle->window_renderer);
  WindowFrame(GlopFrame *inner_frame,
              const WindowRenderer *renderer = gDefaultStyle->window_renderer);

  void Render() {renderer_->Render(GetX(), GetY(), GetX2(), GetY2(), title_frame_, inner_frame_);}
 
 private:
  const WindowRenderer *renderer_;
  GlopFrame *title_frame_, *inner_frame_;
  DISALLOW_EVIL_CONSTRUCTORS(WindowFrame);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ButtonWidget
// ============

// Overloadable renderer
class ButtonRenderer {
 public:
  ButtonRenderer() {}

  // Calculates the padding between the inner frame and the edge of the button.
  virtual void RecomputePadding(bool is_down, int *lp, int *tp, int *rp, int *bp) const = 0;

  // Renders a button with the given metrics. inner_frame MAY be null (for example, while rendering
  // the tab of a slider frame).
  virtual void Render(bool is_down, bool is_primary_focus, int x1, int y1, int x2, int y2,
                      GlopFrame *inner_frame) const = 0;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ButtonRenderer);
};

class DefaultButtonRenderer: public ButtonRenderer {
 public:
  // Constructors
  DefaultButtonRenderer()
  : border_size_(kDefaultButtonBorderSize), selection_color_(kDefaultButtonSelectionColor),
    border_color_(kDefaultButtonBorderColor), highlight_color_(kDefaultButtonHighlightColor),
    lowlight_color_(kDefaultButtonLowlightColor),
    unpressed_inner_color_(kDefaultButtonUnpressedInnerColor),
    pressed_inner_color_(kDefaultButtonPressedInnerColor) {}
  DefaultButtonRenderer(float border_size, const Color &selection_color, const Color &border_color,
                        const Color &highlight_color, const Color &lowlight_color,
                        const Color &unpressed_inner_color, const Color &pressed_inner_color)
  : border_size_(border_size), selection_color_(selection_color), border_color_(border_color),
    highlight_color_(highlight_color), lowlight_color_(lowlight_color),
    unpressed_inner_color_(unpressed_inner_color), pressed_inner_color_(pressed_inner_color) {}

  // Mutators
  void SetBorderSize(float border_size) {border_size_ = border_size;}
  void SetSelectionColor(const Color &c) {selection_color_ = c;}
  void SetBorderColor(const Color &c) {border_color_ = c;}
  void SetHighlightColor(const Color &c) {highlight_color_ = c;}
  void SetLowlightColor(const Color &c) {lowlight_color_ = c;}
  void SetUnpressedInnerColor(const Color &c) {unpressed_inner_color_ = c;}
  void SetPressedInnerColor(const Color &c) {pressed_inner_color_ = c;}

  // Operations
  void RecomputePadding(bool is_down, int *lp, int *tp, int *rp, int *bp) const;
  void Render(bool is_down, bool is_primary_focus, int x1, int y1, int x2, int y2,
              GlopFrame *inner_frame) const;

 private:
  float border_size_;
  Color selection_color_, border_color_, highlight_color_, lowlight_color_, unpressed_inner_color_,
        pressed_inner_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultButtonRenderer);
};

// Unfocusable version of a button with no pushing logic
class AbstractButtonFrame: public PaddedFrame {
 public:
  AbstractButtonFrame(GlopFrame *inner_frame, const ButtonRenderer *renderer)
  : PaddedFrame(inner_frame, 0), renderer_(renderer),
    is_down_(false), full_press_queued_(false), held_down_queued_(false),
    was_held_down_(false), was_pressed_fully_(false) {
    RecomputePadding();
  }

  // Overloaded Glop functions
  void Render() {
    renderer_->Render(is_down_, IsPrimaryFocus(), GetX(), GetY(), GetX2(), GetY2(), GetChild());
  }
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, int dt);

  // Button functions
  bool IsDown() const {return is_down_;}
  bool WasHeldDown() const {return was_held_down_;}
  bool WasPressedFully() const {return was_pressed_fully_;}

 protected:
  void RecomputeSize(int rec_width, int rec_height) {
    RecomputePadding();
    PaddedFrame::RecomputeSize(rec_width, rec_height);
  }
  enum DownType {UpNoFullRelease, UpFullRelease, Down, DownRepeatSoon};
  void SetIsDown(DownType down_type);

 private:
  void RecomputePadding();

  const ButtonRenderer *renderer_;
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
                     const ButtonRenderer *renderer = gDefaultStyle->button_renderer)
  : AbstractButtonFrame(inner_frame, renderer),
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
  ButtonWidget(GlopFrame *frame, const ButtonRenderer *renderer = gDefaultStyle->button_renderer)
  : FocusFrame(button_ = new DefaultButtonFrame(frame, renderer)) {}
  ButtonWidget(GlopFrame *frame, const GlopKey &hot_key,
               const ButtonRenderer *renderer = gDefaultStyle->button_renderer)
  : FocusFrame(button_ = new DefaultButtonFrame(frame, renderer)) {
    button_->AddHotKey(hot_key);
  }
  ButtonWidget(GlopFrame *frame, const GlopKey &hot_key1, const GlopKey &hot_key2,
               const ButtonRenderer *renderer = gDefaultStyle->button_renderer)
  : FocusFrame(button_ = new DefaultButtonFrame(frame, renderer)) {
    button_->AddHotKey(hot_key1);
    button_->AddHotKey(hot_key2);
  }

  // Convenience constructors for text button frames
  ButtonWidget(const string &text, const FrameStyle *style = gDefaultStyle)
  : FocusFrame(button_ = new DefaultButtonFrame(new TextFrame(text, style),
               style->button_renderer)) {}
  ButtonWidget(const string &text, const GlopKey &hot_key, const FrameStyle *style = gDefaultStyle)
  : FocusFrame(button_ = new DefaultButtonFrame(new TextFrame(text, style),
               style->button_renderer)) {
    button_->AddHotKey(hot_key);
  }
  ButtonWidget(const string &text, const GlopKey &hot_key1, const GlopKey &hot_key2,
               const FrameStyle *style = gDefaultStyle)
  : FocusFrame(button_ = new DefaultButtonFrame(new TextFrame(text, style),
               style->button_renderer)) {
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

// Overloadable renderer
class SliderRenderer {
 public:
  SliderRenderer(const ButtonRenderer *renderer): button_renderer_(renderer) {}
  void SetButtonRenderer(const ButtonRenderer *renderer) {button_renderer_ = renderer;}
  const ButtonRenderer *GetButtonRenderer() const {return button_renderer_;}

  // Recomputes the minor-axis size of the slider.
  virtual int RecomputeWidth(bool is_horizontal) const = 0;

  // Recomputes the minimum length of the slider tab. Dimensions are only for the inner part.
  virtual int RecomputeMinTabLength(bool is_horizontal, int inner_width,
                                    int inner_height) const = 0;

  // Renders the slider inner frame given coordinates of the inner frame, and coordinates of the
  // tab relative to the inner frame coordinates.
  virtual void Render(bool is_horizontal, bool is_primary_focus, int x1, int y1, int x2, int y2,
                      int tab_x1, int tab_y1, int tab_x2, int tab_y2) const = 0;

 private:
  const ButtonRenderer *button_renderer_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderRenderer);
};

class DefaultSliderRenderer: public SliderRenderer {
 public:
  // Constructors
  DefaultSliderRenderer(const ButtonRenderer *button_renderer)
  : SliderRenderer(button_renderer), width_(kDefaultSliderWidth),
    background_color_(kDefaultSliderBackgroundColor), border_color_(kDefaultSliderBorderColor) {}
  DefaultSliderRenderer(const ButtonRenderer *button_renderer, float width,
    const Color &background_color, const Color &border_color)
  : SliderRenderer(button_renderer), width_(width), background_color_(background_color),
    border_color_(border_color) {}

  // Mutators
  void SetWidth(float width) {width_ = width;}
  void SetBackgroundColor(const Color &c) {background_color_ = c;}
  void SetBorderColor(const Color &c) {border_color_ = c;}

  // Operations
  int RecomputeWidth(bool is_horizontal) const;
  int RecomputeMinTabLength(bool is_horizontal, int inner_width, int inner_height) const;
  void Render(bool is_horizontal, bool is_primary_focus, int x1, int y1, int x2, int y2,
              int tab_x1, int tab_y1, int tab_x2, int tab_y2) const;

 private:
  float width_;
  Color background_color_, border_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultSliderRenderer);
};

class SliderFrame: public SingleParentFrame {
 public:
  enum Direction {Horizontal, Vertical};
  
  // Constructor - See SliderWidget
  SliderFrame(Direction direction, int tab_size, int total_size, int position,
              bool has_arrow_hot_keys = true, int step_size = -1,
              const SliderRenderer *renderer = gDefaultStyle->slider_renderer);
  
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
  int GetTabPosition() const;
  void SetTabPosition(int position);
  int GetTabSize() const;
  int GetTotalSize() const;

  // Overloaded functions
  void Think(int dt);
  bool OnKeyEvent(const KeyEvent &event, int dt);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
  void OnChildPing(int x1, int y1, int x2, int y2, bool center);

 private:
  void Init(Direction direction, int tab_size, int total_size, int position,
            bool has_arrow_hot_keys, int step_size, const SliderRenderer *renderer);
  
  Direction direction_;
  LightSet<GlopKey> large_dec_keys_, large_inc_keys_;
  DefaultButtonFrame *dec_button_, *inc_button_;
  InnerSliderFrame *inner_slider_;
  const SliderRenderer *renderer_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderFrame);
};

class SliderWidget: public FocusFrame {
 public:
  enum Direction {Horizontal, Vertical};

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
               const SliderRenderer *renderer = gDefaultStyle->slider_renderer)
  : FocusFrame(slider_ = new SliderFrame((SliderFrame::Direction)direction, tab_size, total_size,
                                         position, has_arrow_hot_keys, step_size, renderer)) {}

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

#endif // GLOP_FRAME_WIDGETS_H__
