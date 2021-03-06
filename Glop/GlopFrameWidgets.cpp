// Includes
#include "GlopFrameWidgets.h"
#include "Font.h"
#include "GlopWindow.h"
#include "Image.h"
#include "OpenGl.h"
#include "System.h"
#include "Utils.h"

#ifndef GLOP_LEAN_AND_MEAN

// Constants
const int kSearchTermResetTime = 300;

// HotKeyTracker
// =============
KeyEvent::Type HotKeyTracker::RemoveHotKey(ListId id) {
  // Recalculate the state from scratch. Trying to remove pressed keys can be awkward if, for
  // example, we are removing kAnyKey as a hot key.
  hot_keys_.erase(id);
  for (List<GlopKey>::iterator it = down_hot_keys_.begin(); it != down_hot_keys_.end(); ) {
    bool is_still_good = false;
    for (List<GlopKey>::iterator it2 = hot_keys_.begin(); it2 != hot_keys_.end(); ++it2)
    if (IsMatchingKey(*it2, *it)) {
      is_still_good = true;
      break;
    }
    if (!is_still_good)
      it = down_hot_keys_.erase(it);
    else
      ++it;
  }
  KeyEvent::Type result = KeyEvent::Nothing;
  if (key_state_.IsDownNow() && down_hot_keys_.size() == 0)
    result = KeyEvent::Release;
  return result;
}

bool HotKeyTracker::OnKeyEvent(const KeyEvent &event, KeyEvent::Type *result) {
  // Handle time passing
  *result = KeyEvent::Nothing;
  if (event.dt > 0) {
    key_state_.OnDt(event.dt);
    return false;
  }

  // Handle presses - note that we store event.key(), not the hot key in down_hot_keys. This helps
  // us handle releases if kAnyKey is a hot key.
  bool key_used = false;
  if (event.IsPress()) {
    for (List<GlopKey>::iterator it = hot_keys_.begin(); it != hot_keys_.end(); ++it)
    for (int i = 0; i < (int)event.keys.size(); i++)
    if (IsMatchingKey(*it, event.keys[i])) {
      key_used = true;
      if (event.IsNonRepeatPress())
        down_hot_keys_.push_back(event.keys[i]);
    }
  }
  
  // Handle releases
  else if (event.IsRelease()) {
    for (int i = 0; i < (int)event.keys.size(); i++)
    for (List<GlopKey>::iterator it = down_hot_keys_.begin(); it != down_hot_keys_.end();) {
      if (IsMatchingKey(*it, event.keys[i])) {
        key_used = true;
        it = down_hot_keys_.erase(it);
      } else {
        ++it;
      }
    }
  }

  // Change our state
  if (key_used) {
    key_state_.SetIsDown(down_hot_keys_.size() > 0, false);
    if (key_state_.IsDownNow() || event.type != KeyEvent::Release)
      *result = event.type;
  }
  return key_used;
}

KeyEvent::Type HotKeyTracker::Clear() {
  down_hot_keys_.clear();
  return key_state_.SetIsDown(false, true);
}

bool HotKeyTracker::IsFocusMagnet(const KeyEvent &event) const {
  for (List<GlopKey>::const_iterator it = hot_keys_.begin(); it != hot_keys_.end(); ++it)
  for (int i = 0; i < (int)event.keys.size(); i++)
  if (IsMatchingKey(*it, event.keys[i]))
    return true;
  return false;
}

bool HotKeyTracker::IsMatchingKey(const GlopKey &hot_key, const GlopKey &key) const {
  if (hot_key == kAnyKey)
    return !key.IsModifierKey() && !key.IsMotionKey() && !key.IsDerivedKey();
  else
    return key == hot_key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Basic widgets
// =============

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
  SingleParentFrame::Render();
}

void InputBoxFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), (const PaddedFrame*)GetChild());
}

void InputBoxFrame::RecomputeSize(int rec_width, int rec_height) {
  int lp, tp, rp, bp;
  view_->OnResize(rec_width, rec_height, &lp, &tp, &rp, &bp);
  ((PaddedFrame*)GetChild())->SetPadding(lp, tp, rp, bp);
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
}

ImageFrame::ImageFrame(InputStream input, const Color &bg_color, int bg_tolerance,
                       const Color &color) {
  Init(Texture::Load(input, bg_color, bg_tolerance), true, color);
}
ImageFrame::ImageFrame(InputStream input, const Color &color) {
  Init(Texture::Load(input), true, color);
}
ImageFrame::ImageFrame(const Image *image, const Color &color) {
  Init(new Texture(image), true, color);
}
ImageFrame::ImageFrame(const Texture *texture, const Color &color) {
  Init(texture, false, color);
}

ImageFrame::~ImageFrame() {
  if (is_texture_owned_)
    delete texture_;
}

void ImageFrame::Init(const Texture *texture, bool is_texture_owned, const Color &color) {
  ASSERT(texture != 0);
  texture_ = texture;
  is_texture_owned_ = is_texture_owned;
  color_ = color;
}

void ImageFrame::Render() const {
  GlUtils2d::RenderTexture(GetX(), GetY(), GetX2(), GetY2(), texture_, color_);
}

void ImageFrame::RecomputeSize(int rec_width, int rec_height) {
  SetToMaxSize(rec_width, rec_height, float(texture_->GetWidth()) / texture_->GetHeight());
}

TiledTextureFrame::TiledTextureFrame(InputStream input, const Color &bg_color,
                                     int bg_tolerance, const Color &color) {
  Init(Texture::Load(input, bg_color, bg_tolerance), true, color);
}
TiledTextureFrame::TiledTextureFrame(InputStream input, const Color &color) {
  Init(Texture::Load(input), true, color);
}
TiledTextureFrame::TiledTextureFrame(const Image *image, const Color &color) {
  Init(new Texture(image), true, color);
}
TiledTextureFrame::TiledTextureFrame(const Texture *texture, const Color &color) {
  Init(texture, false, color);
}

TiledTextureFrame::~TiledTextureFrame() {
  if (is_texture_owned_)
    delete texture_;
}

void TiledTextureFrame::Init(const Texture *texture, bool is_texture_owned, const Color &color) {
  ASSERT(texture != 0);
  texture_ = texture;
  is_texture_owned_ = is_texture_owned;
  color_ = color;
}

void TiledTextureFrame::Render() const {
  GlUtils2d::TileRectangle(GetX(), GetY(), GetX2(), GetY2(), texture_, color_);
}

void ArrowFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), (ArrowView::Direction)direction_);
}
void ArrowFrame::RecomputeSize(int rec_width, int rec_height) {
  int width, height;
  view_->OnResize(rec_width, rec_height, (ArrowView::Direction)direction_, &width, &height);
  SetSize(width, height);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TextFrame
// =========

TextFrame::TextFrame(const string &text, const GuiTextStyle &style)
: text_(text), text_style_(style), renderer_(0) {
  ASSERT(style.font != 0);  // Most likely user forgot to call InitDefaultFrameStyle or equivalent
}

TextFrame::~TextFrame() {
  if (renderer_ != 0)
    TextRenderer::FreeRef(renderer_);
}

int TextFrame::GetFontPixelHeight(float height) {
  return int(window()->GetHeight() * height);
}

void TextFrame::Render() const {
  if (renderer_ != 0 && text_.size() > 0)
    renderer_->Print(GetX(), GetY(), text_, text_style_.color);
}

void TextFrame::RecomputeSize(int rec_width, int rec_height) {
  // Obtain a new font if necessary
  TextRenderer *new_renderer = 0;
  if (text_style_.font != 0) {
    new_renderer = text_style_.font->AddRef(GetFontPixelHeight(text_style_.size),
                                            text_style_.flags); 
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
  text()->SetText(Format("%.2f fps", system()->GetFps()));
}

// FancyTextFrame
// ==============

FancyTextFrame::FancyTextFrame(const string &text, const GuiTextStyle &style)
: MultiParentFrame(), text_(text), base_horz_justify_(0.5f), text_style_(style),
  add_soft_returns_(true) {
  ASSERT(style.font != 0);  // Most likely user forgot to call InitDefaultFrameStyle or equivalent
}

FancyTextFrame::FancyTextFrame(const string &text, bool add_soft_returns, float horz_justify,
                               const GuiTextStyle &style)
: MultiParentFrame(), text_(text), base_horz_justify_(horz_justify), text_style_(style),
  add_soft_returns_(add_soft_returns) {
  ASSERT(style.font != 0);  // Most likely user forgot to call InitDefaultFrameStyle or equivalent
}

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

// Utilities for parsing. Note that we keep many parsers "started" at the same time. In effect,
// this means we keeps the font bitmaps in memory until they are transferred over to TextFrames.
void FancyTextFrame::StartParsing(ParseStatus *status, vector<ParseStatus> *active_parsers) {
  status->renderer = status->style.font->AddRef(TextFrame::GetFontPixelHeight(status->style.size),
                                                status->style.flags);
  active_parsers->push_back(*status);
}

void FancyTextFrame::StopParsing(vector<ParseStatus> *active_parsers) {
  for (int i = 0; i < (int)active_parsers->size(); i++)
    TextRenderer::FreeRef((*active_parsers)[i].renderer);
  active_parsers->clear();
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
  const string &s, ParseStatus *status, vector<ParseStatus> *active_parsers, char *ch) {
  // Handle regular characters
  if (s[status->pos] != '\1') {
    *ch = s[status->pos++];
    return Normal;
  }

  // Handle tags
  while (s[status->pos] == '\1') {
    status->pos++;
    bool hit_end_tag = false;
    while (!hit_end_tag) {
      int pos2 = status->pos;
      switch (s[status->pos]) {
        // Handle bold/italics/underline
        case 'B':
          status->style.flags |= kFontBold;
          status->pos++;
          break;
        case 'I':
          status->style.flags |= kFontItalics;
          status->pos++;
          break;
        case 'U':
          status->style.flags |= kFontUnderline;
          status->pos++;
          break;
        case '/':
          if (s[status->pos+1] == 'B') status->style.flags &= (~kFontBold);
          else if (s[status->pos+1] == 'I') status->style.flags &= (~kFontItalics);
          else if (s[status->pos+1] == 'U') status->style.flags &= (~kFontUnderline);
          else return Error;
          status->pos += 2;
          break;

        // Handle color
        case 'C':
          pos2++;
          while ((s[pos2] >= '0' && s[pos2] <= '9') || (s[pos2] >= 'a' && s[pos2] <= 'f'))
            pos2++;
          if (pos2 - status->pos != 7 && pos2 - status->pos != 9)
            return Error;
          status->style.color = Color(ToInt(s.substr(status->pos + 1, 2), 16) / 255.0f,
                                      ToInt(s.substr(status->pos + 3, 2), 16) / 255.0f,
                                      ToInt(s.substr(status->pos + 5, 2), 16) / 255.0f, 1.0f);
          if (pos2 - status->pos == 9)
            status->style.color[3] = ToInt(s.substr(status->pos + 7, 2), 16) / 255.0f;
          status->pos = pos2;
          break;

        // Handle font
        case 'F':
          pos2++;
          while ((s[pos2] >= '0' && s[pos2] <= '9') || (s[pos2] >= 'a' && s[pos2] <= 'f'))
            pos2++;
          status->style.font = (Font*)ToPointer(s.substr(status->pos + 1, pos2 - status->pos - 1));
          status->pos = pos2;
          break;
         
        // Handle justification, sizing
        case 'J':
        case 'S':
          pos2++;
          while ((s[pos2] >= '0' && s[pos2] <= '9') || s[pos2] == '.')
            pos2++;
          if (s[status->pos] == 'J') {
            if (!ToFloat(s.substr(status->pos + 1, pos2 - status->pos - 1), &status->horz_justify))
              return Error;
          } else {
            if (!ToFloat(s.substr(status->pos + 1, pos2 - status->pos - 1), &status->style.size))
              return Error;
            status->style.size *= text_style_.size;
          }
          status->pos = pos2;
          break;

        // Handle other tags
        case 1:
          hit_end_tag = true;
          status->pos++;
          break;
        default:
          return Error;
      }
    }
  }

  // Read the next ASCII character
  *ch = s[status->pos++];
  StartParsing(status, active_parsers);
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
  vector<ParseStatus> active_parsers;
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
      StartParsing(&status, &active_parsers);
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
        if (ParseNextCharacter(text_, &look_ahead_status, &active_parsers, &ch) == Error) {
          StopParsing(&active_parsers);
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
  StartParsing(&status, &active_parsers);
  string cur_part = "";
  for (int row_num = 0; row_num < lines; row_num++) {
    
    // Build TextFrame's for this row
    bool is_row_justify_fixed = false;
    while (1) {
      char ch;
      ParseStatus old_status = status;

      // Get the next character and check for errors
      ParseResult parse_result = ParseNextCharacter(text2, &status, &active_parsers, &ch);
      if (parse_result == Error) {
        StopParsing(&active_parsers);
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
  StopParsing(&active_parsers);
}

// TextPromptFrame
// ===============

DummyTextPromptFrame::DummyTextPromptFrame(const string &text,
                                           const TextPromptView *view)
: SingleParentFrame(0),
  cursor_pos_(0), cursor_time_(0), selection_start_(0), selection_end_(0),
  left_padding_(0), top_padding_(0), right_padding_(0), view_(view) {
  SetChild(new TextFrame(text, view_->GetTextStyle()));
}

void DummyTextPromptFrame::SetText(const string &new_text) {
  if (new_text != text()->GetText()) {
    text()->SetText(new_text);
    SetCursorPos((int)new_text.size());
    SetSelection(0, 0);
  }
}

void DummyTextPromptFrame::SetCursorPos(int pos) {
  cursor_pos_ = max(min(pos, int(GetText().size())), 0);
  cursor_time_ = 0;
}

void DummyTextPromptFrame::SetSelection(int start, int end) {
  int x1 = min(start, end), x2 = max(start, end);
  selection_start_ = max(min(x1, int(GetText().size())), 0);
  selection_end_ = max(min(x2, int(GetText().size())), 0);
}

int DummyTextPromptFrame::PixelToBoundaryPosition(int x) const {
  int len = int(GetText().size());
  if (len == 0)
    return 0;
  x -= (left_padding_ + text()->GetRenderer()->GetCharWidth(GetText()[0], true, len == 1)/2);
  for (int i = 0; ; i++) {
    if (x <= 0) return i;
    if (i == len-1) return len;
    x -= ((text()->GetRenderer()->GetCharWidth(GetText()[i], i == 0, false)+1)/2 +
          text()->GetRenderer()->GetCharWidth(GetText()[i+1], false, i == len-2)/2);
  }
}

int DummyTextPromptFrame::PixelToCharacterPosition(int x) const {
  int len = int(GetText().size());
  if (len == 0)
    return 0;
  x -= (left_padding_ + text()->GetRenderer()->GetCharWidth(GetText()[0], true, len == 1));
  for (int i = 0; ; i++) {
    if (x <= 0) return i;
    if (i == len-2) return len-1;
    x -= text()->GetRenderer()->GetCharWidth(GetText()[i+1], false, false);
  }
}

void DummyTextPromptFrame::GetCursorExtents(int pos, int *x1, int *x2) const {
  int x = 0, len = int(GetText().size());
  for (int i = 0; i < pos; i++)
    x += text()->GetRenderer()->GetCharWidth(GetText()[i], i == 0, i == len-1);
  *x1 = x;
  *x2 = *x1 + left_padding_ + right_padding_ - 1;
}

void DummyTextPromptFrame::GetCharacterExtents(int pos, int *x1, int *x2) const {
  int x = 0, len = int(GetText().size());
  for (int i = 0; i < pos; i++)
    x += text()->GetRenderer()->GetCharWidth(GetText()[i], i == 0, i == len-1);
  *x1 = x + left_padding_;
  *x2 = *x1 + text()->GetRenderer()->GetCharWidth(GetText()[pos], pos == 0, pos == len-1) - 1;
}

void DummyTextPromptFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), cursor_pos_, (int*)&cursor_time_,
                selection_start_, selection_end_, IsInFocus(), text());
}

void DummyTextPromptFrame::Think(int dt) {
  cursor_time_ += dt;
  SingleParentFrame::Think(dt);
}

void DummyTextPromptFrame::SetPosition(int screen_x, int screen_y,
                                       int cx1, int cy1, int cx2, int cy2) {
  GetChild()->SetPosition(screen_x + left_padding_, screen_y + top_padding_, cx1, cy1, cx2, cy2);
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

void DummyTextPromptFrame::RecomputeSize(int rec_width, int rec_height) {
  GetChild()->UpdateSize(rec_width, rec_height);
  int bp;
  view_->OnResize(rec_width, rec_height, text(), &left_padding_, &top_padding_,
                  &right_padding_, &bp);
  SetSize(GetChild()->GetWidth() + left_padding_ + right_padding_,
          GetChild()->GetHeight() + top_padding_ + bp);
}

void DummyTextPromptFrame::OnFocusChange() {
  if (IsInFocus())
    cursor_time_ = 0;
  else
    selection_start_ = selection_end_ = 0;
  SingleParentFrame::OnFocusChange();
}

bool BaseTextPromptFrame::OnKeyEvent(const KeyEvent &event, bool gained_focus) {
  // Track mouse motion with the selection
  if (input()->IsKeyDownNow(kGuiKeyPrimaryClick)) {
    bool is_visible = IsPointVisibleInFocusFrame(input()->GetMouseX(), input()->GetMouseY());
    int mx = input()->GetMouseX() - GetX();

    // Handle double-clicks
    if (event.HasKey(kGuiKeyPrimaryClick) && event.IsDoublePress() && is_visible) {
      int pos = prompt()->PixelToCharacterPosition(mx), pos1 = GetPrevWordBoundary(pos),
          pos2 = GetNextWordBoundary(pos+1);
      selection_anchor_ = pos1;
      SetCursorPos(pos2, false);
      prompt()->SetSelection(pos1, pos2);
    }

    // Handle first clicks
    else if (event.HasKey(kGuiKeyPrimaryClick) && event.IsNonRepeatPress() && is_visible) {
      int pos = prompt()->PixelToBoundaryPosition(mx);
      is_tracking_mouse_ = true;
      selection_anchor_ = pos;
    }

    // Handle dragged selections, but only scroll slowly
    if (is_tracking_mouse_) {
      int pos = prompt()->PixelToBoundaryPosition(mx),
        min_pos = prompt()->PixelToBoundaryPosition(GetClipX1() - GetX()),
        max_pos = prompt()->PixelToBoundaryPosition(GetClipX2() - GetX());
      if (event.HasKey(kGuiKeyPrimaryClick) && event.IsPress()) {
        min_pos = max(min_pos - 1, 0);
        max_pos = min(max_pos + 1, (int)GetText().size());
      }
      pos = max(min(pos, max_pos), min_pos);
      SetCursorPos(pos, false);
      prompt()->SetSelection(selection_anchor_, pos);
      if (event.HasKey(kGuiKeyPrimaryClick)) {
        PingSelection();
        return true;
      }
    }
  } else {
    is_tracking_mouse_ = false;
  }

  // Handle all key presses
  if (event.IsPress()) {
    int ascii = input()->GetAsciiValue(event.GetMainKey());

    // Handle backspace and delete
    if (event.GetMainKey() == kKeyBackspace) {
      if (prompt()->IsSelectionActive())
        DeleteSelection();
      else if (GetCursorPos() > 0)
        DeleteCharacter(false);
      ReformText();
      PingSelection();
      return true;
    } else if (event.GetMainKey() == kKeyDelete) {
      if (prompt()->IsSelectionActive())
        DeleteSelection();
      else if (GetCursorPos() < (int)GetText().size())
        DeleteCharacter(true);
      ReformText();
      PingSelection();
      return true;
    }
    
    // Handle character insertion
    else if (ascii != 0 && CanInsertCharacter(ascii, true)) {
      if (prompt()->IsSelectionActive()) {
        int cursor_pos_cache = GetCursorPos(), selection_anchor_cache = selection_anchor_;
        int selection_start_cache, selection_end_cache;
        prompt()->GetSelection(&selection_start_cache, &selection_end_cache);
        string text_cache = GetText();
        DeleteSelection();
        if (CanInsertCharacter(ascii, false)) {
          InsertCharacter(ascii);
          ReformText();
        } else {
          prompt()->SetText(text_cache);
          prompt()->SetSelection(selection_start_cache, selection_end_cache);
          selection_anchor_ = selection_anchor_cache;
          SetCursorPos(cursor_pos_cache, false);
          selection_anchor_ = selection_anchor_cache;
        }
      } else if (CanInsertCharacter(ascii, false)) {
        InsertCharacter(ascii);
        ReformText();
      }
      PingSelection();
      return true;
    }

    // Handle cursor movement
    else {
      int new_cursor_pos = GetCursorPos();
      bool left_allowed = new_cursor_pos > 0 &&
                          !input()->IsKeyDownNow(kKeyRight) && !input()->IsKeyDownNow(kKeyEnd) &&
                          (!input()->IsNumLockSet() || !input()->IsKeyDownNow(kKeyPad6)) &&
                          (!input()->IsNumLockSet() || !input()->IsKeyDownNow(kKeyPad1));
      bool right_allowed = new_cursor_pos < (int)GetText().size() &&
                           !input()->IsKeyDownNow(kKeyLeft) && !input()->IsKeyDownNow(kKeyHome) &&
                           (!input()->IsNumLockSet() || !input()->IsKeyDownNow(kKeyPad4)) &&
                           (!input()->IsNumLockSet() || !input()->IsKeyDownNow(kKeyPad7));
      if (event.GetMainKey() == kKeyLeft || (event.GetMainKey() == kKeyPad4 &&
                                             !input()->IsNumLockSet())) {
        if (left_allowed) {
          new_cursor_pos = (input()->IsKeyDownFrame(kKeyEitherControl)?
                            GetPrevWordBoundary(new_cursor_pos-1) : new_cursor_pos - 1);
        }
      } else if (event.GetMainKey() == kKeyHome || (event.GetMainKey() == kKeyPad7 &&
                                                    !input()->IsNumLockSet())) {
        if (left_allowed)
          new_cursor_pos = 0;
      } else if (event.GetMainKey() == kKeyRight || (event.GetMainKey() == kKeyPad6 &&
                                                     !input()->IsNumLockSet())) {
        if (right_allowed) {
          new_cursor_pos = (input()->IsKeyDownFrame(kKeyEitherControl)?
                            GetNextWordBoundary(new_cursor_pos+1) : new_cursor_pos + 1);
        }
      } else if (event.GetMainKey() == kKeyEnd || (event.GetMainKey() == kKeyPad1 &&
                                                   !input()->IsNumLockSet())) {
        if (right_allowed)
          new_cursor_pos = int(GetText().size());
      } else {
        return false;
      }

      // Update the position
      SetCursorPos(new_cursor_pos, !input()->IsKeyDownNow(kKeyEitherShift));
      is_tracking_mouse_ = false;
      prompt()->SetSelection(selection_anchor_, new_cursor_pos);
      PingSelection();
      return true;
    }
  }
  return false;
}

BaseTextPromptFrame::BaseTextPromptFrame(const string &text, const TextPromptView *view)
: SingleParentFrame(new DummyTextPromptFrame(text, view)),
  is_tracking_mouse_(false), selection_anchor_(-1), focus_gain_behavior_(SelectAll) {}

void BaseTextPromptFrame::OnFocusChange() {
  if (IsInFocus()) {
    switch (focus_gain_behavior_) {
     case SelectAll:
      selection_anchor_ = 0;
      SetCursorPos(int(GetText().size()), false);
      prompt()->SetSelection(selection_anchor_, GetCursorPos());
      break;
     case CursorToStart:
      SetCursorPos(0, true);
      prompt()->SetSelection(selection_anchor_, GetCursorPos());
      PingSelection();
      break;
     case CursorToEnd:
      SetCursorPos(int(GetText().size()), true);
      prompt()->SetSelection(selection_anchor_, GetCursorPos());
      PingSelection();
      break;
    };
  }
  SingleParentFrame::OnFocusChange();
}

void BaseTextPromptFrame::SetText(const string &text) {
  prompt()->SetText(text);
  prompt()->SetSelection(0, 0);
  selection_anchor_ = GetCursorPos();
}

void BaseTextPromptFrame::PingSelection() {
  if (prompt()->IsSelectionActive()) {
    AddPing(new CharacterPing(this, selection_anchor_));
    AddPing(new CharacterPing(this, prompt()->GetCursorPos()));
  } else {
    AddPing(new CharacterPing(this, prompt()->GetCursorPos()));
  }
}

// Word boundaries are the start of the string, the end of the string, or any place we transition
// from white space to non-white space. pos is clamped to the ranged of the string, and then we
// next or previous word boundary, possibly including pos itself.
int BaseTextPromptFrame::GetPrevWordBoundary(int pos) const {
  pos = max(min(pos, int(GetText().size()) - 1), 0);
  while (pos < int(GetText().size()) && pos > 0 &&
         (GetText()[pos-1] != ' ' || GetText()[pos] == ' '))
    pos--;
  return pos;
}

int BaseTextPromptFrame::GetNextWordBoundary(int pos) const {
  pos = max(min(pos, int(GetText().size()) - 1), 0);
  while (pos < int(GetText().size()) && pos > 0 &&
         (GetText()[pos-1] != ' ' || GetText()[pos] == ' '))
    pos++;
  return pos;
}

void BaseTextPromptFrame::DeleteSelection() {
  int s1, s2;
  prompt()->GetSelection(&s1, &s2);
  string part1 = (s1 == 0? "" : GetText().substr(0, s1)),
         part2 = (s2 == GetText().size()? "" : GetText().substr(s2));
  SetText(part1 + part2);
  SetCursorPos(s1, true);
  is_tracking_mouse_ = false;
}

void BaseTextPromptFrame::DeleteCharacter(bool is_next_character) {
  int i = GetCursorPos() + (is_next_character? 0 : -1);
  string part1 = (i == 0? "" : GetText().substr(0, i)),
         part2 = (i+1 == GetText().size()? "" : GetText().substr(i+1));
  SetText(part1 + part2);
  SetCursorPos(i, true);
}

void BaseTextPromptFrame::InsertCharacter(char ch) {
  int i = GetCursorPos();
  string part1 = (i == 0? "" : GetText().substr(0, i)),
         part2 = (i == GetText().size()? "" : GetText().substr(i));
  SetText(part1 + ch + part2);
  SetCursorPos(i + 1, true);
}

void BaseTextPromptFrame::SetCursorPos(int pos, bool also_set_anchor) {
  prompt()->SetCursorPos(pos);
  if (also_set_anchor)
    selection_anchor_ = GetCursorPos();
}

void BaseTextPromptFrame::CharacterPing::GetCoords(int *x1, int *y1, int *x2, int *y2) {
  DummyTextPromptFrame *frame = ((BaseTextPromptFrame*)GetFrame())->prompt();
  int temp, len = (int)frame->GetText().size();
  int char_x1, char_x2, cur_x1, cur_x2;
  frame->GetCharacterExtents(max(i_ - 1, 0), &char_x1, &temp);
  frame->GetCharacterExtents(min(i_, len-1), &temp, &char_x2);
  frame->GetCursorExtents(i_, &cur_x1, &cur_x2);
  *x1 = min(char_x1, cur_x1);
  *x2 = max(char_x2, cur_x2);
  *y1 = 0;
  *y2 = frame->GetHeight();
}

StringPromptFrame::StringPromptFrame(const string &start_text, int length_limit,
                                     const TextPromptView *view)
: BaseTextPromptFrame(start_text, view), length_limit_(length_limit) {}

void StringPromptFrame::Set(const string &value) {
  if (length_limit_ < (int)value.size())
    SetText(value.substr(0, length_limit_));
  else
    SetText(value);
}

bool StringPromptFrame::CanInsertCharacter(char ch, bool in_theory) const {
  return (ch >= 32 && ch <= 126) &&
         (in_theory || (int)GetText().size() < length_limit_);
}

StringPromptWidget::StringPromptWidget(const string &start_text, int length_limit,
                                       float prompt_width,
                                       const TextPromptView *prompt_view,
                                       const InputBoxView *input_box_view)
: FocusFrame(new InputBoxFrame(new PaddedFrame(sizer_ = new ExactWidthFrame(
    prompt_ = new StringPromptFrame(start_text, length_limit, prompt_view), prompt_width),
    1), input_box_view)) {}

IntegerPromptFrame::IntegerPromptFrame(int start_value, int min_value, int max_value,
                                       const TextPromptView *view)
: BaseTextPromptFrame(Format("%d", start_value), view), min_value_(min_value),
  max_value_(max_value) {
  ReformText();
}

void IntegerPromptFrame::Set(int value) {
  SetText(Format("%d", value));
  ReformText();
}

bool IntegerPromptFrame::CanInsertCharacter(char ch, bool in_theory) const {
  if (!in_theory) {
    if ((ch == '-' && GetCursorPos() > 0) ||
        (ch == '0' && GetText() != "" && GetCursorPos() == 0) ||
        (ch == '0' && GetText()[0] == '-' && GetCursorPos() == 1))
      return false;
  }
  return (ch >= '0' && ch <= '9') || (min_value_ < 0 && ch == '-');
}

void IntegerPromptFrame::ReformText() {
  string min_value = Format("%d", min_value_), max_value = Format("%d", max_value_);
  string s = GetText();
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

IntegerPromptWidget::IntegerPromptWidget(int start_value, int min_value, int max_value,
                                         float prompt_width,
                                         const TextPromptView *prompt_view,
                                         const InputBoxView *input_box_view)
: FocusFrame(new InputBoxFrame(new PaddedFrame(sizer_ = new ExactWidthFrame(
    prompt_ = new IntegerPromptFrame(start_value, min_value, max_value, prompt_view),
    prompt_width), 1), input_box_view)) {}

///////////////////////////////////////////////////////////////////////////////////////////////////
// WindowFrame
// ===========

WindowFrame::WindowFrame(GlopFrame *inner_frame, const string &title,
                         const WindowView *view)
: SingleParentFrame(0), view_(view) {
  padded_title_frame_ = new PaddedFrame(new TextFrame(title, view_->GetTitleStyle()), 0);
  padded_inner_frame_ = new PaddedFrame(inner_frame, 0);
  SetChild(new ColFrame(padded_title_frame_, CellSize::Default(), CellSize::Default(),
                        padded_inner_frame_, CellSize::Default(), CellSize::Max(), kJustifyLeft));
}

WindowFrame::WindowFrame(GlopFrame *inner_frame, const WindowView *view)
: SingleParentFrame(0), view_(view), padded_title_frame_(0) {
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

void DummyButtonFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), is_down_, IsPrimaryFocus(),
                (PaddedFrame*)GetChild());
}

void DummyButtonFrame::SetIsDown(bool is_down) {
  if (is_down_ != is_down) {
    is_down_ = is_down;
    int lp, tp, rp, bp;
    view_->OnResize(GetOldRecWidth(), GetOldRecHeight(), is_down, &lp, &tp, &rp, &bp);
    ((PaddedFrame*)GetChild())->SetPadding(lp, tp, rp, bp);
  }
}

void DummyButtonFrame::RecomputeSize(int rec_width, int rec_height) {
  int lp, tp, rp, bp;
  view_->OnResize(rec_width, rec_height, is_down_, &lp, &tp, &rp, &bp);
  ((PaddedFrame*)GetChild())->SetPadding(lp, tp, rp, bp);
  SingleParentFrame::RecomputeSize(rec_width, rec_height);
}

void ButtonFrame::Think(int dt) {
  button_state_.Think();
  hot_key_tracker_.Think();
  was_pressed_fully_ = false;
}

bool ButtonFrame::OnKeyEvent(const KeyEvent &event, bool gained_focus) {
  bool press_generated = false;

  // Generate held-down repeat events
  if (event.dt > 0)
    button_state_.OnDt(event.dt);

  // Handle hot keys
  KeyEvent::Type event_type;
  bool handled_key = hot_key_tracker_.OnKeyEvent(event, &event_type);
  if (event_type == KeyEvent::Press || event_type == KeyEvent::RepeatPress ||
      event_type == KeyEvent::DoublePress)
    press_generated = true;
  if (IsPrimaryFocus() && event.HasKey(kGuiKeyConfirm)) {
    if (event.IsNonRepeatPress()) {
      is_confirm_key_down_ = true;
      press_generated = true;
      handled_key = true;
    } else if (event.IsRelease()) {
      is_confirm_key_down_ = false;
      handled_key = true;
    }
  }

  // Handle mouse lock
  bool was_mouse_locked_on = is_mouse_locked_on_;
  if (event.HasKey(kGuiKeyPrimaryClick)) {
    if (event.IsRelease() && is_mouse_locked_on_) {
      is_mouse_locked_on_ = false;
      handled_key = true;
    } else if (event.IsPress() && IsPointVisible(input()->GetMouseX(), input()->GetMouseY())) {
      if (event.IsNonRepeatPress())
        is_mouse_locked_on_ = true;
      if (is_mouse_locked_on_) {
        press_generated = true;
        handled_key = true;
      }
    }
  }

  // Set whether we are up or down based on the key event.
  if (hot_key_tracker_.IsDownNow() || is_confirm_key_down_) {
    SetIsDown(Down);
  } else if (IsPointVisible(input()->GetMouseX(), input()->GetMouseY())) {
    if (is_mouse_locked_on_)
      SetIsDown(Down);
    else {
      SetIsDown(UpConfirmPress);
    }
  } else {
    SetIsDown(was_mouse_locked_on? UpCancelPress : UpConfirmPress);
  }
  if (press_generated)
    button_state_.OnKeyEvent(event.type);

  // Delegate and return
  handled_key |= SingleParentFrame::OnKeyEvent(event, gained_focus);
  return handled_key;
}

void ButtonFrame::OnFocusChange() {
  if (!IsInFocus()) {
    is_confirm_key_down_ = false;
    is_mouse_locked_on_ = false;
    hot_key_tracker_.Clear();
    SetIsDown(UpCancelPress);
  }
  SingleParentFrame::OnFocusChange();
}

void ButtonFrame::SetIsDown(DownType down_type) {
  if ((down_type == Down) == button()->IsDown())
    return;
  button()->SetIsDown(down_type == Down);
  button_state_.SetIsDown(down_type == Down, true);
  if (down_type == Down) {
    if (ping_on_press_)
      NewRelativePing(0, 0, 1, 1);
  } else if (down_type == UpConfirmPress) {
    was_pressed_fully_ = true;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Slider
// ======

DummySliderFrame::DummySliderFrame(
  Direction direction, int logical_tab_size, int logical_total_size, int logical_tab_position,
  GlopFrame*(*button_factory)(ArrowFrame::Direction, const ArrowView *, const ButtonView *),
  const SliderView *view)
: direction_(direction),
  view_(view),
  logical_tab_size_(logical_tab_size), logical_total_size_(logical_total_size),
  logical_tab_position_(logical_tab_position) {
  if (direction == Horizontal) {
    dec_button_ = button_factory(ArrowFrame::Left, view_->GetArrowView(), view_->GetButtonView());
    inc_button_ = button_factory(ArrowFrame::Right, view_->GetArrowView(), view_->GetButtonView());
  } else {
    dec_button_ = button_factory(ArrowFrame::Up, view_->GetArrowView(), view_->GetButtonView());
    inc_button_ = button_factory(ArrowFrame::Down, view_->GetArrowView(), view_->GetButtonView());
  }
  AddChild(dec_button_);
  AddChild(inc_button_);
}

void DummySliderFrame::SetTabPosition(int position) {
  logical_tab_position_ = min(max(position, 0), logical_total_size_ - logical_tab_size_);
  RecomputeTabScreenPosition();
}

void DummySliderFrame::SetTabSize(int size) {
  logical_tab_size_ = size;
  SetTabPosition(logical_tab_position_);
}

void DummySliderFrame::SetTotalSize(int size) {
  logical_total_size_ = size;
  SetTabPosition(logical_tab_position_);
}

void DummySliderFrame::GetTabCoordinates(int *x1, int *y1, int *x2, int *y2) const {
  *x1 = tab_x1_;
  *y1 = tab_y1_;
  *x2 = tab_x2_;
  *y2 = tab_y2_;
}

int DummySliderFrame::PixelToPixelLocation(int x, int y) const {
  if (direction_ == Horizontal)
    return x - dec_button_->GetWidth();
  else
    return y - dec_button_->GetHeight();
}

int DummySliderFrame::LogicalPositionToFirstPixelLocation(int logical_position) const {
  if (logical_total_size_ > logical_tab_size_) {
    return logical_position * (bar_pixel_length_ - tab_pixel_length_) /
                              (logical_total_size_ - logical_tab_size_);
  } else {
    return 0;
  }
}

int DummySliderFrame::PixelLocationToLogicalPosition(int pixel_location) const {
 if (bar_pixel_length_ <= tab_pixel_length_)
    return 0;
  int pos = pixel_location * (logical_total_size_ - logical_tab_size_) /
            (bar_pixel_length_ - tab_pixel_length_) - 1;
  while (LogicalPositionToFirstPixelLocation(pos+1) <= pixel_location)
    pos++;
  return pos;
}

void DummySliderFrame::Render() const {
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), direction_ == Horizontal, IsPrimaryFocus(),
                tab_x1_ + GetX(), tab_y1_ + GetY(), tab_x2_ + GetX(), tab_y2_ + GetY(),
                dec_button_, inc_button_);
}

void DummySliderFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  dec_button_->SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  inc_button_->SetPosition(screen_x + GetWidth() - inc_button_->GetWidth(),
                           screen_y + GetHeight() - inc_button_->GetHeight(), cx1, cy1, cx2, cy2);
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

void DummySliderFrame::RecomputeSize(int rec_width, int rec_height) {
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
    RecomputeTabScreenPosition();
  }
}

void DummySliderFrame::RecomputeTabScreenPosition() {
  int min_tab_length;
  if (direction_ == Horizontal)
    min_tab_length = view_->GetMinTabLengthOnResize(bar_pixel_length_, GetHeight(), true);
  else
    min_tab_length = view_->GetMinTabLengthOnResize(GetWidth(), bar_pixel_length_, false);

  // Compute the length of the tab
  if (logical_tab_size_ < logical_total_size_) {
    tab_pixel_length_ = max(logical_tab_size_ * (bar_pixel_length_ - 1) / logical_total_size_,
                            min_tab_length);
  } else {
    tab_pixel_length_ = bar_pixel_length_;
  }

  // Compute its coordinates
  if (direction_ == Horizontal) {
    tab_x1_ = LogicalPositionToFirstPixelLocation(logical_tab_position_) + dec_button_->GetWidth();
    tab_x2_ = tab_x1_ + tab_pixel_length_ - 1;
    tab_y1_ = 0;
    tab_y2_ = GetHeight() - 1;
  } else {
    tab_y1_ = LogicalPositionToFirstPixelLocation(logical_tab_position_) +
      dec_button_->GetHeight();
    tab_y2_ = tab_y1_ + tab_pixel_length_ - 1;
    tab_x1_ = 0;
    tab_x2_ = GetWidth() - 1;
  }
}

// A SliderButtonFrame is a small wrapper around ButtonFrames - it ignores handled keys if pressing
// the button has no effect. Useful, for example, in the case of one ScrollingFrame contained
// within another.
class SliderButtonFrame: public ButtonFrame {
 public:
  static GlopFrame *Factory(ArrowFrame::Direction direction, const ArrowView *arrow_view,
                            const ButtonView *button_view) {
    return new SliderButtonFrame(direction, arrow_view, button_view);
  }
  SliderButtonFrame(ArrowFrame::Direction direction, const ArrowView *arrow_view,
                    const ButtonView *button_view)
  : ButtonFrame(new ArrowFrame(direction, arrow_view), button_view),
    is_dec_(direction == ArrowFrame::Left || direction == ArrowFrame::Up) {
    if (direction == ArrowFrame::Up)
      AddHotKey(kGuiKeyUp);
    else if (direction == ArrowFrame::Right)
      AddHotKey(kGuiKeyRight);
    else if (direction == ArrowFrame::Down)
      AddHotKey(kGuiKeyDown);
    else 
      AddHotKey(kGuiKeyLeft);
  }
  string GetType() const {return "SliderButtonFrame";}
  bool OnKeyEvent(const KeyEvent &event, bool gained_focus) {
    bool active = IsActive();
    SetPingOnPress(active);
    return ButtonFrame::OnKeyEvent(event, gained_focus) && active;
  }
 private:
  bool IsActive() const {
    const DummySliderFrame *slider = (const DummySliderFrame*)GetParent();
    if (is_dec_)
      return slider->GetTabPosition() > 0;
    else
      return slider->GetTabPosition() + slider->GetTabSize() < slider->GetTotalSize();
  }
  bool is_dec_;
  DISALLOW_EVIL_CONSTRUCTORS(SliderButtonFrame);
};

SliderFrame::SliderFrame(Direction direction, int logical_tab_size, int logical_total_size,
                        int logical_tab_position, const SliderView *view)
: SingleParentFrame(new DummySliderFrame((DummySliderFrame::Direction)direction, logical_tab_size,
                                         logical_total_size, logical_tab_position,
                                         SliderButtonFrame::Factory, view)),
  mouse_lock_mode_(None) {}

void SliderFrame::Think(int dt) {
  if (GetDecButton()->WasHeldDown() && !GetIncButton()->IsDown())
    SmallDec();
  if (GetIncButton()->WasHeldDown() && !GetDecButton()->IsDown())
    SmallInc();
  big_dec_tracker_.Think();
  big_inc_tracker_.Think();
  SingleParentFrame::Think(dt);
}

bool SliderFrame::OnKeyEvent(const KeyEvent &event, bool gained_focus) {
  bool result = false;

  // Handle big decrements
  KeyEvent::Type tracker_event;
  bool can_dec = (GetTabPosition() != 0);
  result |= (big_dec_tracker_.OnKeyEvent(event, &tracker_event) && can_dec);
  if (can_dec && (tracker_event == KeyEvent::Press || tracker_event == KeyEvent::RepeatPress ||
                  tracker_event == KeyEvent::DoublePress)) {
    BigDec();
    NewRelativePing(0, 0, 1, 1);
  }

  // Handle big increments
  bool can_inc = (GetTabPosition() != GetTotalSize() - GetTabSize());
  result |= (big_inc_tracker_.OnKeyEvent(event, &tracker_event) && can_inc);
  if (can_inc && (tracker_event == KeyEvent::Press || tracker_event == KeyEvent::RepeatPress ||
                  tracker_event == KeyEvent::DoublePress)) {
    BigInc();
    NewRelativePing(0, 0, 1, 1);
  }

  // Compute the mouse pixel position on the slider
  int mouse_pos = slider()->PixelToPixelLocation(input()->GetMouseX() - GetX(),
                                                 input()->GetMouseY() - GetY());
  if (event.HasKey(kGuiKeyPrimaryClick)) {
    int tab_x1, tab_y1, tab_x2, tab_y2;
    slider()->GetTabCoordinates(&tab_x1, &tab_y1, &tab_x2, &tab_y2);
    int tab_pos1 = slider()->PixelToPixelLocation(tab_x1, tab_y1);
    int tab_pos2 = slider()->PixelToPixelLocation(tab_x2, tab_y2);

    // Handle mouse first clicks
    if (event.IsNonRepeatPress() && IsPointVisible(input()->GetMouseX(), input()->GetMouseY()) &&
        mouse_pos >= 0 && mouse_pos <= slider()->GetMaxPixelLocation()) {
      result = true;
      if (mouse_pos >= tab_pos1 && mouse_pos <= tab_pos2) {
        mouse_lock_mode_ = Tab;
        last_grabbed_mouse_pos_ = mouse_pos;
        tab_grab_position_ = slider()->LogicalPositionToFirstPixelLocation(
          slider()->PixelLocationToLogicalPosition(mouse_pos)) - tab_pos1;
      } else {
        mouse_lock_mode_ = Bar; 
      }
    }

    // Handle repeat clicks
    if (event.IsPress() && mouse_lock_mode_ == Bar &&
        IsPointVisible(input()->GetMouseX(), input()->GetMouseY()) &&
        mouse_pos >= 0 && mouse_pos <= slider()->GetMaxPixelLocation()) {
      result = true;
      if (mouse_pos < tab_pos1)
        BigDec();
      else if (mouse_pos > tab_pos2)
        BigInc();
    }

    // Handle releases
    else if (event.IsRelease()) {
      mouse_lock_mode_ = None;
    }
  }

  // Handle mouse dragging. We do not do this until the mouse has actually moved after grabbing the
  // tab. This is because, if the range of values is bigger than the slider length, just clicking
  // on the tab could otherwise move it.
  if (mouse_lock_mode_ == Tab && mouse_pos != last_grabbed_mouse_pos_) {
    slider()->SetTabPosition(slider()->PixelLocationToLogicalPosition(
      mouse_pos - tab_grab_position_));
  }
  result |= SingleParentFrame::OnKeyEvent(event, gained_focus);
  return result;
}

void SliderFrame::OnFocusChange() {
  if (!IsInFocus())
    mouse_lock_mode_ = None;
  SingleParentFrame::OnFocusChange();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Menu
// ====

DummyMenuFrame::DummyMenuFrame(int num_cols, bool is_vertical, float horz_justify,
                               float vert_justify, const MenuView *view)
: num_cols_(num_cols), selection_(0), is_vertical_(is_vertical), horz_justify_(horz_justify),
  vert_justify_(vert_justify), view_(view) {}

void DummyMenuFrame::SetSelection(int selection) {
  selection_ = max(0, min(selection, GetNumItems() - 1));
}

void DummyMenuFrame::GetItemCoords(int item, int *x1, int *y1, int *x2, int *y2) const {
  *x1 = GetCol(item) * col_width_;
  *y1 = GetRow(item) * row_height_;
  *x2 = *x1 + col_width_ - 1;
  *y2 = *y1 + row_height_ - 1;
}

int DummyMenuFrame::AddItem(GlopFrame *frame, int index) {
  ASSERT(index >= 0 && index <= GetNumItems());
  ListId new_id = AddChild(frame);
  item_ids_.push_back(0);
  for (int i = GetNumItems() - 1; i > index; i--)
    item_ids_[i] = item_ids_[i-1];
  item_ids_[index] = new_id;
  return index;
}

GlopFrame *DummyMenuFrame::RemoveItemNoDelete(int index) {
  ASSERT(index >= 0 && index < GetNumItems());
  GlopFrame *result = RemoveChildNoDelete(item_ids_[index]);
  for (int i = index; i < GetNumItems(); i++)
    item_ids_[i] = item_ids_[i+1];
  item_ids_.pop_back();
  SetSelection(selection_);
  return result;
}

void DummyMenuFrame::SetItem(int item, GlopFrame *frame) {
  ASSERT(item >= 0 && item < GetNumItems());
  RemoveChild(item_ids_[item]);
  item_ids_[item] = AddChild(frame);
}

GlopFrame *DummyMenuFrame::SetItemNoDelete(int item, GlopFrame *frame) {
  ASSERT(item >= 0 && item < GetNumItems());
  GlopFrame *result = RemoveChildNoDelete(item_ids_[item]);
  item_ids_[item] = AddChild(frame);
  return result;
}

void DummyMenuFrame::Clear() {
  ClearChildren();
  item_ids_.clear();
}

void DummyMenuFrame::Render() const {
  int sel_x1, sel_y1, sel_x2, sel_y2;
  if (GetNumItems() > 0) {
    GetItemCoords(selection_, &sel_x1, &sel_y1, &sel_x2, &sel_y2);
    sel_x1 += GetX();
    sel_y1 += GetY();
    sel_x2 += GetX();
    sel_y2 += GetY();
  } else {
    sel_x1 = sel_y1 = sel_x2 = sel_y2 = -1;
  }

  // Only pass on the visible children to the view
  List<GlopFrame*> visible_children;
  for (List<GlopFrame*>::const_iterator it = GetChildren().begin(); it != GetChildren().end(); 
       ++it) {
    int x = (*it)->GetX(), y = (*it)->GetY(), w = (*it)->GetWidth(), h = (*it)->GetHeight();
    if (x+w > GetClipX1() && y+h > GetClipY1() && x <= GetClipX2() && y <= GetClipY2())
      visible_children.push_back(*it);
  }
  view_->Render(GetX(), GetY(), GetX2(), GetY2(), sel_x1, sel_y1, sel_x2, sel_y2, IsInFocus(),
                visible_children);
}

void DummyMenuFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  for (int i = 0; i < GetNumItems(); i++) {
    GetItem(i)->SetPosition(screen_x + GetCol(i) * col_width_ + item_lpadding_,
                            screen_y + GetRow(i) * row_height_ + item_tpadding_,
                            cx1, cy1, cx2, cy2);
  }
}

void DummyMenuFrame::RecomputeSize(int rec_width, int rec_height) {
  view_->OnResize(rec_width, rec_height, &item_lpadding_, &item_tpadding_, &item_rpadding_,
                  &item_bpadding_);
  col_width_ = row_height_ = 0;
  if (GetNumItems() > 0) {
    int col_rec_width = rec_width / GetNumCols() - (item_lpadding_ + item_rpadding_);
    int row_rec_height = rec_height / GetNumRows() - (item_tpadding_ + item_bpadding_);
    for (int i = 0; i < GetNumItems(); i++) {
      GetItem(i)->UpdateSize(col_rec_width, row_rec_height);
      col_width_ = max(col_width_, GetItem(i)->GetWidth() + item_lpadding_ + item_rpadding_);
      row_height_ = max(row_height_, GetItem(i)->GetHeight() + item_tpadding_ + item_bpadding_);
    }
  }
  SetSize(col_width_ * GetNumCols(), row_height_ * GetNumRows());
}

// Customizable menu items for MenuWidget
// ======================================

KeyPromptMenuItem::KeyPromptMenuItem(const string &prompt, const GlopKey &start_value,
                                     const GlopKey &cancel_key, const GlopKey &no_key,
                                     GlopKey *result_address, const MenuView *view)
: GuiMenuItem(0, prompt), prompt_(prompt), cancel_key_(cancel_key), no_key_(no_key),
  value_(start_value), result_address_(result_address), view_(view) {
  *result_address_ = start_value;
  ResetFrame(false);
}

bool KeyPromptMenuItem::OnKeyEvent(bool is_selected, bool is_confirmed, const KeyEvent &event,
                                   Action *action) {
  // Only process input if we are confirmed
  if (!is_selected || !is_confirmed || !event.IsNonRepeatPress())
    return false;

  // Handle cancel / no key
  if (event.HasKey(cancel_key_)) {
    *action = Unconfirm;
    return true;
  } else if (event.HasKey(no_key_)) {
    *action = Unconfirm;
    *result_address_ = value_ = kNoKey;
    return true;
  }

  // Handle real keys
  if (IsValidKey(event.GetMainKey())) {
    *result_address_ = value_ = event.GetMainKey();
    *action = Unconfirm;
    return true;
  }
  return false;
}

void KeyPromptMenuItem::OnConfirmationChange(bool is_selected, bool is_confirmed, Action *action) {
  if (is_selected)
    ResetFrame(is_confirmed);
}

void KeyPromptMenuItem::ResetFrame(bool is_confirmed) {
  GlopFrame *rhs;
  if (is_confirmed)
    rhs = new DummyTextPromptFrame("", view_->GetTextPromptView());
  else
    rhs = new TextFrame(value_.GetName(), view_->GetTextPromptView()->GetTextStyle());
  SetFrame(new RowFrame(new TextFrame(prompt_, view_->GetTextStyle()), rhs));
}

StringSelectMenuItem::StringSelectMenuItem(const string &prompt, const vector<string> &options,
                                           int start_value, int *result_address,
                                           const MenuView *view)
: GuiMenuItem(0, prompt), options_(options), value_(start_value), result_address_(result_address) {
  *result_address = start_value;
  value_frame_ = new TextFrame(options_[value_], view->GetTextPromptView()->GetTextStyle());
  SetFrame(new RowFrame(new TextFrame(prompt, view->GetTextStyle()), value_frame_));
}

void StringSelectMenuItem::OnConfirmationChange(bool is_selected, bool is_confirmed,
                                                Action *action) {
  if (is_selected && is_confirmed) {
    *result_address_ = value_ = (int)((value_ + 1) % options_.size());
    value_frame_->SetText(options_[value_]);
    *action = Unconfirm;
  }
}

StringPromptMenuItem::StringPromptMenuItem(const string &prompt, const string &start_value,
                                           int length_limit, string *result_address,
                                           const MenuView *view)
: GuiMenuItem(0, prompt),
  prompt_(prompt), value_(start_value), result_address_(result_address),
  length_limit_(length_limit), view_(view) {
  *result_address_ = start_value;
  ResetFrame(false);
}

bool StringPromptMenuItem::OnKeyEvent(bool is_selected, bool is_confirmed, const KeyEvent &event,
                                      Action *action) {
  if (is_selected && is_confirmed && event.IsPress()) {
    if (event.GetMainKey() == kKeyEnter || event.GetMainKey() == kKeyPadEnter) {
      *result_address_ = value_ = prompt_frame_->Get();
      *action = Unconfirm;
      return true;
    }
    if (event.GetMainKey() == kKeyEscape) {
      *action = Unconfirm;
      return true;
    }
  }
  return false;
}

void StringPromptMenuItem::OnConfirmationChange(bool is_selected, bool is_confirmed,
                                                Action *action) {
  if (is_selected)
    ResetFrame(is_confirmed);
}
 
void StringPromptMenuItem::ResetFrame(bool is_confirmed) {
  GlopFrame *rhs;
  if (is_confirmed) {
    prompt_frame_ = new StringPromptFrame(value_, length_limit_, view_->GetTextPromptView());
    prompt_frame_->SetFocusGainBehavior(BaseTextPromptFrame::CursorToEnd);
    rhs = new MaxWidthFrame(prompt_frame_);
  } else {
    rhs = new TextFrame(value_, view_->GetTextPromptView()->GetTextStyle());
  }
  SetFrame(new RowFrame(new TextFrame(prompt_, view_->GetTextStyle()), rhs));
}

IntegerPromptMenuItem::IntegerPromptMenuItem(const string &prompt, int start_value, int min_value,
                                             int max_value, int *result_address,
                                             const MenuView *view)
: GuiMenuItem(0, prompt),
  prompt_(prompt), value_(start_value), min_value_(min_value), max_value_(max_value),
  result_address_(result_address), view_(view) {
  *result_address_ = start_value;
  ResetFrame(false);
}

bool IntegerPromptMenuItem::OnKeyEvent(bool is_selected, bool is_confirmed, const KeyEvent &event,
                                       Action *action) {
  if (is_selected && is_confirmed && event.IsPress()) {
    if (event.GetMainKey() == kKeyEnter || event.GetMainKey() == kKeyPadEnter) {
      *result_address_ = value_ = prompt_frame_->Get();
      *action = Unconfirm;
      return true;
    }
    if (event.GetMainKey() == kKeyEscape) {
      *action = Unconfirm;
      return true;
    }
  }
  return false;
}

void IntegerPromptMenuItem::OnConfirmationChange(bool is_selected, bool is_confirmed,
                                                 Action *action) {
  if (is_selected)
    ResetFrame(is_confirmed);
}
 
void IntegerPromptMenuItem::ResetFrame(bool is_confirmed) {
  GlopFrame *rhs;
  if (is_confirmed) {
    prompt_frame_ = new IntegerPromptFrame(value_, min_value_, max_value_,
                                           view_->GetTextPromptView());
    prompt_frame_->SetFocusGainBehavior(BaseTextPromptFrame::CursorToEnd);
    rhs = new MaxWidthFrame(prompt_frame_);
  } else {
    rhs = new TextFrame(Format("%d", value_), view_->GetTextPromptView()->GetTextStyle());
  }
  SetFrame(new RowFrame(new TextFrame(prompt_, view_->GetTextStyle()), rhs));
}

// MenuFrame and MenuWidget
// ========================

void MenuFrame::SetBorderStyle(ItemBorderFactory *border_factory) {
  delete item_border_factory_;
  item_border_factory_ = border_factory;
  for (int i = 0; i < GetNumItems(); i++) {
    items_[i].parent = new EditableSingleParentFrame(items_[i].parent->RemoveChildNoDelete());
    menu()->SetItem(i, item_border_factory_->GetBorderedItem(items_[i].parent));
  }
}

void MenuFrame::SetSelection(int selection) {
  ASSERT(!IsConfirmed());
  menu()->SetSelection(selection);
  vector<pair<int, GuiMenuItem::Action> > actions;
  for (int i = 0; i < (int)items_.size(); i++) {
    GuiMenuItem::Action action = GuiMenuItem::Nothing;
    items_[i].controller->OnSelectionChange(GetSelection() == i, &action);
    RecordAction(i, action, &actions);
  }
  HandleActions(actions);
}

void MenuFrame::SetSelectionAndPing(int selection, bool center) {
  SetSelection(selection);
  menu()->NewItemPing(center);
}

void MenuFrame::PingSelection(bool center) {
  menu()->NewItemPing(center);
}

bool MenuFrame::SelectUp(bool ping) {
  bool result = (GetSelection() != 0);
  int r = menu()->GetRow(GetSelection()), c = menu()->GetCol(GetSelection());
  SetSelection(menu()->GetItemIndex(r - 1, c));
  if (ping)
    PingSelection(false);
  return result;
}

bool MenuFrame::SelectRight(bool ping) {
  bool result = (GetSelection() != GetNumItems() - 1);
  int r = menu()->GetRow(GetSelection()), c = menu()->GetCol(GetSelection());
  SetSelection(menu()->GetItemIndex(r, c + 1));
  if (ping)
    PingSelection(false);
  return result;
}

bool MenuFrame::SelectDown(bool ping) {
  bool result = (GetSelection() != GetNumItems() - 1);
  int r = menu()->GetRow(GetSelection()), c = menu()->GetCol(GetSelection());
  SetSelection(menu()->GetItemIndex(r + 1, c));
  if (ping)
    PingSelection(false);
  return result;
}

bool MenuFrame::SelectLeft(bool ping) {
  bool result = (GetSelection() != 0);
  int r = menu()->GetRow(GetSelection()), c = menu()->GetCol(GetSelection());
  SetSelection(menu()->GetItemIndex(r, c - 1));
  if (ping)
    PingSelection(false);
  return result;
}

bool MenuFrame::PageUp(bool ping) {
  bool result = (GetSelection() != 0);
  int x1, y1, x2, y2;
  menu()->GetItemCoords(GetSelection(), &x1, &y1, &x2, &y2);
  if (y1+GetY() > GetClipY1() && y2+GetY() <= GetClipY2())
    SetSelection(menu()->GetItemByCoords(x1, GetClipY1() - GetY()));
  else
    SetSelection(menu()->GetItemByCoords(x1, min(y2 + GetClipY1() - GetClipY2(), y1-1)));
  if (ping)
    PingSelection(false);
  return result;
}

bool MenuFrame::PageRight(bool ping) {
  bool result = (GetSelection() != GetNumItems()-1);
  int x1, y1, x2, y2;
  menu()->GetItemCoords(GetSelection(), &x1, &y1, &x2, &y2);
  if (x1+GetX() >= GetClipX1() && x2+GetX() < GetClipX2())
    SetSelection(menu()->GetItemByCoords(GetClipX2() - GetX(), y1));
  else
    SetSelection(menu()->GetItemByCoords(max(x1 + GetClipX2() - GetClipX1(), x2+1), y1));
  if (ping)
    PingSelection(false);
  return result;
}

bool MenuFrame::PageDown(bool ping) {
  bool result = (GetSelection() != GetNumItems()-1);
  int x1, y1, x2, y2;
  menu()->GetItemCoords(GetSelection(), &x1, &y1, &x2, &y2);
  if (y1+GetY() >= GetClipY1() && y2+GetY() < GetClipY2())
    SetSelection(menu()->GetItemByCoords(x1, GetClipY2() - GetY()));
  else
    SetSelection(menu()->GetItemByCoords(x1, max(y1 + GetClipY2() - GetClipY1(), y2+1)));
  if (ping)
    PingSelection(false);
  return result;
}

bool MenuFrame::PageLeft(bool ping) {
  bool result = (GetSelection() != 0);
  int x1, y1, x2, y2;
  menu()->GetItemCoords(GetSelection(), &x1, &y1, &x2, &y2);
  if (x1+GetX() > GetClipX1() && x2+GetX() <= GetClipX2())
    SetSelection(menu()->GetItemByCoords(GetClipX1() - GetX(), y1));
  else
    SetSelection(menu()->GetItemByCoords(min(x2 + GetClipX1() - GetClipX2(), x1-1), y1));
  if (ping)
    PingSelection(false);
  return result;
}

void MenuFrame::Confirm(bool is_confirmed) {
  if (is_confirmed == is_confirmed_)
    return;
  if (is_confirmed) {
    // Make sure there is an item to confirm and that we are in the top focus layer
    ASSERT(GetNumItems() > 0);
    GetFocusFrame()->DemandFocus(false);
    ASSERT(GetWindow()->GetFocusFrame() == GetFocusFrame());
    menu()->NewItemPing(false);
  } else {
    // Do not jump to the mouse if it moved while the menu was confirmed
    mouse_x_ = mouse_y_ = -1;
  }

  is_confirmed_ = is_confirmed;
  vector<pair<int, GuiMenuItem::Action> > actions;
  for (int i = 0; i < (int)items_.size(); i++) {
    GuiMenuItem::Action action = GuiMenuItem::Nothing;
    items_[i].controller->OnConfirmationChange(GetSelection() == i, is_confirmed_, &action);
    RecordAction(i, action, &actions);
  }
  HandleActions(actions);
}

int MenuFrame::AddItem(GuiMenuItem *item, int index) {
  items_.push_back(ItemInfo());
  for (int i = GetNumItems() - 1; i > index; i--)
    items_[i] = items_[i-1];
  items_[index] = ItemInfo(item, new EditableSingleParentFrame(item->GetFrame()));
  menu()->AddItem(item_border_factory_->GetBorderedItem(items_[index].parent));
  return index;
}

void MenuFrame::RemoveItem(int index) {
  ASSERT(!is_confirmed_ || GetSelection() != index);
  menu()->RemoveItem(index);
  GuiMenuItem *item = items_[index].controller;
  for (int i = index; i < GetNumItems(); i++)
    items_[i] = items_[i+1];
  items_.pop_back();
  delete item;
}

void MenuFrame::Clear() {
  Confirm(false);
  menu()->Clear();
  for (int i = 0; i < (int)items_.size(); i++)
    delete items_[i].controller;
  items_.clear();
}

void MenuFrame::Think(int dt) {
  SingleParentFrame::Think(dt);
  if (search_term_reset_timer_ > 0)
    search_term_reset_timer_ -= dt;
  else
    search_term_ = "";

  vector<pair<int, GuiMenuItem::Action> > actions;
  for (int i = 0; i < (int)items_.size(); i++) {
    GuiMenuItem::Action action = GuiMenuItem::Nothing;
    items_[i].controller->Think(GetSelection() == i, is_confirmed_, dt, &action);
    RecordAction(i, action, &actions);
  }
  HandleActions(actions);
}

bool MenuFrame::OnKeyEvent(const KeyEvent &event, bool gained_focus) {
  // Allow the menu items to perform logic
  bool used_key = SingleParentFrame::OnKeyEvent(event, gained_focus);
  vector<pair<int, GuiMenuItem::Action> > actions;
  for (int i = 0; i < (int)items_.size(); i++) {
    GuiMenuItem::Action action = GuiMenuItem::Nothing;
    used_key |= items_[i].controller->OnKeyEvent(GetSelection() == i, is_confirmed_, event,
                                                 &action);
    RecordAction(i, action, &actions);
  }
  HandleActions(actions);

  // If the menu is confirmed, do not do menu meta-logic and do not pass the key up the chain
  if (IsConfirmed() || used_key)
    return true;

  // Figure out which menu item the mouse is over
  int mouse_item = -1;
  if (mouse_x_ == -1 && mouse_y_ == -1) {
    mouse_x_ = input()->GetMouseX();
    mouse_y_ = input()->GetMouseY();
  }
  int mouse_x = input()->GetMouseX(), mouse_y = input()->GetMouseY();
  if (IsPointVisible(mouse_x, mouse_y))
    mouse_item = menu()->GetItemByCoords(mouse_x - GetX(), mouse_y - GetY());
  if (mouse_item >= GetNumItems())
    mouse_item = -1;

  // Optionally make that item our selection
  if (selection_style_ == SingleClick && mouse_item != -1 &&
      (mouse_x_ != mouse_x || mouse_y_ != mouse_y)) {
    mouse_x_ = mouse_x;
    mouse_y_ = mouse_y;
    SetSelectionAndPing(mouse_item, false);
  }

  // Handle actual key presses
  if (event.IsPress() && !used_key) {
    // Selection changes
    if (event.HasKey(kGuiKeyUp))
      used_key |= SelectUp(true);
    if (event.HasKey(kGuiKeyDown))
      used_key |= SelectDown(true);
    if (event.HasKey(kGuiKeyRight))
      used_key |= SelectRight(true);
    if (event.HasKey(kGuiKeyLeft))
      used_key |= SelectLeft(true);
    if (event.HasKey(kGuiKeyPageUp) || event.HasKey(kGuiKeyPageLeft)) {
      if (menu()->IsVertical())
        used_key |= PageUp(true);
      else
        used_key |= PageLeft(true);
    }
    if (event.HasKey(kGuiKeyPageDown) || event.HasKey(kGuiKeyPageRight)) {
      if (menu()->IsVertical())
        used_key |= PageDown(true);
      else
        used_key |= PageRight(true);
    }

    // Confirm
    if (event.HasKey(kGuiKeyConfirm)) {
      used_key = true;
      Confirm(true);
    }

    // Click
    if (event.IsNonRepeatPress() && event.HasKey(kGuiKeyPrimaryClick) && mouse_item != -1 &&
        selection_style_ != NoMouse) {
      used_key = true;
      if (GetSelection() == mouse_item && !gained_focus)
        Confirm(true);
      else
        SetSelectionAndPing(mouse_item, false);
    }

    // Typing
    int ascii = input()->GetAsciiValue(event.GetMainKey());
    if (ascii >= 32 && ascii < 127 && !used_key) {
      search_term_ += ascii;
      int match = -1;
      for (int i = 0; i < (int)items_.size(); i++)
      if (items_[i].controller->GetSearchKey().size() >= search_term_.size()) {
        bool is_match = true;
        for (int j = 0; j < (int)search_term_.size(); j++)
        if (items_[i].controller->GetSearchKey()[j] != search_term_[j]) {
          is_match = false;
          break;
        }
        if (is_match) {
          match = i;
          break;
        }
      }
      if (match != -1)
        SetSelectionAndPing(match, false);
      if (match != -1 || search_term_.size() > 1) {
        used_key = true;
        search_term_reset_timer_ = kSearchTermResetTime;
      } else {
        search_term_ = "";
      }
    }
  }
  return used_key;
}

void MenuFrame::RecordAction(int item, GuiMenuItem::Action action,
                             vector<pair<int, GuiMenuItem::Action> > *actions) {
  switch (action) {
   case GuiMenuItem::Nothing:
    return;
   case GuiMenuItem::SelectNoPing:
   case GuiMenuItem::SelectAndPing:
   case GuiMenuItem::SelectAndConfirm:
    ASSERT(!is_confirmed_);
    break;
   case GuiMenuItem::Unconfirm:
    ASSERT(is_confirmed_ && GetSelection() == item);
    break;
   default:
    ASSERT(false);
  };
  actions->push_back(make_pair(item, action));
}

void MenuFrame::HandleActions(const vector<pair<int, GuiMenuItem::Action> > &actions) {
  // Make sure we are rendering the right frames
  for (int i = 0; i < GetNumItems(); i++) {
    if (items_[i].controller->GetFrame() != items_[i].parent->GetChild())
      items_[i].parent->SetChild(items_[i].controller->GetFrame());
  }

  // Handle the actions
  for (int i = 0; i < (int)actions.size(); i++) {
    switch (actions[i].second) {
     case GuiMenuItem::SelectNoPing:
      SetSelection(actions[i].first);
      break;
     case GuiMenuItem::SelectAndPing:
      SetSelection(actions[i].first);
      menu()->NewItemPing(false);
      break;
     case GuiMenuItem::SelectAndConfirm:
      SetSelection(actions[i].first);
      Confirm(true);
      break;
     case GuiMenuItem::Unconfirm:
      Confirm(false);
      break;
     default:
      ASSERT(false);
    };
  }
}

GlopFrame *MenuFrame::BasicItemBorderFactory::GetBorderedItem(GlopFrame *item) {
  if (horz_sizing_ == ExactlyRecSize)
    item = new ExactWidthFrame(item);
  else if (horz_sizing_ == AtLeastRecSize)
    item = new MinWidthFrame(item);
  else if (horz_sizing_ == AtMostRecSize)
    item = new MaxWidthFrame(item);
  if (vert_sizing_ == ExactlyRecSize)
    item = new ExactHeightFrame(item);
  else if (vert_sizing_ == AtLeastRecSize)
    item = new MinHeightFrame(item);
  else if (vert_sizing_ == AtMostRecSize)
    item = new MaxHeightFrame(item);
  if (abs_padding_ > 0)
    item = new PaddedFrame(item, abs_padding_);
  if (rel_padding_ > 0)
    item = new ScalingPaddedFrame(item, rel_padding_);
  return item;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Dialog
// ======

// Globals
bool DialogWidget::is_initialized_ = false;
List<GlopKey> DialogWidget::yes_keys_, DialogWidget::no_keys_, DialogWidget::okay_keys_,
              DialogWidget::cancel_keys_;

// Constructors
void DialogWidget::TextOkay(const string &title, const string &message,
                            const DialogView *view) {
  DoText(title, message, false, false, true, false, view);
}

DialogWidget::Result DialogWidget::TextOkayCancel(const string &title, const string &message,
                                                  const DialogView *view) {
  return DoText(title, message, false, false, true, true, view);
}
DialogWidget::Result DialogWidget::TextYesNo(const string &title, const string &message,
                                             const DialogView *view) {
  return DoText(title, message, true, true, false, false, view);
}

DialogWidget::Result DialogWidget::TextYesNoCancel(const string &title, const string &message,
                                                   const DialogView *view) {
  return DoText(title, message, true, true, false, true, view);
}

string DialogWidget::StringPromptOkay(const string &title, const string &message,
                                      const string &prompt, const string &start_value,
                                      int value_length_limit, const DialogView *view) {
  string prompt_value;
  DoStringPrompt(title, message, prompt, start_value, value_length_limit, &prompt_value, true,
                 false, view);
  return prompt_value;
}

DialogWidget::Result DialogWidget::StringPromptOkayCancel(
  const string &title, const string &message, const string &prompt, const string &start_value,
  int value_length_limit, string *prompt_value, const DialogView *view) {
  return DoStringPrompt(title, message, prompt, start_value, value_length_limit, prompt_value,
                        true, true, view);
}

int DialogWidget::IntegerPromptOkay(
  const string &title, const string &message, const string &prompt, int start_value, int min_value,
  int max_value, const DialogView *view) {
  int prompt_value;
  DoIntegerPrompt(title, message, prompt, start_value, min_value, max_value, &prompt_value, true,
                  false, view);
  return prompt_value;
}

DialogWidget::Result DialogWidget::IntegerPromptOkayCancel(
  const string &title, const string &message, const string &prompt, int start_value, int min_value,
  int max_value, int *prompt_value, const DialogView *view) {
  return DoIntegerPrompt(title, message, prompt, start_value, min_value, max_value, prompt_value,
                         true, true, view);
}

// Utilities
void DialogWidget::Init() {
  if (!is_initialized_) {
    yes_keys_.push_back('y');
    no_keys_.push_back('n');
    okay_keys_.push_back(kGuiKeyConfirm);
    cancel_keys_.push_back(kGuiKeyCancel);
    is_initialized_ = true;
  }
}

GlopFrame *DialogWidget::Create(
  const string &title, const string &message, const string &prompt, GlopFrame *extra_frame,
  bool has_yes_button, bool has_no_button, bool has_okay_button, bool has_cancel_button,
  const DialogView *view, vector<ButtonWidget*> *buttons,
  vector<Result> *button_meanings) {
  Init();

  // Create the buttons
  buttons->clear();
  button_meanings->clear();
  if (has_yes_button) {
    button_meanings->push_back(Yes);
    buttons->push_back(new ButtonWidget("Yes", view->GetButtonTextStyle(), view->GetButtonView()));
    for (List<GlopKey>::iterator it = yes_keys_.begin(); it != yes_keys_.end(); ++it)
      (*buttons)[buttons->size()-1]->AddHotKey(*it);
  }
  if (has_no_button) {
    button_meanings->push_back(No);
    buttons->push_back(new ButtonWidget("No", view->GetButtonTextStyle(), view->GetButtonView()));
    for (List<GlopKey>::iterator it = no_keys_.begin(); it != no_keys_.end(); ++it)
      (*buttons)[buttons->size()-1]->AddHotKey(*it);
  }
  if (has_okay_button) {
    button_meanings->push_back(Okay);
    buttons->push_back(new ButtonWidget("Okay", view->GetButtonTextStyle(),
      view->GetButtonView()));
    for (List<GlopKey>::iterator it = okay_keys_.begin(); it != okay_keys_.end(); ++it)
      (*buttons)[buttons->size()-1]->AddHotKey(*it);
  }
  if (has_cancel_button) {
    button_meanings->push_back(Cancel);
    buttons->push_back(new ButtonWidget("Cancel", view->GetButtonTextStyle(),
                                        view->GetButtonView()));
    for (List<GlopKey>::iterator it = cancel_keys_.begin(); it != cancel_keys_.end(); ++it)
      (*buttons)[buttons->size()-1]->AddHotKey(*it);
  }
  RowFrame *button_row = new RowFrame((int)buttons->size());
  for (int i = 0; i < (int)buttons->size(); i++)
    button_row->SetCell(i, (*buttons)[i]);
  button_row->SetPadding(view->GetInnerHorzPadding());

  // Create the main dialog frame
  FancyTextFrame *message_frame = new FancyTextFrame(message, true, view->GetTextHorzJustify(), 
                                                     view->GetTextStyle());
  ColFrame *main_col = new ColFrame(extra_frame == 0? 2 : 3);
  main_col->SetCell(0, message_frame, view->GetTextHorzJustify());
  if (extra_frame != 0) {
    RowFrame *extra_row = new RowFrame(
      new TextFrame(prompt, view->GetTextStyle()), CellSize::Default(), CellSize::Default(),
      extra_frame, CellSize::Max(), CellSize::Default());
    main_col->SetCell(1, extra_row, view->GetTextHorzJustify());
  }
  main_col->SetCell(main_col->GetNumCells() - 1, button_row, view->GetButtonsHorzJustify());
  main_col->SetPadding(view->GetInnerVertPadding());
  float lp, tp, rp, bp;
  view->GetPadding(&lp, &tp, &rp, &bp);
  GlopFrame *padded_col = new ScalingPaddedFrame(main_col, lp, tp, rp, bp);
  GlopFrame *interior = new ScrollingFrame(padded_col, view->GetSliderView());
  WindowFrame *window = new WindowFrame(interior, title, view->GetWindowView());
  return new RecSizeFrame(window, view->GetRecWidth(), view->GetRecHeight());
}

DialogWidget::Result DialogWidget::Execute(const vector<ButtonWidget*> &buttons,
                                           const vector<Result> &button_meanings) {
  while (1) {
    system()->Think();
    for (int i = 0; i < (int)buttons.size(); i++)
    if (buttons[i]->WasPressedFully())
      return button_meanings[i];
  }
}

DialogWidget::Result DialogWidget::DoText(
  const string &title, const string &message, bool has_yes_button, bool has_no_button,
  bool has_okay_button, bool has_cancel_button, const DialogView *view) {
  vector<ButtonWidget*> buttons;
  vector<Result> button_meanings;
  window()->PushFocus();
  GlopFrame *frame = Create(title, message, "", 0, has_yes_button, has_no_button, has_okay_button,
                            has_cancel_button, view, &buttons, &button_meanings);
  ListId id = window()->AddFrame(frame, 0.5f, view->GetVertJustify(),
                                 0.5f, view->GetVertJustify(), 0);
  Result result = Execute(buttons, button_meanings);
  window()->RemoveFrame(id);
  window()->PopFocus();
  return result;
}

DialogWidget::Result DialogWidget::DoStringPrompt(
  const string &title, const string &message, const string &prompt, const string &start_value,
  int value_length_limit, string *prompt_value, bool has_okay_button, bool has_cancel_button,
  const DialogView *view) {
  vector<ButtonWidget*> buttons;
  vector<Result> button_meanings;
  window()->PushFocus();
  StringPromptWidget *prompt_frame = new StringPromptWidget(
    start_value, value_length_limit, kSizeLimitRec, view->GetTextPromptView(),
    view->GetInputBoxView());
  GlopFrame *frame = Create(title, message, prompt + " ", prompt_frame, false, false,
                     
    has_okay_button, has_cancel_button, view, &buttons,
                            &button_meanings);
  ListId id = window()->AddFrame(frame, 0.5f, view->GetVertJustify(),
                                 0.5f, view->GetVertJustify(), 0);
  Result result = Execute(buttons, button_meanings);
  *prompt_value = prompt_frame->Get();
  window()->RemoveFrame(id);
  window()->PopFocus();
  return result;
}

DialogWidget::Result DialogWidget::DoIntegerPrompt(
  const string &title, const string &message, const string &prompt, int start_value, int min_value,
  int max_value, int *prompt_value, bool has_okay_button, bool has_cancel_button,
  const DialogView *view) {
  vector<ButtonWidget*> buttons;
  vector<Result> button_meanings;
  window()->PushFocus();
  IntegerPromptWidget *prompt_frame = new IntegerPromptWidget(
    start_value, min_value, max_value, kSizeLimitRec, view->GetTextPromptView(),
    view->GetInputBoxView());
  GlopFrame *frame = Create(title, message, prompt + " ", prompt_frame, false, false,
                            has_okay_button, has_cancel_button, view, &buttons,
                            &button_meanings);
  ListId id = window()->AddFrame(frame, 0.5f, view->GetVertJustify(),
                                 0.5f, view->GetVertJustify(), 0);
  Result result = Execute(buttons, button_meanings);
  *prompt_value = prompt_frame->Get();
  window()->RemoveFrame(id);
  window()->PopFocus();
  return result;
}

#endif // GLOP_LEAN_AND_MEAN
