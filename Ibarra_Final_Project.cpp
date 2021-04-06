/*
 * Ibarra_Final_Project.cpp
 *
 *  Created on: Dec 1, 2020
 *      Author: Mitchell Ibarra
 *      CS 330 Final Project
 */
 
/* Header Inclusions */
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

// GLM Match Header Inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// SOIL Image loader inclusion
#include "SOIL2/SOIL2.h"

using namespace std; // Standard namespace

#define WINDOW_TITLE "Ibarra Final Project" // Window title macro

/* Shader program macro */
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

/* Variable declarations for shader, window size initialization, buffer, and array objects */
GLint shaderProgram, lampShaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VBO, VAO, LightVAO, texture;

//Light color, position, and scale
glm::vec3 lightColor(0.8f, 0.9f, 0.8f);
glm::vec3 lightPosition(-1.5f, 5.5f, -1.5f);
glm::vec3 lightScale(0.3f);

GLfloat cameraSpeed = 0.0005f; //Movement speed per frame

GLchar currentKey; // will store current key pressed

GLfloat lastMouseX = 400, lastMouseY = 300; // locks mouse cursor at the center of the screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f; // mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.005f; // used for mouse/ camera rotation sensitivity
bool mouseDetected = true; // initially true when mouse movement is detected
bool orbit = false; // left mouse button and alt button clicked initially set to false
bool is3D = false; // will determine whether the view is perspective or orthographic

//global vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f); // Initial camera position placed 5 units in Z
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f); // Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f); // Temporary z unit vector
glm::vec3 front; // temporary z unit vector for mouse

/* Function prototypes */
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UKeyboard(unsigned char key, int x, int y);
void UMouseMove(int x, int y);
void UMouseClick(int button, int state, int x, int y);
void UGenerateTexture(void);

/* Vertex shader source code*/
const GLchar * vertexShaderSource = GLSL(330,
		layout (location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
		layout (location = 1) in vec3 normal; // VAP position 1 for normals
		layout (location = 2) in vec2 textureCoordinates; //texture data from Vertex Attrib Pointer 2

		out vec3 FragmentPos; //For outgoing color / pixels to fragment shader
		out vec3 Normal; //For outgoing normals to fragment shader
		out vec2 mobileTextureCoordinate;

		// Global variables for the transform matrices
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
		FragmentPos = vec3(model * vec4(position, 1.0f)); //Gets fragment / pixel position in world space only (exclude view and projection)
		Normal = mat3(transpose(inverse(model))) *  normal; //get normal vectors in world space only and exclude normal translation properties
		mobileTextureCoordinate = vec2(textureCoordinates.x, 1.0f - textureCoordinates.y); // flips the texture horizontally
	});

/* Fragment shader source code */
const GLchar * fragmentShaderSource = GLSL(330,
		in vec3 FragmentPos; //For incoming fragment position
		in vec3 Normal; //For incoming normals
		in vec2 mobileTextureCoordinate;

		out vec4 tableColor; //For outgoing pyramid color to the GPU

		//Uniform / Global variables for object color, light color, light position, and camera/view position
		uniform vec3 lightColor;
		uniform vec3 lightPos;
		uniform vec3 viewPosition;

		uniform sampler2D uTexture;

	void main()
	{
		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		//Calculate Ambient Lighting
		float ambientStrength = 0.2f; //Set ambient or global lighting strength
		vec3 ambient = ambientStrength * lightColor; //Generate ambient light color


		//Calculate Diffuse Lighting
		vec3 norm = normalize(Normal); //Normalize vectors to 1 unit
		vec3 lightDirection = normalize(lightPos - FragmentPos); //Calculate distance (light direction) between light source and fragments/pixels on
		float impact = max(dot(norm, lightDirection), 0.0); //Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse = impact * lightColor; //Generate diffuse light color


		//Calculate Specular lighting
		float specularIntensity = 0.0f; //Set specular light strength
		float highlightSize = 16.0f; //Set specular highlight size
		vec3 viewDir = normalize(viewPosition - FragmentPos); //Calculate view direction
		vec3 reflectDir = reflect(-lightDirection, norm); //Calculate reflection vector
		//Calculate specular component
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;

		//Calculate phong result
		vec3 objectColor = texture(uTexture, mobileTextureCoordinate).xyz;
		vec3 phong = (ambient + diffuse) * objectColor + specular;
		tableColor = vec4(phong, 1.0f); //Send lighting results to GPU
	});

/*Lamp Shader Source Code*/
const GLchar * lampVertexShaderSource = GLSL(330,

        layout (location = 0) in vec3 position; //VAP position 0 for vertex position data

        //Uniform / Global variables for the transform matrices
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            gl_Position = projection * view *model * vec4(position, 1.0f); //Transforms vertices into clip coordinates
        }
);


/*Lamp fragment Shader Source Code*/
const GLchar * lampFragmentShaderSource = GLSL(330,

        out vec4 color; //For outgoing lamp color (smaller pyramid) to the GPU

        void main()
        {
            color = vec4(1.0f); //Set color to white (1.0f, 1.0f, 1.0f) with alpha 1.0

        }
);

/* Main program */
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);

	glutReshapeFunc(UResizeWindow);

	glewExperimental = GL_TRUE;
		if(glewInit() != GLEW_OK)
		{
			std::cout << "Failed to initialize GLEW" << std::endl;
			return -1;
		}

	UCreateShader();

	UCreateBuffers();

	UGenerateTexture();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color

	glutDisplayFunc(URenderGraphics);

	glutKeyboardFunc(UKeyboard);

	glutMouseFunc(UMouseClick);

	glutMotionFunc(UMouseMove);

	glutMainLoop();

	// Destroys buffer objects once used
	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &LightVAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}

/* Resizes the window */
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

/* Renders graphics */
void URenderGraphics(void)
{
	glEnable(GL_DEPTH_TEST); // Enable z-depth

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the screen

	glUseProgram(shaderProgram);
	glBindVertexArray(VAO); // Activate the Vertex Array Object before rendering and transforming them

	CameraForwardZ = front; // replaces camera forward vector with radians noramllized as a unit vector

	// Transforms the object
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // Place the object at the center of the viewport
	model = glm::rotate(model, 45.0f, glm::vec3(0.0, 1.0f, 0.0f)); // Rotate the object 15 degrees on the X axis
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f)); // Increase the object size by a scale of 2

	// Transforms the camera
	glm::mat4 view;
	view = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);

	// Creates a perspective projection
	glm::mat4 projection;

	if(is3D)
	{
		projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);
	}
	else
	{
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}

	// Retrieves and passes transform matrices to the Shader program
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Reference matrix uniforms from the pyramid Shader program for the pyramid color, light color, light position, and camera position
	GLint uTextureLoc = glGetUniformLocation(shaderProgram, "uTexture");
	GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");

	//Pass color, light, and camera data to the pyramid Shader programs corresponding uniforms
	glUniform1i(uTextureLoc, 0); // texture unit 0
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	glBindTexture(GL_TEXTURE_2D, texture);

	// draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, 256);

	glBindVertexArray(0); // deactivate the vertex array object

	glutPostRedisplay();
	glutSwapBuffers(); // flips the back buffer with the front buffer every frame
}

/* Creates the shader program*/
void UCreateShader()
{
	// Vertex Shader
	GLint vertexShader = glCreateShader(GL_VERTEX_SHADER); // creates the vertex shader
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // Attaches the vertex shader to the source code
	glCompileShader(vertexShader); // Compiles the vertex shader

	// Fragment shader
	GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // creates the fragment shader
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); // attaches the fragment shader to the source code
	glCompileShader(fragmentShader); // compiles the fragment shader

	// Shader program
	shaderProgram = glCreateProgram(); // Creates the shader program and returns an id
	glAttachShader(shaderProgram, vertexShader); // attaach vertex shader to the shader program
	glAttachShader(shaderProgram, fragmentShader); // attach fragment shader to the shader program
	glLinkProgram(shaderProgram); // link vertex and fragment shaders to Shader program

	// Delete the vertex and fragment shaders once linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	////////////////////////////////////////////////////////////////////

	//Lamp Vertex shader
	GLint lampVertexShader = glCreateShader(GL_VERTEX_SHADER); //Creates the Vertex shader
	glShaderSource(lampVertexShader, 1, &lampVertexShaderSource, NULL); //Attaches the Vertex shader to the source code
	glCompileShader(lampVertexShader); //Compiles the Vertex shader

	//Lamp Fragment shader
	GLint lampFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Creates the Fragment shader
	glShaderSource(lampFragmentShader, 1, &lampFragmentShaderSource, NULL); //Attaches the Fragment shader to the source code
	glCompileShader(lampFragmentShader); //Compiles the Fragment shader

	//Lamp Shader Program
	lampShaderProgram = glCreateProgram(); //Creates the Shader program and returns an id
	glAttachShader(lampShaderProgram, lampVertexShader); //Attach Vertex shader to the Shader program
	glAttachShader(lampShaderProgram, lampFragmentShader); //Attach Fragment shader to the Shader program
	glLinkProgram(lampShaderProgram); //Link Vertex and Fragment shaders to the Shader program

	//Delete the lamp shaders once linked
	glDeleteShader(lampVertexShader);
	glDeleteShader(lampFragmentShader);
}

/* Creates the buffer and array objects */
void UCreateBuffers()
{
	GLfloat vertices[] =
		{	//Position				 //Normals				//Texture Coordinates

			/* Top of the table */
			//positions	side		 //Negative z normals	//texture coordinates
		   -0.5f,  -0.5f, 	-0.5f, 	 0.0f,  0.0f, -1.0f,	0.0f, 0.0f, //bottom left
			2.0f,  -0.5f, 	-0.5f, 	 0.0f,  0.0f, -1.0f,	1.0f, 0.0f, //bottom right
			2.0f,  -0.4f,	-0.5f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f, //top right
			2.0f,  -0.4f, 	-0.5f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f, //top right
		   -0.5f,  -0.4f, 	-0.5f, 	 0.0f,  0.0f, -1.0f,	0.0f, 1.0f, //top left
		   -0.5f,  -0.5f, 	-0.5f, 	 0.0f,  0.0f, -1.0f,	0.0f, 0.0f, //bottom left

		   //positions side			 //negative z normals	// texture
		   -0.5f,  -0.5f,    0.5f, 	 0.0f,  0.0f,  -1.0f,	0.0f, 0.0f,
			2.0f,  -0.5f,    0.5f, 	 0.0f,  0.0f,  -1.0f,	1.0f, 0.0f,
			2.0f,  -0.4f,    0.5f, 	 0.0f,  0.0f,  -1.0f,   1.0f, 1.0f,
			2.0f,  -0.4f,    0.5f, 	 0.0f,  0.0f,  -1.0f,	1.0f, 1.0f,
		   -0.5f,  -0.4f,    0.5f, 	 0.0f,  0.0f,  -1.0f,	0.0f, 1.0f,
		   -0.5f,  -0.5f,    0.5f, 	 0.0f,  0.0f,  -1.0f,	0.0f, 0.0f,

		   //positions side			 //positive x normals	// color (blue)
		   -0.5f,  -0.4f,    0.5f, 	 1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		   -0.5f,  -0.4f,   -0.5f, 	 1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		   -0.5f,  -0.5f,   -0.5f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.5f,  -0.5f,   -0.5f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.5f,  -0.5f,    0.5f, 	 1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		   -0.5f,  -0.4f,    0.5f, 	 1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

		   //positions side			 //negative x normals	//color (yellow)
		    2.0f,  -0.4f,    0.5f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			2.0f,  -0.4f,   -0.5f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			2.0f,  -0.5f,   -0.5f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			2.0f,  -0.5f,   -0.5f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			2.0f,  -0.5f,    0.5f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
			2.0f,  -0.4f,    0.5f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //negative y normals	//color (light blue)
		   -0.5f,  -0.5f, 	-0.5f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			2.0f,  -0.5f, 	-0.5f, 	 0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			2.0f,  -0.5f,  	 0.5f, 	 0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			2.0f,  -0.5f,  	 0.5f, 	 0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		   -0.5f,  -0.5f,  	 0.5f, 	 0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
		   -0.5f,  -0.5f, 	-0.5f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,

		   //positions				 //positive y normals	//color (purple)
		   -0.5f,  -0.4f, 	-0.5f, 	 0.0f, 1.0f, 0.0f,		0.0f, 0.0f,
			2.0f,  -0.4f,	-0.5f, 	 0.0f, 1.0f, 0.0f,		1.0f, 0.0f,
			2.0f,  -0.4f,    0.5f, 	 0.0f, 1.0f, 0.0f,		1.0f, 1.0f,
			2.0f,  -0.4f,    0.5f, 	 0.0f, 1.0f, 0.0f,		1.0f, 1.0f,
		   -0.5f,  -0.4f,    0.5f, 	 0.0f, 1.0f, 0.0f,		0.0f, 1.0f,
		   -0.5f,  -0.4f, 	-0.5f, 	 0.0f, 1.0f, 0.0f,		0.0f, 0.0f,

		   //////////////////////////////////////////////////

		   /* Table leg 1 */
		   //positions				 //negative z normals	//color (white)
		    1.5f,  -1.5f, 	 0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
			1.5f,  -0.5f, 	 0.25f,  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
			1.5f,  -0.5f,  	 0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			1.5f,  -0.5f,  	 0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
		    1.5f,  -1.5f,  	 0.4f, 	 0.0f,  0.0f, -1.0f,    0.0f, 1.0f,
		    1.5f,  -1.5f, 	 0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,

			//positions				 //positive z normals	//color (yellow)
			1.55f, -1.5f, 	 0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,
			1.55f, -0.5f, 	 0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
			1.5f,  -0.5f,  	 0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			1.5f,  -0.5f,  	 0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			1.5f,  -1.5f,  	 0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 1.0f,
			1.55f, -1.5f, 	 0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,

			//positions				 //negative x normals	//color (red)
			1.55f, -1.5f, 	 0.35f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			1.55f, -0.5f, 	 0.25f,  -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			1.55f,  -0.5f,   0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.55f,  -0.5f,   0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.55f,  -1.5f,   0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
			1.55f, -1.5f, 	 0.35f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //positive x normals	//color (purple)
			1.55f,  -1.5f,   0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			1.55f,  -0.5f,   0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			1.5f,  -0.5f,  	 0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.5f,  -0.5f,  	 0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.5f,  -1.5f,  	 0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
			1.55f,  -1.5f,   0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //negative y normals	//color (blue)
			1.55f,  -1.5f,   0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			1.55f, -1.5f, 	 0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			1.5f,  -1.5f, 	 0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.5f,  -1.5f, 	 0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.5f,  -1.5f,  	 0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
			1.55f,  -1.5f,   0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			// 66 total vertices by this point

			//////////////////////////////////////////////////

		    /* Table leg 2 */
			//positions				 //negative z normals	//color (white)
			1.5f,  -1.5f, 	-0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
			1.5f,  -0.5f, 	-0.25f,  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
			1.5f,  -0.5f,  	-0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			1.5f,  -0.5f,  	-0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			1.5f,  -1.5f,  	-0.4f, 	 0.0f,  0.0f, -1.0f,	0.0f, 1.0f,
			1.5f,  -1.5f, 	-0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,

			//positions				 //positive z normals	//color (yellow)
			1.55f, -1.5f, 	-0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,
			1.55f, -0.5f, 	-0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
			1.5f,  -0.5f,  	-0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			1.5f,  -0.5f,  	-0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			1.5f,  -1.5f,  	-0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 1.0f,
			1.55f, -1.5f, 	-0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,

			//positions				 //negative x normals	//color (red)
			1.55f, -1.5f, 	-0.35f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			1.55f, -0.5f, 	-0.25f,  -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			1.55f,  -0.5f,  -0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.55f,  -0.5f,  -0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.55f,  -1.5f,  -0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
			1.55f, -1.5f, 	-0.35f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //positive x normals	//color (purple)
			1.55f,  -1.5f,  -0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			1.55f,  -0.5f,  -0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			1.5f,  -0.5f,  	-0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.5f,  -0.5f,  	-0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.5f,  -1.5f,  	-0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
			1.55f,  -1.5f,  -0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //negative y normals	//color (blue)
			1.55f,  -1.5f,  -0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			1.55f, -1.5f, 	-0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			1.5f,  -1.5f, 	-0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.5f,  -1.5f, 	-0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.5f,  -1.5f,  	-0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
			1.55f,  -1.5f,  -0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			// 96 total vertices by this point

			//////////////////////////////////////////////////

			/* Table leg 3 */
		   //positions				 //negative z normals	//color (white)
			0.0f,  -1.5f, 	 0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
			0.0f,  -0.5f, 	 0.25f,  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
			0.0f,  -0.5f,  	 0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			0.0f,  -0.5f,  	 0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			0.0f,  -1.5f,  	 0.4f, 	 0.0f,  0.0f, -1.0f,	0.0f, 1.0f,
			0.0f,  -1.5f, 	 0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,

			//positions				 //positive z normals	//color (yellow)
		   -0.05f, -1.5f, 	 0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,
		   -0.05f, -0.5f, 	 0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
			0.0f,  -0.5f,  	 0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			0.0f,  -0.5f,  	 0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			0.0f,  -1.5f,  	 0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 1.0f,
		   -0.05f, -1.5f, 	 0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,

			//positions				 //positive x normals	//color (red)
		   -0.05f, -1.5f, 	 0.35f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		   -0.05f, -0.5f, 	 0.25f,  1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		   -0.05f,  -0.5f,   0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.05f,  -0.5f,   0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.05f,  -1.5f,   0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		   -0.05f, -1.5f, 	 0.35f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //negative x normals	//color (purple)
		   -0.05f,  -1.5f,   0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		   -0.05f,  -0.5f,   0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			0.0f,  -0.5f,  	 0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			0.0f,  -0.5f,  	 0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			0.0f,  -1.5f,  	 0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		   -0.05f,  -1.5f,   0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //negative y normals	//color (blue)
		   -0.05f,  -1.5f,   0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
		   -0.05f, -1.5f, 	 0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			0.0f,  -1.5f, 	 0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			0.0f,  -1.5f, 	 0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			0.0f,  -1.5f,  	 0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
		   -0.05f,  -1.5f,   0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			// 126 total vertices by this point

			//////////////////////////////////////////////////

		   /* Table leg 4 */
		   //positions				 //negative z normals	//color (white)
			0.0f,  -1.5f, 	-0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
			0.0f,  -0.5f, 	-0.25f,  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
			0.0f,  -0.5f,  	-0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			0.0f,  -0.5f,  	-0.3f, 	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			0.0f,  -1.5f,  	-0.4f, 	 0.0f,  0.0f, -1.0f,	0.0f, 1.0f,
			0.0f,  -1.5f, 	-0.35f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,

			//positions				 //positive z normals	//color (yellow)
		   -0.05f, -1.5f, 	-0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,
		   -0.05f, -0.5f, 	-0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
			0.0f,  -0.5f,  	-0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			0.0f,  -0.5f,  	-0.25f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			0.0f,  -1.5f,  	-0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 1.0f,
		   -0.05f, -1.5f, 	-0.35f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,

			//positions				 //positive x normals	//color (red)
		   -0.05f, -1.5f, 	-0.35f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		   -0.05f, -0.5f, 	-0.25f,  1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		   -0.05f,  -0.5f,  -0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.05f,  -0.5f,  -0.3f, 	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.05f,  -1.5f,  -0.4f, 	 1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		   -0.05f, -1.5f, 	-0.35f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //negative x normals	//color (purple)
		   -0.05f,  -1.5f,  -0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		   -0.05f,  -0.5f,  -0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			0.0f,  -0.5f,  	-0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			0.0f,  -0.5f,  	-0.3f, 	 -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			0.0f,  -1.5f,  	-0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		   -0.05f,  -1.5f,  -0.4f, 	 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions				 //negative y normals	//color (blue)
		   -0.05f,  -1.5f,  -0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
		   -0.05f, -1.5f, 	-0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			0.0f,  -1.5f, 	-0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			0.0f,  -1.5f, 	-0.35f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			0.0f,  -1.5f,  	-0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
		   -0.05f,  -1.5f,  -0.4f, 	 0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			// 156 total vertices by this point

		   //////////////////////////////////////////////////

		   /* Edge crossbar 1 */
		   //positions					 //negative z normals	//color (white)
			1.525f,  -1.05f, 	 0.37f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
			1.525f,  -0.95f, 	 0.36f,  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
			1.525f,  -0.95f,  	-0.36f,  0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			1.525f,  -0.95f,  	-0.36f,  0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
			1.525f,  -1.05f,  	-0.37f,  0.0f,  0.0f, -1.0f,	0.0f, 1.0f,
			1.525f,  -1.05f, 	 0.37f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,

			//positions					 //positive z normals	//color (teal)
			1.525f,  -0.95f, 	 0.36f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,
			1.575f,  -0.95f, 	 0.36f,  0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
			1.575f,  -0.95f,  	-0.36f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			1.575f,  -0.95f,  	-0.36f,  0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
			1.525f,  -0.95f,  	-0.36f,  0.0f,  0.0f,  1.0f,	0.0f, 1.0f,
			1.525f,  -0.95f, 	 0.36f,  0.0f,  0.0f,  1.0f,	0.0f, 0.0f,

			//positions					 //negative x normals 	//color (blue)
			1.575f,  -1.05f, 	-0.37f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			1.575f,  -0.95f, 	-0.36f,  -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			1.575f,  -0.95f,  	 0.36f,  -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.575f,  -0.95f,  	 0.36f,  -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.575f,  -1.05f,  	 0.37f,  -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
			1.575f,  -1.05f, 	-0.37f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions					 //positive x normals	//color (red)
			1.525f,  -1.05f, 	-0.37f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			1.575f,  -1.05f, 	-0.37f,  1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
			1.575f,  -1.05f,  	 0.37f,  1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.575f,  -1.05f,  	 0.37f,  1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
			1.525f,  -1.05f,  	 0.37f,  1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
			1.525f,  -1.05f, 	-0.37f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions					 //negative y normals	//color (yellow)
			1.575f,  -1.05f, 	 0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			1.575f,  -0.95f, 	 0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			1.525f,  -0.95f,  	 0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.525f,  -0.95f,  	 0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.525f,  -1.05f,  	 0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
			1.575f,  -1.05f, 	 0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,

			//positions					 //negative y normals	//color (yellow)
			1.575f,  -1.05f, 	-0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			1.575f,  -0.95f, 	-0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			1.525f,  -0.95f,  	-0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.525f,  -0.95f,  	-0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
			1.525f,  -1.05f,  	-0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
			1.575f,  -1.05f, 	-0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			// 192 vertices by this point

			//////////////////////////////////////////////////

		   /* Edge crossbar 2 */
		   //positions					 //negative z normals	//color (white)
		   -0.025f,  -1.05f, 	 0.37f,  0.0f,  0.0f, -1.0f,		0.0f, 0.0f,
		   -0.025f,  -0.95f, 	 0.36f,  0.0f,  0.0f, -1.0f,		1.0f, 0.0f,
		   -0.025f,  -0.95f,  	-0.36f,  0.0f,  0.0f, -1.0f,		1.0f, 1.0f,
		   -0.025f,  -0.95f,  	-0.36f,  0.0f,  0.0f, -1.0f,		1.0f, 1.0f,
		   -0.025f,  -1.05f,  	-0.37f,  0.0f,  0.0f, -1.0f,		0.0f, 1.0f,
		   -0.025f,  -1.05f, 	 0.37f,  0.0f,  0.0f, -1.0f,		0.0f, 0.0f,

			//positions					 //negative z normals	//color (teal)
		   -0.025f,  -0.95f, 	 0.36f,  0.0f,  0.0f,  -1.0f, 	0.0f, 0.0f,
		   -0.075f,  -0.95f, 	 0.36f,  0.0f,  0.0f,  -1.0f,	1.0f, 0.0f,
		   -0.075f,  -0.95f,  	-0.36f,  0.0f,  0.0f,  -1.0f,	1.0f, 1.0f,
		   -0.075f,  -0.95f,  	-0.36f,  0.0f,  0.0f,  -1.0f,	1.0f, 1.0f,
		   -0.025f,  -0.95f,  	-0.36f,  0.0f,  0.0f,  -1.0f,	0.0f, 1.0f,
		   -0.025f,  -0.95f, 	 0.36f,  0.0f,  0.0f,  -1.0f,	0.0f, 0.0f,

			//positions					 //positive x normals	//color (blue)
		   -0.075f,  -1.05f, 	-0.37f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		   -0.075f,  -0.95f, 	-0.36f,  1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		   -0.075f,  -0.95f,  	 0.36f,  1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.075f,  -0.95f,  	 0.36f,  1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.075f,  -1.05f,  	 0.37f,  1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		   -0.075f,  -1.05f, 	-0.37f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions					 //negative x normals	//color (red)
		   -0.025f,  -1.05f, 	-0.37f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		   -0.075f,  -1.05f, 	-0.37f,  -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		   -0.075f,  -1.05f,  	 0.37f,  -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.075f,  -1.05f,  	 0.37f,  -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.025f,  -1.05f,  	 0.37f,  -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		   -0.025f,  -1.05f, 	-0.37f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

			//positions					 //negative y normals	//color (yellow)
		   -0.075f,  -1.05f, 	 0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
		   -0.075f,  -0.95f, 	 0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		   -0.025f,  -0.95f,  	 0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		   -0.025f,  -0.95f,  	 0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		   -0.025,  -1.05f,  	 0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
		   -0.075f,  -1.05f, 	 0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,

			//positions					 //negative y normals	//color (yellow)
		   -0.075f,  -1.05f, 	-0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
		   -0.075f,  -0.95f, 	-0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		   -0.025f,  -0.95f,  	-0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		   -0.025f,  -0.95f,  	-0.36f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		   -0.025f,  -1.05f,  	-0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
		   -0.075f,  -1.05f, 	-0.37f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			// 232 vertices by this point

		   //////////////////////////////////////////////////

		   /* Middle crossbar */
		   //positions					 //negative z normals	//color (yellow)
		    1.525f,  -1.05f, 	 0.02f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
			1.525f,  -0.95f, 	 0.02f,  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
		   -0.025f,  -0.95f, 	 0.02f,  0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
		   -0.025f,  -0.95f, 	 0.02f,  0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
		   -0.025f,  -1.05f, 	 0.02f,  0.0f,  0.0f, -1.0f,	0.0f, 1.0f,
			1.525f,  -1.05f, 	 0.02f,  0.0f,  0.0f, -1.0f,	0.0f, 0.0f,

			//positions					 //positive z normals	//color (green)
			1.525f,  -1.05f, 	-0.02f,  0.0f,  0.0f,  -1.0f,	0.0f, 0.0f,
			1.525f,  -0.95f, 	-0.02f,  0.0f,  0.0f,  -1.0f,	1.0f, 0.0f,
		   -0.025f,  -0.95f, 	-0.02f,  0.0f,  0.0f,  -1.0f,	1.0f, 1.0f,
		   -0.025f,  -0.95f, 	-0.02f,  0.0f,  0.0f,  -1.0f,	1.0f, 1.0f,
		   -0.025f,  -1.05f, 	-0.02f,  0.0f,  0.0f,  -1.0f,	0.0f, 1.0f,
			1.525f,  -1.05f, 	-0.02f,  0.0f,  0.0f,  -1.0f,	0.0f, 0.0f,

			//positions					 //negative x normals	//color (teal)
			1.525f,  -0.95f, 	 0.02f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
			1.525f,  -0.95f, 	-0.02f,  -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		   -0.025f,  -0.95f, 	-0.02f,  -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.025f,  -0.95f, 	-0.02f,  -1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		   -0.025f,  -0.95f, 	 0.02f,  -1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		    1.525f,  -0.95f, 	 0.02f,  -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,

		   //positions					 //negative y normals	//color (purple)
			1.525f,  -1.05f, 	 0.02f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			1.525f,  -1.05f, 	-0.02f,  0.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		   -0.025f,  -1.05f, 	-0.02f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		   -0.025f,  -1.05f, 	-0.02f,  0.0f, -1.0f, 0.0f,		1.0f, 1.0f,
		   -0.025f,  -1.05f, 	 0.02f,  0.0f, -1.0f, 0.0f,		0.0f, 1.0f,
		    1.525f,  -1.05f, 	 0.02f,  0.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			// 256 vertices by this point
		};

	// Generate buffer IDs
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Activate the vertex array object before binding and setting any VBOs and Vertex Attribute Pointers
	glBindVertexArray(VAO);

	// Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // copy vertices to VBO

	// set attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0); // enables vertex attribute

	// Set attribute pointer 1 to hold Normal data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1); // enables vertex attribute

	// Set attribute pointer 2 to hold texture coordinate data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); // deactivates the VAO which is good practice

	/////////////////////////////////////////////////////////////////////////////////////////

	//Generate buffer ids for lamp (smaller pyramid)
	glGenVertexArrays(1, &LightVAO); //Vertex Array for pyramid vertex copies to serve as light source

	//Activate the Vertex Array Object before binding and setting any VBOs and Vertex Attribute Pointers
	glBindVertexArray(LightVAO);

	//Referencing the same VBO for its vertices
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Set attribute pointer to 0 to hold Position data (used for the lamp)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

/* Genereate and load the texture*/
void UGenerateTexture(){
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int width, height;

	unsigned char* image = SOIL_load_image("wood015.jpg", &width, &height, 0, SOIL_LOAD_RGBA); // loads texture file

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); // unbind the texture
}

/* Implements the UKeyboard function*/
void UKeyboard(unsigned char key, GLint x, GLint y)
{
	// reacts to spacebar key press to change perspective
	if(key == 32)
	{
		if(is3D)
		{
			is3D = false;
		}
		else
		{
			is3D = true;
		}
	}
}

/* Implements the UMouseClick function*/
void UMouseClick(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		orbit = true;
	}
	else
	{
		orbit = false;
	}
}

/* Implements the UMouseMove function*/
void UMouseMove(int x, int y)
{
	// immediately replaces center locked coordinates with new mouse coordinates
	if(mouseDetected)
	{
		lastMouseX = x;
		lastMouseY = y;
		mouseDetected = false;
	}

	if(orbit && 4 == glutGetModifiers())
	{
		// gets the direction of the mouse was moved in x and y
		mouseXOffset = x - lastMouseX;
		mouseYOffset = lastMouseY - y; // inverted y

		// updates with new mouse coordinates
		lastMouseX = x;
		lastMouseY = y;

		// applies sensistivity to mouse direction
		mouseXOffset *= sensitivity;
		mouseYOffset *= sensitivity;

		// accumulates the yaw and pitch variables
		yaw += mouseXOffset;
		pitch += mouseYOffset;

		// maintsins a 90 degree pitch for gimbal lock
		if(pitch > 89.0f)
			pitch = 89.0f;

		if(pitch < -89.0f)
			pitch = -89.0f;

		// converts mouse coordinates / degrees into radians then to vectors
		front.x = 10.0f * cos(yaw);
		front.y = 10.0f * sin(pitch);
		front.z = sin(yaw) * cos(pitch) * 10.0f;
	}
}
