// Includes
#include "GlopWindow.h"
#include "Color.h"
#include "GlopFrameBase.h"
#include "Image.h"
#include "Input.h"
#include "OpenGl.h"
#include "System.h"
#include "GlopInternalData.h"
#include "Os.h"
#include <cmath>
#include <set>

// Constants
const char *const kDefaultTitle = "Glop Window";

// Globals
GlopWindow *window() {return system()->window();}
List<GlopFrame::Ping*> GlopWindow::ping_list_;

// Window mutators
// ===============

bool GlopWindow::Create(int width, int height, bool full_screen,
                        const GlopWindowSettings &settings) {
  ChooseValidSize(width, height, &width, &height);

  // Make sure the new window settings are different from the current window settings
  if (is_created_ && width == width_ && height == height_ &&
      full_screen == is_full_screen_ && settings_ == settings)
    return true;

  // Destroy the old window and recreate it with the new settings. Note that this will invalidate
  // all Open Gl objects, which may seem wasteful. However, it seems that this is more or less
  // required on Windows where a window style cannot be changed after creation.
  bool was_created = is_created_;
  recreated_this_frame_ = true;
  Destroy();
  os_data_ = Os::CreateWindow(title_, windowed_x_, windowed_y_, width, height,
                              full_screen, settings.stencil_bits, icon_, settings.is_resizable);

  // On failure, try to reset the window to how it was before
  bool was_success = (os_data_ != 0);
  if (!was_success) {
    if (was_created) {
      os_data_ = Os::CreateWindow(title_, windowed_x_, windowed_y_, width_,
                                  height_, is_full_screen_, settings_.stencil_bits, icon_,
                                  settings_.is_resizable);
      ASSERT(os_data_ != 0);
    } else {
      return false;
    }
  }

  // Fix the OpenGl settings for the new Gl window. Many of the Open Gl settings are presumably
  // set by default, but it's better to be safe than sorry.
  glEnable(GL_CULL_FACE);
  glEnableClientState(GL_VERTEX_ARRAY);
  glClearDepth(1.0f);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(false);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kBlack.GetData());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glHint(GL_FOG_HINT, GL_FASTEST);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glFrontFace(GL_CW);
  glViewport(0, 0, width, height);
  GlDataManager::GlInitAll();

  // Store our new settings
  is_created_ = true;
  if (was_success) {
    is_in_focus_ = true;
    is_minimized_ = false;
    is_full_screen_ = full_screen;
    settings_ = settings;
    width_ = width;
    height_ = height;
  }
  return was_success;
}

void GlopWindow::Destroy() {
  if (is_created_) {
    GlDataManager::GlShutDownAll();
    Os::DestroyWindow(os_data_);
    os_data_ = 0;
    is_created_ = false;
  }
}

void GlopWindow::SetIcon(const Image *icon) {
  if (icon != icon_ && is_created_)
    Os::SetIcon(os_data_, icon);
  icon_ = icon;
}

void GlopWindow::SetTitle(const string &title) {
  if (title_ != title && is_created_)
    Os::SetTitle(os_data_, title);
  title_ = title;
}

// Frame accessors
// ===============

string GlopWindow::GetFrameContextString() const {return frame_->GetContextString();}
const GlopFrame *GlopWindow::GetFrame(ListId id) const {return frame_->GetChild(id);}
GlopFrame *GlopWindow::GetFrame(ListId id) {return frame_->GetChild(id);}
List<GlopFrame*>::const_iterator GlopWindow::frame_begin() const {return frame_->children_begin();}
List<GlopFrame*>::const_iterator GlopWindow::frame_end() const {return frame_->children_end();}
float GlopWindow::GetFrameRelX(ListId id) const {return frame_->GetChildRelX(id);}
float GlopWindow::GetFrameRelY(ListId id) const {return frame_->GetChildRelY(id);}
int GlopWindow::GetFrameDepth(ListId id) const {return frame_->GetChildDepth(id);}
float GlopWindow::GetFrameHorzJustify(ListId id) const {
  return frame_->GetChildHorzJustify(id);
}
float GlopWindow::GetFrameVertJustify(ListId id) const {
  return frame_->GetChildVertJustify(id);
}

// Frame mutators
// ==============

ListId GlopWindow::AddFrame(GlopFrame *frame, float rel_x, float rel_y,
                                float horz_justify, float vert_justify, int depth) {
  return frame_->AddChild(frame, rel_x, rel_y, horz_justify, vert_justify, depth);
}
void GlopWindow::MoveFrame(ListId id, int depth) {frame_->MoveChild(id, depth);}
void GlopWindow::MoveFrame(ListId id, float rel_x, float rel_y) {
  frame_->MoveChild(id, rel_x, rel_y);
}
void GlopWindow::MoveFrame(ListId id, float rel_x, float rel_y, int depth) {
  frame_->MoveChild(id, rel_x, rel_y, depth);
}
void GlopWindow::SetFrameJustify(ListId id, float horz_justify, float vert_justify) {
  frame_->SetChildJustify(id, horz_justify, vert_justify);
}
GlopFrame *GlopWindow::RemoveFrameNoDelete(ListId id) {
  return frame_->RemoveChildNoDelete(id);
}
void GlopWindow::RemoveFrame(ListId id) {frame_->RemoveChild(id);}
void GlopWindow::ClearFrames() {frame_->ClearChildren();}

// Internal logic
// ==============

// Instantiates a GlopWindow object without actually creating the window. We set all values to
// defaults, although many are technically undefined.
GlopWindow::GlopWindow()
: os_data_(0), is_created_(false),
  width_(0), height_(0), is_full_screen_(false), settings_(),
  title_(kDefaultTitle), icon_(0),
  is_vsync_requested_(false), is_vsync_setting_current_(false),
  is_in_focus_(false), is_minimized_(false), recreated_this_frame_(false),
  windowed_x_(-1), windowed_y_(-1), is_resolving_ping_(false),
  focus_stack_(1, (FocusFrame*)0),
  frame_(new TableauFrame()) {
  frame_->window_ = this;
  input_ = new Input(this);
}

// Deletes a GlopWindow object - this will only happen on program exit.
GlopWindow::~GlopWindow() {
  Destroy();
  delete frame_;
  delete input_;
}

void GlopWindow::SetFocusPredecessor(FocusFrame *base, FocusFrame *predecessor) {
  ASSERT(base != predecessor && base->layer_ == predecessor->layer_);

  // Unlink predecessor
  FocusFrame *next = predecessor->next_;
  FocusFrame *prev = predecessor->prev_;
  next->prev_ = prev;
  prev->next_ = next;

  // Relink it
  predecessor->prev_ = base->prev_;
  predecessor->next_ = base;
  base->prev_->next_ = predecessor;
  base->prev_ = predecessor;
}

void GlopWindow::SetFocusSuccessor(FocusFrame *base, FocusFrame *successor) {
  ASSERT(base != successor && base->layer_ == successor->layer_);

  // Unlink successor
  FocusFrame *next = successor->next_;
  FocusFrame *prev = successor->prev_;
  next->prev_ = prev;
  prev->next_ = next;

  // Relink it
  successor->prev_ = base;
  successor->next_ = base->next_;
  base->next_->prev_ = successor;
  base->next_ = successor;
}

// Create or delete a focus tracking layer - See GlopFrameBase.h.
// PopFocus is only allowed if there is more than one focus tracking layer, and the topmost
// layer is empty.
void GlopWindow::PushFocus() {
  UpdateFramesInFocus(focus_stack_[focus_stack_.size() - 1], 0, false);
  focus_stack_.push_back(0);
}

void GlopWindow::PopFocus() {
  ASSERT(focus_stack_.size() > 1 && focus_stack_[focus_stack_.size() - 1] == 0);
  focus_stack_.resize(focus_stack_.size() - 1);
  UpdateFramesInFocus(0, focus_stack_[focus_stack_.size() - 1], false);
}

// Handle all logic for this window for a single frame. Returns the number of ticks spent on calls
// to SwapBuffers. This allows us to track the time between SwapBuffer calls and thus sleep through
// most of a vertical refresh instead of blocking on a SwapBuffer call when vertical syncing is
// enabled. This allows the program to take less than 100% cpu time. According to some
// documentation, SwapBuffers should do this sleeping automatically, but it certainly does not in
// all cases.
int GlopWindow::Think(int dt) {
  // If the window is not created, there is nothing to do.
  if (!is_created_)
    return 0;

  // Allow the Os to update its internal data, and then poll it
  if (!is_vsync_setting_current_) {
    Os::EnableVSync(is_vsync_requested_);
    is_vsync_setting_current_ = true;
  }
  Os::WindowThink(os_data_);
  int width, height;
  Os::GetWindowSize(os_data_, &width, &height);
  if (width != width_ || height != height_) {
    ChooseValidSize(width, height, &width_, &height_);
    if (width_ != width || height_ != height)
      Os::SetWindowSize(os_data_, width_, height_);
    glViewport(0, 0, width_, height_);
    frame_->OnWindowResize(width_, height_);
  }

  // Update frame focus resulting from the window going in or out of focus
  bool focus_changed;
  Os::GetWindowFocusState(os_data_, &is_in_focus_, &focus_changed);
  if (GetFocusFrame() != 0 && focus_changed)
    UpdateFramesInFocus();

  // Track window position and size
  is_minimized_ = Os::IsWindowMinimized(os_data_);
  if (!is_full_screen_)
    Os::GetWindowPosition(os_data_, &windowed_x_, &windowed_y_);

  // Allow frames to think - intentionally done before KeyEvents. This makes it easier to use
  // VirtualKeys.
  frame_->Think(dt);

  // Perform input logic, and reset all input key presses if the window has gone out of focus
  // (either naturally or it has been destroyed). If we do not do this, we might miss a key up
  // event and a key could be registered as stuck down. When done, perform all frame logic.
  input_->Think(recreated_this_frame_ || !is_in_focus_ || focus_changed, dt);
  recreated_this_frame_ = false;

  // Resize and position our content frames. We do this before resolving pings since both size and
  // position information might be necessary to correctly do this. Examples: size is needed to ping
  // the bottom-right corner of a frame, and position is needed to ping a child frame.
  frame_->UpdateSize(width_, height_); 
  frame_->SetPosition(0, 0, 0, 0, width_-1, height_-1);

  // Handle all pings
  is_resolving_ping_ = true;
  for (List<GlopFrame::Ping*>::iterator it = ping_list_.begin(); it != ping_list_.end(); ++it) {
    if ((*it)->GetFrame()->GetWindow() == this) {
      PropogatePing(*it);
      it = ping_list_.erase(it);
    } else if ((*it)->GetFrame()->GetWindow() == 0) {
      it = ping_list_.erase(it);
    } else {
      ++it;
    }
  }
  is_resolving_ping_ = false;

  // Render
  int swap_buffer_time = 0;
  if (!is_minimized_) {
    // Clear the old image
    int clear_mode = GL_COLOR_BUFFER_BIT;
    if (settings_.stencil_bits > 0)
      clear_mode |= GL_STENCIL_BUFFER_BIT;
    glClear(clear_mode);

    // And render the new
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glTranslatef(-1,1,-1);
    glScalef(2.0f / width_, -2.0f / height_, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    frame_->Render();
    int old_time = system()->GetTime();
    Os::SwapBuffers(os_data_);
    swap_buffer_time = system()->GetTime() - old_time;
  }
  return swap_buffer_time;
}

// Given a requested window size, this sets our width and height to try to match that size, but
// always requested the minimum and maximum aspect ratio.
void GlopWindow::ChooseValidSize(int width, int height, int *new_width, int *new_height) {
  // Choose the best-fit aspect ratio
  float ar = float(width) / height;
  if (ar < settings_.min_aspect_ratio)
    ar = settings_.min_aspect_ratio;
  if (1/ar < settings_.min_inverse_aspect_ratio)
    ar = 1/settings_.min_inverse_aspect_ratio;
  
  // Choose an exact size with this aspect ratio that preserves the window area as best as possible
  width = max(width, settings_.min_width);
  height = max(height, settings_.min_height);
  float sqrt_ar = sqrt(ar);
  float min_mean = max(settings_.min_width / sqrt_ar, settings_.min_height * sqrt_ar);
  float mean = max(min_mean, sqrt(float(width) * height));
  *new_width = int(mean * sqrt_ar + 0.5f);
  *new_height = int(mean / sqrt_ar + 0.5);
}

// Unregisters all pings a frame has created. A frame does this when it is deleted.
// See GlopFrameBase.h.
void GlopWindow::UnregisterAllPings(GlopFrame *frame) {
  for (List<GlopFrame::Ping*>::iterator it = ping_list_.begin(); it != ping_list_.end();) {
    if ((*it)->GetFrame() == frame) {
      delete *it;
      it = ping_list_.erase(it);
    } else {
      ++it;
    }
  }
}

// We wish to handle a ping as an autonomous unit. This ensures that if ping1 was requested before
// ping2, ping1 finishes propogating before ping2 starts propogating. We need to just store the
// ping until we enter the ping resolution phase, but from that point onwards, all pings are
// handled immediately.
void GlopWindow::RegisterPing(GlopFrame::Ping *ping) {
  GlopWindow *window = ping->GetFrame()->GetWindow();
  if (window != 0 && window->is_resolving_ping_)
    PropogatePing(ping);
  else
    ping_list_.push_back(ping);
}

// Internal utility to propogate a ping upwards to its parent frame.
void GlopWindow::PropogatePing(GlopFrame::Ping *ping) {
  GlopFrame *parent = ping->GetFrame()->GetParent();
  if (parent != 0) {
    int x1, y1, x2, y2;
    ping->GetCoords(&x1, &y1, &x2, &y2);
    parent->OnChildPing(ping->GetFrame(), ping->GetFrame()->GetX() - parent->GetX() + x1,
                        ping->GetFrame()->GetY() - parent->GetY() + y1,
                        ping->GetFrame()->GetX() - parent->GetX() + x2,
                        ping->GetFrame()->GetY() - parent->GetY() + y2,
                        ping->IsCentered());
  }
  delete ping;
}

// Updates focus for the current layer based on a new key event.
void GlopWindow::OnKeyEvent(const KeyEvent &event) {
  int layer = int(focus_stack_.size()) - 1;
  FocusFrame *focus_frame = focus_stack_[layer], *frame;
  if (focus_frame == 0)
    return;

  // Pass the events to the focus frame and see if it processes them
  if (SendKeyEventToFrame(focus_frame, event, false))
    return;

  // Handle mouse clicks
  bool is_mouse_click = false;
  if (event.IsNonRepeatPress())
  for (int i = 0; i < (int)event.keys.size(); i++)
  if (event.keys[i] == kGuiKeyPrimaryClick || event.keys[i] == kGuiKeySecondaryClick)
    is_mouse_click = true;
  if (is_mouse_click) {
    // Figure out our new focus
    vector<FocusFrame*> options;
    for (frame = focus_frame; ; frame = frame->next_) {
      if (frame->IsPointVisible(input()->GetMouseX(), input()->GetMouseY()))
        options.push_back(frame);
      if (frame == focus_frame->prev_)
        break;
    }
    FocusFrame *new_ff = ChooseFocus(focus_frame, options);

    // Handle the click
    if (new_ff != focus_frame)
      DemandFocus(new_ff, true);
    SendKeyEventToFrame(new_ff, event, focus_frame != new_ff);
    return;
  }
 
  // Handle focus magnets - note that a frame might still have a key as a focus magnet even if it
  // does not process it. We do not switch focus in this case.
  if (event.IsNonRepeatPress()) {
    // Figure out our new focus
    vector<FocusFrame*> options;
    for (frame = focus_frame; ; frame = frame->next_) {
      if (frame->IsFocusMagnet(event))
        options.push_back(frame);
      if (frame == focus_frame->prev_)
        break;
    }
    FocusFrame *new_ff = ChooseFocus(focus_frame, options);

    // Handle the event - note that if focus did not change, we have already handled it.
    if (new_ff != focus_frame) {
      DemandFocus(new_ff, true);
      SendKeyEventToFrame(new_ff, event, true);
      return;
    }
  }

  // Handle tabbing
  bool is_select_next = false, is_select_prev = false;
  if (event.IsPress())
  for (int i = 0; i < (int)event.keys.size(); i++)
  if (event.keys[i] == kGuiKeySelectNext)
    is_select_next = true;
  else if (event.keys[i] == kGuiKeySelectPrev)
    is_select_prev = true;
  FocusFrame *new_ff = focus_frame;
  if (is_select_next && !is_select_prev)
    new_ff = GetNextPossibleFocusFrame(focus_frame);
  if (is_select_prev && !is_select_next)
    new_ff = GetPrevPossibleFocusFrame(focus_frame);

  // Handle the event
  if (new_ff != focus_frame) {
    DemandFocus(new_ff, true);
    SendKeyEventToFrame(new_ff, event, true);
  }
}

// Adds or removes a FocusFrame to the topmost focus layer. This includes setting focus, and
// updating the FocusFrame links. If we are registering a frame, its layer is returned.
int GlopWindow::RegisterFocusFrame(FocusFrame *frame) {
  FocusFrame *cur_frame = focus_stack_[focus_stack_.size() - 1];
  if (cur_frame == 0) {
    focus_stack_[focus_stack_.size() - 1] = frame;
    frame->prev_ = frame->next_ = frame;
    if (is_in_focus_)
      UpdateFramesInFocus(0, frame, false);
  } else {
    frame->next_ = cur_frame;
    frame->prev_ = cur_frame->prev_;
    cur_frame->prev_->next_ = frame;
    cur_frame->prev_ = frame;
  }
  return (int)focus_stack_.size() - 1;
}

void GlopWindow::UnregisterFocusFrame(FocusFrame *frame) {
  int layer = frame->layer_;
  if (focus_stack_[layer] == frame) {
    FocusFrame *new_frame = GetPrevPossibleFocusFrame(frame);
    if (new_frame == frame)
      new_frame = 0;
    focus_stack_[layer] = new_frame;
    UpdateFramesInFocus(frame, new_frame, false);
  }
  frame->next_->prev_ = frame->prev_;
  frame->prev_->next_ = frame->next_;
}

// Sets the given frame to be the active frame on the given layer. If this is the topmost layer,
// then the overall focus changes.
void GlopWindow::DemandFocus(FocusFrame *frame, bool ping) {
  ASSERT(frame->CanBePrimaryFocus());
  int layer = frame->layer_;
  FocusFrame *old_frame = focus_stack_[layer];
  focus_stack_[layer] = frame;
  if (layer == focus_stack_.size() - 1)
    UpdateFramesInFocus(old_frame, frame, ping);
}

// Sends messages to GlopFrames to update IsInFocus for the current FocusFrame. This is necessary
// when the window itself changes focus.
void GlopWindow::UpdateFramesInFocus() {
  FocusFrame *frame = GetFocusFrame();
  if (frame == 0 || frame->IsInFocus() == is_in_focus_)
    return;
  while (frame != 0 && frame->layer_ == GetFocusFrame()->layer_) {
    frame->SetIsInFocus(is_in_focus_);
    frame = frame->GetParent()->GetFocusFrame();
  }
}

// Sends messages to GlopFrames as a result of the primary FocusFrame switching from old_frame to
// new_frame. We do not change the internal focus stack data, just the external GlopFrames. A ping
// is generated if ping == true and new_frame != old_frame.
void GlopWindow::UpdateFramesInFocus(FocusFrame *old_frame, FocusFrame *new_frame, bool ping) {
  // If the window is not in focus, then neither are any of its frames
  if (is_in_focus_) {
    // Calculate the list of FocusFrames that were in focus before and that will be made in focus
    vector<FocusFrame*> old_focus, new_focus;
    FocusFrame *frame = new_frame;
    while (frame != 0) {
      new_focus.push_back(frame);
      FocusFrame *parent = frame->GetParent()->GetFocusFrame();
      if (parent != 0 && parent->layer_ != frame->layer_)
        parent = 0;
      frame = parent;
    }
    frame = old_frame;
    while (frame != 0) {
      old_focus.push_back(frame);
      FocusFrame *parent = frame->GetParent()->GetFocusFrame();
      if (parent != 0 && parent->layer_ != frame->layer_)
        parent = 0;
      frame = parent;
    }
    
    // Ignore frames that are unchanged
    while (new_focus.size() > 0 && old_focus.size() > 0 &&
          new_focus[new_focus.size()-1] == old_focus[old_focus.size()-1]) {
      new_focus.pop_back();
      old_focus.pop_back();
    }

    // Make the change
    for (int i = 0; i < (int)old_focus.size(); i++)
      old_focus[i]->SetIsInFocus(false);
    for (int i = 0; i < (int)new_focus.size(); i++)
      new_focus[i]->SetIsInFocus(true);
  }

  // Make the ping (regardless of whether the window is in focus)
  if (new_frame != 0 && new_frame != old_frame && ping)
    new_frame->NewRelativePing(0, 0, 1, 1);
}

// Choose a FocusFrame as our focus, given that it must be descended from a FocusFrame in options
// that is, in turn, not ancestors of other FocusFrames in options. If possible, old_focus is
// always chosen.
FocusFrame *GlopWindow::ChooseFocus(FocusFrame *old_focus, const vector<FocusFrame*> &options) {
  // Which options are ancestors of others.
  set<FocusFrame*> ancestors;
  for (int i = 0; i < (int)options.size(); i++) {
    for (FocusFrame *frame = options[i]->GetParent()->GetFocusFrame(); frame != 0;
         frame = frame->GetParent()->GetFocusFrame())
    if (ancestors.count(frame))
      break;
    else
      ancestors.insert(frame);
  }

  // Choose which option we wish to descend from
  FocusFrame *result = 0;
  for (int i = 0; i < (int)options.size(); i++)
  if (!ancestors.count(options[i])) {
    if (old_focus->IsSubFocusFrame(options[i]))
      return old_focus;
    else
      result = options[i];
  }

  // Choose a possible descendent
  if (result == 0) {
    return old_focus;
  } else {
    if (result->CanBePrimaryFocus())
      return result;
    for (FocusFrame *temp = GetNextPossibleFocusFrame(result); ;
         temp = GetNextPossibleFocusFrame(temp))
    if (temp->IsSubFocusFrame(result))
      return temp;
  }
}

FocusFrame *GlopWindow::GetNextPossibleFocusFrame(FocusFrame *frame) {
  for (frame = frame->next_; !frame->CanBePrimaryFocus(); frame = frame->next_);
  return frame;
}

FocusFrame *GlopWindow::GetPrevPossibleFocusFrame(FocusFrame *frame) {
  for (frame = frame->prev_; !frame->CanBePrimaryFocus(); frame = frame->prev_);
  return frame;
}

bool GlopWindow::SendKeyEventToFrame(FocusFrame *frame, const KeyEvent &event, bool gained_focus) {
  while (frame != 0 && frame->IsInFocus()) {
    if (frame->OnKeyEvent(event, gained_focus))
      return true;
    frame = frame->GetParent()->GetFocusFrame();
  }
  return false;
}
