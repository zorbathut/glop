// Includes
#include "GlopWindow.h"
#include "Color.h"
#include "GlopFrameBase.h"
#include "Image.h"
#include "Input.h"
#include "OpenGl.h"
#include "Os.h"
#include "System.h"

// Constants
const char *const kDefaultTitle = "Glop Window";

// Static accessor convenience method
GlopWindow *window() {
  return gSystem->window();
}

// Window mutators
// ===============

bool GlopWindow::Create(int width, int height, bool full_screen,
                        const GlopWindowSettings &settings) {
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
  input_->StartPolling();

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
  gSystem->GlRegisterAll();

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
    gSystem->GlUnregisterAll();
    input_->StopPolling();
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
LightSetId GlopWindow::RemoveFrame(LightSetId id) {return frame_->RemoveChild(id);}
void GlopWindow::ClearFrames() {frame_->ClearChildren();}

// Internal logic
// ==============

// Instantiates a GlopWindow object without actually creating the window. We set all values to
// defaults, although many are technically undefined.
GlopWindow::GlopWindow()
: os_data_(0),
  is_created_(false),
  width_(0),
  height_(0),
  is_full_screen_(false),
  settings_(),
  title_(kDefaultTitle),
  icon_(0),
  is_in_focus_(false),
  is_minimized_(false),
  recreated_this_frame_(false),
  windowed_x_(-1),
  windowed_y_(-1),
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
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);
    frame_->OnWindowResize(width, height);
  }
  bool focus_changed;
  Os::GetWindowFocusState(os_data_, &is_in_focus_, &focus_changed);
  is_minimized_ = Os::IsWindowMinimized(os_data_);
  if (!is_full_screen_)
    Os::GetWindowPosition(os_data_, &windowed_x_, &windowed_y_);

  // Perform input logic, and reset all input key presses if the window has gone out of focus
  // (either naturally or it has been destroyed). If we do not do this, we might miss a key up
  // event and a key could be registered as stuck down. When done, perform all frame logic.
  input_->Think(recreated_this_frame_ || !is_in_focus_ || focus_changed, dt);
  recreated_this_frame_ = false;
  frame_->Think(dt);

  // Update our content frames. All pings are handled in batch here after frames have resized. This
  // is so that a frame can be guaranteed of it's size being current when it handles a ping, even
  // if it is a new frame. Note, however, that one ping can actually generate another ping while
  // this is going on.
  frame_->UpdateSize(width_, height_); 
  for (LightSetId id = ping_list_.GetFirstId(); id != 0; id = ping_list_.GetNextId(id)) {
    GlopFrame::Ping *ping = ping_list_[id];
    GlopFrame *parent = ping->GetFrame()->GetParent();
    if (parent != 0) {
      int x1, y1, x2, y2;
      ping->GetCoords(&x1, &y1, &x2, &y2);
      parent->OnChildPing(ping->GetFrame()->GetX() - parent->GetX() + x1,
                          ping->GetFrame()->GetY() - parent->GetY() + y1,
                          ping->GetFrame()->GetX() - parent->GetX() + x2,
                          ping->GetFrame()->GetY() - parent->GetY() + y2,
                          ping->GetIsCentered());
    }
    delete ping;
    id = ping_list_.RemoveItem(id);
  }
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

// Unregisters all pings a frame has created. A frame does this when it is deleted.
// See GlopFrameBase.h.
void GlopWindow::UnregisterAllPings(GlopFrame *frame) {
  for (LightSetId id = ping_list_.GetFirstId(); id != 0; id = ping_list_.GetNextId(id))
    if (ping_list_[id]->GetFrame() == frame) {
      delete ping_list_[id];
      id = ping_list_.RemoveItem(id);
    }
}

// Updates focus for the current layer based on a new key event.
void GlopWindow::OnKeyEvent(const KeyEvent &event, int dt) {
  // Make sure the layer is non-empty and the current focus frame might give up focus
  FocusFrame *focus_frame = focus_stack_[focus_stack_.size() - 1];
  if (focus_frame == 0 || (event.IsNonRepeatPress() && focus_frame->IsFocusKeeper(event)))
    goto done;

  // Handle focus magnets
  FocusFrame *frame = focus_frame->next_;
  if (event.IsNonRepeatPress())
  while (frame != focus_frame) {
    if (frame->IsFocusMagnet(event)) {
      DemandFocus((int)focus_stack_.size() - 1, frame);
      goto done;
    }
    frame = frame->next_;
  }

  // Handle tabbing
  if (event.IsPress() && event.key == '\t' && !input()->IsKeyDownNow(kKeyLeftAlt) &&
      !input()->IsKeyDownNow(kKeyRightAlt) && !input()->IsKeyDownNow(kKeyLeftControl) &&
      !input()->IsKeyDownNow(kKeyRightControl)) {
    if (input()->IsKeyDownNow(kKeyLeftShift) || input()->IsKeyDownNow(kKeyRightShift))
      DemandFocus((int)focus_stack_.size() - 1, focus_frame->prev_);
    else
      DemandFocus((int)focus_stack_.size() - 1, focus_frame->next_);
  }

  // Handle mouse clicks
  if (event.IsNonRepeatPress() && event.key.IsMouseKey() && !event.key.IsMouseMotion()) {
    frame = focus_frame->next_;
    do {
      if (frame->IsPointVisible(input()->GetMouseX(), input()->GetMouseY())) {
        DemandFocus((int)focus_stack_.size() - 1, frame);
        goto done;
      }
      frame = frame->next_;
    } while (frame != focus_frame);
  }

  // Pass the key event onto our frames
done:;
  frame_->OnKeyEvent(event, dt);
}

// Adds or removes a FocusFrame to the topmost focus layer. This includes setting focus, and
// updating the FocusFrame links. If we are registering a frame, its layer is returned.
int GlopWindow::RegisterFocusFrame(FocusFrame *frame) {
  FocusFrame *cur_frame = focus_stack_[focus_stack_.size() - 1];
  if (cur_frame == 0) {
    focus_stack_[focus_stack_.size() - 1] = frame;
    frame->prev_ = frame->next_ = frame;
    frame->SetIsInFocus(true);
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
      new_frame->SetIsInFocus(true);
  }
  frame->next_->prev_ = frame->prev_;
  frame->prev_->next_ = frame->next_;
}

// Sets the given frame to be the active frame on the given layer. Focus is updated if necessary.
void GlopWindow::DemandFocus(int layer, FocusFrame *frame) {
  if (focus_stack_[layer] == frame)
    return;
  if (layer == focus_stack_.size() - 1)
    focus_stack_[layer]->SetIsInFocus(false);
  focus_stack_[layer] = frame;
  if (layer == focus_stack_.size() - 1)
    frame->SetIsInFocus(true);
}
