#include <stdio.h>
#include <GL/glut.h>
#include "chip8-CPU.h"

unsigned int draw_flag = 0;
unsigned char gfx_out[RESOLUTION_H * RESOLUTION_W];
int DISPLAY_H = RESOLUTION_H * RESOLUYION_SCALE;
int DISPLAY_W = RESOLUTION_W * RESOLUYION_SCALE;

void display();
void reshape_window(GLsizei w, GLsizei h);
 
uint8_t dispData[RESOLUTION_W][RESOLUTION_H][3]; 
void setupTexture();

int main(int argc, char *argv[]){
	// Initialise the chip8 system and load the game
	cpuInit();

	// Setup OpenGL
	glutInit(&argc, argv);          
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	
	glutInitWindowSize(DISPLAY_W, DISPLAY_H);
	glutInitWindowPosition(320, 320);
	glutCreateWindow("Chip-8 Emulator");
	
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape_window);
	
	setupTexture();
	
	glutMainLoop(); 

	return 0;
}

// Setup Texture
void setupTexture()
{
    // Clear screen
    for(int x = 0; x < RESOLUTION_W; x++)		
        for(int y = 0; y < RESOLUTION_H; y++)
            dispData[x][y][0] = dispData[x][y][1] = dispData[x][y][2] = 0;
 
    // Create a texture 
    glTexImage2D(GL_TEXTURE_2D, 0, 3, RESOLUTION_W, RESOLUTION_H, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)dispData);
 
    // Set up the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 
 
    // Enable textures
    glEnable(GL_TEXTURE_2D);
}

void updateTexture(unsigned char gfx_out[])
{	
    // Update pixels
	for(int x = 0; x < RESOLUTION_W; x++){	
	    for(int y = 0; y < RESOLUTION_H; y++){
            if(gfx_out[(y * 64) + x] == 0)
                dispData[x][y][0] = dispData[x][y][1] = dispData[x][y][2] = 0;	// Disabled
            else 
                dispData[x][y][0] = dispData[x][y][1] = dispData[x][y][2] = 255;  // Enabled
        }
    }
 
    // Update Texture
    glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, RESOLUTION_W, RESOLUTION_H, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)dispData);
 
    glBegin( GL_QUADS );
        glTexCoord2d(0.0, 0.0);		glVertex2d(0.0,			  0.0);
        glTexCoord2d(1.0, 0.0); 	glVertex2d(DISPLAY_W, 0.0);
        glTexCoord2d(1.0, 1.0); 	glVertex2d(DISPLAY_W, DISPLAY_H);
        glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0,			  DISPLAY_H);
    glEnd();
}

void display()
{
    cpuEmulateLoop();
 
    if(draw_flag)
    {
        // Clear framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
 
        // Draw pixels to texture
        updateTexture(gfx_out);
 
        // Swap buffers!
        glutSwapBuffers();    
 
        // Processed frame
        draw_flag = 0;
 	}
}

void reshape_window(GLsizei w, GLsizei h)
{
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
 
    // Resize quad
    DISPLAY_W = w;
    DISPLAY_H = h;
}