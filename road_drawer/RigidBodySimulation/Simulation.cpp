/*
	GPU accelerated rigid body simulation with OpenGL and OpenCL.
	Created by: Sándor Balázs - it.sandor.balazs@gmail.com
	AX400
	---
	The main simulation object. Simulation initialisation and Input handling.
*/

#include <ctime>
#include "Simulation.h"

//Constructor
Simulation::Simulation(void) {
	//Random generator initialisation
	srand((unsigned int)time(NULL));

	//Simulation values initialisation
	gravity = glm::vec3(0.0f, 0.006f, 0.0f);
	resistance = 0.996f;
	ballInitSpeed = 0.2f;
	boxSize = 15.0f;
	barrierShift = glm::vec3(0, -boxSize + ((boxSize) / 6) + 0.01f, 0);
	float barrierSize = boxSize / 6;
}

//Destructor
Simulation::~Simulation(void){}

//Simulation rendering intitialisation
bool Simulation::Init() {
	glClearColor(0.0f, 0.47f, 0.87f, 1.0f);	//BG Color
	glEnable(GL_CULL_FACE);					//Face test
	glEnable(GL_DEPTH_TEST);				//Depth buffer
	glEnable(GL_BLEND);						//Alpha
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Shader loading
	shader.Init({
		{ GL_VERTEX_SHADER, "vertexShader.vert" },
		{ GL_FRAGMENT_SHADER, "fragmentShader.frag" }
		},
	{
		{ 0, "vs_in_pos" },		// VAO 0	 --> vs_in_pos
		{ 1, "vs_in_normal" },	// VAO 1 chanel --> vs_in_normal
		{ 2, "vs_out_tex0" },	// VAO 2 chanel --> vs_in_tex0
	});

	//Building the wall modell
	wallBuilder();

	//Building the road modell
	x[0] = -5.0f;
	y[0] = -0.0f;

	x[1] = -2.0f;
	y[1] = 5.1f;

	x[2] = 2.0f;
	y[2] = 5.1f;

	x[3] = 5.0f;
	y[3] = -0.0f;
	roadBuilder();

	//Building the barrier modell
	barrierBuilder();

	//Load Texture
	texture.FromFile("texture.png");
	texture2.FromFile("texture2.png");
	defaultTeture.FromFile("default.png");

	// mesh betöltése
	ball = ObjParser::parse("sphere.obj");

	//Set Camera
	camera.SetProj(45.0f, 640.0f / 480.0f, 0.01f, 1000.0f);

	//Balls start initialisation
	ballInit();

	return true;
}

//Balls start initialisation
void Simulation::ballInit() {
	float r = boxSize - 2.0f;
	float x = 0;
	float y = boxSize - (boxSize / 4);
	float z = 0;
	for (size_t i = 0; i < numberOfBallsArray; i++) {
		if (randomXZ) {
			x = random(-r, r);
			z = random(-r, r);
		}
		if (randomY)    y = random(-r, r);
		positions[i] = glm::vec3(x, y, z);
		velocities[i] = glm::vec3(random(-ballInitSpeed, ballInitSpeed), random(-ballInitSpeed, ballInitSpeed), random(-ballInitSpeed, ballInitSpeed));
		colors[i] = glm::vec4(random(0.0f, 1.0f), random(0.0f, 1.0f), random(0.0f, 1.0f), 1);
		collisionCheck[i] = true;
	}
	//Copy datas to GPU
	if (GPU_isActive) Update_GPU();
}

void Simulation::Clean() {
	shader.Clean();
}

//Update the Camera
void Simulation::Update() {
	camera.Update();
}

void Simulation::KeyboardDown(SDL_KeyboardEvent& key) {
	camera.KeyboardDown(key);
	if (key.keysym.sym == SDLK_w) lightPos.x++;
	if (key.keysym.sym == SDLK_s) lightPos.x--;
	if (key.keysym.sym == SDLK_a) lightPos.y++;
	if (key.keysym.sym == SDLK_d) lightPos.y--;
}

void Simulation::KeyboardUp(SDL_KeyboardEvent& key) {
	camera.KeyboardUp(key);
}

void Simulation::MouseMove(SDL_MouseMotionEvent& mouse) {
	camera.MouseMove(mouse);
	/*if (barrierIsOn) {
		if (mouse.state & SDL_BUTTON_RMASK) {
			glm::vec3 shifted = barrierShift;
			glm::vec3 shift = glm::vec3(mouse.xrel / 4.0f, 0, mouse.yrel / 4.0f);
			shifted = shifted + shift;
			if (shifted.x >= -boxSize + barrierSize && shifted.x <= boxSize - barrierSize) barrierShift.x = barrierShift.x + shift.x;
			if (shifted.z >= -boxSize + barrierSize && shifted.z <= boxSize - barrierSize) barrierShift.z = barrierShift.z + shift.z;
			Update_GPU(false);
		}
	}*/
	if (mouse.state & SDL_BUTTON_LMASK) {
		glm::vec3 shift = glm::vec3(mouse.xrel / 4.0f, 0, mouse.yrel / 4.0f);
		x[hit] -= shift.x;
		y[hit] -= shift.z;
		roadBuilder();
	}
}

void Simulation::MouseDown(SDL_MouseButtonEvent& mouse) {
	//Step 0: Get size
	GLint m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	float width = m_viewport[2];
	float height = m_viewport[3];

	//Step 1: 3d Normalised Device Coordinates
	float x = (2.0f * mouse.x) / width - 1.0f;
	float y = 1.0f - (2.0f * mouse.y) / height;
	float z = 1.0f;
	glm::vec3 ray_nds = glm::vec3(x, y, z);

	//Step 2: 4d Homogeneous Clip Coordinates
	glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

	//Step 3: 4d Eye (Camera) Coordinates
	glm::vec4 ray_eye = glm::inverse(camera.GetProj()) * ray_clip;
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

	//Step 4: 4d World Coordinates
	glm::vec3 ray_wor = glm::inverse(camera.GetViewMatrix()) * ray_eye;

	//Step 5: Normalize
	ray_wor = glm::normalize(ray_wor);

	std::cout << "ray_wor: " << ray_wor.x << "," << ray_wor.y << "," << ray_wor.z << std::endl;

	std::map<float, int> hits;
	for (size_t i = 0; i < 4; i++) {
		glm::vec3 r0 = camera.GetEye();
		glm::vec3 rd = ray_wor;
		glm::vec3 s0 = glm::vec3(Simulation::x[i], Simulation::z + 0.2f, Simulation::y[i]);
		float sr = 0.5f;

		float a = glm::dot(rd, rd);
		glm::vec3 s0_r0 = r0 - s0;
		float b = 2.0 * glm::dot(rd, s0_r0);
		float c = glm::dot(s0_r0, s0_r0) - (sr * sr);
		float disc = b * b - 4.0 * a * c;
		if (disc < 0.0) {
			//std::cout << "NO HIT" << std::endl;
		}
		else {
			hits[glm::distance(s0, camera.GetEye())] = i;
			std::cout << i << std::endl;
		}
	}
	if (!hits.empty()) Simulation::hit = hits.begin()->second;
}

void Simulation::MouseUp(SDL_MouseButtonEvent& mouse) {}

void Simulation::MouseWheel(SDL_MouseWheelEvent& wheel) {
	camera.MouseWheel(wheel);
}

void Simulation::Resize(int _w, int _h) {
	glViewport(0, 0, _w, _h);
	camera.Resize(_w, _h);
}

float Simulation::random(float lower, float upper) {
	float random = lower + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (upper - lower)));
	return random;
}

std::string currentDateTime() {
	time_t now = time(0);
	std::string ret = ctime(&now);
	ret[ret.length() - 1] = ' ';
	return ret;
}