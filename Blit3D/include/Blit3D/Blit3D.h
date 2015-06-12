/* Blit3D cross-platform game graphics library, written by Darren Reid

version 0.95 - now on Github. Added Blit3DWindowModel::DECORATEDWINDOW_1080P for single-screen debugging; provides a floating window
	that uses graphics scaled from 1080p. Added windowName string to constructor.
version 0.9 - added #define GLEW_STATIC for new build methodology, sprite angles are now in radians (because GLM update required it),
	now link only to opengl32.lib and Blit3D.lib in your Blit3D programs, and only require one dll (FreeImage, as it is not designed 
	for proper static builds). Fixed "warning C4316: 'Sprite' : object allocated on the heap may not be aligned 16" that was caused
	by the updated GLM, which means SSE may not run as efficiently, by commenting out the "#define GLM_HAS_ANONYMOUS_UNION" line in 
	glm/detail/setup.hpp.
version 0.8 - More Angelcode font stuff: fixed character yoffset calculations for Blit3D coordinate system
version 0.77 - fixed bug in Angelcode font loading if the glyph was stored more than once in the font file
version 0.76 - Added Unicode support and kerning to the Angelcode fonts. BFonts should be considered "depreciated" from this version on.
	Changed TextureManager to store textures in an unordered_map instead of a map.
version 0.75 - Added Angelcode bitmap font support
version 0.71 - Now sprites set the texture magnification filter to GL_NEAREST, for pixel graphics goodness. 
version 0.7 - added BORDERLESSFULLSCREEN_1080P window mode. In this mode, the screen is scaled to 1920x1080, so we can run on ANY resolution screen and pretend to be 1080p.
	THIS IS REALLY REALLY USEFULL, and should be the default window mode for most games.
	REMEMBER to scale mouse inputs, using blit3D->trueScreenWidth/blit3D->screenWidth etc.
	Fixed error with user-provided shader not getting updated alpha and scaling in SetMode()
version 0.65 - changed sprite scaling to allow seperate x/y axis scaling. Impacts Sprite->Blit() overloads. Also placed a call to SetMode(Blit3DRenderMode::BLIT2D) before Init().
version 0.6 - Changed Bfont::DrawText() to BFont::BlitText(), to avoid collision with a #define in windows.h
version 0.5 - added pure multithreaded mode, changed simplemultithreaded mode so that it doesn't keep spawning/joining threads
version 0.21 - enabled alpha blending by default
version 0.2 - moved classes into seperate header/cpp files, added spriteSet for automatic deletion of sprites
	created by the sprite factory MakeSprite() calls.
version 0.11
*/

#pragma once

//defining _USE_MATH_DEFINES here means that we have access to some extra defines in math.h
#define _USE_MATH_DEFINES

#ifdef _WIN32
	#define WIN32_EXTRA_LEAN
	#include <Windows.h>
#endif

#define GLEW_STATIC
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library

#include <glm/glm.hpp> //GLM OpenGL math library
#include <glm/gtc/type_ptr.hpp> //used to covert glm data to OpenGL data
#include <glm/gtc/quaternion.hpp> //quaternions for handling rotation
#include < glm/gtc/matrix_transform.hpp > //transformation functiona ala OpenGL

#include <stdio.h>
#include <conio.h>
#include <thread>

#include "Logger.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdarg.h>

#include <Commdlg.h>
#include <cassert>
#include <sstream> 
#include <unordered_set>

#include <atomic>
#include <mutex>

#include "Blit3D/TextureManager.h"
#include "Blit3D/ShaderManager.h"
#include "Blit3D/RenderBuffer.h"
#include "Blit3D/Sprite.h"
#include "Blit3D/BFont.h"
#include "Blit3D/AngelcodeFont.h"

//this macro helps calculate offsets for VBO stuff
//Pass i as the number of bytes for the offset, so be sure to use sizeof() 
//to help calculate bytes accurately.
#define BUFFER_OFFSET(i) ((char *)NULL + (i))


namespace B3D
{
	//structure to store vertex info for textured objects
	class TVertex
	{
	public:
		GLfloat x, y, z;//position		
		GLfloat u, v; //texture coordinates
	};

	class JoystickState
	{
	public:
		int axisCount; //how many axes there are for this joystick...yes, "axes" is the plural of "axis", I know it looks funny
		const float *axisStates; //array of axisCount floats, between -1 and 1 in value
		int buttonCount; //how many buttons there are for this joystick
		const unsigned char *buttonStates; //array of buttonCount unsigned chars, will either be GLFW_PRESS or GLFW_RELEASE in value
	};	
}

enum class Blit3DThreadModel { SINGLETHREADED = 1, SIMPLEMULTITHREADED, MULTITHREADED };

enum class Blit3DWindowModel { DECORATEDWINDOW = 1, FULLSCREEN, BORDERLESSFULLSCREEN, BORDERLESSFULLSCREEN_1080P, DECORATEDWINDOW_1080P };

enum class Blit3DRenderMode { BLIT2D = 0, BLIT3D };


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

class Sprite;
class BFont;
class RenderBuffer;
class AngelcodeFont;

class Blit3D
{
public:
	ShaderManager *sManager;
	TextureManager *tManager;

	GLFWwindow* window;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;

	Blit3DRenderMode mode;

	std::atomic<float> screenWidth, screenHeight, trueScreenWidth, trueScreenHeight;
	Blit3DWindowModel winMode;

	float nearplane, farplane;
	GLSLProgram *shader2d;

	//function pointers
private:
	void (*Init)(void);
	void (*Update)(double);
	void (*Draw)(void);
	void (*DeInit)(void);
	void (*DoInput)(int , int, int, int);
	void (*Sync)(void);
	void(*DoCursor)(double, double);
	void(*DoMouseButton)(int, int, int);
	void(*DoScrollwheel)(double, double);
	void(*DoJoystick)(void);

	std::mutex spriteMutex;
	std::unordered_set<Sprite *> spriteSet;
	std::string windowName;

public:	

	Blit3D(Blit3DWindowModel windowMode, const char* window_name, int width = 1920, int height = 1080);

	//for Josh Cooper...I swear, no one else needs this ;)
	Blit3D(); 

	~Blit3D();
	
	int Run(Blit3DThreadModel threadType);
	void Quit(void);

	Sprite *MakeSprite(GLfloat startX, GLfloat startY, GLfloat width, GLfloat height, std::string TextureFileName);
	Sprite *MakeSprite(RenderBuffer *rb);
	void DeleteSprite(Sprite *sprite);
	
	RenderBuffer *MakeRenderBuffer(int width, int height, std::string name);
	
	BFont *MakeBFont(std::string TextureFileName, std::string widths_file, float fontsize);
	AngelcodeFont *MakeAngelcodeFontFromBinary32(std::string filename);
	
	void Reshape(GLSLProgram *shader);
	void ReshapFBO(int FBOwidth, int FBOheight, GLSLProgram *shader);
	void SetMode(Blit3DRenderMode newMode);
	void SetMode(Blit3DRenderMode newMode, GLSLProgram *shader);
	Blit3DRenderMode GetMode(void);

	//methods for setting callbacks
	void SetInit(void(*func)(void));
	void SetUpdate(void(*func)(double));
	void SetDraw(void(*func)(void));
	void SetDeInit(void(*func)(void));
	void SetDoInput(void(*func)(int, int, int, int));
	void SetSync(void(*func)(void));
	void SetDoCursor(void(*func)(double, double));
	void SetDoMouseButton(void(*func)(int, int, int));
	void SetDoScrollwheel(void(*func)(double, double)); 
	void SetDoJoystick(void(*func)(void));

	//joystick polling, fills out the state struct if it returns true, returns false if the joystick isn't plugged in.
	bool PollJoystick(int joystickNumber, B3D::JoystickState &joystickState);
	//poll joystick to see if it is still plugged in
	bool CheckJoystick(int joystickNumber);
	
	void ShowCursor(bool show);
};
