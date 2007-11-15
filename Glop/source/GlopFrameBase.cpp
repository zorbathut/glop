// Includes
#include "GlopFrameBase.h"
#include "GlopWindow.h"
#include "LightSet.h"
#include <algorithm>
using namespace std;

// Constants
const int kClipInfinity = 1000000000;
const int kClipMinusInfinity = -kClipInfinity;

// FrameStyle
// ==========

FrameStyle::FrameStyle()
: font_color(0, 0, 0),
  font_height(0.025f),
  button_border_size(0.003f),
  button_selection_color(0.2f, 0, 0.8f),
  button_border_color(0.2f, 0.2f, 0.2f),
  button_highlight_color(0.95f, 0.95f, 0.95f),
  button_lowlight_color(0.5f, 0.5f, 0.5f),
  button_unpressed_inner_color(0.85f, 0.85f, 0.85f),
  button_pressed_inner_color(0.65f, 0.65f, 0.65f),
  window_border_highlight_color(0.8f, 0.8f, 0.95f),
  window_border_lowlight_color(0.5f, 0.5f, 0.6f),
  window_inner_color(0.75f, 0.75f, 0.83f),
  window_title_color(0, 0, 0) {}

FrameStyle *gDefaultStyle = new FrameStyle();

// GlopFrame
// =========

// Constructor. Does nothing.
GlopFrame::GlopFrame()
: parent_(0),
  screen_x_(0),
  screen_y_(0),
  clip_x1_(kClipMinusInfinity),
  clip_y1_(kClipMinusInfinity),
  clip_x2_(kClipInfinity),
  clip_y2_(kClipInfinity),
  width_(0),
  height_(0),
  is_in_focus_(false) {
  DirtySize();
}

// Destructor. Deletes all pings that have been queued up for this frame.
GlopFrame::~GlopFrame() {
  window()->UnregisterAllPings(this);
}

// Propogate focus demands up until we reach a FocusFrame.
void GlopFrame::DemandFocus() {
  if (parent_ != 0)
    parent_->DemandFocus();
}

// Marks that this frame needs its size recomputed. The parent must also recompute its size
// for this to happen, so the request is propogated upwards.
void GlopFrame::DirtySize() {
  old_rec_width_ = old_rec_height_ = -1;
  if (parent_ != 0)
    parent_->DirtySize();
}

void GlopFrame::UpdateSize(int rec_width, int rec_height) {
  if (old_rec_width_ == rec_width && old_rec_height_ == rec_height)
    return;
  RecomputeSize(rec_width, rec_height);
  old_rec_width_ = rec_width;
  old_rec_height_ = rec_height;
}

bool GlopFrame::IsPointVisible(int screen_x, int screen_y) const {
  int x1 = max(screen_x_, clip_x1_), y1 = max(screen_y_, clip_y1_),
      x2 = min(screen_x_+width_-1, clip_x2_), y2 = min(screen_y_+height_-1, clip_y2_);
  return (screen_x >= x1 && screen_y >= y1 && screen_x <= x2 && screen_y <= y2);
}

void GlopFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  screen_x_ = screen_x;
  screen_y_ = screen_y;
  clip_x1_ = cx1;
  clip_y1_ = cy1;
  clip_x2_ = cx2;
  clip_y2_ = cy2;
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
  window()->RegisterPing(ping);
}

// SingleParentFrame
// =================

GlopFrame *SingleParentFrame::RemoveChildNoDelete() {
  DirtySize();
  GlopFrame *old_child = child_;
  child_ = 0;
  if (old_child != 0) old_child->parent_ = 0;
  return old_child;
}

void SingleParentFrame::SetChild(GlopFrame *frame) {
  DirtySize();
  if (child_ != 0) delete child_;
  child_ = frame;
  if (child_ != 0) child_->parent_ = this;
}

// MultiParentFrame
// ================

// Renders all child frames. We automatically prune the children if they are completely outside
// the clipping rectangle.
void MultiParentFrame::Render() {
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id)) {
    int x = children_[id]->GetX(), y = children_[id]->GetY(), 
        w = children_[id]->GetWidth(), h = children_[id]->GetHeight();
    if (x+w > clip_x1_ && y+h > clip_y1_ && x <= clip_x2_ && y <= clip_y2_)
      children_[id]->Render();
  }
}

void MultiParentFrame::OnKeyEvent(const KeyEvent &event, int dt) {
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
    children_[id]->OnKeyEvent(event, dt);
}

void MultiParentFrame::Think(int dt) {
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
    children_[id]->Think(dt);
}

// Delegates to set all children to the same position as us.
void MultiParentFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
    children_[id]->SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
}

bool MultiParentFrame::IsFocusMagnet(const KeyEvent &event) const {
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
    if (children_[id]->IsFocusMagnet(event))
      return true;
  return false;
}

bool MultiParentFrame::IsFocusKeeper(const KeyEvent &event) const {
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
    if (children_[id]->IsFocusKeeper(event))
      return true;
  return false;
}

// Delegates to set our size to be our maximum child size - a default implementation that will
// surely not be useful very often.
void MultiParentFrame::RecomputeSize(int rec_width, int rec_height) {
  int new_width = 0, new_height = 0;
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id)) {
    children_[id]->UpdateSize(rec_width, rec_height);
    new_width = max(new_width, children_[id]->GetWidth());
    new_height = max(new_height, children_[id]->GetHeight());
  }
  SetSize(new_width, new_height);
}

void MultiParentFrame::SetIsInFocus(bool is_in_focus) {
  if (is_in_focus != is_in_focus_) {
    GlopFrame::SetIsInFocus(is_in_focus);
    for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
      children_[id]->SetIsInFocus(is_in_focus);
  }
}

LightSetId MultiParentFrame::AddChild(GlopFrame *frame) {
  DirtySize();
  LightSetId result = children_.InsertItem(frame);
  frame->parent_ = this;
  return result;
}

LightSetId MultiParentFrame::RemoveChild(LightSetId id) {
  delete children_[id];
  return children_.RemoveItem(id);
}

GlopFrame *MultiParentFrame::RemoveChildNoDelete(LightSetId id) {
  GlopFrame *old_child = children_[id];
  children_.RemoveItem(id);
  old_child->parent_ = 0;
  return old_child;
}

void MultiParentFrame::ClearChildren() {
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
    id = RemoveChild(id);
}

void MultiParentFrame::OnWindowResize(int width, int height) {
  for (LightSetId id = children_.GetFirstId(); id != 0; id = children_.GetNextId(id))
    children_[id]->OnWindowResize(width, height);
}

// PaddedFrame
// ===========

void PaddedFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  if (GetChild() != 0) {
    GetChild()->SetPosition(screen_x + GetLeftPadding(), screen_y + GetTopPadding(),
                            cx1, cy1, cx2, cy2);
  }
}

void PaddedFrame::RecomputeSize(int rec_width, int rec_height) {
  if (GetChild() != 0) {
    GetChild()->UpdateSize(rec_width - GetLeftPadding() - GetRightPadding(),
                           rec_height - GetTopPadding() - GetBottomPadding());
    SetSize(GetChild()->GetWidth() + GetLeftPadding() + GetRightPadding(),
            GetChild()->GetHeight() + GetTopPadding() + GetBottomPadding());
  } else {
    SetSize(rec_width, rec_height);
  }
}

void PaddedFrame::SetPadding(int left_padding, int top_padding, int right_padding,
                             int bottom_padding) {
  if (left_padding + right_padding != GetLeftPadding() + GetRightPadding() ||
      top_padding + bottom_padding != GetTopPadding() + GetBottomPadding())
    DirtySize();
  left_padding_ = left_padding;
  top_padding_ = top_padding;
  right_padding_ = right_padding;
  bottom_padding_ = bottom_padding;
}

// ScalingPaddedFrame
// ==================

void ScalingPaddedFrame::RecomputeSize(int rec_width, int rec_height) {
  SetPadding(int(scaled_left_padding_ * window()->GetWidth()),
             int(scaled_top_padding_ * window()->GetHeight()),
             int(scaled_right_padding_ * window()->GetWidth()),
             int(scaled_bottom_padding_ * window()->GetHeight()));
  PaddedFrame::RecomputeSize(rec_width, rec_height);
}
// FocusFrame
// ==========
//
// A FocusFrame does little logic on its own - it just passes requests onto the GlopWindow.

FocusFrame::FocusFrame(GlopFrame *frame): SingleParentFrame(frame) {
  layer_ = window()->RegisterFocusFrame(this);
}

FocusFrame::~FocusFrame() {
  window()->UnregisterFocusFrame(layer_, this);
}

void FocusFrame::DemandFocus() {
  window()->DemandFocus(layer_, this);
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
void TableauFrame::Render() {
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
LightSetId TableauFrame::AddChild(GlopFrame *frame, float rel_x, float rel_y,
                                  float horz_justify, float vert_justify, int depth) {
  LightSetId result = MultiParentFrame::AddChild(frame);
  ChildPosition pos;
  pos.rel_x = rel_x;
  pos.rel_y = rel_y;
  pos.horz_justify = horz_justify;
  pos.vert_justify = vert_justify;
  pos.depth = depth;
  pos.order_pos = (int)ordered_children_.size();
  ordered_children_.push_back(result);
  order_dirty_ = true;
  ASSERT(child_pos_.InsertItem(pos) == result);
  return result;
}

// Moves the given child to have the given depth.
// It will always be rendered after any children at the same depth.
void TableauFrame::MoveChild(LightSetId id, int depth) {
  ordered_children_[child_pos_[id].order_pos] = 0;
  child_pos_[id].depth = depth;
  child_pos_[id].order_pos = (int)ordered_children_.size();
  ordered_children_.push_back(id);
  order_dirty_ = true;
}

// Move a child's position within the tableau. Calls DirtySize.
void TableauFrame::MoveChild(LightSetId id, float rel_x, float rel_y) {
  ChildPosition *pos = &child_pos_[id];
  pos->rel_x = rel_x;
  pos->rel_y = rel_y;
  GetChild(id)->DirtySize();
}

// Changes a child's justification within the tableau. Calls DirtySize.
void TableauFrame::SetChildJustify(LightSetId id, float horz_justify, float vert_justify) {
  ChildPosition *pos = &child_pos_[id];
  pos->horz_justify = horz_justify;
  pos->vert_justify = vert_justify;
  GetChild(id)->DirtySize();
}

// Removes a child without deleting it.
GlopFrame *TableauFrame::RemoveChildNoDelete(LightSetId id) {
  ordered_children_[child_pos_[id].order_pos] = 0;
  order_dirty_ = true;
  child_pos_.RemoveItem(id);
  return MultiParentFrame::RemoveChildNoDelete(id);
}

// Removes and deletes a child.
LightSetId TableauFrame::RemoveChild(LightSetId id) {
  ordered_children_[child_pos_[id].order_pos] = 0;
  order_dirty_ = true;
  child_pos_.RemoveItem(id);
  return MultiParentFrame::RemoveChild(id);
}

// Clears all children - overwritten because the ParentFrame version only calls the ParentFrame
// version of RemoveChild.
void TableauFrame::ClearChildren() {
  for (LightSetId id = GetFirstChildId(); id != 0; id = GetNextChildId(id))
    id = RemoveChild(id);
}
// Reposition the tableau and all children (according to child_pos_).
void TableauFrame::SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
  GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  for (LightSetId id = GetFirstChildId(); id != 0; id = GetNextChildId(id)) {
    const ChildPosition &pos = child_pos_[id];
    GlopFrame *child = GetChild(id);
    child->SetPosition(
        GetX() + int(pos.rel_x * GetWidth() - child->GetWidth() * pos.horz_justify),
        GetY() + int(pos.rel_y * GetHeight() - child->GetHeight() * pos.vert_justify),
        cx1, cy1, cx2, cy2);
  }
}

// Recomputes the tableau size. Children are recommended to be the largest size that will keep
// them within the tableau (accounting for position and justification). The tableau itself takes
// the recommended size exactly.
void TableauFrame::RecomputeSize(int rec_width, int rec_height) {
  SetSize(rec_width, rec_height);
  for (LightSetId id = GetFirstChildId(); id != 0; id = GetNextChildId(id)) {
    const ChildPosition &pos = child_pos_[id];
    float x_frac = (pos.horz_justify == kJustifyLeft? 1 - pos.rel_x :
                    pos.horz_justify == kJustifyRight? pos.rel_x :
                    min(pos.rel_x / pos.horz_justify, (1 - pos.rel_x) / (1 - pos.horz_justify)));
    float y_frac = (pos.vert_justify == kJustifyTop? 1 - pos.rel_y :
                    pos.vert_justify == kJustifyBottom? pos.rel_y :
                    min(pos.rel_y / pos.vert_justify, (1 - pos.rel_y) / (1 - pos.vert_justify)));
    GetChild(id)->UpdateSize(int(rec_width*x_frac), int(rec_height*y_frac));
  }
}

// TableFrame
// ==========

// Constructor - memory allocation is done through ResizeTable.
TableFrame::TableFrame(int num_cols, int num_rows, float default_horz_justify,
                       float default_vert_justify)
: num_cols_(0),
  num_rows_(0),
  default_horz_justify_(default_horz_justify),
  default_vert_justify_(default_vert_justify),
  row_info_(0),
  col_info_(0),
  cell_info_(0) {
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
      LightSetId child_id = info.child_id;
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
          if (cell_info_[index].width.type == CellSize::kMaxDoublePass ||
              cell_info_[index].height.type == CellSize::kMaxDoublePass)
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
        if (wtype == CellSize::kMatch)
          w = col_info_[x].size;
        else if (htype == CellSize::kMax || htype == CellSize::kMaxDoublePass)
          w = rec_width - GetWidth() + col_info_[x].size;
        else {
          double mult = (wtype == CellSize::kDefault? 1.0 / num_cols_ :
                         cell_info_[index].width.fraction);
          w = int(mult * rec_width + (col_round_up[x]? 1-1e-6 : 0));
        }
        if (htype == CellSize::kMatch)
          h = row_info_[y].size;
        else if (htype == CellSize::kMax || htype == CellSize::kMaxDoublePass)
          h = rec_height - GetHeight() + row_info_[y].size;
        else {
          double mult = (htype == CellSize::kDefault? 1.0 / num_rows_ :
                         cell_info_[index].height.fraction);
          h = int(mult * rec_height + (row_round_up[y]? 1-1e-6 : 0));
        }

        // Resize the cell and its row, column
        LightSetId child_id = cell_info_[index].child_id;
        GetChild(child_id)->UpdateSize(w, h);
        row_info_[y].size = max(row_info_[y].size, GetChild(child_id)->GetHeight());
        col_info_[x].size = max(col_info_[x].size, GetChild(child_id)->GetWidth());
      }

    // Compute the size left over and row/column positions
    int width = 0, height = 0;
    for (x = 0; x < num_cols_; x++) {
      col_info_[x].pos = width;
      width += col_info_[x].size;
    }
    for (y = 0; y < num_rows_; y++) {
      row_info_[y].pos = height;
      height += row_info_[y].size;
    }
    SetSize(width, height);
  }
  delete[] col_round_up;
  delete[] row_round_up;
}
