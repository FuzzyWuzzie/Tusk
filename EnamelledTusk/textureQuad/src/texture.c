// includes that we will need
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

// BCM?
#include "bcm_host.h"

// GLES and EGL
#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#define check() assert(glGetError() == 0)

// define our state container
typedef struct {
	// Screen objects
	uint32_t screenWidth;
	uint32_t screenHeight;
	
	// OpenGL|ES objects
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	
	// Shader programs
	GLuint programObject;
	
	// Textures
	GLuint texture;

	// Attribute locations
	GLint  positionLoc;
	GLint  texCoordLoc;

	// Sampler location
	GLint samplerLoc;
} STATE_STRUCT;

// and create our state and a pointer to it
static STATE_STRUCT _state, *state = &_state;

// our initialization function
static void initializeEGL(STATE_STRUCT *state) {
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	static EGL_DISPMANX_WINDOW_T nativewindow;

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	static const EGLint attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	static const EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLConfig config;

	// get an EGL display connection
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(state->display!=EGL_NO_DISPLAY);
	check();

	// initialize the EGL display connection
	result = eglInitialize(state->display, NULL, NULL);
	assert(EGL_FALSE != result);
	check();

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);
	check();

	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);
	check();

	// create an EGL rendering context
	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
	assert(state->context!=EGL_NO_CONTEXT);
	check();

	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */, &state->screenWidth, &state->screenHeight);
	assert( success >= 0 );

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = state->screenWidth;
	dst_rect.height = state->screenHeight;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = state->screenWidth << 16;
	src_rect.height = state->screenHeight << 16;		

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );
	 
	dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
	0/*layer*/, &dst_rect, 0/*src*/,
	&src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

	nativewindow.element = dispman_element;
	nativewindow.width = state->screenWidth;
	nativewindow.height = state->screenHeight;
	vc_dispmanx_update_submit_sync( dispman_update );

	check();

	state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
	assert(state->surface != EGL_NO_SURFACE);
	check();

	// connect the context to the surface
	result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
	assert(EGL_FALSE != result);
	check();

	// Set background color and clear buffers
	glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
	glClear( GL_COLOR_BUFFER_BIT );

	check();
}

// our function for when we're done
static void onExit() {
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(state->display, state->surface);
	
	// release opengl resources
	eglMakeCurrent( state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
	eglDestroySurface( state->display, state->surface );
	eglDestroyContext( state->display, state->context );
	eglTerminate( state->display );
	
	printf("Exiting...\n");
}

// create a shader object, load it's source, and compile the shader
GLuint loadShader(GLenum type, const char *shaderSrc) {
	// create the shader object
	GLuint shader = glCreateShader(type);
	
	// make sure it's valid
	if(shader == 0) {
		return 0;
	}
	
	// load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);
	
	// compile the shader
	glCompileShader(shader);
	
	// check the compile status
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	
	// check for compilation errors
	if(!compiled) {
		// store how long our info string is
		GLint infoLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
		
		// print out the info log
		if(infoLength > 1) {
			// allocate memory
			char *infoLog = malloc(sizeof(char) * infoLength);
			
			// load up the log
			glGetShaderInfoLog(shader, infoLength, NULL, infoLog);
			printf("Error compiling shader:\n%s\n", infoLog);
			
			// free up our memory
			free(infoLog);
		}
		
		// clean up
		glDeleteShader(shader);
		return 0;
	}
	
	// return our compiled shader!
	return shader;
}

int initializeShaders(STATE_STRUCT *state) {
	// create our shader sources inline here
	char vertexShaderSrc[] =
		"precision lowp float;\n"
		"attribute vec2 aVertexPosition;\n"
		"attribute vec2 aTexturePosition;\n"
		"varying vec2 vTexturePosition;\n"
		"void main() {\n"
		"	vTexturePosition = aTexturePosition;\n"
		"	gl_Position = vec4(aVertexPosition, 0.0, 1.0);\n"
		"}\n";
	char fragmentShaderSrc[] =
		"precision lowp float;\n"
		"varying vec2 vTexturePosition;\n"
		"uniform sampler2D sTexture;\n"
		"void main() {\n"
		"	gl_FragColor = texture2D(sTexture, vTexturePosition);\n"
		"}\n";
		
	// load our vertex and fragment shaders
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderSrc);
	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
	
	// create a program object to link the two together with
	GLuint programObject = glCreateProgram();
	if(programObject == 0) {
		return 0;
	}
	
	// attach our two shaders to the program
	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);
	
	// link the program up
	glLinkProgram(programObject);
	
	// and check the link status to make sure it linked ok
	GLint linked;
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	
	// check for linker errors
	if(!linked) {
		// get the size of our info buffer
		GLint infoLength = 0;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLength);
		
		// print out the info message
		if(infoLength > 1) {
			// allocate memory for it
			char *infoLog = malloc(sizeof(char) * infoLength);
			
			// actually get the log
			glGetProgramInfoLog(programObject, infoLength, NULL, infoLog);
			printf("Error linking program:\n%s\n", infoLog);
			
			// and free our log memory
			free(infoLog);
		}
		
		// clean up
		glDeleteProgram(programObject);
		return GL_FALSE;
	}
	
	// now store our program object
	state->programObject = programObject;
	
	// get the attribute locations
	state->positionLoc = glGetAttribLocation(state->programObject, "aVertexPosition");
	state->texCoordLoc = glGetAttribLocation(state->programObject, "aTexturePosition");

	// get the sampler location
	state->samplerLoc = glGetUniformLocation(state->programObject, "uTexture");
	
	// and set our clear colour
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	return GL_TRUE;
}

// create a simple texture
GLuint createSimpleTexture2D() {
   // Texture object handle
   GLuint textureId;
   
   // 2x2 Image, 3 bytes per pixel (R, G, B)
   GLubyte pixels[4 * 3] = {  
      255,   0,   0, // Red
        0, 255,   0, // Green
        0,   0, 255, // Blue
      255, 255,   0  // Yellow
   };

   // Use tightly packed data
   glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

   // Generate a texture object
   glGenTextures (1, &textureId);

   // Bind the texture object
   glBindTexture (GL_TEXTURE_2D, textureId);

   // Load the texture
   glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

   // Set the filtering mode
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// set the wrap mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   return textureId;

}

// our draw function!
void draw(STATE_STRUCT *state) {
	// load up our vertices
	GLfloat vertices[] = {
	//	  x      y       s     t
		-1.0f, -1.0f,   0.0f, 1.0f,
		 1.0f, -1.0f,   1.0f, 1.0f,
		-1.0f,  1.0f,   0.0f, 0.0f,
		 1.0f,  1.0f,   1.0f, 0.0f};
		
	// set the viewport
	glViewport(0, 0, state->screenWidth, state->screenHeight);
	
	// clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
	
	// use the program object
	glUseProgram(state->programObject);
	
	// load the vertex data
	glEnableVertexAttribArray(state->positionLoc);
	glVertexAttribPointer(state->positionLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), vertices);
	glEnableVertexAttribArray(state->texCoordLoc);
	glVertexAttribPointer(state->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), vertices + 2);
	
	// bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state->texture);

	// set the sampler texture unit to 0
	glUniform1i(state->samplerLoc, 0);
	
	// and draw with arrays!
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int main() {
	// initialize bcm
	printf("Initializing bcm...");
	bcm_host_init();
	printf(" done!\n");
	
	// clear the application state
	memset(state, 0, sizeof(*state));
	
	// initialize things
	printf("Initializing EGL...");
	initializeEGL(state);
	printf(" done!\n");
	printf("Initializing textures...");
	state->texture = createSimpleTexture2D();
	printf(" done!\n");
	printf("Initializing shaders...");
	initializeShaders(state);
	printf(" done!\n");
	
	printf("--- Info ---\n");
	printf("\tv pos ndx = %d\n\tt pos ndx = %d\n\tsamp ndx = %d\n", state->positionLoc, state->texCoordLoc, state->samplerLoc);
	
	// timing information
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	float totaltime = 0.0f;
	unsigned int frames = 0;
	gettimeofday ( &t1 , &tz );

	// our program loop
	printf("Running main loop...\n");
	while(1) {
		// get timing information
		gettimeofday(&t2, &tz);
		deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
		t1 = t2;

		// TODO: implement update function
		// draw!
		draw(state);

		// swap our buffers
		eglSwapBuffers(state->display, state->surface);

		// update our time calculations
		totaltime += deltatime;
		frames++;
		if (totaltime >  2.0f) {
			// print and calculate FPS
			printf("\r%4d frames rendered in %1.4f seconds -> FPS=%3.4f", frames, totaltime, frames/totaltime);
			totaltime -= 2.0f;
			frames = 0;
		}
	}
	
	// handle exiting
	onExit();
	return 0;
}
