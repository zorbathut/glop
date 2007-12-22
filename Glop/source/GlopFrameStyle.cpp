// Includes
#include "../include/GlopFrameBase.h"
#include "../include/GlopFrameStyle.h"
#include "../include/GlopFrameWidgets.h"
#include "../include/GlopWindow.h"
#include "../include/OpenGl.h"

// FrameStyle
// ==========

TextStyle::TextStyle()
: color(gFrameStyle->text_style.color), size(gFrameStyle->text_style.size),
  font(gFrameStyle->text_style.font), flags(gFrameStyle->text_style.flags) {}
TextStyle::TextStyle(const Color &_color)
: color(_color), size(gFrameStyle->text_style.size), font(gFrameStyle->text_style.font),
  flags(gFrameStyle->text_style.flags) {}
TextStyle::TextStyle(const Color &_color, float _size)
: color(_color), size(_size), font(gFrameStyle->text_style.font),
  flags(gFrameStyle->text_style.flags) {}
TextStyle::TextStyle(const Color &_color, float _size, Font *_font)
: color(_color), size(_size), font(_font), flags(gFrameStyle->text_style.flags) {}

FrameStyle::FrameStyle(Font *font)
: text_style(kBlack, 0.025f, font, 0),
  prompt_highlight_color(0.6f, 0.6f, 1.0f) {
  arrow_view_factory = new DefaultArrowViewFactory();
  button_view_factory = new DefaultButtonViewFactory();
  slider_view_factory = new DefaultSliderViewFactory(arrow_view_factory, button_view_factory);
  window_view_factory = new DefaultWindowViewFactory(font);
}

FrameStyle::~FrameStyle() {
  delete arrow_view_factory;
  delete button_view_factory;
  delete slider_view_factory;
  delete window_view_factory;
}

FrameStyle *gFrameStyle = 0;

// WindowView
// ==========

const TextStyle DefaultWindowView::GetTitleStyle() const {
  return factory_->GetTitleStyle();
}
void DefaultWindowView::OnResize(int rec_width, int rec_height, bool has_title,
                                 int *title_l, int *title_t, int *title_r, int *title_b,
                                 int *inner_l, int *inner_t, int *inner_r, int *inner_b) const {
  *title_l = *title_t = 2;
  *title_r = *title_b = 0;
  *inner_l = *inner_t = *inner_r = *inner_b = 3;
}
void DefaultWindowView::Render(int x1, int y1, int x2, int y2,
                               const PaddedFrame *padded_title_frame,
                               const PaddedFrame *padded_inner_frame) const {
  int title_height;
  if (padded_title_frame == 0)
    title_height = 0;
  else
    title_height = padded_title_frame->GetHeight();
  int ym = y1 + title_height - 1;

  // Draw the fancy title bar
  if (padded_title_frame != 0) {
    glBegin(GL_QUADS);
    GlUtils::SetColor(factory_->GetBorderHighlightColor());
    glVertex2i(x1 + 1, y1 + 1);
    glVertex2i(x2, y1 + 1);
    GlUtils::SetColor(factory_->GetBorderLowlightColor());
    glVertex2i(x2, y1 + title_height/4);
    glVertex2i(x1 + 1, y1 + title_height/4);
    glVertex2i(x1 + 1, y1 + title_height/4);
    glVertex2i(x2, y1 + title_height/4);
    GlUtils::SetColor(factory_->GetBorderHighlightColor());
    glVertex2i(x2, y1 + title_height + 1);
    glVertex2i(x1, y1 + title_height + 1);
    glEnd();
  }

  // Draw the window border
  GlUtils2d::DrawRectangle(x1, y1, x2, y2, factory_->GetBorderLowlightColor());
  GlUtils2d::DrawRectangle(x1 + 1, ym + 1, x2 - 1, y2 - 1, factory_->GetBorderHighlightColor());
  GlUtils2d::DrawRectangle(x1 + 2, ym + 2, x2 - 2, y2 - 2, factory_->GetBorderLowlightColor());

  // Draw the window interior    
  GlUtils2d::FillRectangle(x1 + 3, ym + 3, x2 - 3, y2 - 3, factory_->GetInnerColor());

  // Delegate to the inner frames
  GlUtils::SetColor(kWhite);
  if (padded_title_frame != 0)
    padded_title_frame->Render();
  padded_inner_frame->Render();
}

// ButtonView
// ==========

void DefaultButtonView::OnResize(int rec_width, int rec_height, bool is_down,
                                 int *lp, int *tp, int *rp, int *bp) const {
  float border_size = factory_->GetBorderSize();
  int padding = 2 + int(min(gWindow->GetWidth(), gWindow->GetHeight()) * border_size);
  int offset = (is_down? 1 : 0);
  *lp = *tp = padding + offset - 1;
  *rp = *bp = padding - offset;
}

void DefaultButtonView::Render(int x1, int y1, int x2, int y2, bool is_down, bool is_primary_focus,
                               const PaddedFrame *padded_inner_frame) const {
  int lpadding = padded_inner_frame->GetLeftPadding(),
      rpadding = padded_inner_frame->GetRightPadding();;

  // Handle up buttons
  if (!is_down) {
    // Draw the border
    GlUtils2d::DrawRectangle(x1, y1, x2, y2, factory_->GetBorderColor());

    // Draw the highlight
    GlUtils2d::FillRectangle(x1 + 1, y1 + 1, x2 - 1, y2 - 1, factory_->GetHighlightColor());

    // Draw the lowlight
    GlUtils2d::FillRectangle(x1 + lpadding, y2 - rpadding + 1, x2 - 1, y2 - 1,
                             factory_->GetLowlightColor());
    glBegin(GL_TRIANGLES);
    glVertex2i(x1 + 1, y2);
    glVertex2i(x1 + lpadding, y2 - rpadding + 1);
    glVertex2i(x1 + lpadding, y2);
    glEnd();
    GlUtils2d::FillRectangle(x2 - rpadding + 1, y1 + lpadding, x2 - 1, y2 - 1);
    glBegin(GL_TRIANGLES);
    glVertex2i(x2 - rpadding + 1, y1 + lpadding);
    glVertex2i(x2, y1 + 1);
    glVertex2i(x2, y1 + lpadding);
    glEnd();

    // Draw the button interior
    GlUtils2d::FillRectangle(x1 + lpadding, y1 + lpadding, x2 - rpadding, y2 - rpadding,
                             factory_->GetUnpressedInnerColor());
  } else {
    // Draw a pressed button
    GlUtils2d::DrawRectangle(x1, y1, x2, y2, factory_->GetBorderColor());
    GlUtils2d::FillRectangle(x1 + 1, y1 + 1, x2 - 1, y2 - 1, factory_->GetLowlightColor());
    GlUtils2d::FillRectangle(x1 + lpadding, y1 + lpadding, x2 - rpadding, y2 - rpadding,
                             factory_->GetPressedInnercolor());
  }

  // Draw the inner frame
  GlUtils::SetColor(kWhite);
  padded_inner_frame->Render();

  // Draw the focus display
  if (is_primary_focus) {
    GlUtils2d::DrawRectangle(x1, y1, x2, y2, factory_->GetSelectionColor());
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x5555);
    GlUtils2d::DrawRectangle(x1 + lpadding - 1, y1 + lpadding - 1, x2 - rpadding + 1,
                            y2 - rpadding + 1);
    glLineStipple(1, 0xffff);
    glDisable(GL_LINE_STIPPLE);
    GlUtils::SetColor(kWhite);
  }
}

// ArrowView
// =========

void DefaultArrowView::OnResize(int rec_width, int rec_height, Direction direction,
                                int *width, int *height) const {
  *width = *height = min(rec_width, rec_height);
}

void DefaultArrowView::Render(int x1, int y1, int x2, int y2, Direction direction) const {
  int x = 1 + x1 + (x2-x1)/2, y = 1 + y1 + (y2-y1)/2;
  int d = int((x2-x1+1) * 0.35f + 0.5f);

  GlUtils::SetColor(factory_->GetColor());
  glBegin(GL_TRIANGLES);
  switch(direction) {
    case Up:
      glVertex2i(x + d + 1, y + d);
      glVertex2i(x - d - 1,  y + d);
      glVertex2i(x, y - d - 2);
      break;
    case Right:
      glVertex2i(x - d - 1, y + d + 1);
      glVertex2i(x - d - 1, y - d - 1);           
      glVertex2i(x + d + 1, y);
      break;
    case Down:
      glVertex2i(x - d - 1, y - d - 1);
      glVertex2i(x + d + 1, y - d - 1);
      glVertex2i(x, y + d + 1);
      break;
    case Left:
      glVertex2i(x + d, y - d - 1);
      glVertex2i(x + d, y + d + 1);
      glVertex2i(x - d - 2, y);
      break;
  }
  glEnd();
  GlUtils::SetColor(kWhite);
}

// SliderView
// ==========

const ArrowViewFactory *DefaultSliderView::GetArrowViewFactory() const {
  return factory_->GetArrowViewFactory();
}

const ButtonViewFactory *DefaultSliderView::GetButtonViewFactory() const {
  return factory_->GetButtonViewFactory();
}

int DefaultSliderView::GetWidthOnResize(int rec_width, int rec_height, bool is_horizontal) const {
  return max(int(min(gWindow->GetWidth(), gWindow->GetHeight()) * factory_->GetWidth()), 2);
}

int DefaultSliderView::GetMinTabLengthOnResize(int inner_width, int inner_height,
                                               bool is_horizontal) {
  return min(6, is_horizontal? inner_width: inner_height);
}

void DefaultSliderView::Render(int x1, int y1, int x2, int y2, bool is_horizontal,
                               bool is_primary_focus, int tab_x1, int tab_y1, int tab_x2,
                               int tab_y2, const GlopFrame *dec_button,
                               const GlopFrame *inc_button) const {
  // Draw the buttons
  dec_button->Render();
  inc_button->Render();

  // Elongate the tab a little - this means the border will overlap with the button border
  if (is_horizontal) {
    x1 += dec_button->GetWidth();
    x2 -= inc_button->GetWidth();
    tab_x1--;
    tab_x2++;
  } else {
    y1 += dec_button->GetHeight();
    y2 -= inc_button->GetHeight();
    tab_y1--;
    tab_y2++;
  }

  // Draw the background
  GlUtils2d::FillRectangle(x1, y1, x2, y2, factory_->GetBackgroundColor());
  GlUtils::SetColor(kWhite);

  // Draw the tab - we render it the same way a DefaultButtonStyle renders an unpressed button.
  float tab_border = factory_->GetTabBorderSize();
  int tab_padding = 2 + int(min(gWindow->GetWidth(), gWindow->GetHeight()) * tab_border);
  tab_padding = min(tab_padding, min(tab_x2 - tab_x1 - 2, tab_y2 - tab_y1 - 2)/2);
  GlUtils2d::DrawRectangle(tab_x1, tab_y1, tab_x2, tab_y2, factory_->GetTabBorderColor());
  GlUtils2d::FillRectangle(tab_x1 + 1, tab_y1 + 1, tab_x2 - 1, tab_y2 - 1,
                           factory_->GetTabHighlightColor());
  GlUtils2d::FillRectangle(tab_x1 + tab_padding, tab_y2 - tab_padding + 1, tab_x2 - 1, tab_y2 - 1,
                           factory_->GetTabLowlightColor());
  glBegin(GL_TRIANGLES);
  glVertex2i(tab_x1 + 1, tab_y2);
  glVertex2i(tab_x1 + tab_padding, tab_y2 - tab_padding + 1);
  glVertex2i(tab_x1 + tab_padding, tab_y2);
  glEnd();
  GlUtils2d::FillRectangle(tab_x2 - tab_padding + 1, tab_y1 + tab_padding, tab_x2 - 1, tab_y2 - 1);
  glBegin(GL_TRIANGLES);
  glVertex2i(tab_x2 - tab_padding + 1, tab_y1 + tab_padding);
  glVertex2i(tab_x2, tab_y1 + 1);
  glVertex2i(tab_x2, tab_y1 + tab_padding);
  glEnd();
  GlUtils2d::FillRectangle(tab_x1 + tab_padding, tab_y1 + tab_padding, tab_x2 - tab_padding,
                           tab_y2 - tab_padding, factory_->GetTabInnerColor());

  // Draw the border
  GlUtils::SetColor(factory_->GetBorderColor());
  if (is_horizontal) {
    GlUtils2d::DrawLine(x1, y1, x2, y1);
    GlUtils2d::DrawLine(x1, y2, x2, y2);
  } else {
    GlUtils2d::DrawLine(x1, y1, x1, y2);
    GlUtils2d::DrawLine(x2, y1, x2, y2);
  }
  GlUtils::SetColor(kWhite);
}
