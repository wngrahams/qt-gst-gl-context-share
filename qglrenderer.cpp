/*
 * GStreamer
 * Copyright (C) 2009 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2009 Andrey Nechypurenko <andreynech@gmail.com>
 * Copyright (C) 2010 Nuno Santos <nunosantos@imaginando.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <QGLWidget>
#include <QApplication>
#include <QDebug>
#include <QCloseEvent>
#include <QOpenGLContext>

#include <gst/video/video.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>

#if GST_GL_HAVE_PLATFORM_GLX
#include <GL/glx.h>
#include <QX11Info>
#include <gst/gl/x11/gstgldisplay_x11.h>


#elif GST_GL_HAVE_PLATFORM_EGL
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gst/gl/wayland/gstgldisplay_wayland.h>
#include <wayland-client.h>
#endif

#include "gstthread.h"
#include "qglrenderer.h"
#include "pipeline.h"

#if defined(MACOSX)
extern void *qt_current_nsopengl_context ();
#endif

#define CASE_STR( value ) case value: return #value; 
#if 0 //GST_GL_HAVE_PLATFORM_EGL      
const char* eglGetErrorString( EGLint error )
{
    switch( error )
    {
    CASE_STR( EGL_SUCCESS             )
    CASE_STR( EGL_NOT_INITIALIZED     )
    CASE_STR( EGL_BAD_ACCESS          )
    CASE_STR( EGL_BAD_ALLOC           )
    CASE_STR( EGL_BAD_ATTRIBUTE       )
    CASE_STR( EGL_BAD_CONTEXT         )
    CASE_STR( EGL_BAD_CONFIG          )
    CASE_STR( EGL_BAD_CURRENT_SURFACE )
    CASE_STR( EGL_BAD_DISPLAY         )
    CASE_STR( EGL_BAD_SURFACE         )
    CASE_STR( EGL_BAD_MATCH           )
    CASE_STR( EGL_BAD_PARAMETER       )
    CASE_STR( EGL_BAD_NATIVE_PIXMAP   )
    CASE_STR( EGL_BAD_NATIVE_WINDOW   )
    CASE_STR( EGL_CONTEXT_LOST        )
    default: return "Unknown";
    }
}
#endif

const char* gstGetDisplayTypeString( GstGLDisplayType type )
{
    switch ( type )
    {
    CASE_STR( GST_GL_DISPLAY_TYPE_NONE       )
    CASE_STR( GST_GL_DISPLAY_TYPE_X11        )
    CASE_STR( GST_GL_DISPLAY_TYPE_WAYLAND    )
    CASE_STR( GST_GL_DISPLAY_TYPE_COCOA      )
    CASE_STR( GST_GL_DISPLAY_TYPE_WIN32      )
    CASE_STR( GST_GL_DISPLAY_TYPE_DISPMANX   )
    CASE_STR( GST_GL_DISPLAY_TYPE_EGL        )
    CASE_STR( GST_GL_DISPLAY_TYPE_VIV_FB     )
    CASE_STR( GST_GL_DISPLAY_TYPE_GBM        )
    //CASE_STR( GST_GL_DISPLAY_TYPE_EGL_DEVICE )
    CASE_STR( GST_GL_DISPLAY_TYPE_ANY        )
    default: return "Unknown";
    }
}

const char* gstGetGLAPIString( GstGLAPI api )
{
    switch ( api )
    {
    CASE_STR( GST_GL_API_NONE    )
    CASE_STR( GST_GL_API_OPENGL  )
    CASE_STR( GST_GL_API_OPENGL3 )
    CASE_STR( GST_GL_API_GLES1   )
    CASE_STR( GST_GL_API_GLES2   )
    CASE_STR( GST_GL_API_ANY     )
    default: return "Unknown";
    }
}

const char* glGetErrorString( GLenum err )
{
    switch ( err )
    {
    CASE_STR( GL_NO_ERROR                      )
    CASE_STR( GL_INVALID_ENUM                  )
    CASE_STR( GL_INVALID_VALUE                 )
    CASE_STR( GL_INVALID_OPERATION             )
    CASE_STR( GL_INVALID_FRAMEBUFFER_OPERATION )
    CASE_STR( GL_OUT_OF_MEMORY                 )
#if 0
    CASE_STR( GL_STACK_UNDERFLOW_KHR           )
    CASE_STR( GL_STACK_OVERFLOW_KHR            )
#else
    CASE_STR( GL_STACK_UNDERFLOW               )
    CASE_STR( GL_STACK_OVERFLOW                )
#endif
    default: return "Unknown";
    }
}
#undef CASE_STR

QGLRenderer::QGLRenderer (const QString & videoLocation, QWidget * parent)
    :
QGLWidget (parent),
videoLoc (videoLocation),
gst_thread (NULL),
closing (false),
frame (NULL)
{
  move (20, 10);
  resize (640, 480);
}

QGLRenderer::~QGLRenderer ()
{
}

void
QGLRenderer::initializeGL ()
{
  GstGLContext *context;
  GstGLDisplay *display;

#if GST_GL_HAVE_PLATFORM_GLX
  qDebug("PLATFORM_GLX");
  display =
      (GstGLDisplay *) gst_gl_display_x11_new_with_display (QX11Info::
      display ());
#if 0 //GST_GL_HAVE_PLATFORM_EGL
  const char* extensions = eglQueryString(nullptr, EGL_EXTENSIONS);
  qDebug("%s", extensions);
#endif

#else
  display = gst_gl_display_new ();
#endif

#if GST_GL_HAVE_PLATFORM_WGL
  qDebug("PLATFORM_WGL");
  context =
      gst_gl_context_new_wrapped (display, (guintptr) wglGetCurrentContext (),
      GST_GL_PLATFORM_WGL, GST_GL_API_OPENGL);
#elif GST_GL_HAVE_PLATFORM_CGL
  qDebug("PLATFORM_CGL");
  context =
      gst_gl_context_new_wrapped (display,
      (guintptr) qt_current_nsopengl_context (), GST_GL_PLATFORM_CGL,
      GST_GL_API_OPENGL);
#elif GST_GL_HAVE_PLATFORM_GLX
  qDebug("PLATFORM_GLX");
  context =
      gst_gl_context_new_wrapped (display, (guintptr) glXGetCurrentContext (),
      GST_GL_PLATFORM_GLX, GST_GL_API_OPENGL);
#elif GST_GL_HAVE_PLATFORM_EGL
  qDebug("PLATFORM_EGL");

  EGLDisplay egl_display;
  EGLContext egl_context;
  EGLConfig  egl_config;
  EGLBoolean result;

  /* Get an EGL Display Connection */
  //egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  struct wl_display* wayland_display = wl_display_connect(NULL);
  if (wayland_display == NULL)
      qDebug("Can't connect to wayland display");
  else
      qDebug("Successfully connected to wayland display");

  egl_display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR,
                                      wayland_display,
                                      NULL); 
  assert (egl_display != EGL_NO_DISPLAY);

  /* initialize the EGL display connection */
  result = eglInitialize (egl_display, NULL, NULL);
  assert (EGL_FALSE != result);

  static const EGLint attribute_list[] = {
    EGL_DEPTH_SIZE, 24,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  static const EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  
  EGLint num_config;
  result = eglChooseConfig(egl_display, 
                           attribute_list, 
                           &egl_config, 
                           1, 
                           &num_config);
  assert (EGL_FALSE != result);

  /* create an EGL rendering context */
  egl_context =
      eglCreateContext (egl_display, egl_config, EGL_NO_CONTEXT,
      context_attributes);
  assert (egl_context != EGL_NO_CONTEXT);

#if 1
  EGLint retval;
  if (!eglQueryContext(egl_display, egl_context, EGL_CONFIG_ID, &retval))
        g_print("ERROR: eglQueryContext:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_CONFIG_ID=%d\n", retval);
  if (!eglQueryContext(egl_display, egl_context, EGL_CONTEXT_CLIENT_TYPE, &retval))
        g_print("ERROR: eglQueryContext:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_CONTEXT_CLIENT_TYPE=%d\n", retval);
  if (!eglQueryContext(egl_display, egl_context, EGL_CONTEXT_CLIENT_VERSION, &retval))
        g_print("ERROR: eglQueryContext:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_CONTEXT_CLIENT_VERSION=%d\n", retval);
  if (!eglQueryContext(egl_display, egl_context, EGL_RENDER_BUFFER, &retval))
        g_print("ERROR: eglQueryContext:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_RENDER_BUFFER=%d\n", retval);
#endif
#if 1
  if (!eglQueryString(egl_display, EGL_CLIENT_APIS))
        g_print("ERROR: eglQueryString:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_CLIENT_APIS=%s\n", eglQueryString(egl_display,
                                                                EGL_CLIENT_APIS));
  if (!eglQueryString(egl_display, EGL_VENDOR))
        g_print("ERROR: eglQueryString:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_VENDOR=%s\n", eglQueryString(egl_display,
                                                           EGL_VENDOR));
  if (!eglQueryString(egl_display, EGL_VERSION))
        g_print("ERROR: eglQueryString:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_VERSION=%s\n", eglQueryString(egl_display,
                                                            EGL_VERSION));
  if (!eglQueryString(egl_display, EGL_EXTENSIONS))
        g_print("ERROR: eglQueryString:%s\n", eglGetErrorString(eglGetError()));
  else
        g_print("SUCCESS: EGL_EXTENSIONS=%s\n", eglQueryString(egl_display,
                                                               EGL_EXTENSIONS));
#endif

  
  //display = (GstGLDisplay*) 
  //          gst_gl_display_egl_new_with_egl_display (egl_display);
  display = (GstGLDisplay*) 
            gst_gl_display_wayland_new_with_display(wayland_display);

  GstGLDisplayType type = gst_gl_display_get_handle_type(display);
  g_print("display type:%s\n", gstGetDisplayTypeString(type));

  context =
      gst_gl_context_new_wrapped (display, (guintptr) egl_context,
      GST_GL_PLATFORM_EGL, GST_GL_API_GLES2);

  if (context != NULL)
      g_print("Created wrapped context.\n");
  else
      g_print("ERROR: Failed to create wrapped context.\n");

  //eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  //g_print("eglMakeCurrent:%s\n", eglGetErrorString(eglGetError()));
  //gst_gl_context_activate(context, false);
#endif
  gst_object_unref (display);

  // We need to unset Qt context before initializing gst-gl plugin.
  // Otherwise the attempt to share gst-gl context with Qt will fail.
  this->doneCurrent ();
  this->gst_thread =
      new GstThread (display, context, this->videoLoc,
      SLOT (newFrame ()), this);
  this->makeCurrent ();

  QObject::connect (this->gst_thread, SIGNAL (finished ()),
      this, SLOT (close ()));
  QObject::connect (this, SIGNAL (closeRequested ()),
      this->gst_thread, SLOT (stop ()), Qt::QueuedConnection);

  qglClearColor (QColor(255,0,0));

  initShaders();
  //initTextures();

  //glShadeModel(GL_FLAT);
  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_CULL_FACE);
  /*glEnable (GL_TEXTURE_2D);     // Enable Texture Mapping
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
      g_print("ERROR: glEnable(GL_TEXTURE_2D), %s\n", glGetErrorString(err)); 
  }*/


  geometries = new GeometryEngine;

  this->gst_thread->start ();
}

void QGLRenderer::initShaders() {

    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
        emit closeRequested();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
        emit closeRequested();

    // Link shader pipeline
    if (!program.link())
        emit closeRequested();

    // Bind shader pipeline for use
    if (!program.bind())
        emit closeRequested();
}

void
QGLRenderer::resizeGL (int width, int height)
{
  // Reset The Current Viewport And Perspective Transformation
  /*glViewport (0, 0, width, height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();

  glMatrixMode (GL_MODELVIEW);*/
    /**/
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);

    // Set neaer plane to 3.0, far plane to 7.0, field of view to 45 degrees
    const qreal zNear = 3.0, zFar = 7.0, fov = 45.0;

    // Reset projection
    projection.setToIdentity();

    // Set perspective projection
    projection.perspective(fov, aspect, zNear, zFar);
    /**/
}

void
QGLRenderer::newFrame ()
{
  Pipeline *pipeline = this->gst_thread->getPipeline ();
  if (!pipeline)
    return;

  /* frame is initialized as null */
  if (this->frame)
    pipeline->queue_output_buf.put (this->frame);

  this->frame = pipeline->queue_input_buf.get ();

  /* direct call to paintGL (no queued) */
  this->updateGL ();
}

static void
flushGstreamerGL (GstGLContext * context, void *data G_GNUC_UNUSED)
{
  context->gl_vtable->Flush ();
}

void
QGLRenderer::paintGL ()
{
  static GLfloat xrot = 0;
  static GLfloat yrot = 0;
  static GLfloat zrot = 0;

  if (this->frame) {
    guint tex_id;
    GstMemory *mem;
    GstVideoInfo v_info;
    GstVideoFrame v_frame;
    GstVideoMeta *v_meta;
    GLenum err;

    /*err = glGetError();
    if (err != GL_NO_ERROR) {
        g_print("ERROR: unknown source, %s\n", glGetErrorString(err));
    }*/
    
    mem = gst_buffer_peek_memory (this->frame, 0);
    v_meta = gst_buffer_get_video_meta (this->frame);

    Q_ASSERT (gst_is_gl_memory (mem));

    GstGLMemory *gl_memory = (GstGLMemory *) mem;

    gst_gl_context_thread_add (gl_memory->mem.context, flushGstreamerGL, NULL);

    gst_video_info_set_format (&v_info, v_meta->format, v_meta->width,
        v_meta->height);

    gst_video_frame_map (&v_frame, &v_info, this->frame,
        (GstMapFlags) (GST_MAP_READ | GST_MAP_GL));

    tex_id = *(guint *) v_frame.data[0];

    glEnable (GL_DEPTH_TEST);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        g_print("ERROR: glEnable(GL_DEPTH_TEST), %s\n", glGetErrorString(err)); 
    }

    /*glEnable (GL_TEXTURE_2D);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        g_print("ERROR: glEnable(GL_TEXTURE_2D), %s\n", glGetErrorString(err));
    }*/
    glBindTexture (GL_TEXTURE_2D, tex_id);
    err = glGetError();
    if (err != GL_NO_ERROR) {
      //qDebug ("failed to bind texture that comes from gst-gl");
      g_print("ERROR: glBindTexture(GL_TEXTURE_2D, tex_id), %s\n", glGetErrorString(err));
      //emit closeRequested ();
      return;
    }

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        g_print("ERROR: glTexParameteri(...), %s\n", glGetErrorString(err));
    }
    //glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        g_print("ERROR: glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT), \
                %s\n", glGetErrorString(err));
    }

    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.scale(0.5f, 0.5f, 0.5f);
    matrix.rotate(xrot, 1.0f, 0.0f, 0.0f);
    matrix.rotate(yrot, 0.0f, 1.0f, 0.0f);
    matrix.rotate(zrot, 0.0f, 0.0f, 1.0f);
    matrix.translate(0.0, 0.0, -5.0);

    // Set modelview-projection matrix
    program.setUniformValue("mvp_matrix", projection * matrix);

    program.setUniformValue("texture", 0);

    // Draw cube geometry
    geometries->drawCubeGeometry(&program);

    /*
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    glScalef (0.5f, 0.5f, 0.5f);

    glRotatef (xrot, 1.0f, 0.0f, 0.0f);
    glRotatef (yrot, 0.0f, 1.0f, 0.0f);
    glRotatef (zrot, 0.0f, 0.0f, 1.0f);

    glBegin (GL_QUADS);
    // Front Face
    glTexCoord2f (1.0f, 0.0f);
    glVertex3f (-1.0f, -1.0f, 1.0f);
    glTexCoord2f (0.0f, 0.0f);
    glVertex3f (1.0f, -1.0f, 1.0f);
    glTexCoord2f (0.0f, 1.0f);
    glVertex3f (1.0f, 1.0f, 1.0f);
    glTexCoord2f (1.0f, 1.0f);
    glVertex3f (-1.0f, 1.0f, 1.0f);
    // Back Face
    glTexCoord2f (0.0f, 0.0f);
    glVertex3f (-1.0f, -1.0f, -1.0f);
    glTexCoord2f (0.0f, 1.0f);
    glVertex3f (-1.0f, 1.0f, -1.0f);
    glTexCoord2f (1.0f, 1.0f);
    glVertex3f (1.0f, 1.0f, -1.0f);
    glTexCoord2f (1.0f, 0.0f);
    glVertex3f (1.0f, -1.0f, -1.0f);
    // Top Face
    glTexCoord2f (1.0f, 1.0f);
    glVertex3f (-1.0f, 1.0f, -1.0f);
    glTexCoord2f (1.0f, 0.0f);
    glVertex3f (-1.0f, 1.0f, 1.0f);
    glTexCoord2f (0.0f, 0.0f);
    glVertex3f (1.0f, 1.0f, 1.0f);
    glTexCoord2f (0.0f, 1.0f);
    glVertex3f (1.0f, 1.0f, -1.0f);
    // Bottom Face
    glTexCoord2f (1.0f, 0.0f);
    glVertex3f (-1.0f, -1.0f, -1.0f);
    glTexCoord2f (0.0f, 0.0f);
    glVertex3f (1.0f, -1.0f, -1.0f);
    glTexCoord2f (0.0f, 1.0f);
    glVertex3f (1.0f, -1.0f, 1.0f);
    glTexCoord2f (1.0f, 1.0f);
    glVertex3f (-1.0f, -1.0f, 1.0f);
    // Right face
    glTexCoord2f (0.0f, 0.0f);
    glVertex3f (1.0f, -1.0f, -1.0f);
    glTexCoord2f (0.0f, 1.0f);
    glVertex3f (1.0f, 1.0f, -1.0f);
    glTexCoord2f (1.0f, 1.0f);
    glVertex3f (1.0f, 1.0f, 1.0f);
    glTexCoord2f (1.0f, 0.0f);
    glVertex3f (1.0f, -1.0f, 1.0f);
    // Left Face
    glTexCoord2f (1.0f, 0.0f);
    glVertex3f (-1.0f, -1.0f, -1.0f);
    glTexCoord2f (0.0f, 0.0f);
    glVertex3f (-1.0f, -1.0f, 1.0f);
    glTexCoord2f (0.0f, 1.0f);
    glVertex3f (-1.0f, 1.0f, 1.0f);
    glTexCoord2f (1.0f, 1.0f);
    glVertex3f (-1.0f, 1.0f, -1.0f);
    glEnd ();
    */

    //xrot -= 0.3f;
    //yrot -= 0.2f;
    //zrot -= 0.4f;

    //glLoadIdentity();
    //glDisable(GL_DEPTH_TEST);
    glBindTexture (GL_TEXTURE_2D, 0);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        g_print("ERROR: glBindTexture(GL_TEXTURE_2D, 0), %s\n",
                glGetErrorString(err));
    }

    gst_video_frame_unmap (&v_frame);
  }
}

void
QGLRenderer::closeEvent (QCloseEvent * event)
{
  if (this->closing == false) {
    this->closing = true;
    emit closeRequested ();
    event->ignore ();
  }
}
