// GUI look customization. All Glop widgets render using a View class defined in this file. To
// customize the appearance of these objects, it suffices to overwrite the View class here. Note:
// we are not aiming for perfect flexibility here. A WindowFrame will always be a TextFrame on top
// of an internal frame. However, the window background and border can be customized as desired.
//
// Generically, a View is structured as follows:
//  - There is a ViewFactory class. This has one purpose - to instantiate Views.
//  - By convention, a View contains three kinds of methods:
//     OnResize: Any method with OnResize in it will be guaranteed to be called whenever either the
//               frame or the window resizes. Generally OnResize is given the option of reserving
//               some space for the frame.
//     Render: If a frame has a View, it likely delegates to the View for ALL rendering. The View
//             is responsible for rendering the frame and all of its children.
//     Other methods: These are generally used to construct child frames. For example, a WindowView
//                    can specify the GuiTextStyle used for the window text.
//  - Default___View and Default___ViewFactory classes are provided.
//
// In addition to the View classes, we also include GuiTextStyle, which is full font specification -
// a Font object, size, color, and flags (underline, italics, etc.)

#ifndef GLOP_GLOP_FRAME_STYLE_H__
#define GLOP_GLOP_FRAME_STYLE_H__

// Includes
#include "Base.h"
#include "Color.h"
#include "Input.h"

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
const Color kDefaultTextHighlightColor(0.6f, 0.6f, 1.0f);

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

const Color kDefaultMenuSelectionColor(0.6f, 0.6f, 1.0f);
const Color kDefaultMenuSelectionColorNoFocus(0.8f, 0.8f, 1.0f);

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
  GuiTextStyle(const Color &_color, float _size, Font *_font);
  GuiTextStyle(const Color &_color, float _size, Font *_font, unsigned int _flags)
  : color(_color), size(_size), font(_font), flags(_flags) {}

  // Data
  Color color;
  float size;
  Font *font;
  unsigned int flags;
};

// InputBoxView
// ============

class InputBoxView {
 public:
  virtual ~InputBoxView() {}

  // Returns the padding reserved around the inner frame.
  virtual void OnResize(int rec_width, int rec_height,
                        int *lp, int *tp, int *rp, int *bp) const = 0;

  // Renders the input box. Note the frame already includes the padding given above.
  virtual void Render(int x1, int y1, int x2, int y2, const PaddedFrame *padded_frame) const = 0;

 protected:
  InputBoxView() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(InputBoxView);
};
class InputBoxViewFactory {
 public:
  virtual InputBoxView *Create() const = 0;
  virtual ~InputBoxViewFactory() {}
 protected:
  InputBoxViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(InputBoxViewFactory);
};

// Default implementation
class DefaultInputBoxView: public InputBoxView {
 public:
  void OnResize(int rec_width, int rec_height, int *lp, int *tp, int *rp, int *bp) const;
  void Render(int x1, int y1, int x2, int y2, const PaddedFrame *padded_frame) const;

 private:
  friend class DefaultInputBoxViewFactory;
  DefaultInputBoxView(const DefaultInputBoxViewFactory *factory): factory_(factory) {}
  const DefaultInputBoxViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultInputBoxView);
};
class DefaultInputBoxViewFactory: public InputBoxViewFactory {
 public:
  DefaultInputBoxViewFactory()
  : background_color_(kDefaultInputBoxBackgroundColor),
    border_color_(kDefaultInputBoxBorderColor) {}
  InputBoxView *Create() const {return new DefaultInputBoxView(this);}

  // Accessors and mutators
  const Color &GetBackgroundColor() const {return background_color_;}
  void SetBackgroundColor(const Color &c) {background_color_ = c;}
  const Color &GetBorderColor() const {return border_color_;}
  void SetBorderColor(const Color &c) {border_color_ = c;}

 private:
  Color background_color_, border_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultInputBoxViewFactory);
};

// TextPromptView
// ==============

class TextPromptView {
 public:
  virtual ~TextPromptView() {}

  // Returns the GuiTextStyle that will be used for the text.
  virtual const GuiTextStyle GetTextStyle() const = 0;

  // Returns the padding reserved around the prompt. The text_frame will already be resized, so
  // the TextRenderer there can be used for setting the padding.
  virtual void OnResize(int rec_width, int rec_height, const TextFrame *text_frame, int *lp,
                        int *tp, int *rp, int *bp) const = 0;

  // Renders the text prompt. cursor_pos and selection_start, selection_end are gaps between
  // characters ranging from 0 to length. cursor_time is the number of milliseconds since this
  // frame last gained focus. Render is free to change that value itself.
  virtual void Render(int x1, int y1, int x2, int y2, int cursor_pos, int *cursor_time,
                      int selection_start, int selection_end, bool is_in_focus,
                      const TextFrame *text_frame) const = 0;
 protected:
  TextPromptView() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(TextPromptView);
};
class TextPromptViewFactory {
 public:
  virtual TextPromptView *Create() const = 0;
  virtual ~TextPromptViewFactory() {}
 protected:
  TextPromptViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(TextPromptViewFactory);
};

// Default implementation
class DefaultTextPromptView: public TextPromptView {
 public:
  const GuiTextStyle GetTextStyle() const ;
  void OnResize(int rec_width, int rec_height, const TextFrame *text_frame, int *lp, int *tp,
                int *rp, int *bp) const;
  void Render(int x1, int y1, int x2, int y2, int cursor_pos, int *cursor_time, int selection_start,
              int selection_end, bool is_in_focus, const TextFrame *text_frame) const;
 private:
  friend class DefaultTextPromptViewFactory;
  DefaultTextPromptView(const DefaultTextPromptViewFactory *factory): factory_(factory) {}
  const DefaultTextPromptViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultTextPromptView);
};
class DefaultTextPromptViewFactory: public TextPromptViewFactory {
 public:
  DefaultTextPromptViewFactory(Font *font)
  : highlight_color_(kDefaultTextHighlightColor), cursor_color_(kDefaultTextPromptCursorColor),
    text_style_(kDefaultTextPromptColor, kDefaultTextHeight, font, 0) {}
  TextPromptView *Create() const {return new DefaultTextPromptView(this);}

  // Accessors and mutators
  const Color &GetHighlightColor() const {return highlight_color_;}
  void SetHighlightColor(const Color &c) {highlight_color_ = c;}
  const GuiTextStyle &GetTextStyle() const {return text_style_;}
  void SetTextStyle(const GuiTextStyle &style) {text_style_ = style;}
  const Color &GetCursorColor() const {return cursor_color_;}
  void SetCursorColor(const Color &c) {cursor_color_ = c;}

 private:
  Color highlight_color_, cursor_color_;
  GuiTextStyle text_style_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultTextPromptViewFactory);
};

// WindowView
// ==========

class WindowView {
 public:
  virtual ~WindowView() {}

  // Returns the GuiTextStyle that will be used for rendering the title.
  virtual const GuiTextStyle GetTitleStyle() const = 0;

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
  WindowView() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(WindowView);
};
class WindowViewFactory {
 public:
  virtual WindowView *Create() const = 0;
  virtual ~WindowViewFactory() {}
 protected:
  WindowViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(WindowViewFactory);
};

// Default implementation
class DefaultWindowView: public WindowView {
 public:
  const GuiTextStyle GetTitleStyle() const;
  void OnResize(int rec_width, int rec_height, bool has_title,
                int *title_l, int *title_t, int *title_r, int *title_b,
                int *inner_l, int *inner_t, int *inner_r, int *inner_b) const;
  void Render(int x1, int y1, int x2, int y2, const PaddedFrame *padded_title_frame,
              const PaddedFrame *padded_inner_frame) const;
 private:
  friend class DefaultWindowViewFactory;
  DefaultWindowView(const DefaultWindowViewFactory *factory): factory_(factory) {}
  const DefaultWindowViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultWindowView);
};
class DefaultWindowViewFactory: public WindowViewFactory {
 public:
  DefaultWindowViewFactory(Font *font)
  : border_highlight_color_(kDefaultWindowBorderHighlightColor),
    border_lowlight_color_(kDefaultWindowBorderLowlightColor),
    inner_color_(kDefaultWindowInnerColor),
    title_style_(kDefaultWindowTitleColor, kDefaultTextHeight, font, 0) {}
  WindowView *Create() const {return new DefaultWindowView(this);}

  // Accessors and mutators
  const Color &GetBorderHighlightColor() const {return border_highlight_color_;}
  void SetBorderHighlightColor(const Color &c) {border_highlight_color_ = c;}
  const Color &GetBorderLowlightColor() const {return border_lowlight_color_;}
  void SetBorderLowlightColor(const Color &c) {border_lowlight_color_ = c;}
  const Color &GetInnerColor() const {return inner_color_;}
  void SetInnerColor(const Color &c) {inner_color_ = c;}
  const GuiTextStyle &GetTitleStyle() const {return title_style_;}
  void SetTitleStyle(const GuiTextStyle &style) {title_style_ = style;}
 private:
  Color border_highlight_color_, border_lowlight_color_, inner_color_;
  GuiTextStyle title_style_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultWindowViewFactory);
};

// ButtonView
// ==========

class ButtonView {
 public:
  virtual ~ButtonView() {}

  // Returns the padding reserved around the inner frame.
  // Will be called when the button changes state, in addition to any time the button resizes.
  virtual void OnResize(int rec_width, int rec_height, bool is_down,
                        int *lp, int *tp, int *rp, int *bp) const = 0;

  // Renders the button.
  virtual void Render(int x1, int y1, int x2, int y2, bool is_down, bool is_primary_focus,
                      const PaddedFrame *padded_inner_frame) const = 0;
 protected:
  ButtonView() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ButtonView);
};
class ButtonViewFactory {
 public:
  virtual ButtonView *Create() const = 0;
  virtual ~ButtonViewFactory() {}
 protected:
  ButtonViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ButtonViewFactory);
};

// Default implementation
class DefaultButtonView: public ButtonView {
 public:
  void OnResize(int rec_width, int rec_height, bool is_down,
                int *lp, int *tp, int *rp, int *bp) const;
  void Render(int x1, int y1, int x2, int y2, bool is_down, bool is_primary_focus,
              const PaddedFrame *padded_inner_frame) const ;
 private:
  friend class DefaultButtonViewFactory;
  DefaultButtonView(const DefaultButtonViewFactory *factory): factory_(factory) {}
  const DefaultButtonViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultButtonView);
};
class DefaultButtonViewFactory: public ButtonViewFactory {
 public:
  DefaultButtonViewFactory()
  : border_size_(kDefaultButtonBorderSize), selection_color_(kDefaultButtonSelectionColor),
    border_color_(kDefaultButtonBorderColor), highlight_color_(kDefaultButtonHighlightColor),
    lowlight_color_(kDefaultButtonLowlightColor),
    unpressed_inner_color_(kDefaultButtonUnpressedInnerColor),
    pressed_inner_color_(kDefaultButtonPressedInnerColor) {}
  ButtonView *Create() const {return new DefaultButtonView(this);}

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
  const Color &GetPressedInnercolor() const {return pressed_inner_color_;}
  void SetPressedInnerColor(const Color &c) {pressed_inner_color_ = c;}
 private:
  float border_size_;
  Color selection_color_, border_color_, highlight_color_, lowlight_color_, unpressed_inner_color_,
        pressed_inner_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultButtonViewFactory);
};

// ArrowView
// =========

class ArrowView {
 public:
  virtual ~ArrowView() {}

  // Returns the frame size, including any padding. Should also be called if the arrow direction
  // changes for any reason.
  enum Direction {Up, Right, Down, Left};  // Should match ArrowViewFrame
  virtual void OnResize(int rec_width, int rec_height, Direction direction,
                        int *width, int *height) const = 0;

  // Renders the arrow.
  virtual void Render(int x1, int y1, int x2, int y2, Direction direction) const = 0;
 protected: 
  ArrowView() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ArrowView);
};
class ArrowViewFactory {
 public:
  virtual ArrowView *Create() const = 0;
  virtual ~ArrowViewFactory() {}
 protected:
  ArrowViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ArrowViewFactory);
};

// Default implementation
class DefaultArrowView: public ArrowView {
 public:
  void OnResize(int rec_width, int rec_height, Direction direction, int *width, int *height) const;
  void Render(int x1, int y1, int x2, int y2, Direction direction) const;
 private:
  friend class DefaultArrowViewFactory;
  DefaultArrowView(const DefaultArrowViewFactory *factory): factory_(factory) {}
  const DefaultArrowViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultArrowView);
};
class DefaultArrowViewFactory: public ArrowViewFactory {
 public:
  DefaultArrowViewFactory(): color_(kDefaultArrowColor) {}
  ArrowView *Create() const {return new DefaultArrowView(this);}

  // Accessors and mutators
  const Color &GetColor() const {return color_;}
  void SetColor(const Color &c) {color_ = c;}
 private:
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultArrowViewFactory);
};

// SliderView
// ==========

class SliderView {
 public:
  virtual ~SliderView() {}

  // Returns the view factories for the buttons at the edge of the slider, and for the arrows that
  // are to be displayed on those buttons.
  virtual const ArrowViewFactory *GetArrowViewFactory() const = 0;
  virtual const ButtonViewFactory *GetButtonViewFactory() const = 0;

  // These are both called any time the frame resizes. They return the desired "width" of the slider
  // ("width" is defined as the short dimension - so it is actually measuring y-distance for
  // horizontal sliders), and the minimum length of the tab.
  virtual int GetWidthOnResize(int rec_width, int rec_height, bool is_horizontal) const = 0;
  virtual int GetMinTabLengthOnResize(int inner_width, int inner_height, bool is_horizontal) = 0;

  // Renders the slider. Tab coordinates are relative to the screen.
  virtual void Render(int x1, int y1, int x2, int y2, bool is_horizontal, bool is_primary_focus,
                      int tab_x1, int tab_y1, int tab_x2, int tab_y2, const GlopFrame *dec_button,
                      const GlopFrame *inc_button) const = 0;
 protected:
  SliderView() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(SliderView);
};
class SliderViewFactory {
 public:
  virtual SliderView *Create() const = 0;
  virtual ~SliderViewFactory() {}
 protected:
  SliderViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(SliderViewFactory);
};

// Default implementation
class DefaultSliderView: public SliderView {
 public:
  const ArrowViewFactory *GetArrowViewFactory() const;
  const ButtonViewFactory *GetButtonViewFactory() const;
  int GetWidthOnResize(int rec_width, int rec_height, bool is_horizontal) const;
  int GetMinTabLengthOnResize(int inner_width, int inner_height, bool is_horizontal);
  void Render(int x1, int y1, int x2, int y2, bool is_horizontal, bool is_primary_focus,
              int tab_x1, int tab_y1, int tab_x2, int tab_y2, const GlopFrame *dec_button,
              const GlopFrame *inc_button) const;
 private:
  friend class DefaultSliderViewFactory;
  DefaultSliderView(const DefaultSliderViewFactory *factory): factory_(factory) {}
  const DefaultSliderViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultSliderView);
};
class DefaultSliderViewFactory: public SliderViewFactory {
 public:
  DefaultSliderViewFactory(const ArrowViewFactory *arrow_factory,
                           const ButtonViewFactory *button_factory)
  : button_factory_(button_factory), arrow_factory_(arrow_factory),
    width_(kDefaultSliderWidth), tab_border_size_(kDefaultButtonBorderSize),
    background_color_(kDefaultSliderBackgroundColor), border_color_(kDefaultSliderBorderColor),
    tab_border_color_(kDefaultButtonBorderColor),
    tab_highlight_color_(kDefaultButtonHighlightColor),
    tab_lowlight_color_(kDefaultButtonLowlightColor),
    tab_inner_color_(kDefaultButtonUnpressedInnerColor) {}
  SliderView *Create() const {return new DefaultSliderView(this);}

  // Accessors and mutators
  const ArrowViewFactory *GetArrowViewFactory() const {return arrow_factory_;}
  void SetArrowViewFactory(const ArrowViewFactory *factory) {arrow_factory_ = factory;}
  const ButtonViewFactory *GetButtonViewFactory() const {return button_factory_;}
  void SetButtonViewFactory(const ButtonViewFactory *factory) {button_factory_ = factory;}
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
  const ArrowViewFactory *arrow_factory_;
  const ButtonViewFactory *button_factory_;
  float width_, tab_border_size_;
  Color background_color_, border_color_, tab_border_color_, tab_highlight_color_,
        tab_lowlight_color_, tab_inner_color_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultSliderViewFactory);
};

// MenuView
// ========

class MenuView {
 public:
  virtual ~MenuView() {}

  // Returns the padding reserved around each menu item.
  virtual void OnResize(int rec_width, int rec_height, int *item_lp, int *item_tp, int *item_rp,
                        int *item_bp) const = 0;

  // Renders the menu. Selection coordinates are given relative to the screen. It is guaranteed
  // that items is non-null, and the selection is valid.
  virtual void Render(int x1, int y1, int x2, int y2, int sel_x1, int sel_y1, int sel_x2,
                      int sel_y2, bool is_in_focus, const GlopFrame *items) const = 0;
 protected: 
  MenuView() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MenuView);
};
class MenuViewFactory {
 public:
  virtual MenuView *Create() const = 0;
  virtual ~MenuViewFactory() {}
 protected:
  MenuViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MenuViewFactory);
};

// Default implementation
class DefaultMenuView: public MenuView {
 public:
  void OnResize(int rec_width, int rec_height, int *item_lp, int *item_tp, int *item_rp,
                int *item_bp) const;
  void Render(int x1, int y1, int x2, int y2, int sel_x1, int sel_y1, int sel_x2, int sel_y2,
              bool is_in_focus, const GlopFrame *items) const;
 private:
  friend class DefaultMenuViewFactory;
  DefaultMenuView(const DefaultMenuViewFactory *factory): factory_(factory) {}
  const DefaultMenuViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultMenuView);
};
class DefaultMenuViewFactory: public MenuViewFactory {
 public:
  DefaultMenuViewFactory()
  : selection_color_(kDefaultMenuSelectionColor),
    selection_color_no_focus_(kDefaultMenuSelectionColorNoFocus) {}
  MenuView *Create() const {return new DefaultMenuView(this);}

  // Accessors and mutators
  const Color &GetSelectionColor() const {return selection_color_;}
  void SetSelectionColor(const Color &c) {selection_color_ = c;}
  const Color &GetSelectionColorNoFocus() const {return selection_color_no_focus_;}
  void SetSelectionColorNoFocus(const Color &c) {selection_color_no_focus_ = c;}
 private:
  Color selection_color_, selection_color_no_focus_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultMenuViewFactory);
};

// DialogView
// ==========
//
// Note that there is no DialogView, just a DialogViewFactory. Thus, it is set up differently from
// some of the other classes. The difference results from the fact that there is not actually such
// a thing as a DialogFrame - it is just a combination of other objects.
class DialogViewFactory {
 public:
  virtual ~DialogViewFactory() {}
  virtual const TextPromptViewFactory *GetTextPromptViewFactory() const = 0;
  virtual const InputBoxViewFactory *GetInputBoxViewFactory() const = 0;
  virtual const WindowViewFactory *GetWindowViewFactory() const = 0;
  virtual const ButtonViewFactory *GetButtonViewFactory() const = 0;
  virtual const SliderViewFactory *GetSliderViewFactory() const = 0;
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
  DialogViewFactory() {}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(DialogViewFactory);
};

// Default implementation
class DefaultDialogViewFactory: public DialogViewFactory {
 public:
  DefaultDialogViewFactory(const InputBoxViewFactory *input_box_view_factory,
                           const TextPromptViewFactory *text_prompt_view_factory,
                           const WindowViewFactory *window_view_factory,
                           const ButtonViewFactory *button_view_factory,
                           const SliderViewFactory *slider_view_factory, Font *font)
  : input_box_view_factory_(input_box_view_factory),
    text_prompt_view_factory_(text_prompt_view_factory), window_view_factory_(window_view_factory),
    button_view_factory_(button_view_factory), slider_view_factory_(slider_view_factory),   
    text_style_(kDefaultTextColor, kDefaultTextHeight, font, 0),
    button_text_style_(kDefaultButtonTextColor, kDefaultTextHeight, font, 0),
    vert_justify_(kDefaultDialogVertJustify), rec_width_(kDefaultDialogRecWidth),
    rec_height_(kDefaultDialogRecHeight), text_horz_justify_(kDefaultDialogTextHorzJustify),
    buttons_horz_justify_(kDefaultDialogButtonsHorzJustify),
    left_padding_(kDefaultDialogLeftPadding), top_padding_(kDefaultDialogTopPadding),
    right_padding_(kDefaultDialogRightPadding), bottom_padding_(kDefaultDialogBottomPadding),
    inner_horz_padding_(kDefaultDialogInnerHorzPadding),
    inner_vert_padding_(kDefaultDialogInnerVertPadding) {}

  // Accessors and mutators
  const InputBoxViewFactory *GetInputBoxViewFactory() const {return input_box_view_factory_;}
  void SetInputBoxViewFactory(const InputBoxViewFactory *f) {input_box_view_factory_ = f;}
  const TextPromptViewFactory *GetTextPromptViewFactory() const {return text_prompt_view_factory_;}
  void SetTextPromptViewFactory(const TextPromptViewFactory *f) {text_prompt_view_factory_ = f;}
  const WindowViewFactory *GetWindowViewFactory() const {return window_view_factory_;}
  void SetWindowViewFactory(const WindowViewFactory *f) {window_view_factory_ = f;}
  const ButtonViewFactory *GetButtonViewFactory() const {return button_view_factory_;}
  void SetButtonViewFactory(const ButtonViewFactory *f) {button_view_factory_ = f;}
  const SliderViewFactory *GetSliderViewFactory() const {return slider_view_factory_;}
  void SetSliderViewFactory(const SliderViewFactory *f) {slider_view_factory_ = f;}

  const GuiTextStyle &GetTextStyle() const {return text_style_;}
  void SetTextStyle(const GuiTextStyle &style) {text_style_ = style;}
  const GuiTextStyle &GetButtonTextStyle() const {return button_text_style_;}
  void SetButtonTextStyle(const GuiTextStyle &style) {button_text_style_ = style;}

  float GetVertJustify() const {return vert_justify_;}
  void SetVertJustify(float justify) {vert_justify_ = justify;}
  float GetRecWidth() const {return rec_width_;}
  void SetRecWidth(float rec_width) {rec_width_ = rec_width;}
  float GetRecHeight() const {return rec_height_;}
  void SetRecHeight(float rec_height) {rec_height_ = rec_height;}
  float GetTextHorzJustify() const {return text_horz_justify_;}
  void SetTextHorzJustify(float justify) {text_horz_justify_ = justify;}
  float GetButtonsHorzJustify() const {return buttons_horz_justify_;}
  void SetButtonsHorzJustify(float justify) {buttons_horz_justify_ = justify;}
  void GetPadding(float *lp, float *tp, float *rp, float *bp) const {
    *lp = left_padding_;
    *tp = top_padding_;
    *rp = right_padding_;
    *bp = bottom_padding_;
  }
  void SetPadding (float lp, float tp, float rp, float bp) {
    left_padding_ = lp;
    top_padding_ = tp;
    right_padding_ = rp;
    bottom_padding_ = bp;
  }
  float GetInnerHorzPadding() const {return inner_horz_padding_;}
  void SetInnerHorzPadding(float padding) {inner_horz_padding_ = padding;}
  float GetInnerVertPadding() const {return inner_vert_padding_;}
  void SetInnerVertPadding(float padding) {inner_vert_padding_ = padding;}

 private:
  const InputBoxViewFactory *input_box_view_factory_;
  const ButtonViewFactory *button_view_factory_;
  const SliderViewFactory *slider_view_factory_;
  const TextPromptViewFactory *text_prompt_view_factory_;
  const WindowViewFactory *window_view_factory_;
  GuiTextStyle text_style_, button_text_style_;
  float vert_justify_, rec_width_, rec_height_, text_horz_justify_, buttons_horz_justify_,
        left_padding_, top_padding_, right_padding_, bottom_padding_, inner_horz_padding_,
        inner_vert_padding_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultDialogViewFactory);
};

// Global frame style
// ==================

extern GuiTextStyle *gGuiTextStyle;
extern InputBoxViewFactory *gInputBoxViewFactory;
extern ArrowViewFactory *gArrowViewFactory;
extern TextPromptViewFactory *gTextPromptViewFactory;
extern WindowViewFactory *gWindowViewFactory;
extern ButtonViewFactory *gButtonViewFactory;
extern SliderViewFactory *gSliderViewFactory;
extern MenuViewFactory *gMenuViewFactory;
extern DialogViewFactory *gDialogViewFactory;

// Deletes all global frame styles that is initialized.
void ClearFrameStyle();

// Deletes any pre-existing global frame styles, and replaces them with default values. This is
// called automatically at program start with font == 0.
void InitDefaultFrameStyle(Font *font);

#endif // GLOP_GLOP_FRAME_STYLE_H__
