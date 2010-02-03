//
//  EAGLView.m
//  Untitled
//
//  Created by Ben Wilhelm on 1/30/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "OsIphone_EAGLView.h"

@implementation EAGLView

// You must implement this method
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id) initWithFrame:(CGRect)rect
{    
    if ((self = [super initWithFrame:rect]))
    {
        // Get the layer
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
        
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        
        // Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
        glGenFramebuffersOES(1, &defaultFramebuffer);
        glGenRenderbuffersOES(1, &colorRenderbuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorRenderbuffer);
        
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
        [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:eaglLayer];
        glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
        glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
        
        [EAGLContext setCurrentContext:context];
        
      // This application only creates a single default framebuffer which is already bound at this point.
      // This call is redundant, but needed if dealing with multiple framebuffers.
      glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
      glViewport(0, 0, backingWidth, backingHeight);
      
    {{{{
      static const GLfloat squareVertices[] = {
          -0.5f,  -0.33f,
           0.5f,  -0.33f,
          -0.5f,   0.33f,
           0.5f,   0.33f,
      };
      
      static const GLubyte squareColors[] = {
          255, 255,   0, 255,
          0,   255, 255, 255,
          0,     0,   0,   0,
          255,   0, 255, 255,
      };
      
      static float transY = 0.0f;
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(0.0f, (GLfloat)(sinf(transY)/2.0f), 0.0f);
      transY += 0.075f;
      
      glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      
      glVertexPointer(2, GL_FLOAT, 0, squareVertices);
      glEnableClientState(GL_VERTEX_ARRAY);
      glColorPointer(4, GL_UNSIGNED_BYTE, 0, squareColors);
      glEnableClientState(GL_COLOR_ARRAY);
      
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }}}}
      
      // This application only creates a single color renderbuffer which is already bound at this point.
      // This call is redundant, but needed if dealing with multiple renderbuffers.
      glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
      [context presentRenderbuffer:GL_RENDERBUFFER_OES];
    }
    
    return self;
}

- (void) dealloc
{
    // Tear down GL
    if (defaultFramebuffer)
    {
        glDeleteFramebuffersOES(1, &defaultFramebuffer);
        defaultFramebuffer = 0;
    }

    if (colorRenderbuffer)
    {
        glDeleteRenderbuffersOES(1, &colorRenderbuffer);
        colorRenderbuffer = 0;
    }
    
    // Tear down context
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
    
    [context release];
    context = nil;
    
    [super dealloc];
}

@end
