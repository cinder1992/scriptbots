#include "GLView.h"
#include "World.h"

#include <ctime>

#include "config.h"
#ifdef LOCAL_GLUT32
    #include "glut.h"
#else
    #include <GL/glut.h>
#endif

#include <stdio.h>


GLView* GLVIEW = new GLView(0);
int main(int argc, char **argv) {
    srand(time(0));
    if (conf::WIDTH%conf::CZ!=0 || conf::HEIGHT%conf::CZ!=0) printf("CAREFUL! The cell size variable conf::CZ should divide evenly into  both conf::WIDTH and conf::HEIGHT! It doesn't right now!");
    
    
    printf(" p= pause\n m= toggle drawing (for faster computation)\n f= draw food too\n += faster, -= slower\n b= spawn new bot\n h= spawn new herbivore\n n= spawn new carnivore\n delete= delete selected agent\n tab= reset all agents\n");
    printf("Pan around by moving the mouse near a window edge, and zoom by holding down middle button. Right-click on the world for more options\n");
    
    World* world = new World();
    GLVIEW->setWorld(world);

    //GLUT SETUP
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(30,30);
    glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);
    glutCreateWindow("Scriptbots");
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glutDisplayFunc(gl_renderScene);
    glutIdleFunc(gl_handleIdle);
    glutReshapeFunc(gl_changeSize);

    glutKeyboardFunc(gl_processNormalKeys);
	glutSpecialFunc(gl_processSpecialKeys);
	glutKeyboardUpFunc(gl_processReleasedKeys);
    glutMouseFunc(gl_processMouse);
	glutMotionFunc(gl_processMouseActiveMotion);
	glutPassiveMotionFunc(gl_processMousePassiveMotion);

	//create right click context menu
	GLVIEW->glCreateMenu();

    glutMainLoop();
    return 0;
}
