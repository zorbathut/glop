// Includes
#include "../include/GlopWindow.h"
#include "../include/Color.h"
#include "../include/GlopFrameBase.h"
#include "../include/Image.h"
#include "../include/Input.h"
#include "../include/OpenGl.h"
#include "../include/System.h"
#include "GlopInternalData.h"
#include "Os.h"
#include <cmath>
#include <set>

// Constants
const char *const kDefaultTitle = "Glop Window";

// Globals
GlopWindow *gWindow = 0;

// Window mutators
// ===============

bool GlopWindow::Create(int width, int height, bool full_screen,
                        const GlopWindowSettings &settings) {
  ChooseValidSize(width, height, &width, &height);

  // Make sure the new window settings are different from the current window settings
  bool same_settings = (memcmp(&settings, &settings_, sizeof(settings)) == 0);
  if (is_created_ && width == width_ && height == height_ &&
      full_screen == is_full_screen_ && same_settings)
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
const GlopFrame *GlopWindow::GetFrame(LightSetId id) const {return frame_->GetChild(id);}
GlopFrame *GlopWindow::GetFrame(LightSetId id) {return frame_->GetChild(id);}
LightSetId GlopWindow::GetFirstFrameId() const {return frame_->GetFirstChildId();}
LightSetId GlopWindow::GetNextFrameId(LightSetId id) const {return frame_->GetNextChildId(id);}
float GlopWindow::GetFrameRelX(LightSetId id) const {return frame_->GetChildRelX(id);}
float GlopWindow::GetFrameRelY(LightSetId id) const {return frame_->GetChildRelY(id);}
int GlopWindow::GetFrameDepth(LightSetId id) const {return frame_->GetChildDepth(id);}
float GlopWindow::GetFrameHorzJustify(LightSetId id) const {
  return frame_->GetChildHorzJustify(id);
}
float GlopWindow::GetFrameVertJustify(LightSetId id) const {
  return frame_->GetChildVertJustify(id);
}

// Frame mutators
// ==============

LightSetId GlopWindow::AddFrame(GlopFrame *frame, float rel_x, float rel_y,
                                float horz_justify, float vert_justify, int depth) {
  return frame_->AddChild(frame, rel_x, rel_y, horz_justify, vert_justify, depth);
}
void GlopWindow::MoveFrame(LightSetId id, int depth) {frame_->MoveChild(id, depth);}
void GlopWindow::MoveFrame(LightSetId id, float rel_x, float rel_y) {
  frame_->MoveChild(id, rel_x, rel_y);
}
void GlopWindow::MoveFrame(LightSetId id, float rel_x, float rel_y, int depth) {
  frame_->MoveChild(id, rel_x, rel_y, depth);
}
void GlopWindow::SetFrameJustify(LightSetId id, float horz_justify, float vert_justify) {
  frame_->SetChildJustify(id, horz_justify, vert_justify);
}
GlopFrame *GlopWindow::RemoveFrameNoDelete(LightSetId id) {
  return frame_->RemoveChildNoDelete(id);
}
void GlopWindow::RemoveFrame(LightSetId id) {frame_->RemoveChild(id);}
void GlopWindow::ClearFrames() {frame_->ClearChildren();}

// Internal logic
// ==============

// Instantiates a GlopWindow object without actually creating the window. We set all values to
// defaults, although many are technically undefined.
GlopWindow::GlopWindow()
: os_data_(0), is_created_(false),
  width_(0), height_(0), is_full_screen_(false), settings_(),
  title_(kDefaultTitle), icon_(0),
  is_in_focus_(false), is_minimized_(false), recreated_this_frame_(false),
  windowed_x_(-1), windowed_y_(-1),
  tab_direction_(None), is_resolving_ping_(false),
  focus_stack_(1, (FocusFrame*)0),
  frame_(new TableauFrame()) {
  input_ = new Input(this);
}

// Deletes a GlopWindow object - this will only happen on program exit.
GlopWindow::~GlopWindow() {
  Destroy();
  delete frame_;
  delete input_;
}

// Create or delete a focus tracking layer - See GlopFrameBase.h.
// PopFocus is only allowed if there is one more than one focus tracking layer, and the topmost
// layer is empty.
void GlopWindow::PushFocus() {
  if (focus_stack_[focus_stack_.size() - 1] != 0)
    focus_stack_[focus_stack_.size() - 1]->SetIsInFocus(false);
  focus_stack_.push_back(0);
}

void GlopWindow::PopFocus() {
  ASSERT(focus_stack_.size() > 1 && focus_stack_[focus_stack_.size() - 1] == 0);
  focus_stack_.resize(focus_stack_.size() - 1);
  if (focus_stack_[focus_stack_.size() - 1] != 0)
    focus_stack_[focus_stack_.size() - 1]->SetIsInFocus(true);
}

// Handle all logic for this window for a single frame.
void GlopWindow::Think(int dt) {
  // If the window is not created, there is nothing to do.
  if (!is_created_)
    return;

  // Allow the Os to update its internal data, and then poll it
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

  // Handle focus
  bool focus_changed;
  Os::GetWindowFocusState(os_data_, &is_in_focus_, &focus_changed);
  if (focus_stack_[focus_stack_.size() - 1] != 0 &&
      focus_stack_[focus_stack_.size() - 1]->IsInFocus() != is_in_focus_)
    focus_stack_[focus_stack_.size() - 1]->SetIsInFocus(is_in_focus_);
  is_minimized_ = Os::IsWindowMinimized(os_data_);
  if (!is_full_screen_)
    Os::GetWindowPosition(os_data_, &windowed_x_, &windowed_y_);

  // Allow frames to think - intentionally done before KeyEvents. This makes it easier to use
  // VirtualKeys.
  frame_->Think(dt);

  // Perform input logic, and reset all input key presses if the window has gone out of focus
  // (either naturally or it has been destroyed). If we do not do this, we might miss a key up
  // event and a key could be registered as stuck down. When done, perform all frame logic.
  if (tab_direction_ == Forward && !input()->IsKeyDownNow(kGuiKeySelectNext))
    tab_direction_ = None;
  else if (tab_direction_ == Backward && !input()->IsKeyDownNow(kGuiKeySelectPrev))
    tab_direction_ = None;
  input_->Think(recreated_this_frame_ || !is_in_focus_ || focus_changed, dt);
  recreated_this_frame_ = false;

  // Update our content frames. All pings are handled in batch here after frames have resized. This
  // is so that a frame can be guaranteed of it's size being current when it handles a ping, even
  // if it is a new frame. Note, however, that one ping can actually generate another ping while
  // this is going on.
  frame_->UpdateSize(width_, height_); 
  is_resolving_ping_ = true;
  for (LightSetId id = ping_list_.GetFirstId(); id != 0; id = ping_list_.GetNextId(id)) {
    PropogatePing(ping_list_[id]);
    id = ping_list_.RemoveItem(id);
  }
  is_resolving_ping_ = false;
  frame_->SetPosition(0, 0, 0, 0, width_-1, height_-1);

  // Render
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
    Os::SwapBuffers(os_data_);
  }
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
  for (LightSetId id = ping_list_.GetFirstId(); id != 0; id = ping_list_.GetNextId(id))
    if (ping_list_[id]->GetFrame() == frame) {
      delete ping_list_[id];
      id = ping_list_.RemoveItem(id);
    }
}

// We wish to handle a ping as an autonomous unit. This ensures that if ping1 was requested before
// ping2, ping1 finishes propogating before ping2 starts propogating. We need to just store the ping
// until we enter the ping resolution phase, but from that point onwards, all pings are handled
// immediately.
void GlopWindow::RegisterPing(GlopFrame::Ping *ping) {
  if (is_resolving_ping_)
    PropogatePing(ping);
  else
    ping_list_.InsertItem(ping);
}

// Internal utility to propogate a ping upwards to its child frame.
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
void GlopWindow::OnKeyEvent(const KeyEvent &event, int dt) {
  int layer = int(focus_stack_.size()) - 1;
  FocusFrame *focus_frame = focus_stack_[layer], *frame;
  if (focus_frame == 0)
    return;

  // Handle mouse clicks
  if (event.IsNonRepeatPress() &&
      (event.key == kGuiKeyPrimaryClick || event.key == kGuiKeySecondaryClick)) {
    // Find all clicked frames
    vector<FocusFrame*> clicked_frames;
    set<FocusFrame*> parent_frames;
    frame = focus_frame;
    do {
      if (frame->IsPointVisible(input()->GetMouseX(), input()->GetMouseY())) {
        clicked_frames.push_back(frame);
        parent_frames.insert(frame->GetParent()->GetFocusFrame());
      }
      frame = frame->next_;
    } while (frame != focus_frame);

    // Remove frames that are parents of others
    vector<FocusFrame*> candidates;
    for (int i = 0; i < (int)clicked_frames.size(); i++)
    if (!parent_frames.count(clicked_frames[i]))
      candidates.push_back(clicked_frames[i]);

    // Otherwise just take a generic candidate
    if (candidates.size() > 0) {
      DemandFocus(layer, candidates[0], true);
      focus_frame = candidates[0];
    }
    focus_frame->OnKeyEvent(event, dt);
    goto done;
  }

  // Pass the event to the focus frame, and see if it processes the event
  frame = focus_frame;
  for (frame = focus_frame; frame != 0; frame = frame->GetParent()->GetFocusFrame())
  if (frame->OnKeyEvent(event, dt))
    goto done;
  
  // Handle focus magnets - note that a frame might still have a key as a focus magnet even if it
  // does not process it. We do not switch focus in this case.
  if (event.IsNonRepeatPress()) {
    for (frame = focus_frame; frame != 0; frame = frame->GetParent()->GetFocusFrame())
    if (frame->IsFocusMagnet(event))
      goto done;
    for (frame = focus_frame->next_; frame != focus_frame; frame = frame->next_)
    if (frame->IsFocusMagnet(event)) {
      DemandFocus(layer, frame, true);
      frame->OnKeyEvent(event, dt);
      goto done;
    }
  }

  // Handle tabbing - note that we prevent tabbing to focus frames that have other focus frames
  // as children (e.g. a scrolling frame with a button child).
  if (event.IsPress())
  if ((event.key == kGuiKeySelectNext && tab_direction_ != Backward) ||
      (event.key == kGuiKeySelectPrev && tab_direction_ != Forward)) {
    frame = focus_frame;
    while (1) {
      if (event.key == kGuiKeySelectNext) {
        tab_direction_ = Forward;
        frame = frame->next_;
      } else {
        tab_direction_ = Backward;
        frame = frame->prev_;
      }
      bool is_parent = false;
      for (FocusFrame *temp = frame->next_; temp != frame; temp = temp->next_)
      if (temp->GetParent()->GetFocusFrame() == frame)
        is_parent = true;
      if (!is_parent)
        break;
    }
    if (frame != focus_frame)
      DemandFocus((int)focus_stack_.size() - 1, frame, true);
  } else {
    focus_frame->OnKeyEvent(event, dt);
  }

  // Mark the focus as no longer gained
done:;
  if (focus_stack_[focus_stack_.size() - 1] != 0)
    focus_stack_[focus_stack_.size() - 1]->is_gaining_focus_ = false;
}

// Adds or removes a FocusFrame to the topmost focus layer. This includes setting focus, and
// updating the FocusFrame links. If we are registering a frame, its layer is returned.
int GlopWindow::RegisterFocusFrame(FocusFrame *frame) {
  FocusFrame *cur_frame = focus_stack_[focus_stack_.size() - 1];
  if (cur_frame == 0) {
    focus_stack_[focus_stack_.size() - 1] = frame;
    frame->prev_ = frame->next_ = frame;
    frame->SetIsInFocus(is_in_focus_);
  } else {
    frame->next_ = cur_frame;
    frame->prev_ = cur_frame->prev_;
    cur_frame->prev_->next_ = frame;
    cur_frame->prev_ = frame;
  }
  return (int)focus_stack_.size() - 1;
}

void GlopWindow::UnregisterFocusFrame(int layer, FocusFrame *frame) {
  if (focus_stack_[layer] == frame) {
    frame->SetIsInFocus(false);
    FocusFrame *new_frame = (frame->prev_ == focus_stack_[layer]? 0 : frame->prev_);
    focus_stack_[layer] = new_frame;
    if (new_frame != 0)
      new_frame->SetIsInFocus(is_in_focus_);
  }
  frame->next_->prev_ = frame->prev_;
  frame->prev_->next_ = frame->next_;
}

// Sets the given frame to be the active frame on the given layer. Focus is updated if necessary.
void GlopWindow::DemandFocus(int layer, FocusFrame *frame, bool update_is_gaining_focus) {
  if (focus_stack_[layer] == frame)
    return;
  if (layer == focus_stack_.size() - 1)
    focus_stack_[layer]->SetIsInFocus(false);
  focus_stack_[layer] = frame;
  if (layer == focus_stack_.size() - 1) {
    if (update_is_gaining_focus)
      frame->is_gaining_focus_ = true;
    frame->SetIsInFocus(is_in_focus_);
  }
}
