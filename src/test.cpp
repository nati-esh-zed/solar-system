
#include <cstdio>
#include <cstdlib>
#include <cmath> 
#include <ctime> 
#include <random> 
#include <vector>
#include <iostream>
#include <GL/glut.h>
#include <vmath>
#include <v3d>

static char const *helpPrompt[] = {"Press F1 for help", 0};
static char const *helpText[] = {
	"Rotate: left mouse drag",
	" Scale: right mouse drag up/down",
	"   Pan: middle mouse drag",
	"",
	"Toggle fullScreen: f",
	"Toggle switchLabels: l",
	"Toggle switchTrails: t",
	"Toggle switchRotate: r",
	"Toggle animation: space",
	"Quit: escape",
	0
};

void idle();
void display();
void reshape(int x, int y);
void keyPress(unsigned char key, int x, int y);
void sKeyPress(int key, int x, int y);
void mouse(int bn, int st, int x, int y);
void motion(int x, int y);
void printHelp();
void printFps();
void render();

void renderText(char const * text, float position[2]);

float lastTime      = 0.f;
float elapsedTime   = 0.f;
float deltaTime     = 0.f;
float lastFrameTime = 0.f;
float fps     = 0.f;
long  nFrames = 0;

unsigned int slices = 7, stacks = 6;

int windowWidth   = 0;
int windowHeight  = 0;
float camTheta  = 0.f;
float camPhi    = 25;
float camDist   = 8;
float camPan[3] = {};
float camScale  = 1.f/3.f;
int mouseX = 0;
int mouseY = 0;
bool buttonState[8]  = {};
bool switchAnimation = true;
bool switchLabels    = true;
bool switchTrails    = true;
bool switchRotate    = true;
bool switchHelp      = false;
bool switchWireFrame = false;
bool switchNormals   = false;

/***********************************************************/

/***********************************************************/

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("Test");
    
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPress);
	glutSpecialFunc(sKeyPress);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHT0);
	glShadeModel(GL_SMOOTH);

    // initialize light0 parameters
    {
        float r = 1.f;
        float g = 1.f;
        float b = 1.f;
        float ambientFactor = .1f;
        float specularFactor = .05f;
        float lightAmbient[] = {r*ambientFactor, g*ambientFactor,b*ambientFactor};
        float lightDiffuse[] = {r, g, b};
        float lightSpecular[] = {r*specularFactor, g*specularFactor,b*specularFactor};
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    }

    // for animation
    {
        lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
        glutIdleFunc(switchAnimation ? idle : 0);
    }

	glutMainLoop();
	return 0;
}

/////////////////////////////////////////////////

void render()
{
    glPushMatrix();
    float ambient[]   = {0.f, 0.f, 0.f, 1.f};
    float specular[]  = {1.f, 1.f, 1.f, 1.f};
    float diffuse[]   = {1.f, 1.f, 1.f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glColor4fv(diffuse);
	// glTranslatef(1.f, 0.f, 0.f);
    static v3d::SolidSphere sphere(.4f, slices, stacks);
	if(sphere.getSlices() != slices || sphere.getStacks() != stacks)
	{
		try { sphere = v3d::SolidSphere(.4f, slices, stacks); }
		catch(std::exception e) { std::cerr << e.what() << std::endl; }

	}
    sphere.render(switchWireFrame, switchNormals);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glColor4fv(diffuse);
    static v3d::SolidTorus torus(.4f, 3.f, slices, stacks);
	if(torus.getSlices() != slices || torus.getStacks() != stacks)
	{
		try { torus = v3d::SolidTorus(.4f, 3.f, slices, stacks); }
		catch(std::exception e) { std::cerr << e.what() << std::endl; }

	}
    torus.render(switchWireFrame, switchNormals);
    glPopMatrix();
}

void renderText(char const * text, float position[2])
{
	char const * ch;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glPushMatrix();
    glColor3f(0.1f, 0.1f, 0.1f);
    glRasterPos2f(position[0]+.01f, position[1]-.01f);
    ch = &text[0];
    while(*ch) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *ch++);
    }
    glColor3f(1.f, 0.f, 0.4f);
    glRasterPos2f(position[0], position[1]);
    ch = &text[0];
    while(*ch) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *ch++);
    }
	glPopMatrix();
	glPopAttrib();
}

void idle()
{
	glutPostRedisplay();
}

void display()
{
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    if(switchAnimation)
        elapsedTime += deltaTime;
    if(switchAnimation && switchRotate)
        camTheta += 3.f * deltaTime;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    vm::mat4 mCameraScale  = vm::scale(camScale, camScale, camScale);
    vm::mat4 mCameraPan    = vm::translate(camPan[0], camPan[1], camPan[2]);
    vm::mat4 mCameraOrbitY = vm::rotate_y(vm::deg2rad(camTheta));
    vm::mat4 mCameraTiltX  = vm::rotate_x(vm::deg2rad(camPhi));
    vm::mat4 mCameraZoom  = vm::translate(0.f, 0.f, -camDist);
    vm::mat4 mCamera = mCameraZoom * mCameraTiltX * mCameraOrbitY * mCameraPan * mCameraScale;
	float lightPosition[] = {-2.f, 2.f, 2.f,1.f};
    glLoadMatrixf(vm::transpose(mCamera).ptr());
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    
    glPushMatrix();
        render();
        printHelp();
        printFps();
    glPopMatrix();

	glutSwapBuffers();
    // calculate FPS
    {
        nFrames++;
        float frameTime = currentTime - lastFrameTime;
        if(frameTime > 1.f) {
            fps = float(nFrames) / frameTime;
            nFrames = 0;
            lastFrameTime = currentTime;
        }
    }
}

void reshape(int x, int y)
{
    float const znear = 0.1f;
	float const aspect = (float)x / (float)y;
	windowWidth = x;
	windowHeight = y;
    // vm::mat4 mProjection = vm::ortho<float>(-aspect, aspect, -1, 1, znear, 500.0);
    vm::mat4 mProjection = vm::fov<float>(50.f, aspect, znear, 500.0);
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(vm::transpose(mProjection).ptr());
	glMatrixMode(GL_MODELVIEW);
}

void mouse(int bn, int st, int x, int y)
{
	int bidx = bn - GLUT_LEFT_BUTTON;
	buttonState[bidx] = st == GLUT_DOWN;
	mouseX = x;
	mouseY = y;
}

void motion(int x, int y)
{
	int dx = x - mouseX;
	int dy = y - mouseY;
	mouseX = x;
	mouseY = y;

	if(!(dx | dy)) return;

	if(buttonState[0]) {
		camTheta += dx * 0.5f;
		camPhi += dy * 0.5f;
		if(camPhi < -90.f) 
            camPhi = -90.f;
		if(camPhi > 90.f) 
            camPhi = 90.f;
		glutPostRedisplay();
	}
	if(buttonState[1]) {
		float up[3], right[3];
		float theta = camTheta * vm::PI / 180.0f;
		float phi = camPhi * vm::PI / 180.0f;

		up[0] = -sin(theta) * sin(phi);
		up[1] = -cos(phi);
		up[2] = cos(theta) * sin(phi);
		right[0] = cos(theta);
		right[1] = 0;
		right[2] = sin(theta);

		camPan[0] += (right[0] * dx + up[0] * dy) * 0.01f;
		camPan[1] += up[1] * dy * 0.01f;
		camPan[2] += (right[2] * dx + up[2] * dy) * 0.01f;
		glutPostRedisplay();
	}
	if(buttonState[2]) {
		camDist += dy * 0.1f;
		if(camDist < 0) camDist = 0;
		glutPostRedisplay();
	}
}

static int switchSlicesStacks = 0;
void keyPress(unsigned char key, int x, int y)
{
	static int fullScreen;
	static int prev_xsz, prev_ysz;
	
	switch(key) {
	case 27:
	case 'q':
		exit(0);
		break;
	case 'l':
		switchLabels ^= 1;
		break;
	case 't':
		switchTrails ^= 1;
		break;
	case 'r':
		switchRotate ^= 1;
		break;
	case 's':
		switchSlicesStacks ^= 1;
		printf("mode: %s\n", switchSlicesStacks==0 ? "slices" : "stacks");
		break;
	case 'n':
		switchNormals ^= 1;
		break;
	case 'w':
		switchWireFrame ^= 1;
		break;
	case ' ':
		switchAnimation ^= 1;
		glutIdleFunc(switchAnimation ? idle : 0);
		glutPostRedisplay();
		if(switchAnimation)
        {
            lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
            lastFrameTime = lastTime;
            nFrames = 0;
        }
		break;
	case '\n':
	case '\r':
		if(!(glutGetModifiers() & GLUT_ACTIVE_ALT)) {
			break;
		}
	case 'f':
		fullScreen ^= 1;
		if(fullScreen) {
			prev_xsz = glutGet(GLUT_WINDOW_WIDTH);
			prev_ysz = glutGet(GLUT_WINDOW_HEIGHT);
			glutFullScreen();
		} else {
			glutReshapeWindow(prev_xsz, prev_ysz);
		}
		break;
	}
}

void sKeyPress(int key, int x, int y)
{
	switch(key) {
	case GLUT_KEY_F1:
		switchHelp ^= 1;
		glutPostRedisplay();
	case GLUT_KEY_UP:
		if(switchSlicesStacks == 0)
			slices = vm::max(slices+1,3u);
		else
			stacks = vm::max(stacks+1,2u);
		printf("slices: %3u\t stacks: %3u\n", slices, stacks);
		break;
	case GLUT_KEY_DOWN:
		if(switchSlicesStacks == 0)
			slices = vm::max(slices-1,3u);
		else
			stacks = vm::max(stacks-1,2u);
		printf("slices: %3u\t stacks: %3u\n", slices, stacks);
		break;
	default:
		break;
	}
}

void printHelp()
{
	int i;
	char const *s, **text;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, windowWidth, 0, windowHeight, -1, 1);

	text = switchHelp ? helpText : helpPrompt;

	for(i=0; text[i]; i++) {
		glColor3f(0.f, 0.1f, 0.f);
		glRasterPos2f(7.f, float(windowHeight - (i + 1) * 20 - 2));
		s = text[i];
		while(*s) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		}
		glColor3f(0.f, 0.9f, 0.f);
		glRasterPos2f(5.f, float(windowHeight - (i + 1) * 20));
		s = text[i];
		while(*s) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		}
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}

void printFps()
{
    if(switchAnimation) {
	    char const *s, *text;
        char buffer[128] = {};
        snprintf(buffer, sizeof(buffer), "%4.0f FPS", fps);
        text = buffer;
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, windowWidth, 0, windowHeight, -1, 1);
        glColor3f(0.f, 0.1f, 0.f);
        glRasterPos2f(7.f, 20 );
        s = text;
        while(*s) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
        }
        glColor3f(0.9f, 0.9f, 0.f);
        glRasterPos2f(5.f, 20+2);
        s = text;
        while(*s) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
        }
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();
    }

}
