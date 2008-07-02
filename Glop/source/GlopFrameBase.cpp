// Includes
#include "../include/GlopFrameBase.h"
#include "../include/GlopFrameWidgets.h"
#include "../include/GlopWindow.h"
#include "../include/OpenGl.h"
#include <algorithm>
using namespace std;

// Constants
const int kClipInfinity = 1000000000;
const int kClipMinusInfinity = -kClipInfinity;

// GlopFrame
// =========

// Constructor. Does nothing.
GlopFrame::GlopFrame()
: parent_(0),
  width_(0),
  height_(0),
  screen_x_(0),
  screen_y_(0),
  clip_x1_(kClipMinusInfinity),
  clip_y1_(kClipMinusInfinity),
  clip_x2_(kClipInfinity),
  clip_y2_(kClipInfinity),
  is_in_focus_(false),
  focus_frame_(0) {
  DirtySize();
}

// Destructor. Deletes all pings that have been queued up for this frame.
GlopFrame::~GlopFrame() {
  gWindow->UnregisterAllPings(this);
}

// Marks that this frame needs its size recomputed. The parent must also recompute its size
// for this to happen, so the request is propogated upwards. We do not propogate up if the parent
// is already dirty (the invariant that if you are dirty, your parent is dirty, guarantees that
// is unnecessary).
void GlopFrame::DirtySize() {
  old_rec_width_ = old_rec_height_ = -1;
  if (parent_ != 0 && parent_->old_rec_width_ != -1)
    parent_->DirtySize();
}

void GlopFrame::UpdateSize(int rec_width, int rec_height) {
  if (old_rec_width_ == rec_width && old_rec_height_ == rec_height)
    return;
  RecomputeSize(max(rec_width, 1), max(rec_height, 1));
  old_rec_width_ = rec_width;
  old_rec_height_ = rec_height;
}

void GlopFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  screen_x_ = screen_x;
  screen_y_ = screen_y;
  clip_x1_ = cx1;
  clip_y1_ = cy1;
  clip_x2_ = cx2;
  clip_y2_ = cy2;
}

bool GlopFrame::IsPrimaryFocus() const {
  return is_in_focus_ && screen_x_ == GetFocusFrame()->GetX() &&
    screen_y_ == GetFocusFrame()->GetY() && width_ == GetFocusFrame()->GetWidth() &&
    height_ == GetFocusFrame()->GetHeight();
}

bool GlopFrame::IsPointVisible(int screen_x, int screen_y) const {
  int x1 = max(screen_x_, clip_x1_), y1 = max(screen_y_, clip_y1_),
      x2 = min(screen_x_+width_-1, clip_x2_), y2 = min(screen_y_+height_-1, clip_y2_);
  return (screen_x >= x1 && screen_y >= y1 && screen_x <= x2 && screen_y <= y2);
}

void GlopFrame::SetToMaxSize(int width_bound, int height_bound, float aspect_ratio) {
  width_ = width_bound;
  height_ = (int)(0.5f + width_ / aspect_ratio);
  if (height_ > height_bound) {
    height_ = height_bound;
    width_ = (int)(0.5f + height_ * aspect_ratio);
  }
}

void GlopFrame::AddPing(Ping *ping) {
  gWindow->RegisterPing(ping);
}

string GlopFrame::GetContextStringHelper(bool extend_down, bool extend_up,
                                         const string &prefix) const {
  string result;
  if (extend_up && GetParent() != 0)
    result = GetParent()->GetContextStringHelper(false, true, "");
  if (prefix.size() > 0)
    result += Format("%s+", prefix.substr(0, (int)prefix.size()-1).c_str());
  result += Format("%s: (%d, %d) - (%d, %d)\n",
                   GetType().c_str(), GetX(), GetY(), GetX2(), GetY2());
  return result;
}

void GlopFrame::SetParent(GlopFrame *parent) {
  if (parent == 0) {
    if (!IsFocusFrame()) SetFocusInfo(0, false);
    parent_ = 0;
  } else {
    parent_ = parent;
    if (!IsFocusFrame()) SetFocusInfo(parent->focus_frame_, parent->is_in_focus_);
  }
}

// SingleParentFrame
// =================

GlopFrame *SingleParentFrame::RemoveChildNoDelete() {
  DirtySize();
  GlopFrame *old_child = child_;
  child_ = 0;
  if (old_child != 0) old_child->SetParent(0);
  return old_child;
}

void SingleParentFrame::SetChild(GlopFrame *frame) {
  DirtySize();
  if (child_ != 0) delete child_;
  child_ = frame;
  if (child_ != 0) child_->SetParent(this);
}

string SingleParentFrame::GetContextStringHelper(bool extend_down, bool extend_up,
                                                 const string &prefix) const {
  string result = GlopFrame::GetContextStringHelper(extend_down, extend_up, prefix);
  if (extend_down && child_ != 0)
    result += child_->GetContextStringHelper(true, false, prefix + " ");
  return result;
}

// MultiParentFrame
// ================

// Renders all child frames. We automatically prune the children if they are completely outside
// the clipping rectangle.
void MultiParentFrame::Render() const {
  for (List<GlopFrame*>::const_iterator it = children_.begin(); it != children_.end(); ++it) {
    int x = (*it)->GetX(), y = (*it)->GetY(), w = (*it)->GetWidth(), h = (*it)->GetHeight();
    if (x+w > clip_x1_ && y+h > clip_y1_ && x <= clip_x2_ && y <= clip_y2_)
      (*it)->Render();
  }
}

bool MultiParentFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  bool result = false;
  for (List<GlopFrame*>::iterator it = children_.begin(); it != children_.end(); ++it)
  if (!(*it)->IsFocusFrame())
    result |= (*it)->OnKeyEvent(event, dt);
  return result;
}

void MultiParentFrame::Think(int dt) {
  for (List<GlopFrame*>::iterator it = children_.begin(); it != children_.end(); ++it)
    (*it)->Think(dt);
}

void MultiParentFrame::SetPosition(int screen_x, int screen_y,
                                   int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  for (List<GlopFrame*>::iterator it = children_.begin(); it != children_.end(); ++it)
    (*it)->SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

bool MultiParentFrame::IsFocusMagnet(const KeyEvent &event) const {
  for (List<GlopFrame*>::const_iterator it = children_.begin(); it != children_.end(); ++it)
    if (!(*it)->IsFocusFrame() && (*it)->IsFocusMagnet(event))
      return true;
  return false;
}

void MultiParentFrame::RecomputeSize(int rec_width, int rec_height) {
  int new_width = 0, new_height = 0;
  for (List<GlopFrame*>::iterator it = children_.begin(); it != children_.end(); ++it) {
    (*it)->UpdateSize(rec_width, rec_height);
    new_width = max(new_width, (*it)->GetWidth());
    new_height = max(new_height, (*it)->GetHeight());
  }
  SetSize(new_width, new_height);
}

void MultiParentFrame::OnFocusChange() {
  for (List<GlopFrame*>::iterator it = children_.begin(); it != children_.end(); ++it)
  if ((*it)->GetFocusFrame() != *it)
    (*it)->SetFocusInfo(focus_frame_, is_in_focus_);
}

ListId MultiParentFrame::AddChild(GlopFrame *frame) {
  DirtySize();
  ListId result = children_.push_back(frame);
  frame->SetParent(this);
  return result;
}

ListId MultiParentFrame::RemoveChild(ListId id) {
  delete children_[id];
  return children_.erase(id);
}

GlopFrame *MultiParentFrame::RemoveChildNoDelete(ListId id) {
  GlopFrame *old_child = children_[id];
  children_.erase(id);
  old_child->SetParent(0);
  return old_child;
}

void MultiParentFrame::ClearChildren() {
  for (List<GlopFrame*>::iterator it = children_.begin(); it != children_.end(); ++it)
    delete *it;
  children_.clear();
}

void MultiParentFrame::OnWindowResize(int width, int height) {
  DirtySize();
  for (List<GlopFrame*>::iterator it = children_.begin(); it != children_.end(); ++it)
    (*it)->OnWindowResize(width, height);
}

string MultiParentFrame::GetContextStringHelper(bool extend_down, bool extend_up,
                                                const string &prefix) const {
  string result = GlopFrame::GetContextStringHelper(extend_down, extend_up, prefix);
  if (extend_down)
  for (List<GlopFrame*>::const_iterator it = children_.begin(); it != children_.end(); ++it)
    result += (*it)->GetContextStringHelper(
      true, false, prefix + (it == children_.next_to_end()? " " : "|"));
  return result;
}

// ClippedFrame
// ============

void ClippedFrame::Render() const {
  int old_scissor_test[4];

  // Make sure the clipping rectangle is not empty. This CAN reasonably happen. For example, a
  // ScrollingFrame might request a negative clipping rectangle if the window is squished too much.
  // Instead of making every user trap these cases, we just do the right thing here. (Note that
  // OpenGL does NOT do the right thing.)
  if (GetClipX1() > GetClipX2() || GetClipY1() > GetClipY2())
    return;

  // Adjust the clipping
  int old_clipping_enabled = glIsEnabled(GL_SCISSOR_TEST);
  if (old_clipping_enabled)
    glGetIntegerv(GL_SCISSOR_BOX, old_scissor_test);
  else
    glEnable(GL_SCISSOR_TEST);
  glScissor(GetClipX1(), gWindow->GetHeight() - 1 - GetClipY2(), GetClipX2() - GetClipX1() + 1,
            GetClipY2() - GetClipY1() + 1);

  // Render the child
  SingleParentFrame::Render();

  // Reset the clipping
  if (old_clipping_enabled)
    glScissor(old_scissor_test[0], old_scissor_test[1], old_scissor_test[2], old_scissor_test[3]);
  else
    glDisable(GL_SCISSOR_TEST);
}

void ClippedFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  cx1 = max(cx1, is_standard_clipping_? screen_x : req_clip_x1_);
  cy1 = max(cy1, is_standard_clipping_? screen_y : req_clip_y1_);
  cx2 = min(cx2, is_standard_clipping_? screen_x + GetWidth() - 1: req_clip_x2_);
  cy2 = min(cy2, is_standard_clipping_? screen_y + GetHeight() - 1: req_clip_y2_);
  SingleParentFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

// PaddedFrame
// ===========

void PaddedFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  if (GetChild() != 0)
    GetChild()->SetPosition(screen_x + left_padding_, screen_y + top_padding_, cx1, cy1, cx2, cy2);
}

void PaddedFrame::RecomputeSize(int rec_width, int rec_height) {
  if (GetChild() != 0) {
    GetChild()->UpdateSize(rec_width - left_padding_ - right_padding_,
                           rec_height - top_padding_ - bottom_padding_);
    SetSize(GetChild()->GetWidth() + left_padding_ + right_padding_,
            GetChild()->GetHeight() + top_padding_ + bottom_padding_);
  } else {
    SetSize(rec_width, rec_height);
  }
}

void PaddedFrame::SetPadding(int left_padding, int top_padding, int right_padding,
                             int bottom_padding) {
  if (left_padding + right_padding != left_padding_ + right_padding_ ||
      top_padding + bottom_padding != top_padding_ + bottom_padding_)
    DirtySize();
  left_padding_ = left_padding;
  top_padding_ = top_padding;
  right_padding_ = right_padding;
  bottom_padding_ = bottom_padding;
}

// ScalingPaddedFrame
// ==================

void ScalingPaddedFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2,
                                     int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  if (GetChild() != 0)
    GetChild()->SetPosition(screen_x + left_padding_, screen_y + top_padding_, cx1, cy1, cx2, cy2);
}

void ScalingPaddedFrame::RecomputeSize(int rec_width, int rec_height) {
  if (GetChild() != 0) {
    int base = min(gWindow->GetWidth(), gWindow->GetHeight());
    left_padding_ = int(scaled_left_padding_ * base);
    top_padding_ = int(scaled_top_padding_ * base);
    right_padding_ = int(scaled_right_padding_ * base);
    bottom_padding_ = int(scaled_bottom_padding_ * base);
    GetChild()->UpdateSize(rec_width - left_padding_ - right_padding_,
                           rec_height - top_padding_ - bottom_padding_);
    SetSize(GetChild()->GetWidth() + left_padding_ + right_padding_,
            GetChild()->GetHeight() + top_padding_ + bottom_padding_);
  } else {
    SetSize(rec_width, rec_height);
  }
}

void ScalingPaddedFrame::SetPadding(float left_padding, float top_padding, float right_padding,
                                    float bottom_padding) {
  DirtySize();
  scaled_left_padding_ = left_padding;
  scaled_top_padding_ = top_padding;
  scaled_right_padding_ = right_padding;
  scaled_bottom_padding_ = bottom_padding;
}

// FocusFrame
// ==========
//
// A FocusFrame does little logic on its own - it just passes requests onto the GlopWindow.

FocusFrame::FocusFrame(GlopFrame *frame)
: SingleParentFrame(frame),
  is_gaining_focus_(false) {
  SetFocusInfo(this, false);
  layer_ = gWindow->RegisterFocusFrame(this);
}

FocusFrame::~FocusFrame() {
  gWindow->UnregisterFocusFrame(layer_, this);
}

bool FocusFrame::IsSubFocusFrame(const FocusFrame *frame) const {
  for (const FocusFrame *temp = this; temp != 0; temp = temp->GetParent()->GetFocusFrame())
  if (temp == frame)
    return true;
  return false;
}

void FocusFrame::DemandFocus() {
  gWindow->DemandFocus(layer_, this, false);
}

// TableauFrame
// ============
//
// We store internal data for our frames in two ways:
//  - Each child has a position structure stored in a light set. A position's id within this light
//    set is guaranteed to be precisely the standard child id for the child corresponding to this
//    position.
//  - We also need to be able to iterate over children in depth order. To do this, we have a
//    vector containing the children in order. When a child is added, it is appended to this list,
//    and when a child is deleted, its entry is replaced with a 0. Now, when we go to iterate over
//    the list during rendering, we resort it if it has changed since the last call.
// There are links from child_pos to ordered_children_. These are guaranteed to be valid at all
// times, even if ordered_children_ is dirty.

// Renders all the frames in the tableau, respecting depth.
// This may cause us to rebuild ordered_children_.
void TableauFrame::Render() const {
  if (order_dirty_) {
    // Note that we need to stable_sort so that children with the same depth will remain in the
    // same relative order.
    stable_sort(ordered_children_.begin(), ordered_children_.end(), ChildOrderCompare(this));
    ordered_children_.resize(GetNumChildren());
    for (int i = 0; i < (int)ordered_children_.size(); i++)
      child_pos_[ordered_children_[i]].order_pos = i;
    order_dirty_ = false;
  }
  GlopFrame::Render();
  for (int i = 0; i < (int)ordered_children_.size(); i++)
    GetChild(ordered_children_[i])->Render();
}

// Adds a child at the given position. We use the fact that the id obtained by inserting into
// MultiParentFrame and into child_pos_ will be identical.
ListId TableauFrame::AddChild(GlopFrame *frame, float rel_x, float rel_y,
                                  float horz_justify, float vert_justify, int depth) {
  ListId result = MultiParentFrame::AddChild(frame);
  ChildPosition pos;
  pos.rel_x = rel_x;
  pos.rel_y = rel_y;
  pos.horz_justify = horz_justify;
  pos.vert_justify = vert_justify;
  pos.depth = depth;
  pos.order_pos = (int)ordered_children_.size();
  ordered_children_.push_back(result);
  order_dirty_ = true;
  ASSERT(ListId(child_pos_.push_back(pos)) == result);
  return result;
}

void TableauFrame::MoveChild(ListId id, int depth) {
  ordered_children_[child_pos_[id].order_pos] = 0;
  child_pos_[id].depth = depth;
  child_pos_[id].order_pos = (int)ordered_children_.size();
  ordered_children_.push_back(id);
  order_dirty_ = true;
}

void TableauFrame::MoveChild(ListId id, float rel_x, float rel_y) {
  ChildPosition *pos = &child_pos_[id];
  pos->rel_x = rel_x;
  pos->rel_y = rel_y;
  GetChild(id)->DirtySize();
}

void TableauFrame::SetChildJustify(ListId id, float horz_justify, float vert_justify) {
  ChildPosition *pos = &child_pos_[id];
  pos->horz_justify = horz_justify;
  pos->vert_justify = vert_justify;
  GetChild(id)->DirtySize();
}

GlopFrame *TableauFrame::RemoveChildNoDelete(ListId id) {
  ordered_children_[child_pos_[id].order_pos] = 0;
  order_dirty_ = true;
  child_pos_.erase(id);
  return MultiParentFrame::RemoveChildNoDelete(id);
}

void TableauFrame::RemoveChild(ListId id) {
  ordered_children_[child_pos_[id].order_pos] = 0;
  order_dirty_ = true;
  child_pos_.erase(id);
  MultiParentFrame::RemoveChild(id);
}

void TableauFrame::ClearChildren() {
  while (GetNumChildren())
    RemoveChild(children_begin());
}

void TableauFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  for (List<GlopFrame*>::const_iterator it = children_begin(); it != children_end(); ++it) {
    const ChildPosition &pos = child_pos_[it];
    (*it)->SetPosition(
        GetX() + int(pos.rel_x * GetWidth() - (*it)->GetWidth() * pos.horz_justify),
        GetY() + int(pos.rel_y * GetHeight() - (*it)->GetHeight() * pos.vert_justify),
        cx1, cy1, cx2, cy2);
  }
}

void TableauFrame::RecomputeSize(int rec_width, int rec_height) {
  SetSize(rec_width, rec_height);
  for (List<GlopFrame*>::const_iterator it = children_begin(); it != children_end(); ++it) {
    const ChildPosition &pos = child_pos_[it];
    float x_frac = (pos.horz_justify == kJustifyLeft? 1 - pos.rel_x :
                    pos.horz_justify == kJustifyRight? pos.rel_x :
                    min(pos.rel_x / pos.horz_justify, (1 - pos.rel_x) / (1 - pos.horz_justify)));
    float y_frac = (pos.vert_justify == kJustifyTop? 1 - pos.rel_y :
                    pos.vert_justify == kJustifyBottom? pos.rel_y :
                    min(pos.rel_y / pos.vert_justify, (1 - pos.rel_y) / (1 - pos.vert_justify)));
    (*it)->UpdateSize(int(rec_width*x_frac), int(rec_height*y_frac));
  }
}

// TableFrame
// ==========

// Constructor - memory allocation is done through ResizeTable.
TableFrame::TableFrame(int num_cols, int num_rows, float default_horz_justify,
                       float default_vert_justify)
: num_cols_(0), num_rows_(0),
  horz_padding_(0), vert_padding_(0),
  default_horz_justify_(default_horz_justify), default_vert_justify_(default_vert_justify),
  row_info_(0), col_info_(0), cell_info_(0) {
  Resize(num_cols, num_rows);
}

// Destructor. The inner frames are deleted as well, but that is done in ~ParentFrame.
TableFrame::~TableFrame() {
  if (row_info_ != 0) free(row_info_);  // row_info_ could be 0 if num_rows_ = 0
  if (col_info_ != 0) free(col_info_);
  if (cell_info_ != 0) free(cell_info_);
}

// Resizes this table to now include num_rows rows and num_cols columns, adding blank data as
// needed, and deleting cells that are now gone, but preserving old data where applicable.
void TableFrame::Resize(int num_cols, int num_rows) {
  int x, y;

  // Remove extraneous columns up to row min(num_rows, num_rows_), realigning memory and
  // deleting unused frames. Later rows are untouched.
  if (num_cols < num_cols_) {
    for (y = 0; y < min(num_rows, num_rows_); y++) {
      for (x = 0; x < num_cols; x++)
        cell_info_[y*num_cols+x] = cell_info_[y*num_cols_+x];
      for (x = num_cols; x < num_cols_; x++)
        if (cell_info_[y*num_cols_+x].child_id != 0)
          RemoveChild(cell_info_[y*num_cols_+x].child_id);
    }
  }

  // Remove extraneous rows, deleting unused frames. Note that the memory for these rows has not
  // yet been touched, regardless of whether num_cols < num_cols_.
  if (num_rows < num_rows_) {
    for (y = num_rows; y < num_rows_; y++)
      for (x = 0; x < num_cols_; x++)
        if (cell_info_[y*num_cols_+x].child_id != 0)
          RemoveChild(cell_info_[y*num_cols_+x].child_id);
  }

  // Reallocate memory
  if (num_cols * num_rows != num_cols_ * num_rows_)
    cell_info_ = (CellInfo*)realloc(cell_info_, sizeof(CellInfo) * num_rows * num_cols);
  if (num_cols != num_cols_)
    col_info_ = (LineInfo*)realloc(col_info_, sizeof(LineInfo) * num_cols);
  if (num_rows != num_rows_)
    row_info_ = (LineInfo*)realloc(row_info_, sizeof(LineInfo) * num_rows);

  // If columns were added, adjust memory up until row min(num_rows, num_rows_). Note this
  // memory has remained untouched up until now.
  if (num_cols > num_cols_) {
    for (y = min(num_rows, num_rows_)-1; y >= 0; y--) {
      for (x = num_cols-1; x >= num_cols_; x--)
        cell_info_[y*num_cols+x].child_id = 0;
      for (x = num_cols_-1; x >= 0; x--)
        cell_info_[y*num_cols+x] = cell_info_[y*num_cols_+x];
    }
  }

  // If rows were added, clear their memory
  if (num_rows > num_rows_) {
    for (y = num_rows_; y < num_rows; y++)
      for (x = 0; x < num_cols; x++)
        cell_info_[y*num_cols+x].child_id = 0;
  }

  // Save the result
  num_rows_ = num_rows;
  num_cols_ = num_cols;
  DirtySize();
}

// Row and column insertion and deletion, providing a more flexible resizing interface.
void TableFrame::InsertRow(int row) {
  Resize(num_cols_, num_rows_ + 1);
  for (int y = num_rows_ - 1; y > row; y--)
    for (int x = 0; x < num_cols_; x++)
      cell_info_[y*num_cols_+x] = cell_info_[(y-1)*num_cols_+x];
  for (int x = 0; x < num_cols_; x++)
    cell_info_[row*num_cols_+x].child_id = 0;
}
void TableFrame::InsertCol(int col) {
  Resize(num_cols_ + 1, num_rows_);
  for (int y = 0; y < num_rows_; y++)
    for (int x = num_cols_ - 1; x > col; x--)
      cell_info_[y*num_cols_+x] = cell_info_[y*num_cols_+x-1];
  for (int y = 0; y < num_cols_; y++)
    cell_info_[y*num_cols_+col].child_id = 0;
}
void TableFrame::DeleteRow(int row) {
  for (int x = 0; x < num_cols_; x++)
    if (cell_info_[row*num_cols_+x].child_id != 0)
      RemoveChild(cell_info_[row*num_cols_+x].child_id);
  for (int y = row; y < num_rows_ - 1; y++)
    for (int x = 0; x < num_cols_; x++)
      cell_info_[y*num_cols_+x] = cell_info_[(y+1)*num_cols_+x];
  for (int x = 0; x < num_cols_; x++)
    cell_info_[(num_rows_-1)*num_cols_+x].child_id = 0;
  Resize(num_cols_, num_rows_ - 1);
}
void TableFrame::DeleteCol(int col) {
  for (int y = 0; y < num_rows_; y++)
    if (cell_info_[y*num_cols_+col].child_id != 0)
      RemoveChild(cell_info_[y*num_cols_+col].child_id);
  for (int y = 0; y < num_rows_; y++)
    for (int x = col; x < num_cols_ - 1; x++)
      cell_info_[y*num_cols_+x] = cell_info_[y*num_cols_+x+1];
  for (int y = 0; y < num_rows_; y++)
    cell_info_[y*num_cols_+num_cols_-1].child_id = 0;
  Resize(num_cols_ - 1, num_rows_);
}

// Clears a cell without deleting the frame that was previously located there.
GlopFrame *TableFrame::ClearCellNoDelete(int col, int row) {
  int index = row*num_cols_ + col;
  GlopFrame *result = (cell_info_[index].child_id == 0? 0 :
                       RemoveChildNoDelete(cell_info_[index].child_id));
  cell_info_[index].child_id = 0;
  DirtySize();
  return result;
}

// Sets all cell information for a single cell. If the cell was previously occupied, the
// previous frame is deleted. The new frame may be NULL, in which case the spot is left blank.
void TableFrame::SetCell(int col, int row, GlopFrame *frame, const CellSize &width,
                         const CellSize &height, float horz_justify, float vert_justify) {
  int index = row*num_cols_ + col;
  if (cell_info_[index].child_id != 0)
    delete ClearCellNoDelete(col, row);
  if (frame != 0) {
    cell_info_[index].child_id = AddChild(frame);
    cell_info_[index].width = width;
    cell_info_[index].height = height;
    cell_info_[index].horz_justify = horz_justify;
    cell_info_[index].vert_justify = vert_justify;
  }
}

// Moves the table and all children.
void TableFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  for (int i = 0; i < num_rows_; i++)
    for (int j = 0; j < num_cols_; j++) {
      const CellInfo &info = cell_info_[i*num_cols_ + j];
      ListId child_id = info.child_id;
      if (child_id != 0) {
        GetChild(child_id)->SetPosition(
          GetX() + col_info_[j].pos +
            int(info.horz_justify * (col_info_[j].size - GetChild(child_id)->GetWidth())),
          GetY() + row_info_[i].pos +
            int(info.vert_justify * (row_info_[i].size - GetChild(child_id)->GetHeight())),
          cx1, cy1, cx2, cy2);
      }
    }
}

// Recomputes the size of this table, resizing and repositioning each cell. There are two nasty
// technical details here:
//   1. We have to handle Match and Max sizes. See CellSize in GlopFrameBase.h.
//   2. If cells are specified as fractional or default sizes, there could be cumulative rounding
//      error, which could cause the table to be sized incorrectly even if it it's children can
//      all resize arbitrarily. To prevent this, we choose certain rows and columns, and then
//      round up all heights in that row and all widths in that column.
void TableFrame::RecomputeSize(int rec_width, int rec_height) {
  int x, y;

  // Calculate the padding sizes
  int hpad = int(gWindow->GetWidth() * horz_padding_ + 0.5f);
  int vpad = int(gWindow->GetHeight() * vert_padding_ + 0.5f);
  rec_width -= hpad * (GetNumCols() - 1);
  rec_height -= vpad * (GetNumRows() - 1);

  // Update all sizes to have value 0
  for (y = 0; y < num_rows_; y++)
    row_info_[y].size = 0;
  for (x = 0; x < num_cols_; x++)
    col_info_[x].size = 0;

  // In which columns should we round width up?
  bool *col_round_up = new bool[num_cols_];
  double cum_size = 0;
  for (x = 0; x < num_cols_; x++) {
    double col_size = 0;
    for (y = 0; y < num_rows_; y++) {
      double this_size = 0;
      if (cell_info_[y*num_cols_+x].width.type == CellSize::kDefault)
        this_size = 1.0 / num_cols_;
      else if (cell_info_[y*num_cols_+x].width.type == CellSize::kFraction)
        this_size = cell_info_[y*num_cols_+x].width.fraction;
      col_size = max(this_size * rec_width + 1e-6 / num_cols_, col_size);
    }
    double new_cum_size = cum_size + col_size;
    col_round_up[x] = (int(cum_size) + int(col_size) == int(new_cum_size)? 0 : 1);
    cum_size = new_cum_size;
  }

  // In which rows should we round height up?
  bool *row_round_up = new bool[num_rows_];
  cum_size = 0;
  for (y = 0; y < num_rows_; y++) {
    double row_size = 0;
    for (x = 0; x < num_cols_; x++) {
      double this_size = 0;
      if (cell_info_[y*num_cols_+x].height.type == CellSize::kDefault)
        this_size = 1.0 / num_rows_;
      else if (cell_info_[y*num_cols_+x].height.type == CellSize::kFraction)
        this_size = cell_info_[y*num_cols_+x].height.fraction;
      row_size = max(this_size * rec_height + 1e-6 / num_rows_, row_size);
    }
    double new_cum_size = cum_size + row_size;
    row_round_up[y] = (int(cum_size) + int(row_size) == int(new_cum_size)? 0 : 1);
    cum_size = new_cum_size;
  }

  // All sizing is done in four passes. In each pass, we handle cells with a different kind of
  // CellSize. See CellSize in GlopFrameBase.h.
  for (int pass = 0; pass < 4; pass++) {
    // Compute the size of each square
    for (y = 0; y < num_rows_; y++)
      for (x = 0; x < num_cols_; x++) {
        int index = y * num_cols_ + x;
        if (cell_info_[index].child_id == 0)
          continue;
        
        // Make sure this is the right pass for this cell
        if (pass == 3) {
          if (cell_info_[index].width.type != CellSize::kMaxDoublePass &&
              cell_info_[index].height.type != CellSize::kMaxDoublePass)
            continue;
        } else {
          int real_pass;
          if (cell_info_[index].width.type == CellSize::kMatch ||
              cell_info_[index].height.type == CellSize::kMatch)
            real_pass = 2;
          else if (cell_info_[index].width.type == CellSize::kMax ||
                   cell_info_[index].height.type == CellSize::kMax)
            real_pass = 1;
          else
            real_pass = 0;
          if (real_pass != pass)
            continue;
        }

        // Compute the recommended size for this cell
        int w, h;
        CellSize::Type wtype = cell_info_[index].width.type,
                       htype = cell_info_[index].height.type;
        if (wtype == CellSize::kMatch) {
          w = col_info_[x].size;
        } else if (wtype == CellSize::kMax || wtype == CellSize::kMaxDoublePass) {
          w = rec_width - GetWidth() + col_info_[x].size;
        } else {
          double mult = (wtype == CellSize::kDefault? 1.0 / num_cols_ :
                         cell_info_[index].width.fraction);
          w = int(mult * rec_width + (col_round_up[x]? 1-1e-6 : 0));
        }
        if (htype == CellSize::kMatch) {
          h = row_info_[y].size;
        } else if (htype == CellSize::kMax || htype == CellSize::kMaxDoublePass) {
          h = rec_height - GetHeight() + row_info_[y].size;
        } else {
          double mult = (htype == CellSize::kDefault? 1.0 / num_rows_ :
                         cell_info_[index].height.fraction);
          h = int(mult * rec_height + (row_round_up[y]? 1-1e-6 : 0));
        }

        // Resize the cell and its row, column
        ListId child_id = cell_info_[index].child_id;
        GetChild(child_id)->UpdateSize(w, h);
        row_info_[y].size = max(row_info_[y].size, GetChild(child_id)->GetHeight());
        col_info_[x].size = max(col_info_[x].size, GetChild(child_id)->GetWidth());
      }

    // Compute the size left over and row/column positions
    int width = 0, height = 0;
    for (x = 0; x < num_cols_; x++) {
      col_info_[x].pos = width;
      width += col_info_[x].size + hpad;
    }
    for (y = 0; y < num_rows_; y++) {
      row_info_[y].pos = height;
      height += row_info_[y].size + vpad;
    }
    SetSize(width - hpad, height - vpad);
  }
  delete[] col_round_up;
  delete[] row_round_up;
}

// RecSizeFrame
// ============

void RecWidthFrame::RecomputeSize(int rec_width, int rec_height) {
  SingleParentFrame::RecomputeSize(int(gWindow->GetWidth() * rec_width_override_), rec_height);
}

void RecHeightFrame::RecomputeSize(int rec_width, int rec_height) {
  SingleParentFrame::RecomputeSize(rec_width, int(gWindow->GetWidth() * rec_height_override_));
}

void RecSizeFrame::RecomputeSize(int rec_width, int rec_height) {
  SingleParentFrame::RecomputeSize(int(gWindow->GetWidth() * rec_width_override_),
                                   int(gWindow->GetHeight() * rec_height_override_));
}

// MinSizeFrame
// ============

void MinWidthFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  if (GetChild() != 0)
    GetChild()->SetPosition(screen_x + x_offset_, screen_y, cx1, cy1, cx2, cy2);
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

void MinWidthFrame::RecomputeSize(int rec_width, int rec_height) {
  int w = 0, h = 0;
  if (GetChild() != 0) {
    GetChild()->UpdateSize(rec_width, rec_height);
    w = GetChild()->GetWidth();
    h = GetChild()->GetHeight();
  }
  int min_w = (min_width_ == kSizeLimitRec? rec_width : int(gWindow->GetWidth() * min_width_));
  x_offset_ = int(max(min_w - w, 0) * (horz_justify_));
  SetSize(max(w, min_w), h);
}

void MinHeightFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  if (GetChild() != 0)
    GetChild()->SetPosition(screen_x, screen_y + y_offset_, cx1, cy1, cx2, cy2);
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

void MinHeightFrame::RecomputeSize(int rec_width, int rec_height) {
  int w = 0, h = 0;
  if (GetChild() != 0) {
    GetChild()->UpdateSize(rec_width, rec_height);
    w = GetChild()->GetWidth();
    h = GetChild()->GetHeight();
  }
  int min_h = (min_height_ == kSizeLimitRec? rec_height : int(gWindow->GetHeight() * min_height_));
  y_offset_ = int(max(min_h - h, 0) * (vert_justify_));
  SetSize(w, max(h, min_h));
}

void MinSizeFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  if (GetChild() != 0)
    GetChild()->SetPosition(screen_x + x_offset_, screen_y + y_offset_, cx1, cy1, cx2, cy2);
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

void MinSizeFrame::RecomputeSize(int rec_width, int rec_height) {
  int w = 0, h = 0;
  if (GetChild() != 0) {
    GetChild()->UpdateSize(rec_width, rec_height);
    w = GetChild()->GetWidth();
    h = GetChild()->GetHeight();
  }
  int min_w = (min_width_ == kSizeLimitRec? rec_width : int(gWindow->GetWidth() * min_width_));
  x_offset_ = int(max(min_w - w, 0) * (horz_justify_));
  int min_h = (min_height_ == kSizeLimitRec? rec_height : int(gWindow->GetHeight() * min_height_));
  y_offset_ = int(max(min_h - h, 0) * (vert_justify_));
  SetSize(max(w, min_w), max(h, min_h));
}

// MaxSizeFrame
// ============

// A helper function used by MaxSizeFrame and ScrollingFrame. This indicates where we should
// scroll to on a ping.
static void ScrollToPing(int scroll_x, int scroll_y, int view_w, int view_h, int total_w,
                         int total_h, int x1, int y1, int x2, int y2, bool center,
                         int *new_scroll_x, int *new_scroll_y, bool ignore_ub) {
  // Handle the special case where the pinged region is larger than the total view area (in that
  // case, look at the top-left part of the region).
  if (!center && x2 >= x1 + view_w)
    x2 = x1 + view_w - 1;
  if (!center && y2 >= y1 + view_h)
    y2 = y1 + view_h - 1;

  // Handle horizontal movement
  if (total_w > view_w) {
    if (center) {
      *new_scroll_x = (x1 + x2 - view_w) / 2;
    } else if (x1 < scroll_x) {
      *new_scroll_x = x1;
    } else if (x2 >= scroll_x + view_w) {
      *new_scroll_x = (x2 - view_w + 1);
    } else {
      *new_scroll_x = scroll_x;
    }
  } else {
    *new_scroll_x = 0;
  }
  if (*new_scroll_x < 0)
    *new_scroll_x = 0;
  if (*new_scroll_x > total_w - view_w && !ignore_ub)
    *new_scroll_x = total_w - view_w;

  // Handle vertical movement
  if (total_h > view_h) {
    if (center) {
      *new_scroll_y = (y1 + y2 - view_h) / 2;
    } else if (y1 < scroll_y) {
      *new_scroll_y = y1;
    } else if (y2 >= scroll_y + view_h) {
      *new_scroll_y = (y2 - view_h + 1);
    } else {
      *new_scroll_y = scroll_y;
    }
  } else {
    *new_scroll_y = 0;
  }
  if (*new_scroll_y < 0)
    *new_scroll_y = 0;
  if (*new_scroll_y > total_h - view_h && !ignore_ub)
    *new_scroll_y = total_h - view_h;
}

const float kSizeLimitNone = -1e10f;
MaxWidthFrame::MaxWidthFrame(GlopFrame *frame, float max_width, float horz_justify)
: SingleParentFrame(new MaxSizeFrame(frame, max_width, kSizeLimitNone, horz_justify,
                                     kJustifyTop)) {}

MaxHeightFrame::MaxHeightFrame(GlopFrame *frame, float max_height, float vert_justify)
: SingleParentFrame(new MaxSizeFrame(frame, kSizeLimitNone, max_height, kJustifyLeft,
                                     vert_justify)) {}

MaxSizeFrame::MaxSizeFrame(GlopFrame *frame, float max_width, float max_height, float horz_justify,
                           float vert_justify)
: SingleParentFrame(new ClippedFrame(frame)), must_recenter_(false),
  x_offset_(0), y_offset_(0),
  max_width_(max_width), max_height_(max_height) {
  GetChild()->NewRelativePing(horz_justify, vert_justify, true);
}

void MaxSizeFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  ClippedFrame *clipped_frame = (ClippedFrame*)(GetChild());
  if (clipped_frame != 0) {
    if (max_width_ != kSizeLimitNone && max_height_ != kSizeLimitNone)
      clipped_frame->SetClipping(GetX(), GetY(), GetX2(), GetY2());
    else if (max_width_ != kSizeLimitNone)
      clipped_frame->SetClipping(GetX(), cy1, GetX2(), cy2);
    else if (max_height_ != kSizeLimitNone)
      clipped_frame->SetClipping(cx1, GetY(), cx2, GetY2());
    clipped_frame->SetPosition(GetX() + x_offset_, GetY() + y_offset_, cx1, cy1, cx2, cy2);
  }
}

void MaxSizeFrame::RecomputeSize(int rec_width, int rec_height) {
  if (GetChild() != 0) {
    // Track the old scrolling position
    double old_x = (GetChild()->GetWidth() == 0? 0 :
      (GetX() + GetWidth()*0.5 - GetChild()->GetX()) / GetChild()->GetWidth());
    double old_y = (GetChild()->GetHeight() == 0? 0 :
      (GetY() + GetHeight()*0.5 - GetChild()->GetY()) / GetChild()->GetHeight());

    // Update the child size
    GetChild()->UpdateSize(rec_width, rec_height);

    // Update our size
    int max_w = (max_width_ == kSizeLimitNone? kClipInfinity :
                 max_width_ == kSizeLimitRec? rec_width :
                 int(gWindow->GetWidth() * max_width_));
    int max_h = (max_height_ == kSizeLimitNone? kClipInfinity :
                 max_height_ == kSizeLimitRec? rec_height :
                 int(gWindow->GetHeight() * max_height_));
    SetSize(min(GetChild()->GetWidth(), max_w), min(GetChild()->GetHeight(), max_h));

    // Position the child within the new size to be approximately where it was before
    if (must_recenter_) {
      int px = int(old_x * GetChild()->GetWidth());
      int py = int(old_y * GetChild()->GetHeight());
      ScrollToChildPing(px, py, px, py, true);
      must_recenter_ = false;
    }
  } else {
    SetSize(0, 0);
  }
}

void MaxSizeFrame::OnChildPing(GlopFrame *child, int x1, int y1, int x2, int y2, bool center) {
  // Do the scrolling for a ping
  int old_x = child->GetX(), old_y = child->GetY();
  ScrollToChildPing(x1 - x_offset_, y1 - y_offset_, x2 - x_offset_, y2 - y_offset_, center);

  // Propogate the ping upwards
  int dx = child->GetX() - old_x, dy = child->GetY() - old_y;
  NewAbsolutePing(x1 + dx, y1 + dy, x2 + dx, y2 + dy, center);
}

void MaxSizeFrame::ScrollToChildPing(int x1, int y1, int x2, int y2, bool center) {
  int new_scroll_x, new_scroll_y;
  ::ScrollToPing(-x_offset_, -y_offset_, GetWidth(), GetHeight(), GetChild()->GetWidth(),
                 GetChild()->GetHeight(), x1, y1, x2, y2, center, &new_scroll_x, &new_scroll_y,
                 true);
  x_offset_ = -new_scroll_x;
  y_offset_ = -new_scroll_y;
  SetPosition(GetX(), GetY(), GetClipX1(), GetClipY1(), GetClipX2(), GetClipY2());
}

// ScrollingFrame
// ==============
//
// Note: ScrollingFrame's try to maintain their center in position when the inner frame resizes.
// This is different from the behavior of MaxSizeFrame (as dictated by TextPrompt's). Perhaps
// ScrollingFrame should be changed?
class UnfocusableScrollingFrame: public MultiParentFrame {
 public:
  UnfocusableScrollingFrame(GlopFrame *frame, const SliderViewFactory *factory)
  : MultiParentFrame(),
    horz_slider_(0), vert_slider_(0),
    view_factory_(factory) {
    clipped_inner_frame_ = new ClippedFrame(inner_frame_ = frame);
    AddChild(clipped_inner_frame_);
  }
  string GetType() const {return "UnfocusableScrollingFrame";}
  
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
    GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
    clipped_inner_frame_->SetClipping(GetX(), GetY(), GetX() + max(inner_view_width_ - 1, 0),
                                      GetY() + max(inner_view_height_ - 1, 0));
    clipped_inner_frame_->SetPosition(screen_x -
      (horz_slider_? horz_slider_->GetTabPosition() : 0), screen_y -
      (vert_slider_? vert_slider_->GetTabPosition() : 0), cx1, cy1, cx2, cy2);
    if (horz_slider_)
      horz_slider_->SetPosition(screen_x, screen_y + inner_view_height_, cx1, cy1, cx2, cy2);
    if (vert_slider_)
      vert_slider_->SetPosition(screen_x + inner_view_width_, screen_y, cx1, cy1, cx2, cy2);
  }

 protected:   
  void RecomputeSize(int rec_width, int rec_height) {
    // Compute our old horizontal and vertical scroll positions
    int old_inner_view_width = inner_view_width_, old_inner_view_height = inner_view_height_;
    int old_inner_total_width = inner_frame_->GetWidth(), 
        old_inner_total_height = inner_frame_->GetHeight();
    double old_horz_position = (horz_slider_? 
      (horz_slider_->GetTabPosition() + 0.5*inner_view_width_) / old_inner_total_width : 0);
    double old_vert_position = (vert_slider_? 
      (vert_slider_->GetTabPosition() + 0.5*inner_view_height_) / old_inner_total_height : 0);

    // Figure out which sliders need to exist, and create place-holders
    inner_view_width_ = rec_width;
    inner_view_height_ = rec_height;
    SliderFrame *old_horz_slider = horz_slider_, *old_vert_slider = vert_slider_;
    horz_slider_ = vert_slider_ = 0;
    while (1) {
      bool made_change = false;
      clipped_inner_frame_->UpdateSize(inner_view_width_, inner_view_height_);
      if (clipped_inner_frame_->GetWidth() > inner_view_width_ && horz_slider_ == 0) {
        horz_slider_ = old_horz_slider;
        if (horz_slider_ == 0) {
          horz_slider_ = new SliderFrame(SliderFrame::Horizontal, 0, 0, 0, view_factory_);
          horz_slider_->AddDecHotKey(kGuiKeyScrollLeft);
          horz_slider_->AddBigDecHotKey(kGuiKeyPageLeft);
          horz_slider_->AddIncHotKey(kGuiKeyScrollRight);
          horz_slider_->AddBigIncHotKey(kGuiKeyPageRight);
          horz_slider_id_ = AddChild(horz_slider_);
        }
        horz_slider_->UpdateSize(rec_width, rec_height);
        inner_view_height_ -= horz_slider_->GetHeight();
        made_change = true;
      }
      if (clipped_inner_frame_->GetHeight() > inner_view_height_ && vert_slider_ == 0) {
        vert_slider_ = old_vert_slider;
        if (vert_slider_ == 0) {
          vert_slider_ = new SliderFrame(SliderFrame::Vertical, 0, 0, 0, view_factory_);
          vert_slider_->AddDecHotKey(kGuiKeyScrollUp);
          vert_slider_->AddBigDecHotKey(kGuiKeyPageUp);
          vert_slider_->AddIncHotKey(kGuiKeyScrollDown);
          vert_slider_->AddBigIncHotKey(kGuiKeyPageDown);
          vert_slider_id_ = AddChild(vert_slider_);
        }
        vert_slider_->UpdateSize(rec_width, rec_height);
        inner_view_width_ -= vert_slider_->GetWidth();
        made_change = true;
      }
      if (!made_change)
        break;
    }

    // Set our width and height
    if (horz_slider_ == 0)
      inner_view_width_ = clipped_inner_frame_->GetWidth();
    if (vert_slider_ == 0)
      inner_view_height_ = clipped_inner_frame_->GetHeight();
    int total_width = inner_view_width_, total_height = inner_view_height_;
    if (horz_slider_ != 0)
      total_height += horz_slider_->GetHeight();
    if (vert_slider_ != 0)
      total_width += vert_slider_->GetWidth();
    SetSize(total_width, total_height);

    // Delete old, unused sliders
    if (old_horz_slider != 0 && horz_slider_ == 0) {
      RemoveChild(horz_slider_id_);
      horz_slider_id_ = 0;
    }
    if (old_vert_slider != 0 && vert_slider_ == 0) {
      RemoveChild(vert_slider_id_);
      vert_slider_id_ = 0;
    }

    // Configure the new sliders
    if (horz_slider_) {
      horz_slider_->SetTabSize(inner_view_width_);
      horz_slider_->SetTotalSize(clipped_inner_frame_->GetWidth());
      horz_slider_->SetTabPosition(int(
        old_horz_position*clipped_inner_frame_->GetWidth() - 0.5*inner_view_width_ + 0.5));
    }
    if (vert_slider_) {
      vert_slider_->SetTabSize(inner_view_height_);
      vert_slider_->SetTotalSize(clipped_inner_frame_->GetHeight());
      vert_slider_->SetTabPosition(int(
        old_vert_position*clipped_inner_frame_->GetHeight() - 0.5*inner_view_height_ + 0.5));
    }
  }

  void OnChildPing(GlopFrame *child, int x1, int y1, int x2, int y2, bool center) {
    // Only do something special if it's the inner frame that generated the ping
    if (child != clipped_inner_frame_) {
      MultiParentFrame::OnChildPing(child, x1, y1, x2, y2, center);
      return;
    }

    // Do the scrolling for a ping
    int old_x = clipped_inner_frame_->GetX(), old_y = clipped_inner_frame_->GetY();
    int scroll_x = (horz_slider_? horz_slider_->GetTabPosition() : 0);
    int scroll_y = (vert_slider_? vert_slider_->GetTabPosition() : 0);
    int new_scroll_x, new_scroll_y;
    ScrollToPing(scroll_x, scroll_y, inner_view_width_, inner_view_height_,
                 clipped_inner_frame_->GetWidth(), clipped_inner_frame_->GetHeight(),
                 x1 + scroll_x, y1 + scroll_y, x2 + scroll_x, y2 + scroll_y, center,
                 &new_scroll_x, &new_scroll_y, false);
    if (horz_slider_ != 0) horz_slider_->SetTabPosition(new_scroll_x);
    if (vert_slider_ != 0) vert_slider_->SetTabPosition(new_scroll_y);
    SetPosition(GetX(), GetY(), GetClipX1(), GetClipY1(), GetClipX2(), GetClipY2());

    // Propogate the ping upwards
    int dx = clipped_inner_frame_->GetX() - old_x, dy = clipped_inner_frame_->GetY() - old_y;
    NewAbsolutePing(x1 + dx, y1 + dy, x2 + dx, y2 + dy, center);
  }

  GlopFrame *inner_frame_;
  ClippedFrame *clipped_inner_frame_;
  SliderFrame *horz_slider_, *vert_slider_;
  ListId horz_slider_id_, vert_slider_id_;
  int inner_view_width_, inner_view_height_;
  const SliderViewFactory *view_factory_;
  DISALLOW_EVIL_CONSTRUCTORS(UnfocusableScrollingFrame);
};

ScrollingFrame::ScrollingFrame(GlopFrame *frame, const SliderViewFactory *factory)
: FocusFrame(new UnfocusableScrollingFrame(frame, factory)) {}
