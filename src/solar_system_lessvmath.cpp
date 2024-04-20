
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

void renderSolidSphere(float radius, int slices, int stacks);
void renderSun();
void renderMercury();
void renderVenus();
void renderEarth();
void renderMars();
void renderJupiter();
void renderSaturn();
void renderUranus();
void renderNeptune();
void renderSolarSystem(); 
void renderText(char const * text, float position[2]);

float lastTime      = 0.f;
float elapsedTime   = 0.f;
float deltaTime     = 0.f;
float lastFrameTime = 0.f;
float fps     = 0.f;
long  nFrames = 0;

int windowWidth   = 0;
int windowHeight  = 0;
float camTheta  = 0.f;
float camPhi    = 25;
float camDist   = 8;
float camPan[3] = {};
float camScale  = 1.f/5.f;
int mouseX = 0;
int mouseY = 0;
bool buttonState[8]  = {};
bool switchAnimation = true;
bool switchLabels    = true;
bool switchTrails    = true;
bool switchRotate    = true;
bool switchHelp      = false;

float orbitDurationMercury = 88.f;
float orbitDurationVenus = 225.f;
float orbitDurationEarth = 365.f;
float orbitDurationMars  = 687.f;
float orbitDurationJupiter = 4333.f;
float orbitDurationSaturn = 10759.f;
float orbitDurationUranus = 30687.f;
float orbitDurationNeptune = 60190.f;

float orbitOffsetMercury = 0.0f;
float orbitOffsetVenus = 0.5f;
float orbitOffsetEarth = 0.7f;
float orbitOffsetMars  = 0.3f;
float orbitOffsetJupiter = 0.4f;
float orbitOffsetSaturn = 0.1f;
float orbitOffsetUranus = 0.6f;
float orbitOffsetNeptune = 0.2f;

/***********************************************************/

float orbitDurationPerSec = orbitDurationEarth / 5.f;
float starDistance = 200.f;
int   numStars     = 1000;
std::vector<vm::vec3> starGens;

/***********************************************************/

int main(int argc, char *argv[])
{
    // generate random star coordinates to project onto sphere.  
    { 
        starGens.reserve(numStars);
        std::random_device rd; // obtain a random number from hardware
        std::mt19937 gen(rd()); // seed the generator
        std::uniform_real_distribution<float> distR(.2f, .3f);
        std::uniform_real_distribution<float> distZ(0.f, 360.f);
        std::uniform_real_distribution<float> distY(0.f, 360.f);
        for(int i = 0; i < numStars; i++)
        {
            float radius = distR(gen);
            float y = distY(gen);
            float z = distZ(gen);
            starGens.push_back({radius, y, z});
        }
    }

    glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("Solar System");
    
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

    // initialize light0 parameters
    {
        float r = 1.f;
        float g = .95f;
        float b = .8f;
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

void renderSphere(char const * label, float radius, float distance, float color[4], float tiltAngle = 0.f
                , float orbitDuration = 0.f, float orbitOffset = 0.f)
{
    float orbitAngle = 360.f*(orbitOffset+elapsedTime*orbitDurationPerSec/orbitDuration);
    glColor4fv(color);
    glPushMatrix();
    glRotatef(tiltAngle, 0.f, 0.f, 1.f);
    if(orbitDuration != 0.f)
        glRotatef(orbitAngle, 0.f, 1.f, 0.f);
    glTranslatef(distance, 0.0f, 0.0f);
    glScalef(radius,radius,radius);
    static v3d::SolidSphere sphere(1.f, 28, 24);
    sphere.render();
    if(switchLabels)
    {
        float textPosition[] = {radius, radius};
        renderText(label, textPosition);
    }
    glPopMatrix();
}

void renderRingedSphere(char const * label, float radius, float distance, float color[4]
                      , float ringRadius = 0.f, float ringSize = 0.f, float ringColor[4] = {}
                      , float tiltAngle = 0.f, float orbitDuration = 0.f, float orbitOffset = 0.f)
{
    float orbitAngle = 360.f*(orbitOffset+elapsedTime*orbitDurationPerSec/orbitDuration);
    glColor4fv(color);
    glPushMatrix();
    glRotatef(tiltAngle, 0.f, 0.f, 1.f);
    if(orbitDuration != 0.f)
        glRotatef(orbitAngle, 0.f, 1.f, 0.f);
    glTranslatef(distance, 0.0f, 0.0f);
    glScalef(radius,radius,radius);
    static v3d::SolidSphere sphere(1.f, 28, 24);
    sphere.render();
    if(ringSize > 0.f)
    {
        glColor4fv(ringColor);
        glPushMatrix();
        glRotatef(90.f, 1.f, 0.f, 0.f);
        glutSolidTorus(ringSize/radius, ringRadius/radius, 2, 45);
        glPopMatrix();
    }
    if(switchLabels)
    {
        float textPosition[] = {radius, radius};
        renderText(label, textPosition);
    }
    glPopMatrix();
}

void renderStar(float radius, float yzProjections[2], float color[4]) {
    glColor4fv(color);
    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
    glPushMatrix();
    glRotatef(yzProjections[0], 0.f, 1.f, 0.f);
    glRotatef(yzProjections[1], 0.f, 0.f, 1.f);
    glTranslatef(starDistance, 0.f, 0.f);
    static v3d::SolidSphere sphere(radius, 4, 4);
    sphere.render();
    glPopMatrix();
}

void renderBackground() 
{
    for(size_t i = 0; i < starGens.size(); i++) 
    {
        vm::vec3 const & starGen = starGens[i];
        float color[4] = {1.f,1.f,1.f,1.f};
        float yzProjections[2] = { starGen.y, starGen.z };
        renderStar(starGen.x, yzProjections, color);
    }
}

void renderSun() 
{
    float emission[] = {0.99f, 0.5f, 0.0f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    renderSphere("Sun", .8f, 0.f, emission, 0, 0);
}

void renderMercury() 
{
    float emission[] = {0.f, 0.f, 0.f, 1.f};
    float ambient[] = {0.f, 0.f, 0.f, 1.f};
    float specular[] = {0.48f, 0.25f, 0.09f, 1.f};
    float diffuse[] = {0.48f, 0.25f, 0.09f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderSphere("Mercury", .38f, 3.f, diffuse, -10, orbitDurationMercury, orbitOffsetMercury);
}

void renderVenus() 
{
    float emission[] = {0.f, 0.f, 0.f, 1.f};
    float ambient[] = {0.f, 0.f, 0.f, 1.f};
    float specular[] = {.84f, 0.67f, 0.55f, 1.f};
    float diffuse[]  = {.84f, 0.67f, 0.55f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderSphere("Venus", .4f, 5.f, diffuse, 30, orbitDurationVenus, orbitOffsetVenus);
}

void renderEarth() 
{
    float emission[] = {0.f, 0.f, 0.f, 1.f};
    float ambient[] = {0.f, 0.f, 0.f, 1.f};
    float specular[] = {.19f, 0.78f, 0.95f, 1.f};
    float diffuse[]  = {.19f, 0.78f, 0.95f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderSphere("Earth", .45f, 7.f, diffuse, 0, orbitDurationEarth, orbitOffsetEarth);
}

void renderMars() 
{
    float emission[] = {0.f, 0.f, 0.f, 1.f};
    float ambient[] = {0.f, 0.f, 0.f, 1.f};
    float specular[] = {.83f, 0.24f, 0.16f, 1.f};
    float diffuse[]  = {.83f, 0.24f, 0.16f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderSphere("Mars", .4f, 9.f, diffuse, 20, orbitDurationMars, orbitOffsetMars);
}

void renderJupiter() 
{
    float emission[] = {0.f, 0.f, 0.f, 1.f};
    float ambient[]  = {0.f, 0.f, 0.f, 1.f};
    float specular[] = {.6f, 0.26f, 0.12f, 1.f};
    float diffuse[]  = {.6f, 0.26f, 0.12f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderSphere("Jupiter", .8f, 11.f, diffuse, -20, orbitDurationJupiter, orbitOffsetJupiter);
}

void renderSaturn()  
{
    float emission[]  = {0.f, 0.f, 0.f, 1.f};
    float ambient[]   = {0.f, 0.f, 0.f, 1.f};
    float specular[]  = {.96f, 0.95f, 0.70f, 1.f};
    float diffuse[]   = {.96f, 0.95f, 0.70f, 1.f};
    float ringColor[] = {.97f, 0.88f, 0.81f, .5f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderRingedSphere("Saturn", .6f, 14.f, diffuse, .9f, .14f, ringColor, 15, orbitDurationSaturn, orbitOffsetSaturn);
}

void renderUranus() 
{
    float emission[]  = {0.f, 0.f, 0.f, 1.f};
    float ambient[]   = {0.f, 0.f, 0.f, 1.f};
    float specular[]  = {.46f, 0.82f, 0.70f, 1.f};
    float diffuse[]   = {.46f, 0.82f, 0.70f, 1.f};
    float ringColor[] = {.97f, 0.88f, 0.81f, .5f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderRingedSphere("Uranus", .4f, 17.f, diffuse, .7f, .05f, ringColor, 20, orbitDurationUranus, orbitOffsetUranus);
}

void renderNeptune()  
{
    float emission[] = {0.f, 0.f, 0.f, 1.f};
    float ambient[]  = {0.f, 0.f, 0.f, 1.f};
    float specular[] = {.0f, 0.65f, 0.88f, 1.f};
    float diffuse[]  = {.0f, 0.65f, 0.88f, 1.f};
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, specular);
    renderSphere("Neptune", .4f, 19.f, diffuse, 0, orbitDurationNeptune, orbitOffsetNeptune);
}

void renderSolarSystem() 
{
    glPushMatrix();
    renderSun();
    renderMercury();
    renderVenus();
    renderEarth();
    renderMars();
    renderJupiter();
    renderSaturn();
    renderUranus();
    renderNeptune();
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
	glTranslatef(0, 0, -camDist);
	glRotatef(camPhi, 1, 0, 0);
	glRotatef(camTheta, 0, 1, 0);
	glTranslatef(camPan[0], camPan[1], camPan[2]);
    glScalef(camScale, camScale, camScale);

	float lightPosition[] = {0.f, 0.f, 0.f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    
    glPushMatrix();
        renderBackground();
        renderSolarSystem();
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
