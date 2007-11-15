// Here we provide a set of useful stand-alone frames.
//
// See GlopFrameBase.h.

#ifndef GLOP_FRAME_WIDGETS_H__
#define GLOP_FRAME_WIDGETS_H__

// Includes
#include "Color.h"
#include "GlopFrameBase.h"
#include "Input.h"

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
  // Data
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
  // Data
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(HollowBoxFrame);
};

// TextFrame
// =========
//
// This renders a single line of text in a single style. The following customization is supported:
//  FontOutlineId, Color: Font and color.
//  Height: The text will automatically rescale to be this fraction of the window height.
//  IsUsingEllipsis: If true, the frame will automatically add ellipses should the text go
//                   beyond the clipping reason. True by default.
//  IsFullHeight: If true, the frame height will be set so that it completely covers the line of
//                text. Otherwise the size will not include overhang below the baseline. For
//                example, the frame size will include the bottom part of a 'p' only if
//                IsFullHeight==true. Usually, this would be true, but in a paragraph of text, it
//                is natural to use IsFullHeight == false to pack the text closer together.
class TextFrame: public GlopFrame {
 public:
  TextFrame(const string &text, LightSetId font_outline_id, const Color &color,
            float font_height = gDefaultStyle->font_height, bool is_using_ellipsis = true,
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
  LightSetId GetFontOutlineId() const {return font_outline_id_;}
  void SetFontOutlineId(LightSetId font_outline_id);
  const Color &GetColor() const {return color_;}
  void SetColor(const Color &color) {color_ = color;}
  float GetFontHeight() const {return font_height_;}
  void SetFontHeight(float font_height) {
    if (font_height_ != font_height) {
      font_height_ = font_height;
      DirtySize();
    }
  }
  bool IsUsingEllipsis() const {return is_using_ellipsis_;}
  void SetIsUsingEllipsis(bool is_using_ellipsis) {is_using_ellipsis_ = is_using_ellipsis;}
  bool IsFullHeight() const {return is_full_height_;}
  void SetIsFullHeight(bool is_full_height) {
    if (is_full_height_ != is_full_height) {
      is_full_height_ = is_full_height;
      DirtySize();
    }
  }

  // Standard overloaded functionality
  virtual void Render();
 protected:
  virtual void RecomputeSize(int rec_width, int rec_height);

 private:
  string text_;
  LightSetId font_outline_id_, font_id_;
  Color color_;
  float font_height_;
  bool is_full_height_, is_using_ellipsis_;
  DISALLOW_EVIL_CONSTRUCTORS(TextFrame);
};

// FpsFrame
// ========
//
// This is a TextFrame that always gives the current frame rate.
class FpsFrame: public SingleParentFrame {
 public:
  FpsFrame(LightSetId font_outline_id, const Color &color,
           float font_height = gDefaultStyle->font_height, bool is_using_ellipsis = true,
           bool is_full_height = true)
  : SingleParentFrame(new TextFrame("", font_outline_id, color, font_height,
                              is_using_ellipsis, is_full_height)),
    text_((TextFrame*)GetChild()) {}

  // Text properties
  LightSetId GetFontOutlineId() const {return text_->GetFontOutlineId();}
  void SetFontOutlineId(LightSetId font_outline_id) {text_->SetFontOutlineId(font_outline_id);}
  const Color &GetColor() const {return text_->GetColor();}
  void SetColor(const Color &color) {text_->SetColor(color);}
  float GetFontHeight() const {return text_->GetFontHeight();}
  void SetFontHeight(float font_height) {text_->SetFontHeight(font_height);}
  bool IsUsingEllipsis() const {return text_->IsUsingEllipsis();}
  void SetIsUsingEllipsis(bool is_using_ellipsis) {text_->SetIsUsingEllipsis(is_using_ellipsis);}
  bool IsFullHeight() const {return text_->IsFullHeight();}
  void SetIsFullHeight(bool is_full_height) {text_->SetIsFullHeight(is_full_height);}

  // Logic
  virtual void Think(int dt);

 private:
  TextFrame *text_;
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
//    STag.size * base_font_height.
// All tags are implemented with ASCII value 1 delimiters.
class FancyTextFrame: public SingleParentFrame {
 public:
  // Constructors. horz_justify is used to align different rows of text/
  FancyTextFrame(const string &text, LightSetId font_outline_id,
                 const Color &start_color = gDefaultStyle->font_color,
                 float base_font_height = gDefaultStyle->font_height);
  FancyTextFrame(const string &text, LightSetId font_outline_id, bool add_soft_returns,
                 float horz_justify, const Color &start_color = gDefaultStyle->font_color,
                 float base_font_height = gDefaultStyle->font_height);

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
  float GetBaseFontHeight() const {return base_font_height_;}
  void SetBaseFontHeight(float base_font_height) {
    if (base_font_height_ != base_font_height) {
      base_font_height_ = base_font_height;
      DirtySize();
    }
  }
  float GetHorzJustify() const {return column_->GetDefaultHorzJustify();}
  void SetHorzJustify(float horz_justify) {column_->SetDefaultHorzJustify(horz_justify);}

 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  // Parsing utilities
  struct ParseStatus {
    int pos;
    Color color;
    float font_height;
    LightSetId font_id;
  };
  ParseStatus CreateParseStatus();
  void StartParsing(ParseStatus *status);
  void StopParsing(ParseStatus *status);
  bool ParseNextCharacter(const string &s, ParseStatus *status, char *ch);

  ColFrame *column_;
  string text_;
  LightSetId font_outline_id_;
  bool add_soft_returns_;
  Color start_color_;
  float base_font_height_;
  DISALLOW_EVIL_CONSTRUCTORS(FancyTextFrame);
};

// WindowFrame
// ===========
class WindowFrame: public SingleParentFrame {
 public:
  WindowFrame(GlopFrame *inner_frame, const string &title, LightSetId font_outline_id,
              const FrameStyle *style = gDefaultStyle);
  WindowFrame(GlopFrame *inner_frame, const FrameStyle *style = gDefaultStyle);

  virtual void Render();
 private:
  GlopFrame *title_bar_frame_;
  const FrameStyle *style_;
  DISALLOW_EVIL_CONSTRUCTORS(WindowFrame);
};

// ButtonFrame
// ===========
//
// This is a standard push button. It can be queried similar to the Input class. One can ask if
// it is currently down, if it would generate a down event similar to a key, or if it was just
// pressed and released. The button can be pressed using either mouse or keyboard.
class ButtonFrame: public PaddedFrame {
 public:
  // Constructors.
  //  frame: The frame to be rendered inside the main part of the button.
  //  is_selectable: If true, the button highlights itself when in focus, and can be pressed at this
  //                 point with enter (or key pad enter). This is the default, but it is sometimes
  //                 useful to disable for buttons that should not be possible to tab to
  //                 (e.g. scroll bar buttons).
  //  hot_key: A key that will cause the button to gain focus and be pressed. May be kAnyKey,
  //           although in that case, it will not act as a focus magnet.
  //  style: Look and feel - see GlopFrameBase.h.
  ButtonFrame(GlopFrame *frame, const FrameStyle *style = gDefaultStyle): PaddedFrame(frame, 0) {
    Init(true, kNoKey, kNoKey, style);
  }
  ButtonFrame(GlopFrame *frame, const GlopKey &hot_key, const FrameStyle *style = gDefaultStyle)
  : PaddedFrame(frame, 0) {
    Init(true, hot_key, kNoKey, style);
  }
  ButtonFrame(GlopFrame *frame, const GlopKey &hot_key1, const GlopKey &hot_key2,
              const FrameStyle *style = gDefaultStyle): PaddedFrame(frame, 0) {
    Init(true, hot_key1, hot_key2, style);
  }
  ButtonFrame(GlopFrame *frame, bool is_selectable, const FrameStyle *style = gDefaultStyle)
  : PaddedFrame(frame, 0) {
    Init(is_selectable, kNoKey, kNoKey, style);
  }
  ButtonFrame(GlopFrame *frame, bool is_selectable, const GlopKey &hot_key,
              const FrameStyle *style = gDefaultStyle): PaddedFrame(frame, 0) {
    Init(is_selectable, hot_key, kNoKey, style);
  }
  ButtonFrame(GlopFrame *frame, bool is_selectable, const GlopKey &hot_key1,
              const GlopKey &hot_key2, const FrameStyle *style = gDefaultStyle)
  : PaddedFrame(frame, 0) {
    Init(is_selectable, hot_key1, hot_key2, style);
  }

  // Button state
  bool IsDown() const {return is_down_;}
  bool WasHeldDown() const {return was_held_down_;}
  bool WasPressedFully() const {return was_pressed_fully_;}
  
  // Button modifiers
  LightSetId AddHotKey(const GlopKey &key) {return hot_keys_.InsertItem(key);}
  void RemoveHotKey(LightSetId id) {
    is_hot_key_down_ = false;  // In case the current down key was this hot key
    hot_keys_.RemoveItem(id);
  }

  // Frame overloads
  void Render();
  void OnKeyEvent(const KeyEvent &event, int dt);
  void Think(int dt);
  
  bool IsFocusMagnet(const KeyEvent &event) const {return hot_keys_.Find(event.key) != 0;}
	bool IsFocusKeeper(const KeyEvent &event) const {
    return IsFocusMagnet(event) ||
      (is_selectable_ && IsInFocus() && (event.key == 13 || event.key == kKeyPadEnter));
  }

 protected:
  void RecomputeSize(int rec_width, int rec_height) {
    RecomputePadding();
    PaddedFrame::RecomputeSize(rec_width, rec_height);
  }
  void SetIsInFocus(bool is_in_focus);

 private:
  void Init(bool is_selectable, const GlopKey &hot_key1, const GlopKey &hot_key2,
            const FrameStyle *style);
  void UpdateIsDown(int cx, int cy);
  void RecomputePadding();

  bool is_selectable_;           // See constructor
  LightSet<GlopKey> hot_keys_;   // "             "
  bool is_hot_key_down_,         // Is a button hot-key down, causing the button to be down?
       is_mouse_locked_on_;      // Was the button clicked on, and is the mouse button still down?
                                 //  Does not imply the ButtonFrame is down - the cursor could have
                                 //  moved away.
  bool full_press_queued_,       // The next value for was_pressed_fully_ - set in OnKeyEvent
       held_down_queued_;        // The next value for was_held_down_ - set in OnKeyEvent
  bool is_down_, was_held_down_, // Current button state - see accessors above
       was_pressed_fully_;       // "                                        "
  int held_down_repeat_delay_;   // The time in ms before was_held_down_ is set to true
  const FrameStyle *style_;
  DISALLOW_EVIL_CONSTRUCTORS(ButtonFrame);
};

#endif // GLOP_FRAME_WIDGETS_H__
