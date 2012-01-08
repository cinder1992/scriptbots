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
    
    
    printf(" p= pause\n m= toggle drawing (for faster computation)\n f= draw food too\n += faster, -= slower\n e= spawn new agent\n h= spawn new herbivore\n g= spawn new carnivore\n delete= delete selected agent\n tab= reset all agents\n");
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
    glutMouseFunc(gl_processMouse);
	glutMotionFunc(gl_processMouseActiveMotion);
	glutPassiveMotionFunc(gl_processMousePassiveMotion);

	glutCreateMenu(gl_menu); //(GPA)
	glutAddMenuEntry("Fast Mode", 'm');
	glutAddMenuEntry("Pause", 'p');
	glutAddMenuEntry("Toggle Closed World", 'c');
	glutAddMenuEntry("Follow", 'l');
	glutAddMenuEntry("Follow Oldest", 'o');
//	glutAddMenuEntry("Save Agent", 'v');
//	glutAddMenuEntry("Load Agent", 'l');
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("New Agent", 'e');
	glutAddMenuEntry("New Agent (H)", 'g');
	glutAddMenuEntry("New Agent (C)", 'h');
	glutAddMenuEntry("Delete Agent", 127);
//	glutAddMenuEntry("Save World",200);
//	glutAddMenuEntry("Load World",201);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Exit", 27);
	glutAddMenuEntry("Reset Agents", 9);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();
    return 0;
}
