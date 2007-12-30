// Includes
#include "../include/GlopFrameBase.h"
#include "../include/GlopFrameStyle.h"
#include "../include/GlopFrameWidgets.h"
#include "../include/GlopWindow.h"
#include "../include/OpenGl.h"

// GuiTextStyle
// ============

GuiTextStyle::GuiTextStyle()
: color(gGuiTextStyle->color), size(gGuiTextStyle->size), font(gGuiTextStyle->font),
  flags(gGuiTextStyle->flags) {}

GuiTextStyle::GuiTextStyle(const Color &_color)
: color(_color), size(gGuiTextStyle->size), font(gGuiTextStyle->font),
  flags(gGuiTextStyle->flags) {}

GuiTextStyle::GuiTextStyle(const Color &_color, float _size)
: color(_color), size(_size), font(gGuiTextStyle->font), flags(gGuiTextStyle->flags) {}

GuiTextStyle::GuiTextStyle(const Color &_color, float _size, Font *_font)
: color(_color), size(_size), font(_font), flags(gGuiTextStyle->flags) {}

// InputBoxView
// ============

void DefaultInputBoxView::OnResize(int rec_width, int rec_height,
                                   int *lp, int *tp, int *rp, int *bp) const {
  *lp = *tp = *rp = *bp = 1;
}

void DefaultInputBoxView::Render(int x1, int y1, int x2, int y2,
                                 const PaddedFrame *padded_frame) const {
  GlUtils2d::DrawRectangle(x1, y1, x2, y2, factory_->GetBorderColor());
  GlUtils2d::FillRectangle(x1+1, y1+1, x2-1, y2-1, factory_->GetBackgroundColor());
  padded_frame->Render();
}

// TextPromptView
// ==============

const GuiTextStyle DefaultTextPromptView::GetTextStyle() const  {
  return factory_->GetTextStyle();
}

void DefaultTextPromptView::OnResize(int rec_width, int rec_height, const TextFrame *text_frame,
                                     int *lp, int *tp, int *rp, int *bp) const {
  *tp = *bp = 0;
  *lp = 1;
  *rp = text_frame->GetRenderer()->GetCharWidth('|', true, true) - 1;
}

void DefaultTextPromptView::Render(int x1, int y1, int x2, int y2, int cursor_pos, int *cursor_time,
                                   int selection_start, int selection_end, bool is_in_focus,
                                   const TextFrame *text_frame) const {
  const int kCursorCycleTime = 1000, kCursorFadeTime = 100;

  // Get interesting x-coordinates
  int len = (int)text_frame->GetText().size();
  vector<int> x(1, 0);
  for (int i = 0; i < len; i++) {
    x.push_back(x[i] + text_frame->GetRenderer()->GetCharWidth(text_frame->GetText()[i],
                i == 0, i == len-1));
  }
  int cursor_x = x1 + x[cursor_pos], sel_x1 = text_frame->GetX() + x[selection_start],
      sel_x2 = text_frame->GetX() + x[selection_end];

  // Animate the cursor
  *cursor_time %= kCursorCycleTime;
  Color cursor_color = factory_->GetCursorColor();
  int delim[] = {kCursorCycleTime / 2 - kCursorFadeTime, kCursorCycleTime / 2,
                 kCursorCycleTime - kCursorFadeTime, kCursorCycleTime};
  if (*cursor_time <= delim[0])
    cursor_color[3] = 1;
  else if (*cursor_time <= delim[1])
    cursor_color[3] = 1 - (*cursor_time - delim[0]) / float(delim[1] - delim[0]);
  else if (*cursor_time <= delim[2])
    cursor_color[3] = 0;
  else
    cursor_color[3] = (*cursor_time - delim[2]) / float(delim[3] - delim[2]);

  // Render
  if (selection_start != selection_end)
    GlUtils2d::FillRectangle(sel_x1, y1, sel_x2-1, y2, factory_->GetHighlightColor());
  text_frame->Render();
  if (is_in_focus)
    text_frame->GetRenderer()->Print(cursor_x, y1, "|", cursor_color);
}
 
// WindowView
// ==========

const GuiTextStyle DefaultWindowView::GetTitleStyle() const {
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

// Globals
// =======
GuiTextStyle *gGuiTextStyle = 0;
InputBoxViewFactory *gInputBoxViewFactory = 0;
TextPromptViewFactory *gTextPromptViewFactory = 0;
ArrowViewFactory *gArrowViewFactory = 0;
ButtonViewFactory *gButtonViewFactory = 0;
SliderViewFactory *gSliderViewFactory = 0;
WindowViewFactory *gWindowViewFactory = 0;

template<class T> static void SafeDelete(T *& data) {
  if (data != 0) {
    delete data;
    data = 0;
  }
}

void ClearFrameStyle() {
  SafeDelete(gGuiTextStyle);
  SafeDelete(gInputBoxViewFactory);
  SafeDelete(gTextPromptViewFactory);
  SafeDelete(gArrowViewFactory);
  SafeDelete(gButtonViewFactory);
  SafeDelete(gSliderViewFactory);
  SafeDelete(gWindowViewFactory);
}

void InitDefaultFrameStyle(Font *font) {
  ClearFrameStyle();
  gGuiTextStyle = new GuiTextStyle(kDefaultTextColor, kDefaultTextHeight, font, 0);
  gInputBoxViewFactory = new DefaultInputBoxViewFactory();
  gTextPromptViewFactory = new DefaultTextPromptViewFactory(font);
  gArrowViewFactory = new DefaultArrowViewFactory();
  gButtonViewFactory = new DefaultButtonViewFactory();
  gSliderViewFactory = new DefaultSliderViewFactory(gArrowViewFactory, gButtonViewFactory);
  gWindowViewFactory = new DefaultWindowViewFactory(font);
}
