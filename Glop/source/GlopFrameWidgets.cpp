// Includes
#include "GlopFrameWidgets.h"
#include "GlopWindow.h"
#include "Image.h"
#include "OpenGl.h"
#include "System.h"
#include "Utils.h"

// Constants
const int kTextPromptCursorCycleTime = 800;
const int kTextPromptCursorFadeTime = 80;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generic private helper classes
// ==============================

// ArrowImageFrame
// ===============
//
// Helper class that renders a black arrow covering about 80% of the frame. This is used on buttons.
// See, for example, SliderFrame.
class ArrowImageFrame: public GlopFrame {
 public:
  enum Direction {Up, Down, Left, Right};

  ArrowImageFrame(Direction d, const Color &color = gDefaultStyle->text_color)
  : direction_(d), color_(color) {}

  void Render() {
    int x = GetX() + (GetWidth()+1)/2, y = GetY() + (GetHeight()+1)/2;
    int d = int(GetWidth() * 0.35f + 0.5f);

    GlUtils::SetColor(color_);
    glBegin(GL_TRIANGLES);
    switch(direction_) {
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

  // Keep an aspect ratio of 1.0.
  void RecomputeSize(int rec_width, int rec_height) {
    SetToMaxSize(rec_width, rec_height, 1.0f);
  }

 private:
  Direction direction_;
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(ArrowImageFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Basic widgets
// =============

void SolidBoxFrame::Render() {
  GlUtils2d::FillRectangle(GetX(), GetY(), GetX2(), GetY2(), inner_color_);
  if (has_outer_part_)
    GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), outer_color_);
  GlUtils::SetColor(kWhite);
  PaddedFrame::Render();
}

void HollowBoxFrame::Render() {
  GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), color_);
  GlUtils::SetColor(kWhite);
  PaddedFrame::Render();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TextFrame
// =========

TextFrame::TextFrame(const string &text, const FrameStyle *style, bool is_full_height)
: text_(text),
  font_outline_id_(style->font_outline_id),
  font_id_(0),
  color_(style->text_color),
  text_height_(style->text_height),
  is_full_height_(is_full_height) {}
  
TextFrame::TextFrame(const string &text, const Color &color, float text_height,
                     LightSetId font_outline_id, bool is_full_height)
: text_(text),
  font_outline_id_(font_outline_id),
  font_id_(0),
  color_(color),
  text_height_(text_height),
  is_full_height_(is_full_height) {}

TextFrame::~TextFrame() {
  if (font_id_ != 0)
    gSystem->ReleaseFontRef(font_id_);
}

int TextFrame::GetFontPixelHeight(float height) {
  return int(gWindow->GetHeight() * height);
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
  if (GetX() < GetClipX1() || GetX2() > GetClipX2()) {
    // Calculate the extents of "..." on either side
    int x0, x = GetX();
    int lw = gSystem->GetCharWidth(font_id_, '.', true, false) +
             2 * gSystem->GetCharWidth(font_id_, '.', false, false);
    int rw = gSystem->GetCharWidth(font_id_, '.', false, true) +
             2 * gSystem->GetCharWidth(font_id_, '.', false, false);

    // Figure out where to start printing
    int i = 0, len = (int)text_.size();
    string print_string = "";
    if (GetX() < GetClipX1()) {
      print_string = "...";
      for (; x < GetClipX1() + lw; i++) 
        x += gSystem->GetCharWidth(font_id_, text_[i], i==0, i==len-1);
      x0 = x - lw;
    } else {
      x0 = GetX();
    }

    // Figure out where to end printing
    if (GetX2() > GetClipX2()) {
      if (x + rw <= GetClipX2()) {
        for (; text_[i] != 0; i++) {
          x += gSystem->GetCharWidth(font_id_, text_[i], i==0, i==len-1);
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
  int new_height = GetFontPixelHeight(text_height_);
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
  text()->SetText(Format("%.2f fps", gSystem->GetFps()));
}

// FancyTextFrame
// ==============

FancyTextFrame::FancyTextFrame(const string &text, const FrameStyle *style)
: SingleParentFrame(new ColFrame(0, kJustifyLeft)),
  text_(text),
  font_outline_id_(style->font_outline_id),
  add_soft_returns_(true),
  start_color_(style->text_color),
  base_text_height_(style->text_height) {}

FancyTextFrame::FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                               const FrameStyle *style)
: SingleParentFrame(new ColFrame(0, kJustifyLeft)),
  text_(text),
  font_outline_id_(style->font_outline_id),
  add_soft_returns_(add_soft_returns),
  start_color_(style->text_color),
  base_text_height_(style->text_height) {}

FancyTextFrame::FancyTextFrame(const string &text, const Color &start_color, float base_text_height,
                               LightSetId font_outline_id)
: SingleParentFrame(new ColFrame(0, kJustifyLeft)),
  text_(text),
  font_outline_id_(font_outline_id),
  add_soft_returns_(true),
  start_color_(start_color),
  base_text_height_(base_text_height) {}

FancyTextFrame::FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                               const Color &start_color, float base_text_height,
                               LightSetId font_outline_id)
: SingleParentFrame(new ColFrame(0, horz_justify)),
  text_(text),
  font_outline_id_(font_outline_id),
  add_soft_returns_(add_soft_returns),
  start_color_(start_color),
  base_text_height_(base_text_height) {}

// Creates a ParseStatus corresponding to parsing a new block of text with our default settings.
FancyTextFrame::ParseStatus FancyTextFrame::CreateParseStatus() {
  ParseStatus result;
  result.pos = 0;
  result.color = start_color_;
  result.text_height = base_text_height_;
  result.font_id = 0;
  return result;
}

// Prepares to parse fancy text from the given status. This requires getting a system reference to
// the active font.
void FancyTextFrame::StartParsing(ParseStatus *status) {
  status->font_id = gSystem->AddFontRef(font_outline_id_,
    TextFrame::GetFontPixelHeight(status->text_height));
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
        if (!ToFloat(tag.substr(1), &status->text_height))
          return false;
        status->text_height *= base_text_height_;
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
          column()->Resize(0);
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
        if (x + gSystem->GetCharWidth(status.font_id, '-', false, true) <= rec_width)
          dash_status = status;
        if (x + gSystem->GetCharWidth(look_ahead_status.font_id, ch, x == 0, true) > rec_width) {
          is_soft_return = true;
          break;
        }
        x += gSystem->GetCharWidth(look_ahead_status.font_id, ch, x == 0, false);
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
  column()->Resize(lines);
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
        column()->Resize(0);
        SingleParentFrame::RecomputeSize(rec_width, rec_height);
        return;
      }
      if (ch == 0 || ch == '\n' || old_status.color != status.color ||
          old_status.text_height != status.text_height) {
        TextFrame *frame = new TextFrame(cur_part, old_status.color, old_status.text_height,
                                         font_outline_id_, row_num == lines - 1);
        row.push_back(frame);
        cur_part = "";
        if (ch == 0 || ch == '\n')
          break;
      }
      cur_part += ch;
    }

    // Store the row in our final result
    if (row.size() == 1) {
      column()->SetCell(row_num, row[0]);
    } else {
      RowFrame *row_frame = new RowFrame((int)row.size(), kJustifyBottom);
      for (int i = 0; i < (int)row.size(); i++)
        row_frame->SetCell(i, row[i]);
      column()->SetCell(row_num, row_frame);
    }
  }
  StopParsing(&status);
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
}

// AbstractTextPromptFrame
// =======================

void AbstractTextPromptFrame::Render() {
  SingleParentFrame::Render();

  if (IsInFocus()) {
    // Determine the cursor color (including translucency)
    Color cursor_color((text()->GetColor() + kBlue) / 2);
    int delim[] = {kTextPromptCursorCycleTime / 2 - kTextPromptCursorFadeTime,
                   kTextPromptCursorCycleTime / 2,
                   kTextPromptCursorCycleTime - kTextPromptCursorFadeTime,
                   kTextPromptCursorCycleTime};
    if (cursor_timer_ <= delim[0])
      cursor_color[3] = 1;
    else if (cursor_timer_ <= delim[1])
      cursor_color[3] = 1 - (cursor_timer_ - delim[0]) / float(delim[1] - delim[0]);
    else if (cursor_timer_ <= delim[2])
      cursor_color[3] = 0;
    else
      cursor_color[3] = (cursor_timer_ - delim[2]) / float(delim[3] - delim[2]);

    // Print the cursor
    GlUtils::SetFontTexture(text()->GetFontId());
    GlUtils::SetColor(cursor_color);
    glEnable(GL_BLEND);
    GlUtils2d::Print(GetX() + char_x_[cursor_pos_], GetY(), "|", text()->GetFontId());
    glDisable(GL_BLEND);
    GlUtils::SetColor(kWhite);
    GlUtils::SetNoTexture();
  }
}

void AbstractTextPromptFrame::Think(int dt) {
  cursor_timer_ += dt;
  cursor_timer_ %= kTextPromptCursorCycleTime;
  was_confirmed_ = was_confirm_queued_;
  was_canceled_ = was_cancel_queued_;
  was_confirm_queued_ = was_cancel_queued_ = false;
}

void AbstractTextPromptFrame::RecomputeSize(int rec_width, int rec_height) {
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
  SetSize(GetWidth() + gSystem->GetCharWidth(text()->GetFontId(), '|', true, true), GetHeight());
  char_x_.clear();
  char_x_.push_back(0);
  for (int i = 0; i < (int)text()->GetText().size(); i++) {
    int dx = gSystem->GetCharWidth(text()->GetFontId(), text()->GetText()[i], i == 0, false);
    char_x_.push_back(char_x_[i] + dx);
  }
}

// BasicTextPromptFrame
// ====================

void BasicTextPromptFrame::Render() {
  if (GetCursorPos() != selection_anchor_) {
    int s1 = min(GetCursorPos(), selection_anchor_), s2 = max(GetCursorPos(), selection_anchor_);
    GlUtils2d::FillRectangle(GetX() + GetCharX()[s1], GetY(),
                             GetX() + GetCharX()[s2], GetY2(), selection_color_);
    GlUtils::SetColor(kWhite);
  }
  AbstractTextPromptFrame::Render();
}

bool BasicTextPromptFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  // Track mouse motion with the selection
  if (input()->IsKeyDownNow(kMouseLButton)) {
    if (is_tracking_mouse_ ||
        (event.IsNonRepeatPress() && event.key == kMouseLButton)) {
      if (event.key == kMouseLButton && event.IsDoublePress()) {
        selection_anchor_ = 0;
        SetCursorPos(int(text()->GetText().size()), false);
        return true;
      }
      int mx = input()->GetMouseX() - GetX();
      int pos = BSFindLowerBound(GetCharX(), mx);
      if (pos == -1)
        SetCursorPos(0, !is_tracking_mouse_);
      else if (pos == (int)text()->GetText().size())
        SetCursorPos((int)text()->GetText().size(), !is_tracking_mouse_);
      else if (mx < (GetCharX()[pos] + GetCharX()[pos+1])/2)
        SetCursorPos(pos, !is_tracking_mouse_);
      else
        SetCursorPos(pos+1, !is_tracking_mouse_);
      is_tracking_mouse_ = true;
    }
  } else {
    is_tracking_mouse_ = false;
  }

  // Handle all key presses
  if (event.IsPress()) {
    int ascii = input()->GetAsciiValue(event.key);

    // Handle backspace and delete
    if (event.key == '\b') {
      if (selection_anchor_ != GetCursorPos())
        DeleteSelection();
      else if (GetCursorPos() > 0)
        DeleteCharacter(false);
      ReformTextAndCursor(false);
    } else if (event.key == kKeyDelete) {
      if (selection_anchor_ != GetCursorPos())
        DeleteSelection();
      else if (GetCursorPos() < (int)text()->GetText().size())
        DeleteCharacter(true);
      ReformTextAndCursor(false);
    }
    
    // Handle confirmation and canceling
    else if (event.key == '\n' || event.key == kKeyPadEnter) {
      ReformTextAndCursor(true);
      Confirm();
    } else if (event.key == 27) {
      Cancel();
    }

    // Handle character insertion
    else if (ascii != 0 && CanInsertCharacter(ascii, true)) {
      if (selection_anchor_ != GetCursorPos()) {
        int cursor_pos_cache = GetCursorPos(), selection_anchor_cache = selection_anchor_;
        string text_cache = text()->GetText();
        DeleteSelection();
        if (CanInsertCharacter(ascii, false)) {
          InsertCharacter(ascii);
          ReformTextAndCursor(false);
        } else {
          SetText(text_cache);
          AbstractTextPromptFrame::SetCursorPos(cursor_pos_cache, false);
          selection_anchor_ = selection_anchor_cache;
        }
      } else if (CanInsertCharacter(ascii, false)) {
        InsertCharacter(ascii);
        ReformTextAndCursor(false);
      }
    }

    // Handle cursor movement
    else {
      bool is_shift_down = input()->IsKeyDownNow(kKeyLeftShift) ||
                          input()->IsKeyDownNow(kKeyRightShift);
      if (event.key == kKeyRight || (event.key == kKeyPad6 && !input()->IsNumLockSet()))
        SetCursorPos(GetCursorPos() + 1, !is_shift_down);
      else if (event.key == kKeyLeft || (event.key == kKeyPad4 && !input()->IsNumLockSet()))
        SetCursorPos(GetCursorPos() - 1, !is_shift_down);
      else if (event.key == kKeyHome || (event.key == kKeyPad3 && !input()->IsNumLockSet()))
        SetCursorPos(0, !is_shift_down);
      else if (event.key == kKeyEnd || (event.key == kKeyPad1 && !input()->IsNumLockSet()))
        SetCursorPos((int)text()->GetText().size(), !is_shift_down);
      else
        return false;
    }
    return true;
  }
  return false;
}

BasicTextPromptFrame::BasicTextPromptFrame(const string &text, const FrameStyle *style)
: AbstractTextPromptFrame(text, style),
  stored_value_(text),
  is_tracking_mouse_(false),
  selection_anchor_(GetCursorPos()),
  selection_color_(style->prompt_highlight_color) {}

BasicTextPromptFrame::BasicTextPromptFrame(const string &text, const Color &color,
                                           float text_height, LightSetId font_outline_id,
                                           const Color &selection_color)
: AbstractTextPromptFrame(text, color, text_height, font_outline_id),
  stored_value_(text),
  is_tracking_mouse_(false),
  selection_anchor_(GetCursorPos()),
  selection_color_(selection_color) {}

void BasicTextPromptFrame::DeleteSelection() {
  int s1 = min(GetCursorPos(), selection_anchor_), s2 = max(GetCursorPos(), selection_anchor_);
  string part1 = (s1 == 0? "" : text()->GetText().substr(0, s1)),
         part2 = (s2 == text()->GetText().size()? "" : text()->GetText().substr(s2));
  SetText(part1 + part2);
  SetCursorPos(s1, true);
}

void BasicTextPromptFrame::DeleteCharacter(bool is_next_character) {
  int i = GetCursorPos() + (is_next_character? 0 : -1);
  string part1 = (i == 0? "" : text()->GetText().substr(0, i)),
         part2 = (i+1 == text()->GetText().size()? "" : text()->GetText().substr(i+1));
  SetText(part1 + part2);
  SetCursorPos(i, true);
}

void BasicTextPromptFrame::InsertCharacter(char ch) {
  int i = GetCursorPos();
  string part1 = (i == 0? "" : text()->GetText().substr(0, i)),
         part2 = (i == text()->GetText().size()? "" : text()->GetText().substr(i));
  SetText(part1 + ch + part2);
  SetCursorPos(i + 1, true);
}

void BasicTextPromptFrame::ReformTextAndCursor(bool is_confirmed) {
  ReformText(is_confirmed);
  if (GetCursorPos() > (int)text()->GetText().size())
    SetCursorPos((int)text()->GetText().size(), true);
}

void BasicTextPromptFrame::OnFocusChange() {
  if (!IsInFocus()) {
    selection_anchor_ = GetCursorPos();
  } else {
    selection_anchor_ = 0;
    SetCursorPos(int(text()->GetText().size()), false);
  }
  AbstractTextPromptFrame::OnFocusChange();
}

void BasicTextPromptFrame::SetCursorPos(int pos, bool move_anchor) {
  AbstractTextPromptFrame::SetCursorPos(pos);
  if (move_anchor)
    selection_anchor_ = GetCursorPos();
}

// AbstractTextPromptFrame implementations
// =======================================

bool GlopKeyPromptFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  if (event.IsNonRepeatPress() && !GetFocusFrame()->IsGainingFocus()) {
    if (IsValidSelection(event.key)) {
      Set(event.key);
      Confirm();
      return true;
    } else if (event.key == 27) {
      Cancel();
      return true;
    }
  }
  return false;
}

void StringPromptFrame::Set(const string &value) {
  if (length_limit_ < (int)value.size())
    SetText(value.substr(0, length_limit_));
  else
    SetText(value);
}

bool StringPromptFrame::CanInsertCharacter(char ch, bool in_theory) const {
  return (ch >= kFirstFontCharacter && ch <= kLastFontCharacter) &&
         (in_theory || (int)text()->GetText().size() < length_limit_);
}

bool IntegerPromptFrame::CanInsertCharacter(char ch, bool in_theory) const {
  if (!in_theory) {
    if ((ch == '-' && GetCursorPos() > 0) ||
        (ch == '0' && text()->GetText() != "" && GetCursorPos() == 0) ||
        (ch == '0' && text()->GetText()[0] == '-' && GetCursorPos() == 1))
      return false;
  }
  return (ch >= '0' && ch <= '9') || (min_value_ < 0 && ch == '-');
}

void IntegerPromptFrame::ReformText(bool is_confirmed) {
  string min_value = Format("%d", min_value_), max_value = Format("%d", max_value_);
  string s = text()->GetText();
  while (s[0] == '-' && s[1] == '0')
    s = "-" + s.substr(2);
  while (s[0] == '0' && s.size() > 1)
    s = s.substr(1);
  if (s[0] != '-') {
    if (s.size() > max_value.size() || (s.size() == max_value.size() && s > max_value))
      s = max_value;
  } else {
    if (s.size() > min_value.size() || (s.size() == min_value.size() && s > min_value))
      s = min_value;
  }
  SetText(s);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// WindowFrame
// ===========

GlopFrame *DefaultWindowRenderer::CreateTitleFrame(const string &title) const {
  return new PaddedFrame(new TextFrame(title, title_color_, title_height_, title_font_id_),
                         4, 2, 2, 0);
}

void DefaultWindowRenderer::GetInnerFramePadding(bool has_title, int *lp, int *tp,
                                                 int *rp, int *bp) const {
  *lp = *tp = *rp = *bp = 3;
}

void DefaultWindowRenderer::Render(int x1, int y1, int x2, int y2, GlopFrame *title_frame,
                                   GlopFrame *inner_frame) const {
  int title_height = (title_frame == 0? 0 : title_frame->GetHeight()), ym = y1 + title_height - 1;

  // Draw the fancy title bar
  if (title_frame != 0) {
    glBegin(GL_QUADS);
    GlUtils::SetColor(border_highlight_color_);
    glVertex2i(x1 + 1, y1 + 1);
    glVertex2i(x2, y1 + 1);
    GlUtils::SetColor(border_lowlight_color_);
    glVertex2i(x2, y1 + title_height/4);
    glVertex2i(x1 + 1, y1 + title_height/4);
    glVertex2i(x1 + 1, y1 + title_height/4);
    glVertex2i(x2, y1 + title_height/4);
    GlUtils::SetColor(border_highlight_color_);
    glVertex2i(x2, y1 + title_height + 1);
    glVertex2i(x1, y1 + title_height + 1);
    glEnd();
  }

  // Draw the window border
  GlUtils2d::DrawRectangle(x1, y1, x2, y2, border_lowlight_color_);
  GlUtils2d::DrawRectangle(x1 + 1, ym + 1, x2 - 1, y2 - 1, border_highlight_color_);
  GlUtils2d::DrawRectangle(x1 + 2, ym + 2, x2 - 2, y2 - 2, border_lowlight_color_);

  // Draw the window interior    
  GlUtils2d::FillRectangle(x1 + 3, ym + 3, x2 - 3, y2 - 3, inner_color_);

  // Delegate to the inner frames
  GlUtils::SetColor(kWhite);
  if (title_frame != 0)
    title_frame->Render();
  inner_frame->Render();
}

WindowFrame::WindowFrame(GlopFrame *inner_frame, const string &title,
                         const WindowRenderer *renderer)
: SingleParentFrame(0), renderer_(renderer) {
  ColFrame *col = new ColFrame(2, kJustifyLeft);
  int lp, tp, rp, bp;
  renderer->GetInnerFramePadding(true, &lp, &tp, &rp, &bp);
  col->SetCell(0, title_frame_ = renderer->CreateTitleFrame(title));
  col->SetCell(1, inner_frame_ = new PaddedFrame(inner_frame, lp, tp, rp, bp),
               CellSize::Default(), CellSize::Max());
  SetChild(col);
}

WindowFrame::WindowFrame(GlopFrame *inner_frame, const WindowRenderer *renderer)
: SingleParentFrame(0), renderer_(renderer), title_frame_(0) {
  int lp, tp, rp, bp;
  renderer->GetInnerFramePadding(false, &lp, &tp, &rp, &bp);
  SetChild(inner_frame_ = new PaddedFrame(inner_frame, lp, tp, rp, bp));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ButtonWidget
// ============

void DefaultButtonRenderer::RecomputePadding(bool is_down, int *lp, int *tp,
                                             int *rp, int *bp) const {
  int padding = 2 + int(min(gWindow->GetWidth(), gWindow->GetHeight()) * border_size_);
  int offset = (is_down? 1 : 0);
  *lp = *tp = padding + offset - 1;
  *rp = *bp = padding - offset;
}

void DefaultButtonRenderer::Render(bool is_down, bool is_primary_focus, int x1, int y1, int x2,
                                   int y2, GlopFrame *inner_frame) const {
  int lpadding, rpadding;
  RecomputePadding(is_down, &lpadding, &lpadding, &rpadding, &rpadding);

  // Handle up buttons
  if (!is_down) {
    // Draw the border
    GlUtils2d::DrawRectangle(x1, y1, x2, y2, border_color_);

    // Draw the highlight
    GlUtils2d::FillRectangle(x1 + 1, y1 + 1, x2 - 1, y2 - 1, highlight_color_);

    // Draw the lowlight
    GlUtils2d::FillRectangle(x1 + lpadding, y2 - rpadding + 1, x2 - 1, y2 - 1, lowlight_color_);
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
                             unpressed_inner_color_);
  } else {
    // Draw a pressed button
    GlUtils2d::DrawRectangle(x1, y1, x2, y2, border_color_);
    GlUtils2d::FillRectangle(x1 + 1, y1 + 1, x2 - 1, y2 - 1, lowlight_color_);
    GlUtils2d::FillRectangle(x1 + lpadding, y1 + lpadding, x2 - rpadding, y2 - rpadding,
                             pressed_inner_color_);
  }

  // Draw the inner frame
  GlUtils::SetColor(kWhite);
  if (inner_frame != 0)
    inner_frame->Render();

  // Draw the focus display
  if (is_primary_focus) {
    GlUtils2d::DrawRectangle(x1, y1, x2, y2, selection_color_);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x5555);
    GlUtils2d::DrawRectangle(x1 + lpadding - 1, y1 + lpadding - 1, x2 - rpadding + 1,
                            y2 - rpadding + 1, selection_color_);
    glLineStipple(1, 0xffff);
    glDisable(GL_LINE_STIPPLE);
    GlUtils::SetColor(kWhite);
  }
}

void AbstractButtonFrame::Think(int dt) {
  was_held_down_ = held_down_queued_;
  was_pressed_fully_ = full_press_queued_;
  held_down_queued_ = full_press_queued_ = false;
}

bool AbstractButtonFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  if (is_down_) {
    held_down_repeat_delay_ -= dt;
    while (held_down_repeat_delay_ <= 0) {
      held_down_queued_ = true;
      held_down_repeat_delay_ += kRepeatRate;
    }
  }
  return false;
}

void AbstractButtonFrame::SetIsDown(DownType down_type) {
  bool is_down = (down_type == Down || down_type == DownRepeatSoon);
  if (is_down == is_down_) return;
  is_down_ = is_down;
  RecomputePadding();
  if (is_down_) {
    NewRelativePing(0, 0, 1, 1);
    held_down_queued_ = true;
    held_down_repeat_delay_ = (down_type == Down? kRepeatDelay : kRepeatRate);
  } else if (down_type == UpFullRelease) {
    full_press_queued_ = true;
  }
}

void AbstractButtonFrame::RecomputePadding() {
  int lp, tp, rp, bp;
  renderer_->RecomputePadding(is_down_, &lp, &tp, &rp, &bp);
  SetPadding(lp, tp, rp, bp);
}

bool DefaultButtonFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  bool handled_key = false, was_mouse_locked_on = is_mouse_locked_on_;
  if (event.IsPress() || event.IsRelease()) {
    // Handle enter and hot keys
    bool is_hot_key = (hot_keys_.Find(event.key) != 0) ||
      (hot_keys_.Find(kAnyKey) && !event.key.IsMotionKey() && !event.key.IsModifierKey() &&
      !event.key.IsMouseKey());
    if (((event.key == 13 || event.key == kKeyPadEnter) && IsPrimaryFocus()) || is_hot_key) {
      handled_key = true;
      SetIsHotKeyDown(event.key, event.IsPress());
    }

    // Handle mouse clicks
    else if (event.key == kMouseLButton && event.IsNonRepeatPress()) {
      is_mouse_locked_on_ = IsPointVisible(input()->GetMouseX(), input()->GetMouseY());
      handled_key = true;
    } else if (event.key == kMouseLButton && event.IsRelease()) {
      is_mouse_locked_on_ = false;
      handled_key = false;
    }
  }

  if (down_hot_keys_.GetSize()) {
    SetIsDown(event.IsNonRepeatPress()? Down : DownRepeatSoon);
  } else if (IsPointVisible(input()->GetMouseX(), input()->GetMouseY())) {
    if (is_mouse_locked_on_)
      SetIsDown(was_mouse_locked_on? DownRepeatSoon : Down);
    else {
      SetIsDown(UpFullRelease);
    }
  } else {
    SetIsDown(was_mouse_locked_on? UpNoFullRelease : UpFullRelease);
  }
  AbstractButtonFrame::OnKeyEvent(event, dt);
  return handled_key;
}

void DefaultButtonFrame::OnFocusChange() {
  if (!IsInFocus()) {
    is_mouse_locked_on_ = false;
    down_hot_keys_.Clear();
    SetIsDown(UpNoFullRelease);
  }
  AbstractButtonFrame::OnFocusChange();
}

void DefaultButtonFrame::SetIsHotKeyDown(const GlopKey &key, bool is_down) {
  for (LightSetId id = down_hot_keys_.GetFirstId(); id != 0; id = down_hot_keys_.GetNextId(id))
  if (down_hot_keys_[id] == key)
    id = down_hot_keys_.RemoveItem(id);
  if (is_down)
    down_hot_keys_.InsertItem(key);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SliderWidget
// ============

int DefaultSliderRenderer::RecomputeWidth(bool is_horizontal) const {
  return int(min(gWindow->GetWidth(), gWindow->GetHeight()) * width_);
}

int DefaultSliderRenderer::RecomputeMinTabLength(bool is_horizontal, int inner_width,
                                                 int inner_height) const {
  return 6;
}

void DefaultSliderRenderer::Render(bool is_horizontal, bool is_primary_focus, int x1, int y1,
                                   int x2, int y2, int tab_x1, int tab_y1, int tab_x2,
                                   int tab_y2) const {
  // Elongate the tab a little - this means the border will overlap with the button border
  if (is_horizontal) {
    tab_x1--;
    tab_x2++;
  } else {
    tab_y1--;
    tab_y2++;
  }

  // Draw the background
  GlUtils2d::FillRectangle(x1, y1, x2, y2, background_color_);
  GlUtils::SetColor(kWhite);

  // Draw the tab
  GetButtonRenderer()->Render(false, is_primary_focus, x1 + tab_x1, y1 + tab_y1, x1 + tab_x2,
                              y1 + tab_y2, 0);

  // Draw the border
  if (is_horizontal) {
    GlUtils2d::DrawLine(x1, y1, x2, y1, border_color_);
    GlUtils2d::DrawLine(x1, y2, x2, y2, border_color_);
  } else {
    GlUtils2d::DrawLine(x1, y1, x1, y2, border_color_);
    GlUtils2d::DrawLine(x2, y1, x2, y2, border_color_);
  }
  GlUtils::SetColor(kWhite);
}

// This is the part of a SliderFrame between the two buttons: a long bar with a movable tab in
// the middle.
class InnerSliderFrame: public GlopFrame {
 public:
  // Basic constructor. See SliderWidget for parameter description.
  enum Direction {Horizontal, Vertical};
  InnerSliderFrame(Direction direction, int tab_logical_size, int bar_logical_size,
                   int tab_logical_position, int step_size, const SliderRenderer *renderer)
  : direction_(direction),
    tab_logical_size_(tab_logical_size),
    bar_logical_size_(bar_logical_size),
    step_size_(step_size),
    tab_logical_position_(tab_logical_position),
    mouse_lock_mode_(None),
    bar_pixel_length_(0),
    renderer_(renderer) {}

  void Render() {
    renderer_->Render(direction_ == Horizontal, GetParent()->IsPrimaryFocus(), GetX(), GetY(),
                      GetX2(), GetY2(), tab_x1_, tab_y1_, tab_x2_, tab_y2_);
  }

  bool OnKeyEvent(const KeyEvent &event, int dt) {
    // Compute the mouse pixel position on the slider
    int mouse_pixel, tab_start_pixel, tab_end_pixel;
    if (direction_ == Horizontal) {
      tab_start_pixel = tab_x1_;
      tab_end_pixel = tab_x2_;
      mouse_pixel = input()->GetMouseX() - GetX();
    } else {
      tab_start_pixel = tab_y1_;
      tab_end_pixel = tab_y2_;
      mouse_pixel = input()->GetMouseY() - GetY();
    }

    // Handle mouse first clicks
    if (event.key == kMouseLButton && event.IsNonRepeatPress() &&
        IsPointVisible(input()->GetMouseX(), input()->GetMouseY())) {
      if (mouse_pixel >= tab_start_pixel && mouse_pixel <= tab_end_pixel) {
        mouse_lock_mode_ = Tab;
        tab_grab_position_ = GetLocationPixelStart(GetLocationByPixel(mouse_pixel)) -
                                                   tab_start_pixel;
      } else {
        mouse_lock_mode_ = Bar; 
      }
    } else if (event.key == kMouseLButton && event.IsRelease()) {
      mouse_lock_mode_ = None;
    }

    // Handle mouse repeat clicks
    if (event.key == kMouseLButton && event.IsPress() && mouse_lock_mode_ == Bar &&
        IsPointVisible(input()->GetMouseX(), input()->GetMouseY())) {
      if (mouse_pixel < tab_start_pixel)
        LargeDec();
      else if (mouse_pixel > tab_end_pixel)
        LargeInc();
    }

    // Handle mouse dragging
    if (mouse_lock_mode_ == Tab)
      SetTabPosition(GetLocationByPixel(mouse_pixel - tab_grab_position_));
    return event.key == kMouseLButton;
  }

  // State
  void SetTabPosition(int position) {
    tab_logical_position_ = min(max(position, 0), bar_logical_size_ - tab_logical_size_);
    RecomputeTabScreenPosition();
  }
  int GetTabPosition() const {return tab_logical_position_;}
  void SmallDec() {SetTabPosition(tab_logical_position_ - step_size_);}
  void SmallInc() {SetTabPosition(tab_logical_position_ + step_size_);}
  void LargeDec() {SetTabPosition(tab_logical_position_ - (tab_logical_size_*9+9)/10);}
  void LargeInc() {SetTabPosition(tab_logical_position_ + (tab_logical_size_*9+9)/10);}
  int GetTabSize() const {return tab_logical_size_;}
  int GetTotalSize() const {return bar_logical_size_;}

 protected:
  void RecomputeSize(int rec_width, int rec_height) {
    SetSize(rec_width, rec_height);
    int bar_pixel_length = (direction_ == Horizontal? GetWidth() : GetHeight());
    if (bar_pixel_length != bar_pixel_length_) {
      bar_pixel_length_ = bar_pixel_length;
      mouse_lock_mode_ = None;
      RecomputeTabScreenPosition();
    }
  }

  void OnFocusChange() {
    if (!IsInFocus())
      mouse_lock_mode_ = None;
  }

 private:
  // Given a logical position, this returns the pixel on the slider where that position starts
  int GetLocationPixelStart(int pos) const {
    return pos * (bar_pixel_length_ - tab_pixel_length_) / (bar_logical_size_ - tab_logical_size_);
  }

  // Given a pixel on the slider, this returns the corresponding logical position. We are extra
  // careful to make sure rounding does not not prevent total consistency with
  // getLocationPixelStart. Otherwise, the scroll bar can have weird jumps when it is being
  // dragged.
  int GetLocationByPixel(int pixel_location) {
    if (bar_pixel_length_ <= tab_pixel_length_)
      return 0;	// A truly degenerate case
    int pos = pixel_location * (bar_logical_size_ - tab_logical_size_) /
              (bar_pixel_length_ - tab_pixel_length_) - 1;
    while (GetLocationPixelStart(pos+1) <= pixel_location)
      pos++;
    return pos;
  }

  // Recomputes the screen coordinates for the tab rectangle
  void RecomputeTabScreenPosition() {
    int min_tab_length = renderer_->RecomputeMinTabLength(direction_ == Horizontal,
                                                          GetWidth(), GetHeight());

    // Compute the length of the tab
    if (tab_logical_size_ < bar_logical_size_) {
      tab_pixel_length_ = max(tab_logical_size_ * (bar_pixel_length_ - 1) / bar_logical_size_,
                              min_tab_length);
    } else {
      tab_pixel_length_ = bar_pixel_length_;
    }

    // Compute its coordinates
    if (direction_ == Horizontal) {
      tab_x1_ = GetLocationPixelStart(tab_logical_position_);
      tab_x2_ = tab_x1_ + tab_pixel_length_ - 1;
      tab_y1_ = 0;
      tab_y2_ = GetHeight() - 1;
    } else {
      tab_y1_ = GetLocationPixelStart(tab_logical_position_);
      tab_y2_ = tab_y1_ + tab_pixel_length_ - 1;
      tab_x1_ = 0;
      tab_x2_ = GetWidth() - 1;
    }
  }

  // Data
  enum MouseLockMode {None, Bar, Tab};
  Direction direction_;
  int tab_logical_size_, bar_logical_size_;
  int step_size_;
  int tab_logical_position_;
  MouseLockMode mouse_lock_mode_;
  int bar_pixel_length_;
  int tab_pixel_length_, tab_x1_, tab_y1_, tab_x2_, tab_y2_;
  int tab_grab_position_;
  const SliderRenderer *renderer_;
  DISALLOW_EVIL_CONSTRUCTORS(InnerSliderFrame);
};

SliderFrame::SliderFrame(Direction direction, int tab_size, int total_size, int position,
                         bool has_arrow_hot_keys, int step_size, const SliderRenderer *renderer)
: SingleParentFrame(new TableFrame(0, 0)) {
  // Store the data
  if (step_size == -1)
    step_size = (tab_size + 9) / 10;
  direction_ = direction;
  renderer_ = renderer;

  // Create the table
  TableFrame *table = (TableFrame*)GetChild();
  if (direction == Horizontal) {
    table->Resize(3, 1);
    table->SetCell(0, 0, dec_button_ = new DefaultButtonFrame(
      new ArrowImageFrame(ArrowImageFrame::Left), renderer->GetButtonRenderer()));
    table->SetCell(1, 0, inner_slider_ = new InnerSliderFrame(InnerSliderFrame::Horizontal,
                       tab_size, total_size, position, step_size, renderer),
                     CellSize::Max(), CellSize::Default());
    table->SetCell(2, 0, inc_button_ = new DefaultButtonFrame(
      new ArrowImageFrame(ArrowImageFrame::Right), renderer->GetButtonRenderer()));
  } else {
    table->Resize(1, 3);
    table->SetCell(0, 0, dec_button_ = new DefaultButtonFrame(
      new ArrowImageFrame(ArrowImageFrame::Up), renderer->GetButtonRenderer()));
    table->SetCell(0, 1, inner_slider_ = new InnerSliderFrame(InnerSliderFrame::Vertical,
                       tab_size, total_size, position, step_size, renderer),
                     CellSize::Default(), CellSize::Max());
    table->SetCell(0, 2, inc_button_ = new DefaultButtonFrame(
      new ArrowImageFrame(ArrowImageFrame::Down), renderer->GetButtonRenderer()));
  }

  // Set the hot keys
  if (has_arrow_hot_keys) {
    if (direction == Horizontal) {
      AddDecHotKey(kKeyLeft);
      AddIncHotKey(kKeyRight);
    } else {
      AddDecHotKey(kKeyUp);
      AddIncHotKey(kKeyDown);
    }
  }
}

int SliderFrame::GetTabPosition() const {return inner_slider_->GetTabPosition();}
void SliderFrame::SetTabPosition(int position) {inner_slider_->SetTabPosition(position);}
int SliderFrame::GetTabSize() const {return inner_slider_->GetTabSize();}
int SliderFrame::GetTotalSize() const {return inner_slider_->GetTotalSize();}

void SliderFrame::Think(int dt) {
  SingleParentFrame::Think(dt);
  if (dec_button_->WasHeldDown() && !inc_button_->IsDown())
    inner_slider_->SmallDec();
  if (inc_button_->WasHeldDown() && !dec_button_->IsDown())
    inner_slider_->SmallInc();
}

bool SliderFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  bool result = inner_slider_->OnKeyEvent(event, dt);
  bool dec_result = dec_button_->OnKeyEvent(event, dt);
  bool inc_result = inc_button_->OnKeyEvent(event, dt);
  if (inner_slider_->GetTabPosition() != 0) {
    result |= dec_result;
    if (event.IsPress() && large_dec_keys_.Find(event.key)) {
      inner_slider_->LargeDec();
      result = true;
      NewRelativePing(0, 0, 1, 1);
    }
  }
  if (inner_slider_->GetTabPosition() != inner_slider_->GetTotalSize() -
                                         inner_slider_->GetTabSize()) {
    result |= inc_result;
    if (event.IsPress() && large_inc_keys_.Find(event.key)) {
      inner_slider_->LargeInc();
      result = true;
      NewRelativePing(0, 0, 1, 1);
    }
  }
  return result;
}

void SliderFrame::RecomputeSize(int rec_width, int rec_height) {
  int width = renderer_->RecomputeWidth(direction_ == Horizontal);
  if (direction_ == Horizontal)
    rec_height = width;
  else
    rec_width = width;
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
}

void SliderFrame::OnChildPing(int x1, int y1, int x2, int y2, bool center) {
  if ((x1 == dec_button_->GetX() - GetX() && y1 == dec_button_->GetY() - GetY() &&
       inner_slider_->GetTabPosition() == 0) ||
      (x1 == inc_button_->GetX() - GetX() && y1 == inc_button_->GetY() - GetY() &&
       inner_slider_->GetTabPosition() == inner_slider_->GetTotalSize() -
                                          inner_slider_->GetTabSize()))
    return;
  SingleParentFrame::OnChildPing(x1, y1, x2, y2, center);
}
