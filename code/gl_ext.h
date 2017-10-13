#ifndef __GL_EXT_H
#define __GL_EXT_H 1

#include <SDL/SDL_opengl.h>
#include <stdio.h>

#ifdef WINDOWS
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;
extern PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB;
extern PFNGLBLENDEQUATIONPROC glBlendEquation;
extern PFNGLWINDOWPOS2IPROC glWindowPos2i;
#else
void glActiveTextureARB (GLenum);
void glMultiTexCoord4fARB (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
void glVertexAttrib4fvARB (GLuint index, GLfloat v[4]);
void glSecondaryColor3fv (GLfloat v[3]);
void glWindowPos2i (GLint, GLint);
void glBlendEquation (GLenum);
#endif

extern float glmat[16];
float *gl_get_texmat (void);
float *gl_get_projmat (void);
float *gl_get_mvmat (void);


#endif //__GL_EXT_H
