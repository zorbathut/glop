// Includes
#include "GlopFrameWidgets.h"
#include "GlopWindow.h"
#include "Image.h"
#include "OpenGl.h"
#include "System.h"
#include "Utils.h"

// SolidBoxFrame
// =============

void SolidBoxFrame::Render() {
  GlUtils2d::FillRectangle(GetX(), GetY(), GetX2(), GetY2(), inner_color_);
  if (has_outer_part_)
    GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), outer_color_);
  GlUtils::SetColor(kWhite);
  PaddedFrame::Render();
}

// HollowBoxFrame
// ==============

void HollowBoxFrame::Render() {
  GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), color_);
  GlUtils::SetColor(kWhite);
  PaddedFrame::Render();
}

// TextFrame
// =========

TextFrame::TextFrame(const string &text, LightSetId font_outline_id, const Color &color,
                     float font_height, bool is_using_ellipsis, bool is_full_height)
: text_(text),
  font_outline_id_(font_outline_id),
  font_id_(0),
  color_(color),
  font_height_(font_height),
  is_using_ellipsis_(is_using_ellipsis),
  is_full_height_(is_full_height) {
}

TextFrame::~TextFrame() {
  if (font_id_ != 0)
    gSystem->ReleaseFontRef(font_id_);
}

int TextFrame::GetFontPixelHeight(float height) {
  return int(window()->GetHeight() * height);
}

void TextFrame::SetFontOutlineId(LightSetId font_outline_id) {
  if (font_outline_id != font_outline_id_) {
    font_outline_id_ = font_outline_id;
    if (font_id_ != 0)
      gSystem->ReleaseFontRef(font_id_);
    font_id_ = 0;
    DirtySize();
  }
}

void TextFrame::Render() {
  if (font_id_ == 0)
    return;

  // Set the Gl properties for this text
  glEnable(GL_BLEND);
  GlUtils::SetFontTexture(font_id_);
  GlUtils::SetColor(color_);

  // If our text does not fit in the required space, add "..." and clip off entire characters
  // to make it fit
  if (is_using_ellipsis_ && (GetX() < GetClipX1() || GetX2() > GetClipX2())) {
    // Calculate the extents of "..." on either side
    int x0, x = GetX();
    int lw = gSystem->GetCharWidth(font_id_, '.', true) +
             2 * gSystem->GetCharWidth(font_id_, '.', false);
    int rw = 3 * gSystem->GetCharWidth(font_id_, '.', false);

    // Figure out where to start printing
    int i = 0;
    string print_string = "";
    if (GetX() < GetClipX1()) {
      print_string = "...";
      for (; x < GetClipX1() + lw; i++) 
        x += gSystem->GetCharWidth(font_id_, text_[i], i==0);
      x0 = x - lw;
    } else {
      x0 = GetX();
    }

    // Figure out where to end printing
    if (GetX2() > GetClipX2()) {
      if (x + rw <= GetClipX2()) {
        for (; text_[i] != 0; i++) {
          x += gSystem->GetCharWidth(font_id_, text_[i], i==0);
          if (x + rw > GetClipX2())
            break;
          print_string += text_[i];
        }
        print_string += "...";
      }
    } else if (i < (int)text_.size()) {
      print_string += text_.substr(i);
    }

    // Print the truncated string
    GlUtils2d::Print(x0, GetY(), print_string, font_id_);
  } else {
    GlUtils2d::Print(GetX(), GetY(), text_, font_id_);
  }

  // Undo the Gl property changes
  glDisable(GL_BLEND);
  GlUtils::SetNoTexture();
  GlUtils::SetColor(kWhite);
}

void TextFrame::RecomputeSize(int rec_width, int rec_height) {
  // Obtain a new font if need be
  int new_height = GetFontPixelHeight(font_height_);
  if (font_id_ == 0 || new_height != gSystem->GetFontHeight(font_id_, false)) {
    if (font_id_ != 0)
      gSystem->ReleaseFontRef(font_id_);
    font_id_ = gSystem->AddFontRef(font_outline_id_, new_height);
  }
  
  // Compute our size after the font switch
  if (font_id_ != 0) {
    SetSize(gSystem->GetTextWidth(font_id_, text_),
            gSystem->GetFontHeight(font_id_, is_full_height_));
  } else {
    SetSize(0, 0);
  }
}

// FpsFrame
// ========

void FpsFrame::Think(int dt) {
  text_->SetText(Format("%.2f fps", gSystem->GetFps()));
}

// FancyTextFrame
// ==============

FancyTextFrame::FancyTextFrame(const string &text, LightSetId font_outline_id,
                               const Color &start_color, float base_font_height)
: SingleParentFrame(new ColFrame(0, kJustifyLeft)),
  column_((ColFrame*)GetChild()),
  text_(text),
  font_outline_id_(font_outline_id),
  add_soft_returns_(true),
  start_color_(start_color),
  base_font_height_(base_font_height) {}

FancyTextFrame::FancyTextFrame(const string &text, LightSetId font_outline_id,
                               bool add_soft_returns, float horz_justify, const Color &start_color,
                               float base_font_height)
: SingleParentFrame(new ColFrame(0, horz_justify)),
  column_((ColFrame*)GetChild()),
  text_(text),
  font_outline_id_(font_outline_id),
  add_soft_returns_(add_soft_returns),
  start_color_(start_color),
  base_font_height_(base_font_height) {}

// Creates a ParseStatus corresponding to parsing a new block of text with our default settings.
FancyTextFrame::ParseStatus FancyTextFrame::CreateParseStatus() {
  ParseStatus result;
  result.pos = 0;
  result.color = start_color_;
  result.font_height = base_font_height_;
  result.font_id = 0;
  return result;
}

// Prepares to parse fancy text from the given status. This requires getting a system reference to
// the active font.
void FancyTextFrame::StartParsing(ParseStatus *status) {
  status->font_id = gSystem->AddFontRef(font_outline_id_,
    TextFrame::GetFontPixelHeight(status->font_height));
  ASSERT(status->font_id != 0);
}

// Stop parsing fancy text, given our last active status. This requires releasing the last font
// reference we had.
void FancyTextFrame::StopParsing(ParseStatus *status) {
  gSystem->ReleaseFontRef(status->font_id);
  status->font_id = 0;
}

// Given a string to parse, and our current status, this read the next ASCII character in the
// string (and returns it via ch), and updates status as well.
//  - status.pos gives the first character index AFTER ch
//  - The remaining information gives formatting UP TO ch.
bool FancyTextFrame::ParseNextCharacter(const string &s, ParseStatus *status, char *ch) {
  // Handle tags
  while (s[status->pos] == '\1') {
    // Read until the end of the tag
	int pos2;
    for (pos2 = status->pos+1; s[pos2] != 0 && s[pos2] != '\1'; pos2++);
    if (pos2 == status->pos+1 || s[status->pos] == 0)
      return false;
    string tag = s.substr(status->pos + 1, pos2 - status->pos -1);
    switch (tag[0]) {
      case 'c':
        // Handle color tags
        if (tag.size() != 7 && tag.size() != 9)
          return false;
        int c[4];
        if (!ToInt(tag.substr(1, 2), &c[0], 16) || !ToInt(tag.substr(3, 2), &c[1], 16) ||
            !ToInt(tag.substr(5, 2), &c[2], 16))
          return false;
        if (tag.size() == 7)
          c[3] = 255;
        else if (!ToInt(tag.substr(7, 2), &c[3], 16))
          return false;
        status->color = Color(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f);
        break;
      case 's':
        // Handle size tags
        if (!ToFloat(tag.substr(1), &status->font_height))
          return false;
        status->font_height *= base_font_height_;
        StopParsing(status);
        StartParsing(status);
        break;
      default:
        return false;
    }
    status->pos = pos2 + 1;
  }

  // Read the next ASCII character
  *ch = s[status->pos++];
  return true;
}

// Rebuilds the fancy text frame as a collection of standard text frames.
void FancyTextFrame::RecomputeSize(int rec_width, int rec_height) {
  int lines = 0;

  // Add soft returns and store the result in text2
  string text2;
  if (add_soft_returns_) {
    bool is_done = false;
    ParseStatus status = CreateParseStatus();
    while (!is_done) {
      int start_pos = status.pos, x = 0;
      bool is_soft_return;
      StartParsing(&status);
      ParseStatus word_start_status = status, dash_status = status;

      // Read a line of text
      while (1) {
        char ch;
        ParseStatus look_ahead_status = status;
        if (!ParseNextCharacter(text_, &look_ahead_status, &ch)) {
          StopParsing(&status);
          column_->Resize(0);
          SingleParentFrame::RecomputeSize(rec_width, rec_height);
          return;
        }
        if (ch == 0 || ch == '\n') {
          status = look_ahead_status;
          is_soft_return = false;
          is_done = (ch == 0);
          break;
        }
        if (ch == ' ')
          word_start_status = look_ahead_status;
        if (x + gSystem->GetCharWidth(status.font_id, '-', false) < rec_width) {
          dash_status = status;
        }
        x += gSystem->GetCharWidth(look_ahead_status.font_id, ch, x == 0);
        if (x > rec_width) {
          is_soft_return = true;
          break;
        }
        status = look_ahead_status;
      }
      lines++;
      StopParsing(&status);

      // Backtrack to the beginning of the line
      if (is_soft_return && word_start_status.pos > start_pos) {
        text2 += text_.substr(start_pos, word_start_status.pos - start_pos) + "\n";
        status = word_start_status;
        while (text_[status.pos] == ' ')
          status.pos++;
        if (text_[status.pos] == 0)
          is_done = true;
      } else if (is_soft_return && dash_status.pos > start_pos) {
        text2 += text_.substr(start_pos, dash_status.pos - start_pos) + "-\n";
        status = dash_status;
      } else if (is_soft_return) {
        if (status.pos == start_pos)
          status.pos++;
        text2 += text_.substr(start_pos, status.pos - start_pos) + "\n";
      } else {
        text2 += text_.substr(start_pos, status.pos - start_pos);
      }
    }
  } else {
    // Without soft returns, move everything to text2 and store the line count
    text2 = text_;
    for (int i = 0; i < (int)text2.size(); i++)
      lines += (text2[i] == '\n'? 1 : 0);
    lines++;
  }

  // Build the table of TextFrame's
  column_->Resize(lines);
  ParseStatus status = CreateParseStatus();
  StartParsing(&status);
  string cur_part = "";
  for (int row_num = 0; row_num < lines; row_num++) {
    vector<GlopFrame*> row;

    // Build TextFrame's for this row
    while (1) {
      char ch;
      ParseStatus old_status = status;
      if (!ParseNextCharacter(text2, &status, &ch)) {
        StopParsing(&status);
        column_->Resize(0);
        SingleParentFrame::RecomputeSize(rec_width, rec_height);
        return;
      }
      if (ch == 0 || ch == '\n' || old_status.color != status.color ||
          old_status.font_height != status.font_height) {
        TextFrame *frame = new TextFrame(cur_part, font_outline_id_,
                                         old_status.color, old_status.font_height,
                                         false, row_num == lines - 1);
        row.push_back(frame);
        cur_part = "";
        if (ch == 0 || ch == '\n')
          break;
      }
      cur_part += ch;
    }

    // Store the row in our final result
    if (row.size() == 1) {
      column_->SetCell(row_num, row[0]);
    } else {
      RowFrame *row_frame = new RowFrame((int)row.size(), kJustifyBottom);
      for (int i = 0; i < (int)row.size(); i++)
        row_frame->SetCell(i, row[i]);
      column_->SetCell(row_num, row_frame);
    }
  }
  StopParsing(&status);
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
}


// WindowFrame
// ===========

WindowFrame::WindowFrame(GlopFrame *inner_frame, const string &title, LightSetId font_outline_id,
                         const FrameStyle *style)
: SingleParentFrame(new ColFrame(0, new PaddedFrame(inner_frame, 3), kJustifyLeft)),
  title_bar_frame_(new PaddedFrame(new TextFrame(title, font_outline_id, style->window_title_color,
                     style->font_height), 4, 2, 2, 0)),
  style_(style) {
  ((ColFrame*)GetChild())->SetCell(0, title_bar_frame_);
}

WindowFrame::WindowFrame(GlopFrame *inner_frame, const FrameStyle *style)
: SingleParentFrame(new PaddedFrame(inner_frame, 3)),
  title_bar_frame_(0),
  style_(style) {}

void WindowFrame::Render() {
  int title_height = (title_bar_frame_ == 0? 0 : title_bar_frame_->GetHeight());

  // Draw the fancy title bar
  if (title_bar_frame_ != 0) {
    glBegin(GL_QUADS);
    GlUtils::SetColor(style_->window_border_highlight_color);
    glVertex2i(GetX() + 1, GetY() + 1);
    glVertex2i(GetX2(), GetY() + 1);
    GlUtils::SetColor(style_->window_border_lowlight_color);
    glVertex2i(GetX2(), GetY() + title_height/4);
    glVertex2i(GetX() + 1, GetY() + title_height/4);
    glVertex2i(GetX() + 1, GetY() + title_height/4);
    glVertex2i(GetX2(), GetY() + title_height/4);
    GlUtils::SetColor(style_->window_border_highlight_color);
    glVertex2i(GetX2(), GetY() + title_height + 1);
    glVertex2i(GetX(), GetY() + title_height + 1);
    glEnd();
  }

  // Draw the window border
  GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), style_->window_border_lowlight_color);
  GlUtils2d::DrawRectangle(GetX() + 1, GetY() + title_height + 1, GetX2() - 1, GetY2() - 1,
                           style_->window_border_highlight_color);
  GlUtils2d::DrawRectangle(GetX() + 2, GetY() + title_height + 2, GetX2() - 2, GetY2() - 2,
                           style_->window_border_lowlight_color);

  // Draw the window interior    
  GlUtils2d::FillRectangle(GetX() + 3, GetY() + 3 + title_height, GetX2() - 3, GetY2() - 3, 
                           style_->window_inner_color);  

  // Delegate to the inner frame
  GlUtils::SetColor(kWhite);
  SingleParentFrame::Render();
}

// ButtonFrame
// ===========

void ButtonFrame::Render() {
  int lpadding = GetLeftPadding(), rpadding = GetRightPadding();

  // Handle up buttons
  if (!is_down_) {
    // Draw the border
    GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), style_->button_border_color); 

    // Draw the highlight
    GlUtils2d::FillRectangle(GetX() + 1, GetY() + 1, GetX2() - 1, GetY2() - 1,
                             style_->button_highlight_color);

    // Draw the lowlight
    GlUtils2d::FillRectangle(GetX() + lpadding, GetY2() - rpadding + 1, GetX2() - 1, GetY2() - 1,
                             style_->button_lowlight_color);
    glBegin(GL_TRIANGLES);
    glVertex2i(GetX() + 1, GetY2());
    glVertex2i(GetX() + lpadding, GetY2() - rpadding + 1);
    glVertex2i(GetX() + lpadding, GetY2());
    glEnd();
    GlUtils2d::FillRectangle(GetX2() - rpadding + 1, GetY() + lpadding, GetX2() - 1, GetY2() - 1);
    glBegin(GL_TRIANGLES);
    glVertex2i(GetX2() - rpadding + 1, GetY() + lpadding);
    glVertex2i(GetX2(), GetY() + 1);
    glVertex2i(GetX2(), GetY() + lpadding);
    glEnd();

    // Draw the button interior
    GlUtils2d::FillRectangle(GetX() + lpadding, GetY() + lpadding, GetX2() - rpadding,
                             GetY2() - rpadding, style_->button_unpressed_inner_color);
  } else {
    GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), style_->button_border_color);
    GlUtils2d::FillRectangle(GetX() + 1, GetY() + 1, GetX2() - 1, GetY2() - 1,
                             style_->button_lowlight_color);
    GlUtils2d::FillRectangle(GetX() + lpadding, GetY() + lpadding, GetX2() - rpadding,
                             GetY2() - rpadding, style_->button_pressed_inner_color);
  }
  GlUtils::SetColor(kWhite);

  // Render the interior
  PaddedFrame::Render();
  if (IsInFocus() && is_selectable_) {
    GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), style_->button_selection_color);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x5555);
    GlUtils2d::DrawRectangle(GetX() + lpadding - 1, GetY() + lpadding - 1,
                             GetX2() - rpadding + 1, GetY2() - rpadding + 1,
                             style_->button_selection_color);
    glLineStipple(1, 0xffff);
    glDisable(GL_LINE_STIPPLE);
    GlUtils::SetColor(kWhite);
  }
}

// Sets is_down_ and then repositions the inner frame accordingly
void ButtonFrame::UpdateIsDown(int cx, int cy) {
  bool old_is_down = is_down_;
  if (!IsInFocus())
    is_down_ = false;
  else if (is_hot_key_down_)
    is_down_ = true;
  else
    is_down_ = (is_mouse_locked_on_ && IsPointVisible(cx, cy));
  if (old_is_down != is_down_)
    RecomputePadding();
}

// Handles key presses. All logic goes here so that everything will work well independent of
// the frame rate.
void ButtonFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  if (event.IsReleaseDefocus()) {
    is_mouse_locked_on_ = is_hot_key_down_ = false;
    UpdateIsDown(0, 0);
    return;
  }
  if (!IsInFocus())
    return;

  // Update was_held_down_
  if (dt > 0 && is_down_) {
    held_down_repeat_delay_ -= dt;
    if (held_down_repeat_delay_ <= 0) {
      held_down_repeat_delay_ = kRepeatRate;
      held_down_queued_ = true;
    }
  }

  // Update is_down_ based on mouse motion
  bool old_is_down = is_down_;
  UpdateIsDown(input()->GetMouseX(), input()->GetMouseY());

  // Now restrict to key presses and key releases
  if (!event.IsNonRepeatPress() && !event.IsRelease())
    return;

  // Handle enter and hot keys
  bool is_hot_key = (hot_keys_.Find(event.key) != 0) ||
    (hot_keys_.Find(kAnyKey) && !event.key.IsMotionKey() && !event.key.IsModifierKey() &&
     !event.key.IsMouseKey());
  if (((event.key == 13 || event.key == kKeyPadEnter) && is_selectable_) || is_hot_key)
    is_hot_key_down_ = event.IsPress();

  // Handle mouse clicks
  if (event.key == kMouseLButton) {
    if (event.IsPress())
      is_mouse_locked_on_ = IsPointVisible(input()->GetMouseX(), input()->GetMouseY());
    else
      is_mouse_locked_on_ = false;
  }

  // Update
  UpdateIsDown(input()->GetMouseX(), input()->GetMouseY());
  if (!is_down_ && old_is_down && !is_mouse_locked_on_)
    full_press_queued_ = true;
  else if (is_down_ && !old_is_down)
    held_down_repeat_delay_ = kRepeatDelay;
}

// Just finalize the values from OnKeyEvent.
void ButtonFrame::Think(int dt) {
  was_held_down_ = held_down_queued_;
  was_pressed_fully_ = full_press_queued_;
  held_down_queued_ = full_press_queued_ = false;
}

void ButtonFrame::RecomputePadding() {
  int padding = 2+int(min(window()->GetWidth(), window()->GetHeight())*style_->button_border_size);
  int offset = (is_down_? 1 : 0);
  SetPadding(padding + offset - 1, padding + offset - 1, padding - offset, padding - offset);
}

// Overrides SetIsInFocus to unpress the button if we lose focus
void ButtonFrame::SetIsInFocus(bool is_in_focus) {
  if (!is_in_focus) {
    is_mouse_locked_on_ = is_hot_key_down_ = false;
    UpdateIsDown(0, 0);
  }
  PaddedFrame::SetIsInFocus(is_in_focus);
}

// Helper function that does the work for the constructors
void ButtonFrame::Init(bool is_selectable, const GlopKey &hot_key1, const GlopKey &hot_key2,
                       const FrameStyle *style) {
  is_selectable_ = is_selectable;
  if (hot_key1 != kNoKey) hot_keys_.InsertItem(hot_key1);
  if (hot_key2 != kNoKey) hot_keys_.InsertItem(hot_key2);
  is_hot_key_down_ = is_mouse_locked_on_ = false;
  full_press_queued_ = held_down_queued_ = false;
  is_down_ = was_held_down_ = was_pressed_fully_ = false;
  style_ = style;
}
