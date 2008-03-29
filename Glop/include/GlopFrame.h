// GlopFrames are the fundamental rendering tools in Glop. Each one is assigned a certain region on
// the screen, and every frame, it is given the opportunity to render to that region. A single
// GlopFrame might be a button, a window, or an entire game view. Note that a GlopFrame may own
// several helper GlopFrames, although it certainly does not need to. Even a complicated game view
// would likely handle all rendering by itself. In addition to rendering, GlopFrames can influence
// the sizing and positioning of both themselves and their children, and they can also respond to
// various events.
//
// Here we discuss how to get started using GlopFrames in a fairly simple way. For more precise
// specifications of exactly how they work, see GlopFrameBase.h. For a variety of useful pre-made
// frames, see GlopFrameWidgets.h
//
//
// Getting started: Rendering a GlopFrame
// ======================================
//
// The main thing that virtually every GlopFrame has to do is overwrite the void Render() const
// function. For example, the code below is a simple frame that renders two crossing yellow lines.
//
//   class TestFrame: public GlopFrame {
//    public:
//     void Render() const {
//       GlUtils2d::DrawLine(GetX(), GetY(), GetX2(), GetY2(), kYellow);
//       GlUtils2d::DrawLine(GetX2(), GetY(), GetX(), GetY2(), kYellow);
//     }
//   };
//
// WARNING: When finished, Every GlopFrame::Render should reset EVERY OpenGL setting it changes
// except the color. Conversely, every GlopFrame may assume every OpenGL setting except the color is
// at its default state when Render is called. (It is not practical for Glop to enforce this, so
// GlopFrame's just have to make sure they follow the rules, or other GlopFrames will render badly.
//
// Note the use of GetX(), GetY(), GetX2() and GetY2() in the above example. These, along with
// GetWidth() and GetHeight() return information on where the frame is positioned at any given time.
// The size and position of the frame is updated shortly before rendering (see GlopFrameBase.h for a
// full pipeline of how frame updates work). Note that TestFrame does not have any control over
// this - it is positioned where it is told to be. (The rendering, of course, could ignore this
// position, but that is not recommended.)
//
// To actually see the frame, we need to add it to do the window. This can be done with AddFrame,
// which if no parameters are given, will try to make TestFrame as big as possible, centered in the
// window.
//
//   int main() {
//     System::Init();
//     gWindow->Create(1024, 768, false);
//     gWindow->AddFrame(new TestFrame());
//     input()->WaitForKeyPress();
//     return 0;
//   }
//
// Note that adding a frame TRANSFERS OWNERSHIP of the frame to the window. This means you should
// NOT delete the frame yourself. When you remove it from the window with RemoveFrame or with
// ClearFrames, it will automatically be deleted. If for some reason, this is not okay, you can call
// RemoveFrameNoDelete instead. This is standard practice throughout Glop - if a frame is added to
// another as a child, it becomes owned by the parent.
//
//
// Getting started: Positioning and sizing a GlopFrame
// ===================================================
//
// GlopFrame sizing is a little more complicated. There are two separate notions - a recommended
// size for each frame, and its actual size. The recommended size is provided from context, and the
// actual size is chosen by the frame itself, based upon the recommended size. In the case of our
// TestFrame above, the process worked as follows:
//  - The GlopWindow recommends that the TestFrame have size 1024x768.
//  - Based on this recommendation, the TestFrame sets its size to 1024x768.
//  - Given this size, the GlopWindow moves the TestFrame to (0,0)-(1023,767).
//
// Why do we have this multi-step process? Generally, it is convenient for you to be telling the
// frame how big it should be (e.g. I want this heads-up display here and I want it to take up 10%
// of the screen size). However, not every frame can reasonably work at every size. For example, an
// image might need to display at a certain aspect ratio, or a paragraph of text might need a
// certain number of lines to display everything. Thus, each frame will try to accomodate your
// request, but in the end, it ultimately decides what size it has to be.
//
// With that digression out of the way, we can talk about how you actually control the layout of
// frames. The simplest method is just to use the tools provided in GlopFrameBase.h.
//
// - TableauFrame: This is what GlopWindow->AddFrame is based on. When you add a frame, you choose
//    a position and a justification for it. The TableauFrame tries to make the frame as large as
//    possible without going off the edge of the screen.
//     Example: gWindow->AddFrame(new FpsFrame(), 1.0f, 1.0f, kJustifyRight, kJustifyBottom);
//    This will display the FPS on the bottom-right corner of the screen. (Note the FpsFrame
//    ignores the recommended size, and uses a text size set in GlopFrameStyle.h).
//
// - PaddedFrame, ScalingPaddedFrame: These reserve blank space around a frame as padding. As with
//    all GlopFrames, they update the recommended size intelligently.
//     Example: gWindow->AddFrame(new ScalingPaddedFrame(new TestFrame(), 0.2f));
//    This is similar to our original example, but the TestFrame will now only take up 60% of the
//    screen size, since it will have 20% padding on each side.
//
// - TableFrame, RowFrame, ColFrame: These are what they sound like. They arrange a number of frames
//    in a table, in a row, or in a column. By default, the TableFrame recommends each frame have
//    size: (rec_width / num_cols, rec_height / num_rows), but this behavior can be overridden.
//     Example: new ColFrame(new GameViewFrame(), CellSize::Default(), CellSize::Max(),
//                           new HudFrame(), CellSize::Default(), CellSize::Default());
//    The (optional) cell size parameters here tell the column frame that the GameViewFrame should
//    take up all the space that the the HudFrame does not end up using.
//
// - RecSizeFrame, RecWidthFrame, RecHeightFrame: These are convenience classes that override the
//    recommended size for children to be a specific fraction of the screen size.
//     Example: gWindow->AddFrame(new RecSizeFrame(new TestFrame(), 0.6f, 0.6f));
//    This has the same effect as the ScalingPaddedFrame example above.
//
// - MinSizeFrame, MinWidthFrame, MinHeightFrame: These add padding to a frame until it is at least
//    a certain size.
//     Example: new MinSizeFrame(new ImageFrame("image.jpg"), kSizeLimitRec, kSizeLimitRec,
//                               kJustifyCenter, kJustifyCenter);
//    This pads an image frame (which will not take up the full recommended size unless the aspect
//    ratio is just right) until it takes up the recommended size. This might be useful for an image
//    preview box, so the box itself didn't reshape itself to fit each image.
//
// - MaxSizeFrame, MaxWidthFrame, MaxHeightFrame: These clip a frame until is at most a certain
//    size.
//     Example: new MaxWidthFrame(new TextFrame(long_text));
//    This clips a line of text until it fits within the recommended width. After it is initially
//    created, you can choose which part of the text is displayed using pings (see below).
//
// - ExactSizeFrame, ExactWidthFrame, ExactHeightFrame: Convenience classes for combinining
//    MinSizeFrame and MaxSizeFrame.
//
// - ScrollingFrame: Similar to a MaxSizeFrame, except scroll bars are added automatically if the
//    frame does not complete fit within the recommended size. You can programmatically alter what
//    is being scrolling to with "pings". A ping specifies that a certain region of a certain frame
//    should be made visible to the user. ScrollingFrames and MaxSizeFrames are then scrolled so as
//    to make this happen.
//     Example: new ScrollingFrame(new RecSizeFrame(new ImageFrame("image.jpg"), 5.0f, 5.0f));
//    This allows you to scroll through a really big view of image.jpg.
//
// For the most part, there is nothing special about these tools, and you should be able to make
// your own versions if you want.
//
// Finally, if you want to change how a frame sets its actual size, based on its recommended size,
// you need to overload the RecomputeSize function. For example, if we want to make TestFrame always
// have aspect ratio 1, we could add this to it:
//
//   void RecomputeSize(int rec_width, int rec_height) {
//     SetToMaxSize(rec_width, rec_height, 1.0f);
//   }
//
// This will be called whenever it is time for the TestFrame to be resized (when the TestFrame is
// first created, when the window resizes, when rec_width or rec_height change, or when DirtySize
// is called on TestFrame.) Here, rec_width and rec_height are, of course, the recommended size.
// SetToMaxSize sets the frame size to be as large as possible while maintaining the given aspect
// ratio, and without exceeding the given width and height. In general, it is ALWAYS better to be
// under the recommended size than it is to be over.
//
// While important to understand, this is not fundamental to actually layout out frames on the
// screen. By and large, that is done with various built-in helper classes located in GlopFrameBase.
// These helper classes will end up setting rec_width and rec_height, and then afterwards, setting
// the frame position themselves.
//
//
// Getting started: Responding to events
// =====================================
//
// Often, all a GlopFrame needs to do is render and reposition itself. This is true for any static
// GlopFrame (e.g. a line of text), as well as for GlopFrames that just provide a view onto a world
// that is changed elsewhere (e.g. most game view frames). However, it is often convenient,
// especially for GUI controls, to make a GlopFrame react directly to the user or otherwise perform
// logic.
//
// The simplest mechanism for thos is to override Think. This is called once per rendering cycle,
// and it is given the number of milliseconds that have elapsed since the last call. For example, we
// could make TestFrame have pulsating yellow lines by adding this:
//
//   void Think(int dt) {
//     total_dt_ += dt;
//     brightness_ = (0.75f + 0.25f*cos(dt));
//   }
//
// and then rendering the lines with the given brightness.
//
// Next, let's consider user input. While it is possible to track input in Think, it is generally
// not recommended. Instead, GlopFrames have a notion of focus. If your frame is in focus, this
// means (1) the entire window is in focus, and (2) your frame has been marked as the target for
// user input. Initially this will never happen. To change that, you must wrap your frame in a
// FocusFrame. Then, it and all of its children will track focus as a single autonomous unit:
//
//   gWindow->AddFrame(new FocusFrame(new TestFrame()));
//
// For the basic GUI controls, we provide wrappers that create the FocusFrame for you. They are
// called "widgets". For example, use ButtonWidget instead of ButtonFrame. ScrollingFrames also
// implicitly create a FocusFrame.
//
// You can also call PushFocus and PopFocus in GlopWindow. These temporarily disable and then
// reenable focus to all existing FocusFrames. This is useful, for example, for dialog boxes that
// disable everything else while they are in existence.
//
// Once you are in focus, you will start receiving notification every time Input generates a key
// event, which you can then respond to. For example, the base color in TestFrame could be
// user-edited as follows:
//
//   bool OnKeyEvent(const KeyEvent &event, int dt) {
//     if (event.IsNonRepeatPress() && event.key == 'y')
//       color_ = kYellow;
//     else if (event.IsNonRepeatPress() && event.key == 'r')
//       color_ = kRed;
//     else
//       return false;
//     return true;
//   }
//
// As with KeyListeners, dt here is the time since the last key event. It is not related to this
// key event in particular, nor is it the same as the dt in Think. OnKeyEvent should return true if
// the event was processed by this frame. If it is NOT processed, then other frames that are
// interested in that event can immediately steal focus. To make your frame try to steal focus from
// other frames, you must override IsMagnetKey.
//
//
// Child frames:
// ============
//
// It is often useful for one GlopFrame to have several "child" frames that it controls. To do this,
// you MUST extend SingleParentFrame or MultiParentFrame instead of the standard GlopFrame. Doing
// this will automatically update the child frames whenever you update, although you might wish to
// override RecomputeSize and SetChildPosition to customize the sizing and positioning of the child
// frames.
//
//
// More information:
// =================
//
// See GlopFrameBase.h for more information on how GlopFrames are working internally, and see
// GlopFrameWidgets.h for a variety of pre-built frames (text, buttons, images, menus, etc.)

#ifndef GLOP_GLOP_FRAME_H__
#define GLOP_GLOP_FRAME_H__

// Includes
#include "GlopFrameBase.h"
#include "GlopFrameStyle.h"
#include "GlopFrameWidgets.h"

#endif // GLOP_GLOP_FRAME_H__
