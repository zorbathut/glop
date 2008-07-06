// A GlopFrame is the basic unit of autonomous logic in Glop. See GlopFrame.h for a high-level on
// how to use them. Here, we discuss the lower level details.
//
// A frame can render itself, be positioned and sized, think each tick, and trap input events. The
// exact pipeline is as follows:
//  - The Glop client calls system()->Think()
//    - All frames receive OnWindowResize messages if appropriate.
//    - All frames think.
//    - All focus is updated from tabs, magnet keys, etc. (see below)
//    - All frames receive OnKeyEvent notifications from Input.
//    - All other KeyListeners receive OnKeyEvent notifications from Input.
//    - All frames resize themselves if necessary.
//    - All pings are resolved.
//    - All frames reposition themselves and update their clipping rectangle.
//    - All frames render.
//  - Repeat.
//
// All GlopFrames are organized in a tree structure. The top-level frame is a TableauFrame that is
// maintained by the GlopWindow. All other frames have exactly one parent frame, and any number of
// children frames.
//
// Overview of focus: A frame is said to be "in focus" if it should be responding to user input.
//   All focus is handled within the context of FocusFrames. A FocusFrame and anything descended
//   from it are considered an autonomous unit of focus. If the FocusFrame gains focus, so do all
//   of its children. One exception to this is if a FocusFrame contains another FocusFrame, they
//   are considered different. For example, a scrolling window might contain a button, but they are
//   treated as separate units of focus.
//   A frame will only ever be given focus if it is wrapped inside a FocusFrame. The GlopWindow
//   maintains a list of FocusFrames, and controls which one has focus (possibly none of them, if
//   the entire GlopWindow is out of focus), taking into account the tab key, mouse clicking, etc.
//   (Some of that behavior can be customized by rebinding the GUI derived keys in Input.) This
//   focus is then immediately propogated down to descendants of the FocusFrame. Also:
//     - Whenever a key press occurs, it is sent to the active FocusFrame. If that FocusFrame
//       does not handle the event in OnKeyEvent, then any ancestor FocusFrames are given a chance
//       to respond to the KeyEvent. If they also do not handle the event, ANY FocusFrame is allowed
//       to trap it. If any FocusFrame (or child of a FocusFrame) declares that event to be a
//       "magnet" KeyEvent, focus immediately switches to that FocusFrame, and that FocusFrame then
//       also receives normal notification of the same event.
//       Note: Only key presses (repeat, single, or double) can be focus magnet events. For example,
//             a key release cannot be a focus magnet event.
//     - A GlopFrame is notified via OnFocusChange whenever its focus changed. It can use this
//       opportunity to react accordingly.
//     - A GlopWindow may PushFocus. If this happens, all current FocusFrames lose focus, and will
//       never gain it back again until PopFocus is called. In the meantime, a new circular queue
//       of FocusFrames is formed.
//   Note that all input frames should be inside a FocusFrame. An individual frame may find its
//   encapsulating focus frame. Frames all track what FocusFrame, if any, owns them. The primary use
//   of this is so they can determine what exactly is in focus. For example, if a button is directly
//   encapsulated in a focus frame, it responds differently from a button owned by a slider frame,
//   which is then encapsulated in a focus frame.
//
// Overview of sizing: A frame's size is limited in two ways: its logical size, and its clipping
//   rectangle (stored in WINDOW coordinates). The clipping coordinates are propogated via
//   SetPosition, and are most likely changed only by ClippedFrames. The logical size is set in the
//   following manner:
//     - Every tick, the topmost frame receives an UpdateSize request with recommended size of
//       the entire screen.
//     - If a frame receives an UpdateSize request, it does the following:
//       - If its size is not dirty and if it has been recommended the same size as on the
//         previous call, it exits immediately.
//       - Otherwise it executes the virtual function, RecomputeSize.
//       - This function calls UpdateSize on all children with recommended sizes chosen as it
//         sees fit. Once this is done, it sets an actual size for itself.
//       - It then marks itself as not dirty and stores the previous recommended size for
//         checking against future calls to UpdateSize.
//   Note that RecomputeSize can be relatively heavy-weight since it should only be called if the
//   frame is actually changing size.
//
// Overview of pinging: Very often, a particular location in a frame is of particular interest,
//   and Glop should ensure this location is visible to the user, even in the context of a
//   ScrollingFrame. For example, if a user selects a new menu item, that item should be made
//   visible. This is accomplished by pinging a specific rectangle (in frame coordinates). This
//   ping is propogated upwards until it reaches a ScrollingFrame, and the ScrollingFrame then
//   makes the object visible (or, if requested, also centers the object on the screen).
//
//   A ping is resolved after a frame is resized, which means the coordinates it uses will be
//   completely up-to-date. However, it can be useful to first register a ping even when an
//   object's size is dirty, which makes it difficult to specify exact coordinates. Therefore, a
//   "ping" is actually an abstract class with GetX1(), GetY1(), GetX2(), and GetY2() functions
//   that only will be called when a ping resolves. Thus, we have things like a RelativePing where
//   a user can ping the bottom-right corner of a frame, even if its size is currently unknown.
//
// In GlopFrameBase.h, we define a set of "support" frames. As a rule of thumb, these do not render
// anything, but they help organize other frames.

#ifndef GLOP_GLOP_FRAME_BASE_H__
#define GLOP_GLOP_FRAME_BASE_H__

// Includes
#include "Base.h"
#include "GlopFrameStyle.h"
#include "List.h"
#include <vector>
using namespace std;

// Class declarations
class FocusFrame;
struct KeyEvent;

////////////////////////////////////////////////////////////////////////////////////////////////////

// GlopFrame
// =========
//
// This is the base class for all frames.
class GlopFrame {
 public:
  GlopFrame();
  virtual ~GlopFrame();

  // Debugging tools. GetType returns the class name of any GlopFrame (or should, if the GlopFrame
  // overrides GetType correctly). GetContextString returns a string representation of all
  // descendants and ascendants of this frame, including type and position.
  virtual string GetType() const {return "GlopFrame";}
  string GetContextString() const {return GetContextStringHelper(true, true, "");}

  // All frames except unused frames and the topmost tableau frame have a parent.
  const GlopFrame *GetParent() const {return parent_;}
  GlopFrame *GetParent() {return parent_;}

  // Main GlopFrame functions. OnKeyEvent is the most accurate way to respond to key presses
  // (see discussion in Input.h). Think will be called after all OnKeyEvent calls, and it is
  // suitable for all other logic.
  virtual void Render() const {}
  virtual bool OnKeyEvent(const KeyEvent &event, int dt) {return false;}
  virtual void Think(int dt) {}
  
  // Size and position accessors. All values are in pixels, and x, y are relative to the top-left
  // of the screen.
  int GetX() const {return screen_x_;}
  int GetY() const {return screen_y_;}
  int GetX2() const {return screen_x_ + width_ - 1;}
  int GetY2() const {return screen_y_ + height_ - 1;}
  int GetClipX1() const {return clip_x1_;}
  int GetClipY1() const {return clip_y1_;}
  int GetClipX2() const {return clip_x2_;}
  int GetClipY2() const {return clip_y2_;}
  int GetWidth() const {return width_;}
  int GetHeight() const {return height_;}
  
  // Size and position mutators. See discussion at the top of the file.
  void DirtySize();
  void UpdateSize(int rec_width, int rec_height);
  virtual void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);

  // Pinging. A Ping object returns a location to ping. We do not pass the location directly so
  // that it can be computed at the last second with all information available (see discussion at
  // the top of the file).
  class Ping {
   public:
    Ping(GlopFrame *frame, bool center): frame_(frame), is_centered_(center) {}
    GlopFrame *GetFrame() const {return frame_;}
    virtual void GetCoords(int *x1, int *y1, int *x2, int *y2) = 0;
    bool IsCentered() const {return is_centered_;}
   private:
    GlopFrame *frame_;
    bool is_centered_;
    DISALLOW_EVIL_CONSTRUCTORS(Ping);
  };
  
  // Ping an exact pixel location
  void NewAbsolutePing(int x, int y, bool center = false) {
    AddPing(new AbsolutePing(this, x, y, center));
  }
  void NewAbsolutePing(int x1, int y1, int x2, int y2, bool center = false) {
    AddPing(new AbsolutePing(this, x1, y1, x2, y2, center));
  }

  // Ping the location x*Width, y*Height (see discussion of Pinging at the top of the file)
  void NewRelativePing(float x, float y, bool center = false) {
    AddPing(new RelativePing(this, x, y, center));
  }
  void NewRelativePing(float x1, float y1, float x2, float y2, bool center = false) {
    AddPing(new RelativePing(this, x1, y1, x2, y2, center));
  }
  
  // Focus tracking. See discussion at the top of the file. PrimaryFocus is used to determine
  // whether this object is the entire thing in focus (e.g. a regular button) or whether it is
  // merely inheriting focus from something that owns in it (e.g. a button in a slider).
  virtual bool IsFocusMagnet(const KeyEvent &event) const {return false;}
  bool IsInFocus() const {return is_in_focus_;}
  bool IsPrimaryFocus() const;
  bool IsFocusFrame() const {return ((GlopFrame*)GetFocusFrame()) == this;}
  const FocusFrame *GetFocusFrame() const {return focus_frame_;}
  FocusFrame *GetFocusFrame() {return focus_frame_;}
  
  // Returns whether a point (given in WINDOW coordinates) is over this frame, accounting for both
  // clipping and logical size. This is useful for focus tracking due to mouse clicks for example.
  // It can be overridden if the frame's visible extent is not the same as its size.
  virtual bool IsPointVisible(int screen_x, int screen_y) const;
  
 protected:
  // Resizing and repositioning utilities.
  //  - See above for RecomputeSize. It is guaranteed that rec_width and rec_height will be at
  //    least 1, even if UpdateSize was given negative values. However, a frame must correctly
  //    handle all values of 1 or above.
  //  - SetToMaxSize sets width_ and height_ to be as large as possible while respecting the
  //    size limits and an aspect ratio. Like SetSize, this should only be used within UpdateSize.
  int GetOldRecWidth() const {return old_rec_width_;}
  int GetOldRecHeight() const {return old_rec_height_;}
  virtual void RecomputeSize(int rec_width, int rec_height) {SetSize(rec_width, rec_height);}
  void SetSize(int width, int height) {
    width_ = width;
    height_ = height;
  }
  void SetToMaxSize(int width_bound, int height_bound, float aspect_ratio);
  
  // Internal pinging functions. AddPing registers a new ping with the GlopWindow who will
  // eventually propogate it up to the parent frame. OnChildPing is called by GlopWindow when
  // resolving child pings. By default, it registers the ping and propogates it further upwards.
  void AddPing(Ping *ping);
  virtual void OnChildPing(GlopFrame *child, int x1, int y1, int x2, int y2, bool center) {
    NewAbsolutePing(x1, y1, x2, y2, center);
  }

  // Handles the fact that our focus has changed.
  virtual void OnFocusChange() {}

  // Dirties a frame's size and the size of all of its children due to the window resizing.
  // This will probably take care of itself via rec_width and rec_height. However, even if it
  // does not, we still need to note the difference in case a frame sizes itself based on the
  // window size.
  virtual void OnWindowResize(int width, int height) {DirtySize();}

  // See GetContextString above. Returns a description of this function, optionally its ancestors,
  // and optionally its descendants. prefix is outputted at the beginning of each line.
  virtual string GetContextStringHelper(bool extend_down, bool extend_up,
                                        const string &prefix) const;
 private:
  // Ping types. See above.
  class AbsolutePing: public Ping {
   public:
    AbsolutePing(GlopFrame *frame, int x, int y, bool center)
    : Ping(frame, center), x1_(x), y1_(y), x2_(x), y2_(y) {}
    AbsolutePing(GlopFrame *frame, int x1, int y1, int x2, int y2, bool center)
    : Ping(frame, center), x1_(x1), y1_(y1), x2_(x2), y2_(y2) {}
    void GetCoords(int *x1, int *y1, int *x2, int *y2) {
      *x1 = x1_;
      *y1 = y1_;
      *x2 = x2_;
      *y2 = y2_;
    }
   private:
    int x1_, y1_, x2_, y2_;
    DISALLOW_EVIL_CONSTRUCTORS(AbsolutePing);
  };
  class RelativePing: public Ping {
   public:
    RelativePing(GlopFrame *frame, float x, float y, bool center)
    : Ping(frame, center), x1_(x), y1_(y), x2_(x), y2_(y) {}
    RelativePing(GlopFrame *frame, float x1, float y1, float x2, float y2, bool center)
    : Ping(frame, center), x1_(x1), y1_(y1), x2_(x2), y2_(y2) {}
    void GetCoords(int *x1, int *y1, int *x2, int *y2) {
      *x1 = int(GetFrame()->GetWidth() * x1_);
      *y1 = int(GetFrame()->GetHeight() * y1_);
      *x2 = int((GetFrame()->GetWidth() - 1) * x2_);
      *y2 = int((GetFrame()->GetHeight() - 1) * y2_);
    }
   private:
    float x1_, y1_, x2_, y2_;
    DISALLOW_EVIL_CONSTRUCTORS(RelativePing);
  };  

  // Changes our current parent frame, and inherits any appropriate settings from our parent.
  virtual void SetParent(GlopFrame *parent);

  // Updates our focus setting, and calls OnFocusChange if it changed. It propogates down to
  // children via OnFocusChange.
  void SetFocusInfo(FocusFrame *focus_frame, bool is_in_focus) {
    if (is_in_focus != is_in_focus_ || focus_frame != focus_frame_) {
      is_in_focus_ = is_in_focus;
      focus_frame_ = focus_frame;
      OnFocusChange();
    }
  }

  // Data
  GlopFrame *parent_;
  int old_rec_width_, old_rec_height_;
  int width_, height_;
  int screen_x_, screen_y_, clip_x1_, clip_y1_, clip_x2_, clip_y2_;
  bool is_in_focus_;
  FocusFrame *focus_frame_;
  
  friend class GlopWindow;
  friend class SingleParentFrame;
  friend class MultiParentFrame;
  friend class FocusFrame;
  DISALLOW_EVIL_CONSTRUCTORS(GlopFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////

// SingleParentFrame
// =================
//
// This is a specialization of GlopFrame for any frame that has 0 or 1 children of its own.
// Default implementations of RecomputeSize and OnSetPosition are provided, but they just position
// the child at our position, and set our size to be the child's size (we use the recommended size
// if we have no child). A MultiParentFrame can handle any number of children, but this
// implementation is more efficient both time and space-wise where it applies.
class SingleParentFrame: public GlopFrame {
 public:
  SingleParentFrame(): child_(0) {}
  SingleParentFrame(GlopFrame *child): child_(0) {SetChild(child);}
  ~SingleParentFrame() {SetChild(0);}
  string GetType() const {return "SingleParentFrame";}

  // Child delegation functions
  virtual void Render() const {if (child_ != 0) child_->Render();}
  virtual bool OnKeyEvent(const KeyEvent &event, int dt) {
    if (child_ != 0 && !child_->IsFocusFrame())
      return child_->OnKeyEvent(event, dt);
    else
      return false;
  }
  virtual void Think(int dt) {if (child_ != 0) child_->Think(dt);}
  virtual void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2) {
    GlopFrame::SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
    if (child_ != 0) child_->SetPosition(screen_x, screen_y, cx1, cy1, cx2, cy2);
  }
  virtual bool IsFocusMagnet(const KeyEvent &event) const {
    return (child_ != 0 && !child_->IsFocusFrame() && child_->IsFocusMagnet(event));
  }

 protected:
  virtual void RecomputeSize(int rec_width, int rec_height) {
    if (child_ != 0) {
      child_->UpdateSize(rec_width, rec_height);
      SetSize(child_->GetWidth(), child_->GetHeight());
    } else {
      GlopFrame::RecomputeSize(rec_width, rec_height);
    }
  }
  virtual void OnFocusChange() {
    if (child_ != 0 && !child_->IsFocusFrame()) child_->SetFocusInfo(focus_frame_, is_in_focus_);
  }

  // Child manipulation. Note that any child added to a parent frame becomes owned by the
  // parent, and will be deleted on removal from the parent unless RemoveChildNoDelete is called.
  const GlopFrame *GetChild() const {return child_;}
  GlopFrame *GetChild() {return child_;}
  GlopFrame *RemoveChildNoDelete();
  void SetChild(GlopFrame *frame);
  void OnWindowResize(int width, int height) {
    DirtySize();
    if (child_ != 0) child_->OnWindowResize(width, height);
  }
  string GetContextStringHelper(bool extend_down, bool extend_up, const string &prefix) const;

 private:
  GlopFrame *child_;
  DISALLOW_EVIL_CONSTRUCTORS(SingleParentFrame);
};

// MultiParentFrame
// ================
//
// This is similar to SingleParentFrame above, except that it supports multiple children. Default
// implementations of RecomputeSize and OnSetPosition are provided, but they just position all
// children at our position, and set our size to be their max size.
//
// It is guaranteed that a MultiParentFrame will assign id's to its children using push_back from
// List. This is so that auxiliary data can easily be stored for each child. See for example
// TableauFrame.
class MultiParentFrame: public GlopFrame {
 public:
  MultiParentFrame() {}
  virtual ~MultiParentFrame() {ClearChildren();}
  string GetType() const {return "MultiParentFrame";}
  
  // Child delegation functions
  virtual void Render() const;
  virtual bool OnKeyEvent(const KeyEvent &event, int dt);
  virtual void Think(int dt);
  virtual void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
  virtual bool IsFocusMagnet(const KeyEvent &event) const;

 protected:
  virtual void RecomputeSize(int rec_width, int rec_height);
  virtual void OnFocusChange();

  // Child manipulation. Note that any child added to a parent frame becomes owned by the
  // parent, and will be deleted on removal from the parent unless RemoveChildNoDelete is
  // called.
  const GlopFrame *GetChild(ListId id) const {return children_[id];}
  GlopFrame *GetChild(ListId id) {return children_[id];}
  int GetNumChildren() const {return children_.size();}
  List<GlopFrame*>::const_iterator children_begin() const {return children_.begin();}
  List<GlopFrame*>::const_iterator children_end() const {return children_.end();}
  ListId AddChild(GlopFrame *frame);
  ListId RemoveChild(ListId id);
  GlopFrame *RemoveChildNoDelete(ListId id);
  void ClearChildren();
  void OnWindowResize(int width, int height);
  string GetContextStringHelper(bool extend_down, bool extend_up, const string &prefix) const;
  
 private:
  friend class GlopWindow;
  List<GlopFrame*> children_;
  DISALLOW_EVIL_CONSTRUCTORS(MultiParentFrame);
};

// ClippedFrame
// ============
//
// By default, a frame is free to render anywhere it sees fit, regardless of its size.
// A ClippedFrame prevents this. It restricts rendering from its children to within a specific
// box (usually but not always the bounding box it's child reports). The child will be aware of
// its clipping rectangle, which is useful primarily for mouse tracking. Clicking on a part of a
// button that is outside of the clipped area should not generate a response. The clipping info is
// also used to cull objects for rendering.
class ClippedFrame: public SingleParentFrame {
 public:
  ClippedFrame(GlopFrame *frame): SingleParentFrame(frame), is_standard_clipping_(true) {}
  string GetType() const {return "ClippedFrame";}

  void Render() const;
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
  void SetStandardClipping() {is_standard_clipping_ = true;}
  void SetClipping(int x1, int y1, int x2, int y2) {
    is_standard_clipping_ = false;
    req_clip_x1_ = x1;
    req_clip_y1_ = y1;
    req_clip_x2_ = x2;
    req_clip_y2_ = y2;
  }

 private:
  bool is_standard_clipping_;
  int req_clip_x1_, req_clip_y1_, req_clip_x2_, req_clip_y2_;
  DISALLOW_EVIL_CONSTRUCTORS(ClippedFrame);
};

// PaddedFrame
// ===========
//
// This is similar to a SingleParentFrame except that it reserves a certain amount of empty space
// around the frame border. All padding amounts are in pixels. This means that for external use,
// ScalingPaddedFrame is likely better. If a PaddedFrame has no child, it fills up the recommended
// region as normal.
class PaddedFrame: public SingleParentFrame {
 public:
  PaddedFrame(GlopFrame *frame, int padding = 0)
  : SingleParentFrame(frame), left_padding_(padding), top_padding_(padding),
    right_padding_(padding), bottom_padding_(padding) {}
  PaddedFrame(GlopFrame *frame, int left_padding, int top_padding, int right_padding,
              int bottom_padding)
  : SingleParentFrame(frame), left_padding_(left_padding), top_padding_(top_padding),
    right_padding_(right_padding), bottom_padding_(bottom_padding) {}
  string GetType() const {return "PaddedFrame";}

  // Accessors/mutators
  const GlopFrame *GetInnerFrame() const {return GetChild();}
  GlopFrame *GetInnerFrame() {return GetChild();}
  int GetLeftPadding() const {return left_padding_;}
  int GetTopPadding() const {return top_padding_;}
  int GetRightPadding() const {return right_padding_;}
  int GetBottomPadding() const {return bottom_padding_;}
  void SetPadding(int padding) {SetPadding(padding, padding, padding, padding);}
  void SetPadding(int left_padding, int top_padding, int right_padding, int bottom_padding);

  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  int left_padding_, top_padding_, right_padding_, bottom_padding_;
  DISALLOW_EVIL_CONSTRUCTORS(PaddedFrame);
};

// ScalingPaddedFrame
// ==================
//
// This is a PaddedFrame whose padding is taken to be a constant multiplier of the window size.
class ScalingPaddedFrame: public SingleParentFrame {
 public:
  ScalingPaddedFrame(GlopFrame *frame, float padding = 0)
  : SingleParentFrame(frame), scaled_left_padding_(padding), scaled_top_padding_(padding),
    scaled_right_padding_(padding), scaled_bottom_padding_(padding) {}
  ScalingPaddedFrame(GlopFrame *frame, float left_padding, float top_padding, float right_padding,
                     float bottom_padding)
  : SingleParentFrame(frame), scaled_left_padding_(left_padding), scaled_top_padding_(top_padding),
    scaled_right_padding_(right_padding), scaled_bottom_padding_(bottom_padding) {}
  string GetType() const {return "ScalingPaddedFrame";}

  // Accessors/mutators
  const GlopFrame *GetInnerFrame() const {return GetChild();}
  GlopFrame *GetInnerFrame() {return GetChild();}
  int GetAbsLeftPadding() const {return left_padding_;}
  int GetAbsTopPadding() const {return top_padding_;}
  int GetAbsRightPadding() const {return right_padding_;}
  int GetAbsBottomPadding() const {return bottom_padding_;}
  float GetRelLeftPadding() const {return scaled_left_padding_;}
  float GetRelTopPadding() const {return scaled_top_padding_;}
  float GetRelRightPadding() const {return scaled_right_padding_;}
  float GetRelBottomPadding() const {return scaled_bottom_padding_;}
  void SetPadding(float padding) {SetPadding(padding, padding, padding, padding);}
  void SetPadding(float left_padding, float top_padding, float right_padding, float bottom_padding);

  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
  int left_padding_, top_padding_, right_padding_, bottom_padding_;
  float scaled_left_padding_, scaled_top_padding_, scaled_right_padding_, scaled_bottom_padding_;
  DISALLOW_EVIL_CONSTRUCTORS(ScalingPaddedFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////

// FocusFrame
// ==========
//
// See comments at the top of the file.
class FocusFrame: public SingleParentFrame {
 public:
  FocusFrame(GlopFrame *frame);
  ~FocusFrame();
  string GetType() const {return "FocusFrame";}

  // Returns whether we are descended from the given focus frame.
  bool IsSubFocusFrame(const FocusFrame *frame) const;

  // Returns whether a KeyEvent is being processed which just gave us focus. This is useful if we
  // do not want that event to be processed. For example, a GlopKeyPromptFrame should not process
  // the tab or mouse click event that gave it focus.
  bool IsGainingFocus() const {return is_gaining_focus_;}

  // Immediately makes this FocusFrame in focus (for it's level).
  void DemandFocus();

 private:
  // Overload SetParent to not overwrite our focus information.
  virtual void SetParent(GlopFrame *parent) {parent_ = parent;}

  // GlopWindow interface
  friend class GlopWindow;
  void SetIsInFocus(bool is_in_focus) {
    if (is_in_focus && !IsInFocus()) NewRelativePing(0, 0, 1, 1);
    SetFocusInfo(this, is_in_focus);
  }

  bool is_gaining_focus_;
  int layer_;
  FocusFrame *next_, *prev_;
  DISALLOW_EVIL_CONSTRUCTORS(FocusFrame);
};

// TableauFrame
// ============
//
// This is a maximally sized frame to which one can add child frames in any position (x, y, and
// depth) with any justification. The topmost frame in the frame tree is always a TableauFrame.
//
// Objects always render in increasing order of depth so, for example, one could add an Fps frame
// with a high depth to always ensure it rendered on top of everything else.
class TableauFrame: public MultiParentFrame {
 public:
  TableauFrame(): order_dirty_(false) {}
  string GetType() const {return "TableauFrame";}
  
  // Render is overwritten to draw frames in depth order
  virtual void Render() const;
  
  // Child accessors
  const GlopFrame *GetChild(ListId id) const {return MultiParentFrame::GetChild(id);}
  GlopFrame *GetChild(ListId id) {return MultiParentFrame::GetChild(id);}
  float GetChildRelX(ListId id) const {return child_pos_[id].rel_x;}
  float GetChildRelY(ListId id) const {return child_pos_[id].rel_y;}
  int GetChildDepth(ListId id) const {return child_pos_[id].depth;}
  float GetChildHorzJustify(ListId id) const {return child_pos_[id].horz_justify;}
  float GetChildVertJustify(ListId id) const {return child_pos_[id].vert_justify;}
  
  // Child mutators. Note that MoveChild(int) moves a child in front of any other children at
  // the same depth. Also note that MoveChild(float, float) and SetChildJustify(float, float)
  // both invoke DirtySize.
  ListId AddChild(GlopFrame *frame, float rel_x, float rel_y,
                  float horz_justify, float vert_justify, int depth = 0);
  ListId AddChild(GlopFrame *frame, int depth = 0) {
    return AddChild(frame, 0.5f, 0.5f, kJustifyCenter, kJustifyCenter, depth);
  }
  void MoveChild(ListId id, int depth);
  void MoveChild(ListId id, float rel_x, float rel_y);
  void MoveChild(ListId id, float rel_x, float rel_y, int depth) {
    MoveChild(id, depth);
    MoveChild(id, rel_x, rel_y);
  }
  void SetChildJustify(ListId id, float horz_justify, float vert_justify);
  void RemoveChild(ListId id);
  GlopFrame *RemoveChildNoDelete(ListId id);
  void ClearChildren();
  
  // Sizing and positioning (see GlopFrame)
  virtual void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  virtual void RecomputeSize(int rec_width, int rec_height);
  
 private:
  // Data
  struct ChildPosition {
    float horz_justify, vert_justify;
    float rel_x, rel_y;
    int depth;
    mutable int order_pos;
  };
  class ChildOrderCompare {
   public:
     ChildOrderCompare(const TableauFrame *tableau): tableau_(tableau) {}
     bool operator()(ListId id1, ListId id2) {
       if (id1 == 0 || id2 == 0)
         return (id2 == 0);
       else
         return tableau_->child_pos_[id1].depth < tableau_->child_pos_[id2].depth;
     }
   private:
    const TableauFrame *tableau_;
  };
  List<ChildPosition> child_pos_;
  mutable vector<ListId> ordered_children_;
  mutable bool order_dirty_;
  DISALLOW_EVIL_CONSTRUCTORS(TableauFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////

// CellSize
// ========
//
// Every cell in a TableFrame (see below) needs to be given a width and a height specified as a
// CellSize, which is used to determine the cell's recommended size. The options (discussed using
// width terminology) are as follows:
//  - Default: This is the same as Fraction(1/n) where n is the number of cells in this row.
//  - Fraction(f): The cell is recommended a width of f * TableRecommendedWidth.
//  - Max: The cell is recommended a width equal to TableRecommendedWidth - width of all other
//         cells in this row.
//  - Match: The cell is recommended a width equal to the widest cell in its column.
//  - MaxDoublePass: Same as Max, but see below.
// Note that Max and Match depend on the size of other cells in the table. To help mitigate this
// problem, the cells are resized in the following order: (1) cells not requiring Match or Max
// checks, (2) cells requiring Max checks, (3) cells requiring Match checks, (4) cells requiring
// MaxDoublePass checks. In the case of MaxDoublePass, a cell is resized TWICE, once for both of
// its dimensions. This is a hack to handle cases such as: a row containing A and B, where A is
// square with height Match, and B has a fixed height with width Max.
struct CellSize {
  static CellSize Default() {return CellSize(kDefault, 0);}
  static CellSize Fraction(float fraction) {return CellSize(kFraction, fraction);}
  static CellSize Max() {return CellSize(kMax, 0);}
  static CellSize Match() {return CellSize(kMatch, 0);}
  static CellSize MaxDoublePass() {return CellSize(kMaxDoublePass, 0);}

  enum Type {kDefault, kFraction, kMax, kMatch, kMaxDoublePass};
  Type type;
  float fraction;
 private:
  CellSize(Type _type, float _fraction): type(_type), fraction(_fraction) {}
};

// TableFrame
// ==========
//
// An abstract table of Frames. Cells may be 0, in which case they are interpreted as empty.
class TableFrame: public MultiParentFrame {
 public:
  TableFrame(int num_cols, int num_rows, float default_horz_justify = kJustifyCenter,
             float default_vert_justify = kJustifyCenter);
  virtual ~TableFrame();
  string GetType() const {return "TableFrame";}
  float GetHorzPadding() const {return horz_padding_;}
  float GetVertPadding() const {return vert_padding_;}
  float GetDefaultHorzJustify() const {return default_horz_justify_;}
  float GetDefaultVertJustify() const {return default_vert_justify_;}
  void SetPadding(float horz_padding, float vert_padding) {
    if (horz_padding_ != horz_padding || vert_padding_ != vert_padding) {
      horz_padding_ = horz_padding;
      vert_padding_ = vert_padding;
      DirtySize();
    }
  }
  void SetDefaultHorzJustify(float horz_justify) {default_horz_justify_ = horz_justify;}
  void SetDefaultVertJustify(float vert_justify) {default_vert_justify_ = vert_justify;}

  // Table size and position
  void Resize(int num_cols, int num_rows);
  void InsertRow(int row);
  void InsertCol(int col);
  void DeleteRow(int row);
  void DeleteCol(int col);
  int GetNumRows() const {return num_rows_;}
  int GetNumCols() const {return num_cols_;}
  int GetColPosition(int col) const {return col_info_[col].pos;}
  int GetColSize(int col) const {return col_info_[col].size;}
  int GetRowPosition(int row) const {return row_info_[row].pos;}
  int GetRowSize(int row) const {return row_info_[row].size;}

  // Cell accessors
  const GlopFrame *GetCell(int col, int row) const {
    return (cell_info_[row*num_cols_+col].child_id == 0? 0 :
      GetChild(cell_info_[row*num_cols_+col].child_id));
  }
  GlopFrame *GetCell(int col, int row) {
    return (cell_info_[row*num_cols_+col].child_id == 0? 0 :
      GetChild(cell_info_[row*num_cols_+col].child_id));
  }
  const CellSize &GetCellWidth(int col, int row) const {
    return cell_info_[row*num_cols_+col].width;
  }
  const CellSize &GetCellHeight(int col, int row) const {
    return cell_info_[row*num_cols_+col].height;
  }
  float GetCellHorzJustify(int col, int row) const {
    return cell_info_[row*num_cols_+col].horz_justify;
  }
  float GetCellVertJustify(int col, int row) const {
    return cell_info_[row*num_cols_+col].vert_justify;
  }

  // Cell mutators. In general, setting a cell will always delete whatever content used to be
  // in that cell. This behavior can be overridden by calling ClearCellNoDelete.
  GlopFrame *ClearCellNoDelete(int col, int row);
  void SetCell(int col, int row, GlopFrame *frame) {
    SetCell(col, row, frame, CellSize::Default(), CellSize::Default(),
      default_horz_justify_, default_vert_justify_);
  }
  void SetCell(int col, int row, GlopFrame *frame, const CellSize &width,
               const CellSize &height) {
    SetCell(col, row, frame, width, height, default_horz_justify_, default_vert_justify_);
  }
  void SetCell(int col, int row, GlopFrame *frame, float horz_justify, float vert_justify) {
    SetCell(col, row, frame, CellSize::Default(), CellSize::Default(),
      horz_justify, vert_justify);
  }
  void SetCell(int col, int row, GlopFrame *frame, const CellSize &width, const CellSize &height,
               float horz_justify, float vert_justify);
  void SetCellSize(int col, int row, const CellSize &width, const CellSize &height) {
    cell_info_[row*num_cols_+col].width = width;
    cell_info_[row*num_cols_+col].height = height;
  }
  void SetCellJustify(int col, int row, float horz_justify, float vert_justify) {
    cell_info_[row*num_cols_+col].horz_justify = horz_justify;
    cell_info_[row*num_cols_+col].vert_justify = vert_justify;
  }

  // Sizing and positioning (see GlopFrame).
  virtual void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  virtual void RecomputeSize(int rec_width, int rec_height);
  
 private:
  // Data
  int num_cols_, num_rows_;
  float horz_padding_, vert_padding_;
  float default_horz_justify_, default_vert_justify_;
  struct LineInfo {
    int pos, size;
  };
  LineInfo *row_info_, *col_info_;
  struct CellInfo {
    CellSize width, height;
    float horz_justify, vert_justify;
    ListId child_id;
  };
  CellInfo *cell_info_;
  DISALLOW_EVIL_CONSTRUCTORS(TableFrame);
};

// RowFrame
// ========
//
// A simple wrapper around TableFrame for tables with one row.
class RowFrame: public SingleParentFrame {
 public:
  // Constructors - a basic one and convenience constructors that initialize up to three cells
  RowFrame(int num_cells, float default_vert_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(num_cells, 1, kJustifyCenter, default_vert_justify)) {}
  RowFrame(GlopFrame *frame, float default_vert_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 1, kJustifyCenter, default_vert_justify)) {
    SetCell(0, frame);
  }
  RowFrame(GlopFrame *frame, const CellSize &width, const CellSize &height,
           float default_vert_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 1, kJustifyCenter, default_vert_justify)) {
    SetCell(0, frame, width, height);
  }
  RowFrame(GlopFrame *frame1, GlopFrame *frame2, float default_vert_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(2, 1, kJustifyCenter, default_vert_justify)) {
    SetCell(0, frame1);
    SetCell(1, frame2);
  }
  RowFrame(GlopFrame *frame1, const CellSize &width1, const CellSize &height1,
           GlopFrame *frame2, const CellSize &width2, const CellSize &height2,
           float default_vert_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(2, 1, kJustifyCenter, default_vert_justify)) {
    SetCell(0, frame1, width1, height1);
    SetCell(1, frame2, width2, height2);
  }
  RowFrame(GlopFrame *frame1, GlopFrame *frame2, GlopFrame *frame3,
           float default_vert_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(3, 1, kJustifyCenter, default_vert_justify)) {
    SetCell(0, frame1);
    SetCell(1, frame2);
    SetCell(2, frame3);
  }
  RowFrame(GlopFrame *frame1, const CellSize &width1, const CellSize &height1,
           GlopFrame *frame2, const CellSize &width2, const CellSize &height2,
           GlopFrame *frame3, const CellSize &width3, const CellSize &height3,
           float default_vert_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(3, 1, kJustifyCenter, default_vert_justify)) {
    SetCell(0, frame1, width1, height1);
    SetCell(1, frame2, width2, height2);
    SetCell(2, frame3, width3, height3);
  }
  string GetType() const {return "RowFrame";}
  float GetPadding() const {return table()->GetHorzPadding();}
  float GetDefaultVertJustify() const {return table()->GetDefaultVertJustify();}
  void SetPadding(float padding) {table()->SetPadding(padding, 0);}
  void SetDefaultVertJustify(float vert_justify) {table()->SetDefaultVertJustify(vert_justify);}

  // Table size and position
  void Resize(int num_cells) {table()->Resize(num_cells, 1);}
  void InsertCell(int cell, GlopFrame *frame = 0) {
    table()->InsertRow(cell);
    table()->SetCell(cell, 0, frame);
  }
  void DeleteCell(int cell) {table()->DeleteRow(cell);}
  int GetNumCells() const {return table()->GetNumCols();}
  int GetCellPosition(int cell) const {return table()->GetColPosition(cell);}
  int GetCellSize(int cell) const {return table()->GetColSize(cell);}

  // Cell accessors
  const GlopFrame *GetCell(int cell) const {return table()->GetCell(cell, 0);}
  GlopFrame *GetCell(int cell) {return table()->GetCell(cell, 0);}
  const CellSize &GetCellWidth(int cell) const {return table()->GetCellWidth(cell, 0);}
  const CellSize &GetCellHeight(int cell) const {return table()->GetCellHeight(cell, 0);}
  float GetCellHorzJustify(int cell) const {return table()->GetCellHorzJustify(cell, 0);}
  float GetCellVertJustify(int cell) const {return table()->GetCellVertJustify(cell, 0);}

  // Cell mutators
  GlopFrame *ClearCellNoDelete(int cell) {return table()->ClearCellNoDelete(cell, 0);}
  void SetCell(int cell, GlopFrame *frame) {table()->SetCell(cell, 0, frame);}
  void SetCell(int cell, GlopFrame *frame, const CellSize &width, const CellSize &height) {
    table()->SetCell(cell, 0, frame, width, height);
  }
  void SetCell(int cell, GlopFrame *frame, float horz_justify, float vert_justify) {
    table()->SetCell(cell, 0, frame, horz_justify, vert_justify);
  }
  void SetCell(int cell, GlopFrame *frame, const CellSize &width, const CellSize &height,
               float vert_justify) {
    table()->SetCell(cell, 0, frame, width, height, kJustifyCenter, vert_justify);
  }
  void SetCellSize(int cell, const CellSize &width, const CellSize &height) {
    table()->SetCellSize(cell, 0, width, height);
  }
  void SetCellJustify(int cell, float vert_justify) {
    table()->SetCellJustify(cell, 0, kJustifyCenter, vert_justify);
  }

 private:
  const TableFrame *table() const {return (TableFrame*)GetChild();}
  TableFrame *table() {return (TableFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(RowFrame);
};

// ColFrame
// ========
//
// A simple wrapper around TableFrame for tables with one column.
class ColFrame: public SingleParentFrame {
 public:
  // Constructors - a basic one and convenience constructors that initialize up to three cells
  ColFrame(int num_cells, float default_horz_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, num_cells, default_horz_justify, kJustifyCenter)) {}
  ColFrame(GlopFrame *frame, float default_horz_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 1, default_horz_justify, kJustifyCenter)) {
    SetCell(0, frame);
  }
  ColFrame(GlopFrame *frame, const CellSize &width, const CellSize &height,
           float default_horz_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 1, default_horz_justify, kJustifyCenter)) {
    SetCell(0, frame, width, height);
  }
  ColFrame(GlopFrame *frame1, GlopFrame *frame2, float default_horz_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 2, default_horz_justify, kJustifyCenter)) {
    SetCell(0, frame1);
    SetCell(1, frame2);
  }
  ColFrame(GlopFrame *frame1, const CellSize &width1, const CellSize &height1,
           GlopFrame *frame2, const CellSize &width2, const CellSize &height2,
           float default_horz_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 2, default_horz_justify, kJustifyCenter)) {
    SetCell(0, frame1, width1, height1);
    SetCell(1, frame2, width2, height2);
  }
  ColFrame(GlopFrame *frame1, GlopFrame *frame2, GlopFrame *frame3,
           float default_horz_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 3, default_horz_justify, kJustifyCenter)) {
    SetCell(0, frame1);
    SetCell(1, frame2);
    SetCell(2, frame3);
  }
  ColFrame(GlopFrame *frame1, const CellSize &width1, const CellSize &height1,
           GlopFrame *frame2, const CellSize &width2, const CellSize &height2,
           GlopFrame *frame3, const CellSize &width3, const CellSize &height3,
           float default_horz_justify = kJustifyCenter)
  : SingleParentFrame(new TableFrame(1, 3, default_horz_justify, kJustifyCenter)) {
    SetCell(0, frame1, width1, height1);
    SetCell(1, frame2, width2, height2);
    SetCell(2, frame3, width3, height3);
  }
  string GetType() const {return "ColFrame";}
  float GetPadding() const {return table()->GetVertPadding();}
  float GetDefaultHorzJustify() const {return table()->GetDefaultHorzJustify();}
  void SetPadding(float padding) {table()->SetPadding(0, padding);}
  void SetDefaultHorzJustify(float horz_justify) {table()->SetDefaultHorzJustify(horz_justify);}
  
  // Table size and position
  void Resize(int num_cells) {table()->Resize(1, num_cells);}
  void InsertCell(int cell, GlopFrame *frame = 0) {
    table()->InsertRow(cell);
    table()->SetCell(0, cell, frame);
  }
  void DeleteCell(int cell) {table()->DeleteRow(cell);}
  int GetNumCells() const {return table()->GetNumRows();}
  int GetCellPosition(int cell) const {return table()->GetRowPosition(cell);}
  int GetCellSize(int cell) const {return table()->GetRowSize(cell);}

  // Cell accessors
  const GlopFrame *GetCell(int cell) const {return table()->GetCell(0, cell);}
  GlopFrame *GetCell(int cell) {return table()->GetCell(0, cell);}
  const CellSize &GetCellWidth(int cell) const {return table()->GetCellWidth(0, cell);}
  const CellSize &GetCellHeight(int cell) const {return table()->GetCellHeight(0, cell);}
  float GetCellHorzJustify(int cell) const {return table()->GetCellHorzJustify(0, cell);}
  float GetCellVertJustify(int cell) const {return table()->GetCellVertJustify(0, cell);}

  // Cell mutators
  GlopFrame *ClearCellNoDelete(int cell) {return table()->ClearCellNoDelete(0, cell);}
  void SetCell(int cell, GlopFrame *frame) {table()->SetCell(0, cell, frame);}
  void SetCell(int cell, GlopFrame *frame, const CellSize &width, const CellSize &height) {
    table()->SetCell(0, cell, frame, width, height);
  }
  void SetCell(int cell, GlopFrame *frame, float horz_justify) {
    table()->SetCell(0, cell, frame, horz_justify, kJustifyCenter);
  }
  void SetCell(int cell, GlopFrame *frame, const CellSize &width, const CellSize &height,
               float horz_justify) {
    table()->SetCell(0, cell, frame, width, height, horz_justify, kJustifyCenter);
  }
  void SetCellSize(int cell, const CellSize &width, const CellSize &height) {
    table()->SetCellSize(0, cell, width, height);
  }
  void SetCellJustify(int cell, float horz_justify) {
    table()->SetCellJustify(0, cell, horz_justify, kJustifyCenter);
  }

 private:
  const TableFrame *table() const {return (TableFrame*)GetChild();}
  TableFrame *table() {return (TableFrame*)GetChild();}
  DISALLOW_EVIL_CONSTRUCTORS(ColFrame);
};

///////////////////////////////////////////////////////////////////////////////////////////////////

// RecSizeFrame
// ============
//
// This overrides the recommended size for its child frame to be the given fraction of the window
// size. RecWidthFrame and RecHeightFrame override the recommended width and height only.
class RecWidthFrame: public SingleParentFrame {
 public:
  RecWidthFrame(GlopFrame *frame, float rec_width)
  : SingleParentFrame(frame), rec_width_override_(rec_width) {}
  string GetType() const {return "RecWidthFrame";}
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  float rec_width_override_;
  DISALLOW_EVIL_CONSTRUCTORS(RecWidthFrame);
};

class RecHeightFrame: public SingleParentFrame {
 public:
  RecHeightFrame(GlopFrame *frame, float rec_height)
  : SingleParentFrame(frame), rec_height_override_(rec_height) {}
  string GetType() const {return "RecHeightFrame";}
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  float rec_height_override_;
  DISALLOW_EVIL_CONSTRUCTORS(RecHeightFrame);
};

class RecSizeFrame: public SingleParentFrame {
 public:
  RecSizeFrame(GlopFrame *frame, float rec_width, float rec_height)
  : SingleParentFrame(frame), rec_width_override_(rec_width), rec_height_override_(rec_height) {}
  string GetType() const {return "RecSizeFrame";}
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  float rec_width_override_, rec_height_override_;
  DISALLOW_EVIL_CONSTRUCTORS(RecSizeFrame);
};

// MinSizeFrame
// ============
//
// This pads a frame until it reaches the recommended width/height/size. Alternatively, the size
// limit can be given as a fraction of the corresponding window dimension. Also, justification can
// be specified. If padding is added, this determines where the inner frame appears within the new
// padded frame.
const float kSizeLimitRec = -1e20f;
class MinWidthFrame: public SingleParentFrame {
 public:
  MinWidthFrame(GlopFrame *frame, float min_width = kSizeLimitRec,
                float horz_justify = kJustifyLeft)
  : SingleParentFrame(frame), min_width_(min_width), horz_justify_(horz_justify) {}
  string GetType() const {return "MinWidthFrame";}
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  float min_width_, horz_justify_;
  int x_offset_; 
  DISALLOW_EVIL_CONSTRUCTORS(MinWidthFrame);
};

class MinHeightFrame: public SingleParentFrame {
 public:
  MinHeightFrame(GlopFrame *frame, float min_height = kSizeLimitRec,
                 float vert_justify = kJustifyTop)
  : SingleParentFrame(frame), min_height_(min_height), vert_justify_(vert_justify) {}
  string GetType() const {return "MinHeightFrame";}
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  float min_height_, vert_justify_;
  int y_offset_;
  DISALLOW_EVIL_CONSTRUCTORS(MinHeightFrame);
};

class MinSizeFrame: public SingleParentFrame {
 public:
  MinSizeFrame(GlopFrame *frame, float min_width = kSizeLimitRec, float min_height = kSizeLimitRec,
               float horz_justify = kJustifyLeft, float vert_justify = kJustifyTop)
  : SingleParentFrame(frame), min_width_(min_width), min_height_(min_height),
    horz_justify_(horz_justify), vert_justify_(vert_justify) {}
  string GetType() const {return "MinSizeFrame";}
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  float min_width_, min_height_, horz_justify_, vert_justify_;
  int x_offset_, y_offset_;
  DISALLOW_EVIL_CONSTRUCTORS(MinSizeFrame);
};

// MaxSizeFrame
// ============
//
// This clips a frame until it fits in the recommended width/height/size. Alternatively, the size
// limit can be given as a fraction of the corresponding window dimension. MaxSizeFrame is similar
// to ScrollingFrame, but it has a few differences:
//   - There are no scroll bars, and thus,
//   - it cannot be user-controlled (although it does respond to pings)
//   - It is possible to scroll to regions past the right and bottom extents of the frame if they
//     are pinged, or if the inner frame shrinks. This is useful, for example, with text boxes.
class MaxWidthFrame: public SingleParentFrame {
 public:
  MaxWidthFrame(GlopFrame *frame, float max_width = kSizeLimitRec,
                float horz_justify = kJustifyLeft);
  string GetType() const {return "MaxWidthFrame";}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MaxWidthFrame);
};

class MaxHeightFrame: public SingleParentFrame {
 public:
  MaxHeightFrame(GlopFrame *frame, float max_height = kSizeLimitRec,
                 float vert_justify = kJustifyTop);
  string GetType() const {return "MaxHeightFrame";}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MaxHeightFrame);
};

class MaxSizeFrame: public SingleParentFrame {
 public:
  MaxSizeFrame(GlopFrame *frame, float max_width = kSizeLimitRec, float max_height = kSizeLimitRec,
               float horz_justify = kJustifyLeft, float vert_justify = kJustifyTop);
  string GetType() const {return "MaxSizeFrame";}
  void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);
  void OnChildPing(GlopFrame *child, int x1, int y1, int x2, int y2, bool center);
  void OnWindowResize(int width, int height) {
    must_recenter_ = true;
    SingleParentFrame::OnWindowResize(width, height);
  }

 private:
  void ScrollToChildPing(int x1, int y1, int x2, int y2, bool center);
  bool must_recenter_;
  float max_width_, max_height_;
  int x_offset_, y_offset_;
  DISALLOW_EVIL_CONSTRUCTORS(MaxSizeFrame);
};

// ExactSizeFrame
// ==============
//
// A combined MinSizeFrame and MaxSizeFrame. justify_max is used if the frame is too small,
// justify_min is used if the frame is too large.
class ExactWidthFrame: public MinWidthFrame {
 public:
  ExactWidthFrame(GlopFrame *frame, float width = kSizeLimitRec,
                  float horz_justify_max = kJustifyLeft, float horz_justify_min = kJustifyLeft)
  : MinWidthFrame(new MaxWidthFrame(frame, width, horz_justify_max), width, horz_justify_min) {}
  string GetType() const {return "ExactWidthFrame";}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ExactWidthFrame);
};
class ExactHeightFrame: public MinHeightFrame {
 public:
  ExactHeightFrame(GlopFrame *frame, float height = kSizeLimitRec,
                   float vert_justify_max = kJustifyTop, float vert_justify_min = kJustifyTop)
  : MinHeightFrame(new MaxHeightFrame(frame, height, vert_justify_max),
                   height, vert_justify_min) {}
  string GetType() const {return "ExactHeightFrame";}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ExactHeightFrame);
};
class ExactSizeFrame: public MinSizeFrame {
 public:
  ExactSizeFrame(GlopFrame *frame, float width = kSizeLimitRec, float height = kSizeLimitRec,
                 float horz_justify_max = kJustifyLeft, float vert_justify_max = kJustifyTop,
                 float horz_justify_min = kJustifyLeft, float vert_justify_min = kJustifyTop)
  : MinSizeFrame(new MaxSizeFrame(frame, width, height, horz_justify_max, vert_justify_max),
                 width, height, horz_justify_min, vert_justify_min) {}
  string GetType() const {return "ExactSizeFrame";}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ExactSizeFrame);
};

// ScrollingFrame
// ==============
//
// A user-scrollable frame. If the frame fits in the recommended size, embedding it within a
// ScrollingFrame has no effect. Otherwise, it adds user-controllable scroll bars as necessary
// to allow the user to scroll through the frame. It also responds to pings.
//
// See also MaxSizeFrame.
class ScrollingFrame: public FocusFrame {
 public:
  ScrollingFrame(GlopFrame *frame, const SliderViewFactory *factory = gSliderViewFactory);
  string GetType() const {return "ScrollingFrame";}
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScrollingFrame);
};

#endif // GLOP_GLOP_FRAME_BASE_H__
