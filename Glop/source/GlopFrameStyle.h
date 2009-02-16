// GUI look customization. All Glop widgets render using a View class defined in this file. To
// customize the appearance of these objects, it suffices to overwrite the View class here. Note:
// we are not aiming for perfect flexibility here. A WindowFrame will always be a TextFrame on top
// of an internal frame. However, the window background and border can be customized as desired.
//
// By convention, a View contains three kinds of methods:
//   OnResize: Any method with OnResize in it will be guaranteed to be called whenever either the
//             frame or the window resizes. Generally OnResize is given the option of reserving
//             some space for the frame.
//   Render: If a frame has a View, it likely delegates to the View for ALL rendering. The View
//           is responsible for rendering the frame and all of its children, often including
//           explicit calls to Render on the child frames.
//   Other methods: These are generally used to construct child frames. For example, a WindowView
//                  can specify the GuiTextStyle used for the window text.
// A child View object will NOT be owned by any View.
//
// In addition to the View classes, we also include GuiTextStyle, which is a full font
// specification - a Font object, size, color, and flags (underline, italics, etc.)
//
// WARNING: Behavior is undefined if a View object is modified or deleted while a GlopFrame is
//          using that View.

#ifndef GLOP_GLOP_FRAME_STYLE_H__
#define GLOP_GLOP_FRAME_STYLE_H__

// Includes
#include "Base.h"
#include "Color.h"
#include "Input.h"
#include <vector>
using namespace std;

// Class declarations
class Font;
class GlopFrame;
class PaddedFrame;
class TextFrame;

// Style constants
const float kDefaultTextHeight(0.025f);
const Color kDefaultTextColor(kBlack);
const Color kDefaultTextPromptColor(0, 0, 0.5f);
const Color kDefaultTextPromptCursorColor(0, 0, 0.75f);
const Color kDefaultTextPromptHighlightColor(0.6f, 0.6f, 1.0f);

const Color kDefaultInputBoxBackgroundColor(1.0f, 1.0f, 1.0f);
const Color kDefaultInputBoxBorderColor(0.2f, 0.2f, 0.2f);

const Color kDefaultWindowBorderHighlightColor(0.9f, 0.9f, 0.95f);
const Color kDefaultWindowBorderLowlightColor(0.6f, 0.6f, 0.7f);
const Color kDefaultWindowInnerColor(0.8f, 0.8f, 0.8f);
const Color kDefaultWindowTitleColor(0, 0, 0);

const float kDefaultButtonBorderSize = 0.003f;
const Color kDefaultButtonSelectionColor(0, 0, 1.0f);
const Color kDefaultButtonBorderColor(0.2f, 0.2f, 0.2f);
const Color kDefaultButtonHighlightColor(1.0f, 1.0f, 1.0f);
const Color kDefaultButtonLowlightColor(0.5f, 0.5f, 0.5f);
const Color kDefaultButtonTextColor(0, 0, 0.25f);
const Color kDefaultButtonUnpressedInnerColor(0.9f, 0.9f, 0.9f);
const Color kDefaultButtonPressedInnerColor(0.75f, 0.75f, 0.77f);

const Color kDefaultArrowColor(0, 0, 0);

const float kDefaultSliderWidth = 0.03f;
const Color kDefaultSliderBackgroundColor(0.7f, 0.7f, 0.7f);
const Color kDefaultSliderBorderColor(0.2f, 0.2f, 0.2f);

const Color kDefaultMenuSelectionColor(0.55f, 0.55f, 0.9f);
const Color kDefaultMenuSelectionColorNoFocus(0.7f, 0.7f, 0.95f);
const Color kDefaultMenuTextPromptColor(0, 0, 0.5f);
const Color kDefaultMenuTextPromptCursorColor(0, 0, 0.75f);
const Color kDefaultMenuTextPromptHighlightColor(0.75f, 0.75f, 1.0f);

const float kDefaultDialogVertJustify = 0.4f;
const float kDefaultDialogRecWidth = 0.7f;
const float kDefaultDialogRecHeight = 0.6f;
const float kDefaultDialogTextHorzJustify = kJustifyLeft;
const float kDefaultDialogButtonsHorzJustify = kJustifyCenter;
const float kDefaultDialogLeftPadding = 0.02f;
const float kDefaultDialogTopPadding = 0.02f;
const float kDefaultDialogRightPadding = 0.02f;
const float kDefaultDialogBottomPadding = 0.02f;
const float kDefaultDialogInnerHorzPadding = 0.03f;
const float kDefaultDialogInnerVertPadding = 0.03f;

// GuiTextStyle
// ============

struct GuiTextStyle {
  // Every GuiTextStyle object requires a color, size, font and flags. The size is given as a
  // fraction of the window height. Flags are bitwise combinations of kFontBold, kFontItalics and
  // kFontUnderline (or kFontNormal).
  // When values are omitted, they are copied from gGuiTextStyle.
  GuiTextStyle();
  GuiTextStyle(const Color &_color);
  GuiTextStyle(const Color &_color, float _size);
  GuiTextStyle(const Color &_color, float _size, const Font *_font);
  GuiTextStyle(const Color &_color, float _size, const Font *_font, unsigned int _flags)
  : color(_color), size(_size), font(_font), flags(_flags) {}

  // Data
  Color color;
  float size;
  const Font *font;
  unsigned int flags;
};

// InputBoxView
// ============

class InputBoxView {
 public:
  static void DeleteAll();
  virtual ~InputBoxView() {}

  // Returns the padding reserved around the inner frame.
  virtual void OnResize(int rec_width, int rec_height,
                        int *lp, int *tp, int *rp, int *bp) const = 0;

  // Renders the input box. Note the frame already includes the padding given above.
  virtual void Render(int x1, int y1, int x2, int y2, const PaddedFrame *padded_frame) const = 0;

 protected:
  InputBoxView() {instances_.push_back(this);}
 private:
  static vector<InputBoxView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(InputBoxView);
};

class DefaultInputBoxView: public InputBoxView {
 public:
  DefaultInputBoxView()
  : background_color_(kDefaultInputBoxBackgroundColor),
    border_color_(kDefaultInputBoxBorderColor) {}

  // View methods
  void OnResize(int rec_width, int rec_height, int *lp, int *tp, int *rp, int *bp) const;
  void Render(int x1, int y1, int x2, int y2, const PaddedFrame *padded_frame) const;

  // Accessors and mutators
  const Color &GetBackgroundColor() const {return background_color_;}
  void SetBackgroundColor(const Color &c) {background_color_ = c;}
  const Color &GetBorderColor() const {return border_color_;}
  void SetBorderColor(const Color &c) {border_color_ = c;}
 private:
  Color background_color_, border_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultInputBoxView);
};

// TextPromptView
// ==============

class TextPromptView {
 public:
  static void DeleteAll();
  virtual ~TextPromptView() {}

  // Returns the GuiTextStyle that will be used for the text.
  virtual const GuiTextStyle &GetTextStyle() const = 0;

  // Returns the padding reserved around the prompt. The text_frame will already be resized, so
  // the TextRenderer there can be used for setting the padding.
  virtual void OnResize(int rec_width, int rec_height, const TextFrame *text_frame, int *lp,
                        int *tp, int *rp, int *bp) const = 0;

  // Renders the text prompt. cursor_pos and selection_start, selection_end are gaps between
  // characters and they range from 0 to length. cursor_time is the number of milliseconds since
  // this frame last gained focus. Render is free to change that value itself.
  virtual void Render(int x1, int y1, int x2, int y2, int cursor_pos, int *cursor_time,
                      int selection_start, int selection_end, bool is_in_focus,
                      const TextFrame *text_frame) const = 0;
 protected:
  TextPromptView() {instances_.push_back(this);}
 private:
  static vector<TextPromptView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(TextPromptView);
};

class DefaultTextPromptView: public TextPromptView {
 public:
  DefaultTextPromptView(Font *font)
  : highlight_color_(kDefaultTextPromptHighlightColor),
    cursor_color_(kDefaultTextPromptCursorColor),
    text_style_(kDefaultTextPromptColor, kDefaultTextHeight, font, 0) {}

  // View methods
  const GuiTextStyle &GetTextStyle() const {return text_style_;}
  void OnResize(int rec_width, int rec_height, const TextFrame *text_frame, int *lp, int *tp,
                int *rp, int *bp) const;
  void Render(int x1, int y1, int x2, int y2, int cursor_pos, int *cursor_time,
              int selection_start, int selection_end, bool is_in_focus,
              const TextFrame *text_frame) const;

  // Accessors and mutators
  const Color &GetHighlightColor() const {return highlight_color_;}
  void SetHighlightColor(const Color &c) {highlight_color_ = c;}
  void SetTextStyle(const GuiTextStyle &style) {text_style_ = style;}
  const Color &GetCursorColor() const {return cursor_color_;}
  void SetCursorColor(const Color &c) {cursor_color_ = c;}
 private:
  Color highlight_color_, cursor_color_;
  GuiTextStyle text_style_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultTextPromptView);
};

// WindowView
// ==========

class WindowView {
 public:
  static void DeleteAll();
  virtual ~WindowView() {}

  // Returns the GuiTextStyle that will be used for rendering the title.
  virtual const GuiTextStyle &GetTitleStyle() const = 0;

  // Returns the padding reserved around the title frame and around the inner frame.
  // If has_title is false, the title padding is ignored.
  virtual void OnResize(int rec_width, int rec_height, bool has_title,
    int *title_l, int *title_t, int *title_r, int *title_b,
    int *inner_l, int *inner_t, int *inner_r, int *inner_b) const = 0;

  // Renders the window. Note that: (1) padded_title_frame may be NULL, and (2) the frames both
  // include all padding given above.
  virtual void Render(int x1, int y1, int x2, int y2, const PaddedFrame *title_frame,
                      const PaddedFrame *inner_frame) const = 0;
 protected:
  WindowView() {instances_.push_back(this);}
 private:
  static vector<WindowView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(WindowView);
};

class DefaultWindowView: public WindowView {
 public:
  DefaultWindowView(Font *font)
  : border_highlight_color_(kDefaultWindowBorderHighlightColor),
    border_lowlight_color_(kDefaultWindowBorderLowlightColor),
    inner_color_(kDefaultWindowInnerColor),
    title_style_(kDefaultWindowTitleColor, kDefaultTextHeight, font, 0) {}

  // View methods
  const GuiTextStyle &GetTitleStyle() const {return title_style_;}
  void OnResize(int rec_width, int rec_height, bool has_title,
                int *title_l, int *title_t, int *title_r, int *title_b,
                int *inner_l, int *inner_t, int *inner_r, int *inner_b) const;
  void Render(int x1, int y1, int x2, int y2, const PaddedFrame *padded_title_frame,
              const PaddedFrame *padded_inner_frame) const;

  // Accessors and mutators
  const Color &GetBorderHighlightColor() const {return border_highlight_color_;}
  void SetBorderHighlightColor(const Color &c) {border_highlight_color_ = c;}
  const Color &GetBorderLowlightColor() const {return border_lowlight_color_;}
  void SetBorderLowlightColor(const Color &c) {border_lowlight_color_ = c;}
  const Color &GetInnerColor() const {return inner_color_;}
  void SetInnerColor(const Color &c) {inner_color_ = c;}
  void SetTitleStyle(const GuiTextStyle &style) {title_style_ = style;}
 private:
  Color border_highlight_color_, border_lowlight_color_, inner_color_;
  GuiTextStyle title_style_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultWindowView);
};

// ButtonView
// ==========

class ButtonView {
 public:
  static void DeleteAll();
  virtual ~ButtonView() {}

  // Returns the padding reserved around the inner frame.
  // Will be called when the button changes state, in addition to any time the button resizes.
  virtual void OnResize(int rec_width, int rec_height, bool is_down,
                        int *lp, int *tp, int *rp, int *bp) const = 0;

  // Renders the button.
  virtual void Render(int x1, int y1, int x2, int y2, bool is_down, bool is_primary_focus,
                      const PaddedFrame *padded_inner_frame) const = 0;
 protected:
  ButtonView() {instances_.push_back(this);}
 private:
  static vector<ButtonView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(ButtonView);
};

class DefaultButtonView: public ButtonView {
 public:
  DefaultButtonView()
  : border_size_(kDefaultButtonBorderSize), selection_color_(kDefaultButtonSelectionColor),
    border_color_(kDefaultButtonBorderColor), highlight_color_(kDefaultButtonHighlightColor),
    lowlight_color_(kDefaultButtonLowlightColor),
    unpressed_inner_color_(kDefaultButtonUnpressedInnerColor),
    pressed_inner_color_(kDefaultButtonPressedInnerColor) {}

  // View methods
  void OnResize(int rec_width, int rec_height, bool is_down,
                int *lp, int *tp, int *rp, int *bp) const;
  void Render(int x1, int y1, int x2, int y2, bool is_down, bool is_primary_focus,
              const PaddedFrame *padded_inner_frame) const ;

  // Accessors and mutators
  const float GetBorderSize() const {return border_size_;}
  void SetBorderSize(float border_size) {border_size_ = border_size;}
  const Color &GetSelectionColor() const {return selection_color_;}
  void SetSelectionColor(const Color &c) {selection_color_ = c;}
  const Color &GetBorderColor() const {return border_color_;}
  void SetBorderColor(const Color &c) {border_color_ = c;}
  const Color &GetHighlightColor() const {return highlight_color_;}
  void SetHighlightColor(const Color &c) {highlight_color_ = c;}
  const Color &GetLowlightColor() const {return lowlight_color_;}
  void SetLowlightColor(const Color &c) {lowlight_color_ = c;}
  const Color &GetUnpressedInnerColor() const {return unpressed_inner_color_;}
  void SetUnpressedInnerColor(const Color &c) {unpressed_inner_color_ = c;}
  const Color &GetPressedInnerColor() const {return pressed_inner_color_;}
  void SetPressedInnerColor(const Color &c) {pressed_inner_color_ = c;}
 private:
  float border_size_;
  Color selection_color_, border_color_, highlight_color_, lowlight_color_, unpressed_inner_color_,
        pressed_inner_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultButtonView);
};

// ArrowView
// =========

class ArrowView {
 public:
  static void DeleteAll();
  virtual ~ArrowView() {}

  // Returns the frame size, including any padding. Should also be called if the arrow direction
  // changes for any reason.
  enum Direction {Up, Right, Down, Left};
  virtual void OnResize(int rec_width, int rec_height, Direction direction,
                        int *width, int *height) const = 0;

  // Renders the arrow.
  virtual void Render(int x1, int y1, int x2, int y2, Direction direction) const = 0;
 protected: 
  ArrowView() {instances_.push_back(this);}
 private:
  static vector<ArrowView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(ArrowView);
};

class DefaultArrowView: public ArrowView {
 public:
  DefaultArrowView(): color_(kDefaultArrowColor) {}

  // View methods
  void OnResize(int rec_width, int rec_height, Direction direction, int *width, int *height) const;
  void Render(int x1, int y1, int x2, int y2, Direction direction) const;

  // Accessors and mutators
  const Color &GetColor() const {return color_;}
  void SetColor(const Color &c) {color_ = c;}
 private:
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultArrowView);
};

// SliderView
// ==========

class SliderView {
 public:
  static void DeleteAll();
  virtual ~SliderView() {}

  // Returns the view factories for the buttons at the edge of the slider, and for the arrows that
  // are to be displayed on those buttons.
  virtual const ArrowView *GetArrowView() const = 0;
  virtual const ButtonView *GetButtonView() const = 0;

  // These are both called any time the frame resizes. They return the desired "width" of the
  // slider ("width" is defined as the short dimension - so it is actually measuring y-distance
  // for horizontal sliders), and the minimum length of the tab.
  virtual int GetWidthOnResize(int rec_width, int rec_height, bool is_horizontal) const = 0;
  virtual int GetMinTabLengthOnResize(int inner_width, int inner_height,
                                      bool is_horizontal) const = 0;

  // Renders the slider. Tab coordinates are relative to the screen.
  virtual void Render(int x1, int y1, int x2, int y2, bool is_horizontal, bool is_primary_focus,
                      int tab_x1, int tab_y1, int tab_x2, int tab_y2, const GlopFrame *dec_button,
                      const GlopFrame *inc_button) const = 0;
 protected:
  SliderView() {instances_.push_back(this);}
 private:
  static vector<SliderView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderView);
};

class DefaultSliderView: public SliderView {
 public:
  DefaultSliderView(const ArrowView *arrow_view, const ButtonView *button_view)
  : arrow_view_(arrow_view), button_view_(button_view),
    width_(kDefaultSliderWidth), tab_border_size_(kDefaultButtonBorderSize),
    background_color_(kDefaultSliderBackgroundColor), border_color_(kDefaultSliderBorderColor),
    tab_border_color_(kDefaultButtonBorderColor),
    tab_highlight_color_(kDefaultButtonHighlightColor),
    tab_lowlight_color_(kDefaultButtonLowlightColor),
    tab_inner_color_(kDefaultButtonUnpressedInnerColor) {}

  // View methods
  const ArrowView *GetArrowView() const {return arrow_view_;}
  const ButtonView *GetButtonView() const {return button_view_;}
  int GetWidthOnResize(int rec_width, int rec_height, bool is_horizontal) const;
  int GetMinTabLengthOnResize(int inner_width, int inner_height, bool is_horizontal) const;
  void Render(int x1, int y1, int x2, int y2, bool is_horizontal, bool is_primary_focus,
              int tab_x1, int tab_y1, int tab_x2, int tab_y2, const GlopFrame *dec_button,
              const GlopFrame *inc_button) const;

  // Accessors and mutators
  void SetArrowView(const ArrowView *view) {arrow_view_ = view;}
  void SetButtonView(const ButtonView *view) {button_view_ = view;}
  float GetWidth() const {return width_;}
  void SetWidth(float width) {width_ = width;}
  float GetTabBorderSize() const {return tab_border_size_;}
  void SetTabBorderSize(float size) {tab_border_size_ = size;}
  const Color &GetBackgroundColor() const {return background_color_;}
  void SetBackgroundColor(const Color &c) {background_color_ = c;}
  const Color &GetBorderColor() const {return border_color_;}
  void SetBorderColor(const Color &c) {border_color_ = c;}
  const Color &GetTabBorderColor() const {return tab_border_color_;}
  void SetTabBorderColor(const Color &c) {tab_border_color_ = c;}
  const Color &GetTabHighlightColor() const {return tab_highlight_color_;}
  void SetTabHighlightColor(const Color &c) {tab_highlight_color_ = c;}
  const Color &GetTabLowlightColor() const {return tab_lowlight_color_;}
  void SetTabLowlightColor(const Color &c) {tab_lowlight_color_ = c;}
  const Color &GetTabInnerColor() const {return tab_inner_color_;}
  void SetTabInnerColor(const Color &c) {tab_inner_color_ = c;}
 private:
  const ArrowView *arrow_view_;
  const ButtonView *button_view_;
  float width_, tab_border_size_;
  Color background_color_, border_color_, tab_border_color_, tab_highlight_color_,
        tab_lowlight_color_, tab_inner_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultSliderView);
};

// MenuView
// ========

class MenuView {
 public:
  static void DeleteAll();
  virtual ~MenuView() {}

  // Returns the default style used for rendering any text appearing in the menu.
  virtual const GuiTextStyle &GetTextStyle() const = 0;

  // Returns the default style used for rendering any prompts appearing when an option is confirmed
  // in the menu.
  virtual const TextPromptView *GetTextPromptView() const = 0;

  // Returns the padding reserved around each menu item.
  virtual void OnResize(int rec_width, int rec_height, int *item_lp, int *item_tp, int *item_rp,
                        int *item_bp) const = 0;

  // Renders the menu. Selection coordinates are given relative to the screen. It is guaranteed
  // that items is non-null, and the selection is valid
  virtual void Render(int x1, int y1, int x2, int y2, int sel_x1, int sel_y1, int sel_x2,
                      int sel_y2, bool is_in_focus,
                      const List<GlopFrame *> &visible_items) const = 0;

 protected: 
  MenuView() {instances_.push_back(this);}
 private:
  static vector<MenuView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(MenuView);
};

class DefaultMenuView: public MenuView {
 public:
  DefaultMenuView(Font *font, const TextPromptView *text_prompt_view)
  : text_style_(kDefaultTextColor, kDefaultTextHeight, font, 0),
    text_prompt_view_(text_prompt_view),
    selection_color_(kDefaultMenuSelectionColor),
    selection_color_no_focus_(kDefaultMenuSelectionColorNoFocus) {}

  // View methods
  const GuiTextStyle &GetTextStyle() const {return text_style_;}
  const TextPromptView *GetTextPromptView() const {return text_prompt_view_;}
  void OnResize(int rec_width, int rec_height, int *item_lp, int *item_tp, int *item_rp,
                int *item_bp) const;
  void Render(int x1, int y1, int x2, int y2, int sel_x1, int sel_y1, int sel_x2, int sel_y2,
              bool is_in_focus, const List<GlopFrame *> &visible_items) const;

  // Accessors and mutators
  void SetTextStyle(const GuiTextStyle &style) {text_style_ = style;}
  void SetTextPromptView(const TextPromptView *view) {text_prompt_view_ = view;}
  const Color &GetSelectionColor() const {return selection_color_;}
  void SetSelectionColor(const Color &c) {selection_color_ = c;}
  const Color &GetSelectionColorNoFocus() const {return selection_color_no_focus_;}
  void SetSelectionColorNoFocus(const Color &c) {selection_color_no_focus_ = c;}
 private:
  GuiTextStyle text_style_;
  const TextPromptView *text_prompt_view_;
  Color selection_color_, selection_color_no_focus_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultMenuView);
};

// DialogView
// ==========
//
// Since there is no actual DialogFrame class similar to, say, ButtonFrame, this view is set up a
// little different from the other views.

class DialogView {
 public:
  static void DeleteAll();
  virtual ~DialogView() {}
  virtual const TextPromptView *GetTextPromptView() const = 0;
  virtual const InputBoxView *GetInputBoxView() const = 0;
  virtual const WindowView *GetWindowView() const = 0;
  virtual const ButtonView *GetButtonView() const = 0;
  virtual const SliderView *GetSliderView() const = 0;
  virtual const GuiTextStyle &GetTextStyle() const = 0;
  virtual const GuiTextStyle &GetButtonTextStyle() const = 0;
  virtual float GetVertJustify() const = 0;
  virtual float GetRecWidth() const = 0;
  virtual float GetRecHeight() const = 0;
  virtual float GetTextHorzJustify() const = 0;
  virtual float GetButtonsHorzJustify() const = 0;
  virtual void GetPadding(float *lp, float *tp, float *rp, float *bp) const = 0;
  virtual float GetInnerHorzPadding() const = 0;
  virtual float GetInnerVertPadding() const = 0;
 protected:
  DialogView() {instances_.push_back(this);}
 private:
  static vector<DialogView*> instances_;
  DISALLOW_EVIL_CONSTRUCTORS(DialogView);
};

class DefaultDialogView: public DialogView {
 public:
  DefaultDialogView(const InputBoxView *input_box_view,
                    const TextPromptView *text_prompt_view,
                    const WindowView *window_view,
                    const ButtonView *button_view,
                    const SliderView *slider_view, Font *font)
  : input_box_view_(input_box_view),
    button_view_(button_view), 
    slider_view_(slider_view),
    text_prompt_view_(text_prompt_view),
    window_view_(window_view),
    text_style_(kDefaultTextColor, kDefaultTextHeight, font, 0),
    button_text_style_(kDefaultButtonTextColor, kDefaultTextHeight, font, 0),
    vert_justify_(kDefaultDialogVertJustify), rec_width_(kDefaultDialogRecWidth),
    rec_height_(kDefaultDialogRecHeight), text_horz_justify_(kDefaultDialogTextHorzJustify),
    buttons_horz_justify_(kDefaultDialogButtonsHorzJustify),
    left_padding_(kDefaultDialogLeftPadding), top_padding_(kDefaultDialogTopPadding),
    right_padding_(kDefaultDialogRightPadding), bottom_padding_(kDefaultDialogBottomPadding),
    inner_horz_padding_(kDefaultDialogInnerHorzPadding),
    inner_vert_padding_(kDefaultDialogInnerVertPadding) {}

  // View methods
  const InputBoxView *GetInputBoxView() const {return input_box_view_;}
  const TextPromptView *GetTextPromptView() const {return text_prompt_view_;}
  const WindowView *GetWindowView() const {return window_view_;}
  const ButtonView *GetButtonView() const {return button_view_;}
  const SliderView *GetSliderView() const {return slider_view_;}
  const GuiTextStyle &GetTextStyle() const {return text_style_;}
  const GuiTextStyle &GetButtonTextStyle() const {return button_text_style_;}
  float GetVertJustify() const {return vert_justify_;}
  float GetRecWidth() const {return rec_width_;}
  float GetRecHeight() const {return rec_height_;}
  float GetTextHorzJustify() const {return text_horz_justify_;}
  float GetButtonsHorzJustify() const {return buttons_horz_justify_;}
  void GetPadding(float *lp, float *tp, float *rp, float *bp) const {
    *lp = left_padding_;
    *tp = top_padding_;
    *rp = right_padding_;
    *bp = bottom_padding_;
  }
  float GetInnerHorzPadding() const {return inner_horz_padding_;}
  float GetInnerVertPadding() const {return inner_vert_padding_;}

  // Accessors and mutators
  void SetInputBoxView(const InputBoxView *view) {input_box_view_ = view;}
  void SetTextPromptView(const TextPromptView *view) {text_prompt_view_ = view;}
  void SetWindowView(const WindowView *view) {window_view_ = view;}
  void SetButtonView(const ButtonView *view) {button_view_ = view;}
  void SetSliderView(const SliderView *view) {slider_view_ = view;}
  void SetTextStyle(const GuiTextStyle &style) {text_style_ = style;}
  void SetButtonTextStyle(const GuiTextStyle &style) {button_text_style_ = style;}
  void SetVertJustify(float justify) {vert_justify_ = justify;}
  void SetRecWidth(float rec_width) {rec_width_ = rec_width;}
  void SetRecHeight(float rec_height) {rec_height_ = rec_height;}
  void SetTextHorzJustify(float justify) {text_horz_justify_ = justify;}
  void SetButtonsHorzJustify(float justify) {buttons_horz_justify_ = justify;}
  void SetPadding (float lp, float tp, float rp, float bp) {
    left_padding_ = lp;
    top_padding_ = tp;
    right_padding_ = rp;
    bottom_padding_ = bp;
  }
  void SetInnerHorzPadding(float padding) {inner_horz_padding_ = padding;}
  void SetInnerVertPadding(float padding) {inner_vert_padding_ = padding;}
 private:
  const InputBoxView *input_box_view_;
  const ButtonView *button_view_;
  const SliderView *slider_view_;
  const TextPromptView *text_prompt_view_;
  const WindowView *window_view_;
  GuiTextStyle text_style_, button_text_style_;
  float vert_justify_, rec_width_, rec_height_, text_horz_justify_, buttons_horz_justify_,
        left_padding_, top_padding_, right_padding_, bottom_padding_, inner_horz_padding_,
        inner_vert_padding_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultDialogView);
};

// Global frame style
// ==================

extern GuiTextStyle gGuiTextStyle;
extern InputBoxView *gInputBoxView;
extern ArrowView *gArrowView;
extern TextPromptView *gTextPromptView;
extern WindowView *gWindowView;
extern ButtonView *gButtonView;
extern SliderView *gSliderView;
extern MenuView *gMenuView;
extern DialogView *gDialogView;

// Deletes all frame styles that have been created.
void ClearFrameStyle();

// Deletes any pre-existing global frame views, and replaces them with default values. This is
// called automatically at program start with font == 0. Views owned by global views are not
// deleted automatically unless they, too, are global views.
void InitDefaultFrameStyle(Font *font);

#endif // GLOP_GLOP_FRAME_STYLE_H__
