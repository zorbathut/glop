// GUI look customization. All Glop widgets render using a View class defined in this file. To
// customize the appearance of these objects, it suffices to overwrite the View class here. Note:
// we are not aiming for perfect flexibility here. A WindowFrame will always be a TextFrame on top
// of an internal frame. However, the window background and border can be customized as desired.
//
// Generically, a View is structured as follows:
//  - There is a ViewFactory class. This has one purpose - to instantiate Views.
//  - A View contains three kinds of methods:
//     OnResize: Any method with OnResize in it will be guaranteed to be called whenever either the
//               frame or the window resizes. Generally OnResize is given the option of reserving
//               some space for the frame.
//     Render: If a frame has a View, it likely delegates to the View for ALL rendering. The View
//             is responsible for rendering the frame and all of its children.
//     Other methods: These are generally used to construct child frames. For example, a WindowView
//                    can specify the TextStyle used for the window text.
//  - Default___View and Default___ViewFactory classes are provided.
//
// In addition to the View classes, we all include TextStyle, which is full font information - a
// Font object, size, color, and flags (underline, italics, etc.)

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

const Color kDefaultArrowColor(0, 0, 0);

const float kDefaultSliderWidth = 0.03f;
const Color kDefaultSliderBackgroundColor(0.7f, 0.7f, 0.7f);
const Color kDefaultSliderBorderColor(0.2f, 0.2f, 0.2f);

// TextStyle
// =========
struct TextStyle {
  // Every TextStyle object requires a color, size, font and flags. The size is given as a fraction
  // of the window height. The flags are specified as in Font.h.
  // When types are omitted, they are copied from the TextStyle in gDefaultStyle.
  TextStyle();
  TextStyle(const Color &_color);
  TextStyle(const Color &_color, float _size);
  TextStyle(const Color &_color, float _size, Font *_font);
  TextStyle(const Color &_color, float _size, Font *_font, unsigned int _flags)
  : color(_color), size(_size), font(_font), flags(_flags) {}

  // Data
  Color color;
  float size;
  Font *font;
  unsigned int flags;
};

// WindowView
// ==========
class WindowView {
 public:
  virtual ~WindowView() {}

  // Returns the TextStyle that will be used for rendering the title.
  virtual const TextStyle GetTitleStyle() const = 0;

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
  virtual const TextStyle GetTitleStyle() const;
  virtual void OnResize(int rec_width, int rec_height, bool has_title,
    int *title_l, int *title_t, int *title_r, int *title_b,
    int *inner_l, int *inner_t, int *inner_r, int *inner_b) const;
  virtual void Render(int x1, int y1, int x2, int y2, const PaddedFrame *padded_title_frame,
                      const PaddedFrame *padded_inner_frame) const;
 private:
  friend class DefaultWindowViewFactory;
  DefaultWindowView(const DefaultWindowViewFactory *factory): factory_(factory) {}
  const DefaultWindowViewFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(DefaultWindowView);
};
class DefaultWindowViewFactory: public WindowViewFactory {
 public:
  // Main operations
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
  const TextStyle &GetTitleStyle() const {return title_style_;}
  void SetTitleStyle(const TextStyle &style) {title_style_ = style;}
 private:
  Color border_highlight_color_, border_lowlight_color_, inner_color_;
  TextStyle title_style_;
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
  // Main operations
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
  // Main operations
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

  // These are both calle any time the frame resizes. They return the desired "width" of the slider
  // ("width" is defined as the short dimension - so it is actually measuring y-distance for
  // horizontal sliders), and the minimum length of the tab.
  virtual int GetWidthOnResize(int rec_width, int rec_height, bool is_horizontal) const = 0;
  virtual int GetMinTabLengthOnResize(int inner_width, int inner_height, bool is_horizontal) = 0;

  // Renders the slider. tab coordinates are relative to the screen.
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
  // Main operations
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

// This specifies various constants that are used for constructing GUI frames (e.g. font color &
// size). A new FrameStyle can be created to override existing settings. If a FrameStyle is already
// in use, changing it may or may not affect existing frames. The results are undefined.
struct FrameStyle {
  FrameStyle(Font *font);
  ~FrameStyle();

  // General style
  TextStyle text_style;
  Color prompt_highlight_color;       // Background color of text we highlight in a TextPrompt

  // Renderers
  ArrowViewFactory *arrow_view_factory;
  ButtonViewFactory *button_view_factory;
  SliderViewFactory *slider_view_factory;
  WindowViewFactory *window_view_factory;
};
extern FrameStyle *gFrameStyle;

#endif // GLOP_GLOP_FRAME_STYLE_H__
