// A GlopFrame is the basic unit of autonomous logic in Glop. A frame can render itself,
// be positioned and sized, think each tick, and trap input events. The exact pipeline is as
// follows:
//  - The Glop client calls gSystem->Think()
//    - All focus is updated from tabs, magnet keys, etc. (see below)
//    - All frames receive OnKeyEvent notifications from Input.
//    - All other KeyListeners receive OnKeyEvent notifications from Input.
//    - All frames think.
//    - All frames resize themselves.
//    - All pings are resolved.
//    - All frames reposition themselves and update their clipping rectangle.
//    - All frames render.
//  - Repeat.
//
// Overview of focus: A frame is said to be "in focus" if it should be responding to user input.
//   All focus is handled within the context of FocusFrames. A Focus Frame and anything descended
//   from it is considered an autonomous unit of focus. If the FocusFrame gains focus, so do all
//   of its children. One exception to this is if a FocusFrame contains another FocusFrame, they
//   are considered different. For example, a scrolling window might control a button. 
//   A frame will only ever be given focus if it is wrapped inside a FocusFrame. The GlopWindow
//   maintains a list of FocusFrames, and controls which one has focus (possibly none of them, if
//   the entire GlopWindow is out of focus), taking into account the tab key and mouse clicking.
//   This focus is then immediately propogated down to descendants of the FocusFrame. Also:
//     - Whenever a KeyEvent occurs, a FocusFrame queries its children. If any of them call this a
//       "magnet" KeyEvent, and no child of the active FocusFrame calls it a "focus keeper"
//       KeyEvent, the FocusFrame immediately takes focus. This occurs before the children receive
//       the KeyEvent, so they can now process the event normally.
//     - A GlopFrame is notified via OnFocusChange whenever its focus changed. It can use this
//       opportunity to respond.
//     - A GlopWindow may PushFocus. If this happens, all current FocusFrames lose focus, and will
//       never gain it back again until PopFocus is called. In the meantime, a new circular queue
//       of FocusFrames is formed.
//   Note that ALL input frames should be inside a FocusFrame. Thus, if we have a scrolling menu,
//   it should be encapsulated as FocusFrame -> ScrollingFrame -> MenuFrame, because the
//   ScrollingFrame needs to receive input. An individual frame may find it's encapsulating focus
//   frame. This can be useful for various purposes: using it to demand focus, finding it's size
//   so as to know what clicks are part of the same "object" as seen by the user, etc.
//   Frames all track what FocusFrame, if any, owns them. The primary use of this is so they can
//   determine what exactly is in focus. For example, if a button is directly encapsulated in a
//   focus frame, it responds differently from a button owned by a slider frame, which is then
//   encapsulated in a focus frame.
//
// Overview of sizing: A frame's size is limited in two ways: its logical size, and its clipping
//   rectangle (stored in WINDOW coordinates). The clipping coordinates are propogated via
//   SetPosition, and are most likely changed only by ClippedFrames. The logical size is set in the
//   following manner:
//     - Every tick, the topmost frame receives an UpdateSize request with recommended size of
//        the entire screen.
//     - If a frame receives an UpdateSize request, it does the following:
//       - If it's size is not dirty and if it has been recommended the same size as on the
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

#ifndef GLOP_FRAME_BASE_H__
#define GLOP_FRAME_BASE_H__

// Includes
#include "Base.h"
#include "Color.h"
#include "LightSet.h"
#include <vector>
using namespace std;

// Class declarations
class ButtonRenderer;
class SliderRenderer;
class WindowRenderer;
class FocusFrame;
struct GlopKey;
struct KeyEvent;

// FrameStyle
// ==========
//
// This specifies various constants that are used for constructing GUI frames (e.g. font color &
// size). A new FrameStyle can be created to override existing settings. If a FrameStyle is already
// in use, changing it may or may not affect existing frames. The results are undefined.
struct FrameStyle {
  FrameStyle(LightSetId _font_outline_id = 0);
  ~FrameStyle();

  // General style
  LightSetId font_outline_id;         // Default font for plain text
  Color text_color;                   // Default color for plain text
  float text_height;                  // Default size for all text
  Color prompt_highlight_color;        // Background color of text we highlight in a TextPrompt

  // Renderers
  ButtonRenderer *button_renderer;
  SliderRenderer *slider_renderer;
  WindowRenderer *window_renderer;
};
extern FrameStyle *gDefaultStyle;

///////////////////////////////////////////////////////////////////////////////////////////////////

// GlopFrame
// =========
//
// This is the base class for all frames.
class GlopFrame {
 public:
  GlopFrame();
  virtual ~GlopFrame();

  // All frames except unused frames and the topmost tableau frame have a parent.
  const GlopFrame *GetParent() const {return parent_;}
  GlopFrame *GetParent() {return parent_;}

  // Main GlopFrame functions. OnKeyEvent is the most accurate way to respond to key presses
  // (see discussion in Input.h). Think will be called after all OnKeyEvent calls, and it is
  // suitable for all other logic.
  virtual void Render() {}
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
  //  - See above for RecomputeSize.
  //  - SetToMaxSize sets width_ and height_ to be as large as possible while respecting the
  //    size limits and an aspect ratio. Like SetSize, this should only be used within UpdateSize.
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
  virtual void OnChildPing(int x1, int y1, int x2, int y2, bool center) {
    NewAbsolutePing(x1, y1, x2, y2, center);
  }

  // Handles the fact that our focus has changed.
  virtual void OnFocusChange() {}

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
      *x2 = int(GetFrame()->GetWidth() * x2_);
      *y2 = int(GetFrame()->GetHeight() * y2_);
    }
   private:
    float x1_, y1_, x2_, y2_;
    DISALLOW_EVIL_CONSTRUCTORS(RelativePing);
  };  

  // Dirties a frame's size and the size of all of its children due to the window resizing.
  // This will probably take care of itself via rec_width and rec_height. However, even if it
  // does not, we still need to note the difference in case a frame sizes itself based on the
  // window size.
  virtual void OnWindowResize(int width, int height) {DirtySize();}
  
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

  // Child delegation functions
  virtual void Render() {if (child_ != 0) child_->Render();}
  virtual bool OnKeyEvent(const KeyEvent &event, int dt) {
    return (child_ != 0 && !child_->IsFocusFrame() && child_->OnKeyEvent(event, dt));
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

 private:
  void OnWindowResize(int width, int height) {
    if (child_ != 0) child_->OnWindowResize(width, height);
  }

  // Data
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
// It is guaranteed that a MultiParentFrame will assign id's to its children using InsertItem and
// RemoveItem from LightSet. This is so that auxiliary data can easily be stored for each
// child. See for example TableauFrame.
class MultiParentFrame: public GlopFrame {
 public:
  MultiParentFrame() {}
  virtual ~MultiParentFrame() {ClearChildren();}
  
  // Child delegation functions
  virtual void Render();
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
  const GlopFrame *GetChild(LightSetId id) const {return children_[id];}
  GlopFrame *GetChild(LightSetId id) {return children_[id];}
  LightSetId GetFirstChildId() const {return children_.GetFirstId();}
  int GetNumChildren() const {return children_.GetSize();}
  LightSetId GetNextChildId(LightSetId id) const {return children_.GetNextId(id);}
  LightSetId AddChild(GlopFrame *frame);
  LightSetId RemoveChild(LightSetId id);
  GlopFrame *RemoveChildNoDelete(LightSetId id);
  void ClearChildren();
  
 private:
  virtual void OnWindowResize(int width, int height);
  
  // Data
  friend class GlopWindow;
  LightSet<GlopFrame*> children_;
  DISALLOW_EVIL_CONSTRUCTORS(MultiParentFrame);
};

// ClippedFrame
// ============

class ClippedFrame: public SingleParentFrame {
 public:
  ClippedFrame(GlopFrame *frame): SingleParentFrame(frame), is_standard_clipping_(true) {}

  void Render();
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
  PaddedFrame(GlopFrame *frame, int padding): SingleParentFrame(frame) {SetPadding(padding);}
  PaddedFrame(GlopFrame *frame, int left_padding, int top_padding, int right_padding,
              int bottom_padding): SingleParentFrame(frame) {
    SetPadding(left_padding, top_padding, right_padding, bottom_padding);
  }
 
 	void SetPosition(int screen_x, int screen_y, int cx1, int cy1, int cx2, int cy2);
 protected:
  void RecomputeSize(int rec_width, int rec_height);

  // Padding accessors/mutators
  int GetLeftPadding() const {return left_padding_;}
  int GetTopPadding() const {return top_padding_;}
  int GetRightPadding() const {return right_padding_;}
  int GetBottomPadding() const {return bottom_padding_;}
  void SetPadding(int padding) {SetPadding(padding, padding, padding, padding);}
  void SetPadding(int left_padding, int top_padding, int right_padding, int bottom_padding);

 private:
  int left_padding_, top_padding_, right_padding_, bottom_padding_;
  DISALLOW_EVIL_CONSTRUCTORS(PaddedFrame);
};

// ScalingPaddedFrame
// ==================
//
// This is a PaddedFrame whose padding is taken to be a constant multiplier of the window size.
class ScalingPaddedFrame: public PaddedFrame {
 public:
  ScalingPaddedFrame(GlopFrame *frame, float padding)
  : PaddedFrame(frame, 0), scaled_left_padding_(padding), scaled_top_padding_(padding),
    scaled_right_padding_(padding), scaled_bottom_padding_(padding) {}
  ScalingPaddedFrame(GlopFrame *frame, float left_padding, float top_padding, float right_padding,
                     float bottom_padding)
  : PaddedFrame(frame, 0), scaled_left_padding_(left_padding), scaled_top_padding_(top_padding),
    scaled_right_padding_(right_padding), scaled_bottom_padding_(bottom_padding) {}
 
 protected:
  void RecomputeSize(int rec_width, int rec_height);

 private:
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
  
  // Render is overwritten to draw frames in depth order
  virtual void Render();
  
  // Child accessors
  const GlopFrame *GetChild(LightSetId id) const {return MultiParentFrame::GetChild(id);}
  GlopFrame *GetChild(LightSetId id) {return MultiParentFrame::GetChild(id);}
  float GetChildRelX(LightSetId id) const {return child_pos_[id].rel_x;}
  float GetChildRelY(LightSetId id) const {return child_pos_[id].rel_y;}
  int GetChildDepth(LightSetId id) const {return child_pos_[id].depth;}
  float GetChildHorzJustify(LightSetId id) const {return child_pos_[id].horz_justify;}
  float GetChildVertJustify(LightSetId id) const {return child_pos_[id].vert_justify;}
  
  // Child mutators. Note that MoveChild(int) moves a child in front of any other children at
  // the same depth. Also note that MoveChild(float, float) and SetChildJustify(float, float)
  // both invoke DirtySize.
  LightSetId AddChild(GlopFrame *frame, float rel_x, float rel_y,
                      float horz_justify, float vert_justify, int depth = 0);
  void MoveChild(LightSetId id, int depth);
  void MoveChild(LightSetId id, float rel_x, float rel_y);
  void MoveChild(LightSetId id, float rel_x, float rel_y, int depth) {
    MoveChild(id, depth);
    MoveChild(id, rel_x, rel_y);
  }
  void SetChildJustify(LightSetId id, float horz_justify, float vert_justify);
  void RemoveChild(LightSetId id);
  GlopFrame *RemoveChildNoDelete(LightSetId id);
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
    int depth, order_pos;
  };
  class ChildOrderCompare {
   public:
     ChildOrderCompare(TableauFrame *tableau): tableau_(tableau) {}
     bool operator()(LightSetId id1, LightSetId id2) {
       if (id1 == 0 || id2 == 0)
         return (id2 == 0);
       else
         return tableau_->child_pos_[id1].depth < tableau_->child_pos_[id2].depth;
     }
   private:
    TableauFrame *tableau_;
  };
  LightSet<ChildPosition> child_pos_;
  vector<LightSetId> ordered_children_;
  bool order_dirty_;
  DISALLOW_EVIL_CONSTRUCTORS(TableauFrame);
};

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
  float GetDefaultHorzJustify() const {return default_horz_justify_;}
  float GetDefaultVertJustify() const {return default_vert_justify_;}
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
  float default_horz_justify_, default_vert_justify_;
  struct LineInfo {
    int pos, size;
  };
  LineInfo *row_info_, *col_info_;
  struct CellInfo {
    CellSize width, height;
    float horz_justify, vert_justify;
    LightSetId child_id;
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
  float GetDefaultVertJustify() const {return table()->GetDefaultVertJustify();}
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
  float GetDefaultHorzJustify() const {return table()->GetDefaultHorzJustify();}
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

// RecSizeFrame
// ============
//
// This overrides the recommended size for its child frame to be the given fraction of the window
// size. RecWidthFrame and RecHeightFrame override the recommended width and height only.
class RecWidthFrame: public SingleParentFrame {
 public:
  RecWidthFrame(GlopFrame *frame, float rec_width)
  : SingleParentFrame(frame), rec_width_override_(rec_width) {}
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
 protected:
  void RecomputeSize(int rec_width, int rec_height);
 private:
  float rec_width_override_, rec_height_override_;
  DISALLOW_EVIL_CONSTRUCTORS(RecSizeFrame);
};

// ScrollingFrame
// ==============

class ScrollingFrame: public FocusFrame {
 public:
  ScrollingFrame(GlopFrame *frame, const SliderRenderer *renderer = gDefaultStyle->slider_renderer);
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScrollingFrame);
};

#endif // GLOP_FRAME_BASE_H__
