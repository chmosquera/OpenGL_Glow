/* Lab 6 base code - transforms using local matrix functions
	to be written by students -
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog, prog_blur, prog_final;

	// Shape to be used (from obj file)
	shared_ptr<Shape> shape;
	
	//camera
	camera mycam;

	//texture for sim
	GLuint TextureEarth;
	GLuint TextureMoon,TexColor, TexAlpha, BlurredTexAlpha , fb, fb2, depth_rb, depth_rb2;

	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;
	
	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{


		GLSL::checkVersion();

		
		// Set background color.
		//glClearColor(0.12f, 0.34f, 0.56f, 1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		//culling:
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		//transparency
		glEnable(GL_BLEND);
		//next function defines how to mix the background color with the transparent pixel in the foreground. 
		//This is the standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		if (! prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");


		prog_blur = make_shared<Program>();
		prog_blur->setVerbose(true);
		prog_blur->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/blur_frag.glsl");
		if (!prog_blur->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_blur->init();
		prog_blur->addUniform("P");
		prog_blur->addUniform("V");
		prog_blur->addUniform("M");
		prog_blur->addUniform("horizontal");
		prog_blur->addAttribute("vertPos");
		prog_blur->addAttribute("vertTex");

		
		prog_final = make_shared<Program>();
		prog_final->setVerbose(true);
		prog_final->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/final_frag.glsl");
		if (!prog_final->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_final->init();
		prog_final->addUniform("P");
		prog_final->addUniform("V");
		prog_final->addUniform("M");
		prog_final->addAttribute("vertPos");
		prog_final->addAttribute("vertTex");
	}

	void initGeom(const std::string& resourceDirectory)
	{
		//init rectangle mesh (2 triangles) for the post processing
		glGenVertexArrays(1, &VertexArrayIDBox);
		glBindVertexArray(VertexArrayIDBox);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

		GLfloat *rectangle_vertices = new GLfloat[18];
		// front
		int verccount = 0;

		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;


		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTex);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

		float t = 1. / 100.;
		GLfloat *rectangle_texture_coords = new GLfloat[12];
		int texccount = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(2);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		// Initialize mesh.
		shape = make_shared<Shape>();
		shape->loadMesh(resourceDirectory + "/sphere.obj");
		shape->resize(); 
		shape->init();
			
		
		int width, height, channels;
		char filepath[1000];

		//texture earth diffuse
		string str = resourceDirectory + "/earth.jpg";
		strcpy(filepath, str.c_str());		
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureEarth);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureEarth);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		width = 640; height = 480;

		//texture moon
		str = resourceDirectory + "/moon.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureMoon);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureMoon);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		// [prog] Set texture units in cpu to samplers in gpu
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		// [final prog] Set texture units in cpu to samplers in gpu
		Tex1Location = glGetUniformLocation(prog_final->pid, "tex");//tex, tex2... sampler in the fragment shader
		Tex2Location = glGetUniformLocation(prog_final->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog_final->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		/**************************************************************/
		/* --------- Frame Buffer - Color, Glow Source ---------------*/
		/**************************************************************/
		/* Color Texture */
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		glGenTextures(1, &TexColor);
		glBindTexture(GL_TEXTURE_2D, TexColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//NULL means reserve texture memory, but texels are undefined
		//**** Tell OpenGL to reserve level 0. Why? Defines the mipmap. 0 is highest res.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		//You must reserve memory for other mipmaps levels as well either by making a series of calls to
		//glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
		//Here, we'll use :
		glGenerateMipmap(GL_TEXTURE_2D);

		/* Brightness Texture */
		glGenTextures(1, &TexAlpha);
		glBindTexture(GL_TEXTURE_2D, TexAlpha);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
		

		/* Frame Buffer */
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);

		//Attach texture(s) to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexColor, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, TexAlpha, 0);

		//-------------------------
		glGenRenderbuffers(1, &depth_rb);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

		//-------------------------
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

		// Set texture units in cpu to samplers in gpu
		Tex1Location = glGetUniformLocation(prog_blur->pid, "tex");//tex, tex2... sampler in the fragment shader
		glUseProgram(prog_blur->pid);
		glUniform1i(Tex1Location, 0);
		/*****************************************************/
		/* --------- Frame Buffer - Blurring ---------------*/
		/****************************************************/
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		glGenTextures(1, &BlurredTexAlpha);
		glBindTexture(GL_TEXTURE_2D, BlurredTexAlpha);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		//-------------------------
		glGenFramebuffers(1, &fb2);
		glBindFramebuffer(GL_FRAMEBUFFER, fb2);

		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BlurredTexAlpha, 0);

		//-------------------------
		glGenRenderbuffers(1, &depth_rb2);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb2);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

		//-------------------------
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb2);



		//-------------------------
		//Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer: good";
			break;
		default:
			cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	//*************************************
	double get_last_elapsed_time()
		{
		static double lasttime = glfwGetTime();
		double actualtime = glfwGetTime();
		double difference = actualtime - lasttime;
		lasttime = actualtime;
		return difference;
		}
	//*************************************
	
	void render_to_texture() // aka render to framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };	// color and alpha
		glDrawBuffers(2, buffers);

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		glm::mat4 M, V, S, T, P;
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		V = mycam.process();


		// Clear framebuffer.
		// Why do you clear the framebuffer?
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//bind shader and copy matrices
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos.x);

		//	******		earth		******
		static float angle = 0;
		//angle += 0.02;
		M = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -5));
		glm::mat4 Ry = glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0));
		float pih = -3.1415926 / 2.0;
		glm::mat4 Rx = glm::rotate(glm::mat4(1.f), pih, glm::vec3(1, 0, 0));
		M = M * Ry * Rx;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureEarth);
	
		shape->draw(prog);	//draw earth

		//	******		moon		******
		static float moonangle = 0;
		//moonangle += 0.005;
		M = glm::translate(glm::mat4(1.f), glm::vec3(-1.5, 0, 1.5));
		glm::mat4 Ryrad = glm::rotate(glm::mat4(1.f), moonangle, glm::vec3(0, 1, 0));
		T = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -5));
		S = glm::scale(glm::mat4(1.f), glm::vec3(0.25, 0.25, 0.25));
		M = T * Ryrad * M * Rx * S;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureMoon);
		
		shape->draw(prog);	//draw moon

		//done, unbind stuff
		prog->unbind();
	}

	void render_blur_1() // aka render to framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb2);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0};	// alpha
		glDrawBuffers(1, buffers);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		// Setup MVP
		auto P = std::make_shared<MatrixStack>();
		P->pushMatrix();
		P->perspective(70., width, height, 0.1, 100.0f);
		glm::mat4 M, V, S, T;
		V = glm::mat4(1);
		M = glm::scale(glm::mat4(1), glm::vec3(1.2, 1, 1)) * glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));

		// Bind to shader
		prog_blur->bind();

		int horizontal = 0;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TexAlpha);
		glUniformMatrix4fv(prog_blur->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog_blur->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog_blur->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform1i(prog_blur->getUniform("horizontal"), horizontal);			// Are we doing the second blur? no
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog_blur->unbind();
	}

	void render_blur_2() // aka render to framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb2);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };	// alpha
		glDrawBuffers(1, buffers);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		// Setup MVP
		auto P = std::make_shared<MatrixStack>();
		P->pushMatrix();
		P->perspective(70., width, height, 0.1, 100.0f);
		glm::mat4 M, V, S, T;
		V = glm::mat4(1);
		M = glm::scale(glm::mat4(1), glm::vec3(1.2, 1, 1)) * glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));

		// Bind to shader
		prog_blur->bind();

		int horizontal = 1;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TexAlpha);
		glUniformMatrix4fv(prog_blur->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog_blur->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog_blur->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform1i(prog_blur->getUniform("horizontal"), horizontal);			// Are we doing the second blur? no
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog_blur->unbind();
	}

	void render_to_screen()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Get current frame buffer size.
		double frametime = get_last_elapsed_time();

		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		auto P = std::make_shared<MatrixStack>();
		P->pushMatrix();
		P->perspective(70., width, height, 0.1, 100.0f);
		glm::mat4 M, V, S, T;

		V = glm::mat4(1);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// testing - shows that the only objects is a plane with a texture
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		prog_final->bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, BlurredTexAlpha);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, TexColor);
		M = glm::scale(glm::mat4(1), glm::vec3(1.2, 1, 1)) * glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));
		glUniformMatrix4fv(prog_final->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog_final->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog_final->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog_final->unbind();

	}

};
//*********************************************************************************************************
int main(int argc, char **argv)
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render_to_texture();
		application->render_blur_1();
		application->render_blur_2();
		application->render_to_screen();
		
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
