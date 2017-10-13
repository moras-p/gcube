
#include "gl_ext.h"


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
