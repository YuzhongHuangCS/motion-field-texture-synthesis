#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <boost/format.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "util.hpp"

using namespace std;
using namespace boost;
using namespace glm;

const static int WIDTH = 512;
const static int HEIGHT = 512;
const static int COUNT = 10240;

int main(int argc, char* argv[]) {
	if (!glfwInit()) {
		throw runtime_error("Failed to initialize GLFW");
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Render", NULL, NULL);
	if (!window) {
		throw runtime_error("Failed to open GLFW window");
	}
	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		throw runtime_error("Failed to initialize GLEW");
	}
	checkError("init");

	// motion
	const unsigned char* motionData = readBMP("motion.bmp");
	GLuint motion;
	glGenTextures(1, &motion);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, motion);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, motionData);
	glBindImageTexture(0, motion, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
	checkError("motion");

	// position
	float* positionData = new float[COUNT * 4];
	for (int i = 0; i < COUNT; i += 4) {
		positionData[i] = float(rand()) / RAND_MAX;
		positionData[i + 1] = float(rand()) / RAND_MAX;
		positionData[i + 2] = float(rand()) / RAND_MAX;
		positionData[i + 3] = float(rand()) / RAND_MAX;
	}
	GLuint position;
	glGenTextures(1, &position);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, position);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, COUNT, 0, GL_RGBA, GL_FLOAT, positionData);
	glBindImageTexture(1, position, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	checkError("position");

	// velocity
	float* velocityData = new float[COUNT * 4];
	GLuint velocity;
	glGenTextures(1, &velocity);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_1D, velocity);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, COUNT, 0, GL_RGBA, GL_FLOAT, velocityData);
	glBindImageTexture(2, velocity, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	// control
	int* controlData = new int[COUNT * 4];
	GLuint control;
	glGenTextures(1, &control);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_1D, control);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32I, COUNT, 0, GL_RGBA, GL_INT, controlData);
	glBindImageTexture(3, control, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32I);

	// display
	GLuint display;
	glGenTextures(1, &display);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, display);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindImageTexture(4, display, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8UI);
	glGenerateMipmap(GL_TEXTURE_2D);

	// compute shader
	const static char* computeSrc =
		#include "compute.shader"
	;
	GLenum computeType[] = { GL_COMPUTE_SHADER };
	GLuint compute = buildProgram(&computeSrc, computeType, 1);

	// draw shader
	const static char* vertexSrc =
		#include "vertex.shader"
	;
	const static char* fragmentSrc =
		#include "fragment.shader"
	;
	const char* drawSrc[] = { vertexSrc, fragmentSrc };
	GLenum drawType[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
	GLuint draw = buildProgram(drawSrc, drawType, 2);
	glUseProgram(draw);
	glUniform2i(glGetUniformLocation(draw, "size"), WIDTH, HEIGHT);

	// draw data
	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	const static GLfloat vertexData[] = {
		-1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f
	};

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	// main loop
	double lastTime = glfwGetTime();
	double nowTime;
	char fpsTitle[32];
	do {
		glUseProgram(compute);
		glDispatchCompute(COUNT / 256, 1, 1);
		glUseProgram(draw);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(vertexData));
		glfwSwapBuffers(window);
		glfwPollEvents();
		glClearTexImage(display, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		nowTime = glfwGetTime();
		sprintf(fpsTitle, "Render - FPS: %.2f", 1 / (nowTime - lastTime));
		glfwSetWindowTitle(window, fpsTitle);
		lastTime = nowTime;
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
	
	return EXIT_SUCCESS;
}