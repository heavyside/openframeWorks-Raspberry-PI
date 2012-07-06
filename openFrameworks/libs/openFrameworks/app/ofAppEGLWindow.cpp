#include "ofAppEGLWindow.h"
#include "ofBaseApp.h"
#include "ofEvents.h"
#include "ofUtils.h"
#include "ofGraphics.h"
#include "ofAppRunner.h"
#include "ofConstants.h"

#ifdef TARGET_WIN32
	
#endif
#ifdef TARGET_OSX

#endif
#ifdef TARGET_LINUX
	#warning "JVC"

	#include <GLES/gl.h>
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/glext.h>

#endif


// glut works with static callbacks UGH, so we need static variables here:

static int			windowMode;
static bool			bNewScreenMode;
static float		timeNow, timeThen, fps;
static int			nFramesForFPS;
static int			nFrameCount;
static int			buttonInUse;
static bool			bEnableSetupScreen;
static bool			bDoubleBuffered; 


static bool			bFrameRateSet;
static int 			millisForFrame;
static int 			prevMillis;
static int 			diffMillis;

static float 		frameRate;

static double		lastFrameTime;

static int			requestedWidth;
static int			requestedHeight;
static int 			nonFullScreenX;
static int 			nonFullScreenY;
static int			windowW;
static int			windowH;
static int          nFramesSinceWindowResized;
static ofOrientation	orientation;
static ofBaseApp *  ofAppPtr;



//----------------------------------------------------------
ofAppEGLWindow::ofAppEGLWindow(){
	timeNow				= 0;
	timeThen			= 0;
	fps					= 60.0; //give a realistic starting value - win32 issues
	frameRate			= 60.0;
	windowMode			= OF_WINDOW;
	bNewScreenMode		= true;
	nFramesForFPS		= 0;
	nFramesSinceWindowResized = 0;
	nFrameCount			= 0;
	buttonInUse			= 0;
	bEnableSetupScreen	= true;
	bFrameRateSet		= false;
	millisForFrame		= 0;
	prevMillis			= 0;
	diffMillis			= 0;
	requestedWidth		= 0;
	requestedHeight		= 0;
	nonFullScreenX		= -1;
	nonFullScreenY		= -1;
	lastFrameTime		= 0.0;
	displayString		= "";
	orientation			= OF_ORIENTATION_DEFAULT;
	bDoubleBuffered = true; // LIA

}

//lets you enable alpha blending using a display string like:
// "rgba double samples>=4 depth" ( mac )
// "rgb double depth alpha samples>=4" ( some pcs )
//------------------------------------------------------------
 void ofAppEGLWindow::setGlutDisplayString(string displayStr){
	displayString = displayStr;
 }


void ofAppEGLWindow::setDoubleBuffering(bool _bDoubleBuffered){ 
	bDoubleBuffered = _bDoubleBuffered;
}



//------------------------------------------------------------
void ofAppEGLWindow::setupOpenGL(int w, int h, int screenMode){

	int argc = 1;
	char *argv = (char*)"openframeworks";
	char **vptr = &argv;
	glutInit(&argc, vptr);

	if( displayString != ""){
		glutInitDisplayString( displayString.c_str() );
	}else{
		if(bDoubleBuffered){  
			glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA );
		}else{
			glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH | GLUT_ALPHA );
		}
	}

	windowMode = screenMode;
	bNewScreenMode = true;

	if (windowMode != OF_GAME_MODE){
		glutInitWindowSize(w, h);
		glutCreateWindow("");

		/*
		ofBackground(200,200,200);		// default bg color
		ofSetColor(0xFFFFFF); 			// default draw color
		// used to be black, but
		// black + texture = black
		// so maybe grey bg
		// and "white" fg color
		// as default works the best...
		*/

		requestedWidth  = glutGet(GLUT_WINDOW_WIDTH);
		requestedHeight = glutGet(GLUT_WINDOW_HEIGHT);
	} else {
		if( displayString != ""){
			glutInitDisplayString( displayString.c_str() );
		}else{
			glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA );
		}

    	// w x h, 32bit pixel depth, 60Hz refresh rate
		char gameStr[64];
		sprintf( gameStr, "%dx%d:%d@%d", w, h, 32, 60 );

    	glutGameModeString(gameStr);

    	if (!glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)){
    		ofLog(OF_LOG_ERROR,"game mode error: selected format (%s) not available \n", gameStr);
    	}
    	// start fullscreen game mode
    	glutEnterGameMode();
	}
	windowW = glutGet(GLUT_WINDOW_WIDTH);
	windowH = glutGet(GLUT_WINDOW_HEIGHT);
}

//------------------------------------------------------------
void ofAppEGLWindow::initializeWindow(){


    //----------------------
    // setup the callbacks

    glutMouseFunc(mouse_cb);
    glutMotionFunc(motion_cb);
    glutPassiveMotionFunc(passive_motion_cb);
    glutIdleFunc(idle_cb);
    glutDisplayFunc(display);

    glutKeyboardFunc(keyboard_cb);
    glutKeyboardUpFunc(keyboard_up_cb);
    glutSpecialFunc(special_key_cb);
    glutSpecialUpFunc(special_key_up_cb);

    glutReshapeFunc(resize_cb);
	glutEntryFunc(entry_cb);

#ifdef TARGET_OSX
	glutDragEventFunc(dragEvent);
#endif

    nFramesSinceWindowResized = 0;

    #ifdef TARGET_WIN32
        //----------------------
        // this is specific to windows (respond properly to close / destroy)
        fixCloseWindowOnWin32();
    #endif

}

//------------------------------------------------------------
void ofAppEGLWindow::runAppViaInfiniteLoop(ofBaseApp * appPtr){
	ofAppPtr = appPtr;

	ofNotifySetup();
	ofNotifyUpdate();

	glutMainLoop();
}



//------------------------------------------------------------
float ofAppEGLWindow::getFrameRate(){
	return frameRate;
}

//------------------------------------------------------------
double ofAppEGLWindow::getLastFrameTime(){
	return lastFrameTime;
}

//------------------------------------------------------------
int ofAppEGLWindow::getFrameNum(){
	return nFrameCount;
}

//------------------------------------------------------------
void ofAppEGLWindow::setWindowTitle(string title){
	glutSetWindowTitle(title.c_str());
}

//------------------------------------------------------------
ofPoint ofAppEGLWindow::getWindowSize(){
	return ofPoint(windowW, windowH,0);
}

//------------------------------------------------------------
ofPoint ofAppEGLWindow::getWindowPosition(){
	int x = -1;
	int y = -1;
	return ofPoint(x,y,0);
}

//------------------------------------------------------------
ofPoint ofAppEGLWindow::getScreenSize(){
	int width = -1;
	int height = -1;
	return ofPoint(width, height,0);
}

//------------------------------------------------------------
int ofAppEGLWindow::getWidth(){
	if( orientation == OF_ORIENTATION_DEFAULT || orientation == OF_ORIENTATION_180 ){
		return windowW;
	}
	return windowH;
}

//------------------------------------------------------------
int ofAppEGLWindow::getHeight(){
	if( orientation == OF_ORIENTATION_DEFAULT || orientation == OF_ORIENTATION_180 ){
		return windowH;
	}
	return windowW;
}

//------------------------------------------------------------
void ofAppEGLWindow::setOrientation(ofOrientation orientationIn){
	orientation = orientationIn;
}

//------------------------------------------------------------
ofOrientation ofAppEGLWindow::getOrientation(){
	return orientation;
}

//------------------------------------------------------------
void ofAppEGLWindow::setWindowPosition(int x, int y){
	glutPositionWindow(x,y);
}

//------------------------------------------------------------
void ofAppEGLWindow::setWindowShape(int w, int h){
	glutReshapeWindow(w, h);
	// this is useful, esp if we are in the first frame (setup):
	requestedWidth  = w;
	requestedHeight = h;
}

//------------------------------------------------------------
void ofAppEGLWindow::hideCursor(){

}

//------------------------------------------------------------
void ofAppEGLWindow::showCursor(){

}

//------------------------------------------------------------
void ofAppEGLWindow::setFrameRate(float targetRate){
	// given this FPS, what is the amount of millis per frame
	// that should elapse?

	// --- > f / s

	if (targetRate == 0){
		bFrameRateSet = false;
		return;
	}

	bFrameRateSet 			= true;
	float durationOfFrame 	= 1.0f / (float)targetRate;
	millisForFrame 			= (int)(1000.0f * durationOfFrame);

	frameRate				= targetRate;

}

//------------------------------------------------------------
int ofAppEGLWindow::getWindowMode(){
	return windowMode;
}

//------------------------------------------------------------
void ofAppEGLWindow::toggleFullscreen(){
	if( windowMode == OF_GAME_MODE)return;

	if( windowMode == OF_WINDOW ){
		windowMode = OF_FULLSCREEN;
	}else{
		windowMode = OF_WINDOW;
	}

	bNewScreenMode = true;
}

//------------------------------------------------------------
void ofAppEGLWindow::setFullscreen(bool fullscreen){
    if( windowMode == OF_GAME_MODE)return;

    if(fullscreen && windowMode != OF_FULLSCREEN){
        bNewScreenMode  = true;
        windowMode      = OF_FULLSCREEN;
    }else if(!fullscreen && windowMode != OF_WINDOW) {
        bNewScreenMode  = true;
        windowMode      = OF_WINDOW;
    }
}

//------------------------------------------------------------
void ofAppEGLWindow::enableSetupScreen(){
	bEnableSetupScreen = true;
}

//------------------------------------------------------------
void ofAppEGLWindow::disableSetupScreen(){
	bEnableSetupScreen = false;
}


//------------------------------------------------------------
void ofAppEGLWindow::display(void){

	//--------------------------------
	// when I had "glutFullScreen()"
	// in the initOpenGl, I was gettings a "heap" allocation error
	// when debugging via visual studio.  putting it here, changes that.
	// maybe it's voodoo, or I am getting rid of the problem
	// by removing something unrelated, but everything seems
	// to work if I put fullscreen on the first frame of display.

	if (windowMode != OF_GAME_MODE){
		if ( bNewScreenMode ){
			if( windowMode == OF_FULLSCREEN){

				//----------------------------------------------------
				// before we go fullscreen, take a snapshot of where we are:
				nonFullScreenX = glutGet(GLUT_WINDOW_X);
				nonFullScreenY = glutGet(GLUT_WINDOW_Y);
				//----------------------------------------------------

				glutFullScreen();

				#ifdef TARGET_OSX
					SetSystemUIMode(kUIModeAllHidden,NULL);
					#ifdef MAC_OS_X_VERSION_10_7 //needed for Lion as when the machine reboots the app is not at front level
						if( nFrameCount <= 10 ){  //is this long enough? too long? 
							ProcessSerialNumber psn;							
							OSErr err = GetCurrentProcess( &psn );
							if ( err == noErr ){
								SetFrontProcess( &psn );
							}
						}
					#endif
				#endif

			}else if( windowMode == OF_WINDOW ){

				glutReshapeWindow(requestedWidth, requestedHeight);

				//----------------------------------------------------
				// if we have recorded the screen posion, put it there
				// if not, better to let the system do it (and put it where it wants)
				if (nFrameCount > 0){
					glutPositionWindow(nonFullScreenX,nonFullScreenY);
				}
				//----------------------------------------------------

				#ifdef TARGET_OSX
					SetSystemUIMode(kUIModeNormal,NULL);
				#endif
			}
			bNewScreenMode = false;
		}
	}

	// set viewport, clear the screen
	ofViewport(0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));		// used to be glViewport( 0, 0, width, height );
	float * bgPtr = ofBgColorPtr();
	bool bClearAuto = ofbClearBg();

    // to do non auto clear on PC for now - we do something like "single" buffering --
    // it's not that pretty but it work for the most part

    #ifdef TARGET_WIN32
    if (bClearAuto == false){
        glDrawBuffer (GL_FRONT);
    }
    #endif

	if ( bClearAuto == true || nFrameCount < 3){
		ofClear(bgPtr[0]*255,bgPtr[1]*255,bgPtr[2]*255, bgPtr[3]*255);
	}

	if( bEnableSetupScreen )ofSetupScreen();

	ofNotifyDraw();

    #ifdef TARGET_WIN32
    if (bClearAuto == false){
        // on a PC resizing a window with this method of accumulation (essentially single buffering)
        // is BAD, so we clear on resize events.
        if (nFramesSinceWindowResized < 3){
        	ofClear(bgPtr[0]*255,bgPtr[1]*255,bgPtr[2]*255, bgPtr[3]*255);
        } else {
            if ( (nFrameCount < 3 || nFramesSinceWindowResized < 3) && bDoubleBuffered)    glutSwapBuffers();
            else                                                     glFlush();
        }
    } else {
        if(bDoubleBuffered){
			glutSwapBuffers();
		} else{
			glFlush();
		}
    }
    #else
		if (bClearAuto == false){
			// in accum mode resizing a window is BAD, so we clear on resize events.
			if (nFramesSinceWindowResized < 3){
				ofClear(bgPtr[0]*255,bgPtr[1]*255,bgPtr[2]*255, bgPtr[3]*255);
			}
		}
		if(bDoubleBuffered){
			glutSwapBuffers();
		} else{
			glFlush();
		}
    #endif

    nFramesSinceWindowResized++;

	//fps calculation moved to idle_cb as we were having fps speedups when heavy drawing was occuring
	//wasn't reflecting on the actual app fps which was in reality slower.
	//could be caused by some sort of deferred drawing?

	nFrameCount++;		// increase the overall frame count

	//setFrameNum(nFrameCount); // get this info to ofUtils for people to access

}

//------------------------------------------------------------
void rotateMouseXY(ofOrientation orientation, int &x, int &y) {
	int savedY;
	switch(orientation) {
		case OF_ORIENTATION_180:
			x = ofGetWidth() - x;
			y = ofGetHeight() - y;
			break;

		case OF_ORIENTATION_90_RIGHT:
			savedY = y;
			y = x;
			x = ofGetWidth()-savedY;
			break;

		case OF_ORIENTATION_90_LEFT:
			savedY = y;
			y = ofGetHeight() - x;
			x = savedY;
			break;

		case OF_ORIENTATION_DEFAULT:
		default:
			break;
	}
}

//------------------------------------------------------------
void ofAppEGLWindow::mouse_cb(int button, int state, int x, int y) {
	rotateMouseXY(orientation, x, y);

	if (nFrameCount > 0){
		if (state == GLUT_DOWN) {
			ofNotifyMousePressed(x, y, button);
		} else if (state == GLUT_UP) {
			ofNotifyMouseReleased(x, y, button);
		}

		buttonInUse = button;
	}
}

//------------------------------------------------------------
void ofAppEGLWindow::motion_cb(int x, int y) {
	rotateMouseXY(orientation, x, y);

	if (nFrameCount > 0){
		ofNotifyMouseDragged(x, y, buttonInUse);
	}

}

//------------------------------------------------------------
void ofAppEGLWindow::passive_motion_cb(int x, int y) {
	rotateMouseXY(orientation, x, y);

	if (nFrameCount > 0){
		ofNotifyMouseMoved(x, y);
	}
}

//------------------------------------------------------------
void ofAppEGLWindow::dragEvent(char ** names, int howManyFiles, int dragX, int dragY){

	// TODO: we need position info on mac passed through
	ofDragInfo info;
	info.position.x = dragX;
	info.position.y = ofGetHeight()-dragY;

	for (int i = 0; i < howManyFiles; i++){
		string temp = string(names[i]);
		info.files.push_back(temp);
	}

	ofNotifyDragEvent(info);
}


//------------------------------------------------------------
void ofAppEGLWindow::idle_cb(void) {

	//	thanks to jorge for the fix:
	//	http://www.openframeworks.cc/forum/viewtopic.php?t=515&highlight=frame+rate

	if (nFrameCount != 0 && bFrameRateSet == true){
		diffMillis = ofGetElapsedTimeMillis() - prevMillis;
		if (diffMillis > millisForFrame){
			; // we do nothing, we are already slower than target frame
		} else {
			int waitMillis = millisForFrame - diffMillis;
			#ifdef TARGET_WIN32
				Sleep(waitMillis);         //windows sleep in milliseconds
			#else
				usleep(waitMillis * 1000);   //mac sleep in microseconds - cooler :)
			#endif
		}
	}
	prevMillis = ofGetElapsedTimeMillis(); // you have to measure here

    // -------------- fps calculation:
	// theo - now moved from display to idle_cb
	// discuss here: http://github.com/openframeworks/openFrameworks/issues/labels/0062#issue/187
	//
	//
	// theo - please don't mess with this without letting me know.
	// there was some very strange issues with doing ( timeNow-timeThen ) producing different values to: double diff = timeNow-timeThen;
	// http://www.openframeworks.cc/forum/viewtopic.php?f=7&t=1892&p=11166#p11166

	timeNow = ofGetElapsedTimef();
	double diff = timeNow-timeThen;
	if( diff  > 0.00001 ){
		fps			= 1.0 / diff;
		frameRate	*= 0.9f;
		frameRate	+= 0.1f*fps;
	 }
	 lastFrameTime	= diff;
	 timeThen		= timeNow;
  	// --------------

	ofNotifyUpdate();

	glutPostRedisplay();
}


//------------------------------------------------------------
void ofAppEGLWindow::keyboard_cb(unsigned char key, int x, int y) {
	ofNotifyKeyPressed(key);
}

//------------------------------------------------------------
void ofAppEGLWindow::keyboard_up_cb(unsigned char key, int x, int y){
	ofNotifyKeyReleased(key);
}

//------------------------------------------------------
void ofAppEGLWindow::special_key_cb(int key, int x, int y) {
	ofNotifyKeyPressed(key | OF_KEY_MODIFIER);
}

//------------------------------------------------------------
void ofAppEGLWindow::special_key_up_cb(int key, int x, int y) {
	ofNotifyKeyReleased(key | OF_KEY_MODIFIER);
}

//------------------------------------------------------------
void ofAppEGLWindow::resize_cb(int w, int h) {
	windowW = w;
	windowH = h;

	ofNotifyWindowResized(w, h);

	nFramesSinceWindowResized = 0;
}

void ofAppEGLWindow::entry_cb( int state ) {
	
	ofNotifyWindowEntry( state );
	
}