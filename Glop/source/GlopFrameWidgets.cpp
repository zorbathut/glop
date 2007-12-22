// Includes
#include "../include/GlopFrameWidgets.h"
#include "../include/Font.h"
#include "../include/GlopWindow.h"
#include "../include/Image.h"
#include "../include/OpenGl.h"
#include "../include/System.h"
#include "../include/Utils.h"

// Constants
const int kTextPromptCursorCycleTime = 800;
const int kTextPromptCursorFadeTime = 80;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Basic widgets
// =============

void ArrowImageFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), (ArrowView::Direction)direction_);
}
void ArrowImageFrame::RecomputeSize(int rec_width, int rec_height) {
  int width, height;
  view_->OnResize(rec_width, rec_height, (ArrowView::Direction)direction_, &width, &height);
  SetSize(width, height);
}

void SolidBoxFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  int p = (has_outer_part_? 1 : 0);
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  if (GetChild() != 0)
    GetChild()->SetPosition(screen_x + p, screen_y + p, cx1, cy1, cx2, cy2);
}
void SolidBoxFrame::RecomputeSize(int rec_width, int rec_height) {
  int p = (has_outer_part_? 1 : 0);
  if (GetChild() != 0) {
    GetChild()->UpdateSize(rec_width - 2*p, rec_height - 2*p);
    SetSize(GetChild()->GetWidth() + 2*p, GetChild()->GetHeight() + 2*p);
  } else {
    SetSize(rec_width, rec_height);
  }
}
void SolidBoxFrame::Render() const {
  GlUtils2d::FillRectangle(GetX(), GetY(), GetX2(), GetY2(), inner_color_);
  if (has_outer_part_)
    GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), outer_color_);
  GlUtils::SetColor(kWhite);
  SingleParentFrame::Render();
}

void HollowBoxFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  if (GetChild() != 0)
    GetChild()->SetPosition(screen_x + 1, screen_y + 1, cx1, cy1, cx2, cy2);
}
void HollowBoxFrame::RecomputeSize(int rec_width, int rec_height) {
  if (GetChild() != 0) {
    GetChild()->UpdateSize(rec_width - 2, rec_height - 2);
    SetSize(GetChild()->GetWidth() + 2, GetChild()->GetHeight() + 2);
  } else {
    SetSize(rec_width, rec_height);
  }
}
void HollowBoxFrame::Render() const {
  GlUtils2d::DrawRectangle(GetX(), GetY(), GetX2(), GetY2(), color_);
  GlUtils::SetColor(kWhite);
  SingleParentFrame::Render();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TextFrame
// =========

TextFrame::TextFrame(const string &text, const TextStyle &style)
: text_(text), text_style_(style), renderer_(0) {}

TextFrame::~TextFrame() {
  if (renderer_ != 0)
    TextRenderer::FreeRef(renderer_);
}

int TextFrame::GetFontPixelHeight(float height) {
  return int(gWindow->GetHeight() * height);
}

void TextFrame::Render() const {
  if (renderer_ != 0 && text_.size() > 0) {
    renderer_->Print(GetX(), GetY(), text_);
    GlUtils::SetColor(kWhite);
  }
}

void TextFrame::RecomputeSize(int rec_width, int rec_height) {
  // Obtain a new font if need be
  TextRenderer *new_renderer = 0;
  if (text_style_.font != 0) {
    new_renderer = text_style_.font->AddRef(GetFontPixelHeight(text_style_.size),
                                            text_style_.color, text_style_.flags); 
  }
  if (renderer_ != 0)
    TextRenderer::FreeRef(renderer_);
  renderer_ = new_renderer;
  
  // Compute our size after the font switch
  if (renderer_ != 0)
    SetSize(renderer_->GetTextWidth(text_), renderer_->GetFullHeight());
  else
    SetSize(0, 0);
}

// FpsFrame
// ========

void FpsFrame::Think(int dt) {
  text()->SetText(Format("%.2f fps", gSystem->GetFps()));
}

// FancyTextFrame
// ==============

FancyTextFrame::FancyTextFrame(const string &text, const TextStyle &style)
: MultiParentFrame(), text_(text), base_horz_justify_(0.5f), text_style_(style),
  add_soft_returns_(true) {}

FancyTextFrame::FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                               const TextStyle &style)
: MultiParentFrame(), text_(text), base_horz_justify_(horz_justify), text_style_(style),
  add_soft_returns_(add_soft_returns) {}

// Parsing utilities. A ParseStatus gives the position and style of text at a given location in
// our string. When we start parsing, a ParseStatus also maintains a TextRenderer for the
FancyTextFrame::ParseStatus FancyTextFrame::CreateParseStatus() {
  ParseStatus result;
  result.pos = 0;
  result.horz_justify = base_horz_justify_;
  result.style = text_style_;
  result.renderer = 0;
  return result;
}

void FancyTextFrame::StartParsing(ParseStatus *status) {
  status->renderer = status->style.font->AddRef(
    TextFrame::GetFontPixelHeight(status->style.size), status->style.color, status->style.flags);
}

void FancyTextFrame::StopParsing(ParseStatus *status) {
  TextRenderer::FreeRef(status->renderer);
  status->renderer = 0;
}

// Given a string to parse, and our current status, this read the next ASCII character in the
// string (and returns it via ch), and updates status as well. Possible return types:
//  - Normal: A character (possibly 0) was read, and the text style was not changed.
//  - NewRenderer: A character (possibly 0) was read, but the character is in a new text style,
//      which is now reflected in status. We stop parsing in the old style, and start with the
//      new style.
//  - Error: There was a parsing error. We also stop parsing.
//
// Note this maintains the following invariants:
//  - status.pos gives the first character index AFTER ch
//  - status.style, etc. gives the status FOR ch
FancyTextFrame::ParseResult FancyTextFrame::ParseNextCharacter(
  const string &s, ParseStatus *status, char *ch) {
  // Handle regular characters
  if (s[status->pos] != '\1') {
    *ch = s[status->pos++];
    return Normal;
  }

  // Handle tags
  StopParsing(status);
  while (s[status->pos] == '\1') {
    // Read until the end of the tag
    int pos2;
    for (pos2 = status->pos+1; s[pos2] != 0 && s[pos2] != '\1'; pos2++);
    if (pos2 == status->pos+1 || s[status->pos] == 0)
      return Error;

    // Read the tag
    string tag = s.substr(status->pos + 1, pos2 - status->pos -1);
    switch (tag[0]) {
      // Bold, italics, underline
      case 'b':
      case 'i':
      case 'u':
      case '/':
        for (int i = 0; i < (int)tag.size(); i++) {
          if (tag[i] == 'b') {
            status->style.flags |= kFontBold;
          } else if (tag[i] == 'i') {
            status->style.flags |= kFontItalics;
          } else if (tag[i] == 'u') {
            status->style.flags |= kFontUnderline;
          } else if (tag[i] == '/') {
            if (tag[i+1] == 'b') status->style.flags &= (~kFontBold);
            else if (tag[i+1] == 'i') status->style.flags &= (~kFontItalics);
            else if (tag[i+1] == 'u') status->style.flags &= (~kFontUnderline);
            else return Error;
            i++;
          } else {
            return Error;
          }
        }
        break;
      // Color
      case 'c':
        if (tag.size() != 7 && tag.size() != 9)
          return Error;
        int c[4];
        if (!ToInt(tag.substr(1, 2), &c[0], 16, true) ||
            !ToInt(tag.substr(3, 2), &c[1], 16, true) ||
            !ToInt(tag.substr(5, 2), &c[2], 16, true))
          return Error;
        if (tag.size() == 7)
          c[3] = 255;
        else if (!ToInt(tag.substr(7, 2), &c[3], 16, true))
          return Error;
        status->style.color = Color(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f);
        break;
      // Font
      case 'f':
        if (!ToPointer(tag.substr(1), (void**)&status->style.font))
          return Error;
        break;
      // Justify
      case 'j':
        if (!ToFloat(tag.substr(1), &status->horz_justify)) return Error;
        break;
      // Size
      case 's':
        if (!ToFloat(tag.substr(1), &status->style.size))
          return Error;
        status->style.size *= text_style_.size;
        break;
      default:
        return Error;
    }
    status->pos = pos2 + 1;
  }

  // Read the next ASCII character
  *ch = s[status->pos++];
  StartParsing(status);
  return NewRenderer;
}

void FancyTextFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  for (int i = 0; i < (int)text_blocks_.size(); i++)
  for (int j = 0; j < (int)text_blocks_[i].size(); j++) {
    GetChild(text_blocks_[i][j].child_id)->SetPosition(screen_x + text_blocks_[i][j].x,
                                                       screen_y + text_blocks_[i][j].y,
                                                       cx1, cy1, cx2, cy2);
  }
}

// Rebuilds the fancy text frame as a collection of standard text frames.
void FancyTextFrame::RecomputeSize(int rec_width, int rec_height) {
  int lines = 0;

  // If soft returns are enabled, do a first pass to add them. Regardless, we output text2, which
  // is formatted identically to text_, except that soft returns are manually added. We also count
  // the number of output lines.
  string text2;
  if (add_soft_returns_) {
    bool is_done = false;
    ParseStatus status = CreateParseStatus();

    // Loop through each line
    while (!is_done) {
      int start_pos = status.pos, x = 0;
      bool is_soft_return;
      StartParsing(&status);
      ParseStatus word_start_status = status, dash_status = status;

      // Read a line of text, maintaining the following:
      //  - status: The parse status as of the last character we successfully read
      //  - look_ahead_status: The parse status after reading the current character
      //  - word_start_status: The parse status after reading the most recent space
      //  - dash_status: The parse status after reading the last character that can be appended
      //      with a '-' and not extend outside the view size.
      while (1) {
        char ch;

        // Get the next character and check for errors
        ParseStatus look_ahead_status = status;
        if (ParseNextCharacter(text_, &look_ahead_status, &ch) == Error) {
          text_blocks_.clear();
          ClearChildren();
          MultiParentFrame::RecomputeSize(rec_width, rec_height);
          return;
        }

        // Check for end-lines
        if (ch == 0 || ch == '\n') {
          status = look_ahead_status;
          is_soft_return = false;
          is_done = (ch == 0);
          StopParsing(&look_ahead_status);
          break;
        }

        // Update our values and break early. Regardless, status should represent that active
        // parser when we leave this loop.
        if (ch == ' ')
          word_start_status = status;
        if (x + look_ahead_status.renderer->GetCharWidth('-', false, true) <= rec_width)
          dash_status = look_ahead_status;
        if (x + look_ahead_status.renderer->GetCharWidth(ch, x == 0, true) > rec_width) {
          is_soft_return = true;
          StopParsing(&look_ahead_status);
          break;
        }
        x += look_ahead_status.renderer->GetCharWidth(ch, x == 0, false);
        status = look_ahead_status;
      }
      lines++;
      
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

  // Build the new frames. We postpone deleting the old frames, so that their fonts don't get
  // deleted yet. This means that if we still need those fonts, we can get them for free.
  vector<vector<TextFrame*> > new_frames(lines);
  vector<float> row_justify(lines);
  ParseStatus status = CreateParseStatus();
  StartParsing(&status);
  string cur_part = "";
  for (int row_num = 0; row_num < lines; row_num++) {
    
    // Build TextFrame's for this row
    bool is_row_justify_fixed = false;
    while (1) {
      char ch;
      ParseStatus old_status = status;

      // Get the next character and check for errors
      ParseResult parse_result = ParseNextCharacter(text2, &status, &ch);
      if (parse_result == Error) {
        for (int i = 0; i < (int)new_frames.size(); i++)
        for (int j = 0; j < (int)new_frames[i].size(); j++)
          delete new_frames[i][j];
        text_blocks_.clear();
        ClearChildren();
        MultiParentFrame::RecomputeSize(rec_width, rec_height);
        return;
      }

      // Handle new text blocks
      if (ch == 0 || ch == '\n' || parse_result == NewRenderer)
      if (cur_part != "" || ch == 0 || ch == '\n') {
        new_frames[row_num].push_back(new TextFrame(cur_part, old_status.style));
        cur_part = "";
        if (ch == 0 || ch == '\n')
          break;
      }

      // Move to the next character
      if (!is_row_justify_fixed)
        row_justify[row_num] = status.horz_justify;
      cur_part += ch;
      is_row_justify_fixed = true;
    }
  }
  StopParsing(&status);

  // Update our children.
  // Pass #1: Add the frames as children, and calculate each frame dx, each row width, and each
  //  row ascent.
  text_blocks_.clear();
  ClearChildren();
  text_blocks_.resize(lines);
  vector<int> row_ascent(lines, 0), row_width(lines, 0);
  vector<vector<int> > frame_dx(lines);
  int total_width = 0;
  for (int i = 0; i < lines; i++) {
    for (int j = 0; j < (int)new_frames[i].size(); j++) {
      TextFrame *frame = new_frames[i][j];
      TextBlock block;
      block.child_id = AddChild(frame);
      text_blocks_[i].push_back(block);
      frame->UpdateSize(rec_width, rec_height);
      frame_dx[i].push_back(frame->GetRenderer()->GetTextWidth(frame->GetText(), j == 0,
                                                               j+1 == (int)new_frames[i].size()));
      row_width[i] += frame_dx[i][j];
      row_ascent[i] = max(row_ascent[i],  frame->GetRenderer()->GetAscent());
    }
    total_width = max(total_width, row_width[i]);
  }

  // Pass #2: Position all children.
  int row_pos = 0;
  for (int i = 0; i < lines; i++) {
    int x = int(row_justify[i] * (total_width - row_width[i])), next_row_pos = row_pos;
    for (int j = 0; j < (int)new_frames[i].size(); j++) {
      TextFrame *frame = new_frames[i][j];
      int y = row_pos + row_ascent[i] - frame->GetRenderer()->GetAscent();
      text_blocks_[i][j].x = x;
      text_blocks_[i][j].y = y;
      x += frame_dx[i][j];
      next_row_pos = max(next_row_pos, y + frame->GetRenderer()->GetFullHeight());
    }
    row_pos = next_row_pos;
  }
  SetSize(total_width, row_pos);
}

// AbstractTextPromptFrame
// =======================

void AbstractTextPromptFrame::Render() const {
  SingleParentFrame::Render();

  if (IsInFocus()) {
    // Determine the cursor color (including translucency)
    Color cursor_color((text()->GetStyle().color + kBlue) / 2);
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
    text()->GetRenderer()->Print(GetX() + char_x_[cursor_pos_], GetY(), "|");
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
  SetSize(GetWidth() + text()->GetRenderer()->GetCharWidth('|', true, true), GetHeight());
  char_x_.clear();
  char_x_.push_back(0);
  for (int i = 0; i < (int)text()->GetText().size(); i++) {
    int dx = text()->GetRenderer()->GetCharWidth(text()->GetText()[i], i == 0, false);
    char_x_.push_back(char_x_[i] + dx);
  }
}

// BasicTextPromptFrame
// ====================

void BasicTextPromptFrame::Render() const {
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
: AbstractTextPromptFrame(text, style->text_style),
  stored_value_(text),
  is_tracking_mouse_(false),
  selection_anchor_(GetCursorPos()),
  selection_color_(style->prompt_highlight_color) {}

BasicTextPromptFrame::BasicTextPromptFrame(const string &text, const TextStyle &text_style,
                                           const Color &selection_color)
: AbstractTextPromptFrame(text, text_style),
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
  return (ch >= 32 && ch <= 126) &&
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

WindowFrame::WindowFrame(GlopFrame *inner_frame, const string &title,
                         const WindowViewFactory *factory)
: SingleParentFrame(0), view_(factory->Create()) {
  padded_title_frame_ = new PaddedFrame(new TextFrame(title, view_->GetTitleStyle()), 0);
  padded_inner_frame_ = new PaddedFrame(inner_frame, 0);
  SetChild(new ColFrame(padded_title_frame_, CellSize::Default(), CellSize::Default(),
                        padded_inner_frame_, CellSize::Default(), CellSize::Max(), kJustifyLeft));
}

WindowFrame::WindowFrame(GlopFrame *inner_frame, const WindowViewFactory *factory)
: SingleParentFrame(0), view_(factory->Create()), padded_title_frame_(0) {
  padded_inner_frame_ = new PaddedFrame(inner_frame, 0);
  SetChild(padded_inner_frame_);
}

void WindowFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), padded_title_frame_, padded_inner_frame_);
}

void WindowFrame::RecomputeSize(int rec_width, int rec_height) {
  int title_l, title_t, title_r, title_b, inner_l, inner_t, inner_r, inner_b;
  view_->OnResize(rec_width, rec_height, padded_title_frame_ != 0,
                  &title_l, &title_t, &title_r, &title_b,
                  &inner_l, &inner_t, &inner_r, &inner_b);
  if (padded_title_frame_ != 0)
    padded_title_frame_->SetPadding(title_l, title_t, title_r, title_b);
  padded_inner_frame_->SetPadding(inner_l, inner_t, inner_r, inner_b);
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ButtonWidget
// ============

void AbstractButtonFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), is_down_, IsPrimaryFocus(),
                (PaddedFrame*)GetChild());
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
  DirtySize();
  if (is_down_) {
    NewRelativePing(0, 0, 1, 1);
    held_down_queued_ = true;
    held_down_repeat_delay_ = (down_type == Down? kRepeatDelay : kRepeatRate);
  } else if (down_type == UpFullRelease) {
    full_press_queued_ = true;
  }
}

void AbstractButtonFrame::RecomputeSize(int rec_width, int rec_height) {
  int lp, tp, rp, bp;
  view_->OnResize(rec_width, rec_height, is_down_, &lp, &tp, &rp, &bp);
  ((PaddedFrame*)GetChild())->SetPadding(lp, tp, rp, bp);
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
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

  // Set whether we are up or down based on the key event.
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

SliderFrame::SliderFrame(Direction direction, int tab_size, int total_size, int position,
                         bool has_arrow_hot_keys, int step_size, const SliderViewFactory *factory)
: MultiParentFrame(),
  direction_(direction),
  tab_logical_position_(position),
  tab_logical_size_(tab_size),
  total_logical_size_(total_size),
  step_size_(step_size == -1? (tab_size + 9) / 10 : step_size),  
  mouse_lock_mode_(None),
  bar_pixel_length_(0),
  view_(factory->Create()) {

  // Create the buttons
  if (direction == Horizontal) {
    dec_button_ = new DefaultButtonFrame(new ArrowImageFrame(ArrowImageFrame::Left,
      view_->GetArrowViewFactory()), view_->GetButtonViewFactory());
    inc_button_ = new DefaultButtonFrame(new ArrowImageFrame(ArrowImageFrame::Right,
      view_->GetArrowViewFactory()), view_->GetButtonViewFactory());
  } else {
    dec_button_ = new DefaultButtonFrame(new ArrowImageFrame(ArrowImageFrame::Up,
      view_->GetArrowViewFactory()), view_->GetButtonViewFactory());
    inc_button_ = new DefaultButtonFrame(new ArrowImageFrame(ArrowImageFrame::Down,
      view_->GetArrowViewFactory()), view_->GetButtonViewFactory());
  }
  AddChild(dec_button_);
  AddChild(inc_button_);

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

void SliderFrame::SetTabPosition(int position) {
  tab_logical_position_ = min(max(position, 0), total_logical_size_ - tab_logical_size_);
  RecomputeTabScreenPosition();
}

void SliderFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), direction_ == Horizontal,
                GetParent()->IsPrimaryFocus(), tab_x1_ + inner_bar_x_, tab_y1_ + inner_bar_y_,
                tab_x2_ + inner_bar_x_, tab_y2_ + inner_bar_y_, dec_button_, inc_button_);
}

void SliderFrame::Think(int dt) {
  MultiParentFrame::Think(dt);
  if (dec_button_->WasHeldDown() && !inc_button_->IsDown())
    SmallDec();
  if (inc_button_->WasHeldDown() && !dec_button_->IsDown())
    SmallInc();
}

bool SliderFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  // Delegate to the decrement and increment buttons
  bool result = false;
  bool dec_result = dec_button_->OnKeyEvent(event, dt);
  bool inc_result = inc_button_->OnKeyEvent(event, dt);

  // Handle decrement requests - do not mark a key as used if it is used by a currently irrelevant
  // decrement button
  if (tab_logical_position_ != 0) {
    result |= dec_result;
    if (event.IsPress() && large_dec_keys_.Find(event.key)) {
      LargeDec();
      result = true;
      NewRelativePing(0, 0, 1, 1);
    }
  }

  // Handle increment requests - do not mark a key as used if it is used by a currently irrelevant
  // increment button
  if (tab_logical_position_ != total_logical_size_ - tab_logical_size_) {
    result |= inc_result;
    if (event.IsPress() && large_inc_keys_.Find(event.key)) {
      LargeInc();
      result = true;
      NewRelativePing(0, 0, 1, 1);
    }
  }

  // Compute the mouse pixel position on the slider
  int mouse_pixel, tab_start_pixel, tab_end_pixel;
  if (direction_ == Horizontal) {
    tab_start_pixel = tab_x1_;
    tab_end_pixel = tab_x2_;
    mouse_pixel = input()->GetMouseX() - inner_bar_x_;
  } else {
    tab_start_pixel = tab_y1_;
    tab_end_pixel = tab_y2_;
    mouse_pixel = input()->GetMouseY() - inner_bar_y_;
  }

  // Handle mouse first clicks
  if (event.key == kMouseLButton && event.IsNonRepeatPress() &&
      IsPointVisible(input()->GetMouseX(), input()->GetMouseY()) &&
      mouse_pixel >= 0 && mouse_pixel < bar_pixel_length_) {
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
  result |= (event.key == kMouseLButton);
  return result;
}

void SliderFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  dec_button_->SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  inc_button_->SetPosition(screen_x + GetWidth() - inc_button_->GetWidth(),
                           screen_y + GetHeight() - inc_button_->GetHeight(), cx1, cy1, cx2, cy2);
  if (direction_ == Horizontal) {
    inner_bar_x_ = dec_button_->GetX2() + 1;
    inner_bar_y_ = screen_y;
  } else {
    inner_bar_x_ = screen_x;
    inner_bar_y_ = dec_button_->GetY2() + 1;
  }
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

void SliderFrame::RecomputeSize(int rec_width, int rec_height) {
  int width = view_->GetWidthOnResize(rec_width, rec_height, direction_ == Horizontal);
  int bar_pixel_length;
  if (direction_ == Horizontal) {
    dec_button_->UpdateSize(rec_width/2, width);
    inc_button_->UpdateSize(rec_width/2, width);
    bar_pixel_length = rec_width - dec_button_->GetWidth() - inc_button_->GetWidth();
    SetSize(rec_width, width);
  } else {
    dec_button_->UpdateSize(width, rec_height/2);
    inc_button_->UpdateSize(width, rec_height/2);
    bar_pixel_length = rec_height - dec_button_->GetHeight() - inc_button_->GetHeight();
    SetSize(width, rec_height);
  }
  if (bar_pixel_length != bar_pixel_length_) {
    bar_pixel_length_ = bar_pixel_length;
    mouse_lock_mode_ = None;
    RecomputeTabScreenPosition();
  }
}

void SliderFrame::OnChildPing(GlopFrame *child, int x1, int y1, int x2, int y2, bool center) {
  if ((child == dec_button_ && tab_logical_position_ == 0) ||
      (child == inc_button_ && tab_logical_position_ == total_logical_size_ - tab_logical_size_))
    return;
  MultiParentFrame::OnChildPing(child, x1, y1, x2, y2, center);
}

void SliderFrame::OnFocusChange() {
  if (!IsInFocus())
    mouse_lock_mode_ = None;
  MultiParentFrame::OnFocusChange();
}

// Given a logical position, this returns the pixel on the slider where that position starts
int SliderFrame::GetLocationPixelStart(int pos) const {
  return pos * (bar_pixel_length_ - tab_pixel_length_) / (total_logical_size_ - tab_logical_size_);
}

// Given a pixel on the slider, this returns the corresponding logical position. We are extra
// careful to make sure rounding does not not prevent total consistency with
// getLocationPixelStart. Otherwise, the scroll bar can have weird jumps when it is being
// dragged.
int SliderFrame::GetLocationByPixel(int pixel_location) {
  if (bar_pixel_length_ <= tab_pixel_length_)
    return 0;	// A truly degenerate case
  int pos = pixel_location * (total_logical_size_ - tab_logical_size_) /
            (bar_pixel_length_ - tab_pixel_length_) - 1;
  while (GetLocationPixelStart(pos+1) <= pixel_location)
    pos++;
  return pos;
}

// Recomputes the screen coordinates for the tab rectangle
void SliderFrame::RecomputeTabScreenPosition() {
  int min_tab_length;
  if (direction_ == Horizontal) {
    min_tab_length = view_->GetMinTabLengthOnResize(
      GetWidth() - dec_button_->GetWidth() - inc_button_->GetWidth(), GetHeight(), true);
  } else {
    min_tab_length = view_->GetMinTabLengthOnResize(
      GetWidth(), GetHeight() - dec_button_->GetHeight() - inc_button_->GetHeight(), false);
  }

  // Compute the length of the tab
  if (tab_logical_size_ < total_logical_size_) {
    tab_pixel_length_ = max(tab_logical_size_ * (bar_pixel_length_ - 1) / total_logical_size_,
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
