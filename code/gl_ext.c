
#include "gl_ext.h"
#include "general.h"
#include <SDL/SDL.h>

#ifdef WINDOWS
GLAPI void APIENTRY gl_ext_null (void) {}

PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = (void *) gl_ext_null;
PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB = (void *) gl_ext_null;
PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB = (void *) gl_ext_null;
PFNGLBLENDEQUATIONPROC glBlendEquation = (void *) gl_ext_null;
PFNGLWINDOWPOS2IPROC glWindowPos2i = (void *) gl_ext_null;

int gl_load_ext (void)
{
    glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress ("glActiveTextureARB");
    glMultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC) SDL_GL_GetProcAddress ("glMultiTexCoord4fARB");
    glVertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARBPROC) SDL_GL_GetProcAddress ("glVertexAttrib4fvARB");
    glBlendEquation = (PFNGLBLENDEQUATIONPROC) SDL_GL_GetProcAddress ("glBlendEquation");
    glWindowPos2i = (PFNGLWINDOWPOS2IPROC) SDL_GL_GetProcAddress ("glWindowPos2i");

    return TRUE;
}

#endif


float glmat[16];

void print_matrix (float m[16])
{
	printf ("%5.3f %5.3f %5.3f %5.3f\n", m[0], m[1], m[2], m[3]);
	printf ("%5.3f %5.3f %5.3f %5.3f\n", m[4], m[5], m[6], m[7]);
	printf ("%5.3f %5.3f %5.3f %5.3f\n", m[8], m[9], m[10], m[11]);
	printf ("%5.3f %5.3f %5.3f %5.3f\n", m[12], m[13], m[14], m[15]);
}


float *gl_get_texmat (void)
{
	glGetFloatv (GL_TEXTURE_MATRIX, glmat);
	print_matrix (glmat);
	return glmat;
}


float *gl_get_projmat (void)
{
	glGetFloatv (GL_PROJECTION_MATRIX, glmat);
	print_matrix (glmat);
	return glmat;
}


float *gl_get_mvmat (void)
{
	glGetFloatv (GL_MODELVIEW_MATRIX, glmat);
	print_matrix (glmat);
	return glmat;
}
