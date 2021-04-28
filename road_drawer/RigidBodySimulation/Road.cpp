/*
	GPU accelerated rigid body simulation with OpenGL and OpenCL.
	Created by: Sándor Balázs - it.sandor.balazs@gmail.com
	AX400
	---
	The dynamic barrier modell generator.
*/

#include "simulation.h"
#include <iostream>

void Simulation::bezierCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
	float x[4], y[4];
	x[0] = a.x;
	y[0] = a.z;

	x[1] = b.x;
	y[1] = b.z;

	x[2] = c.x;
	y[2] = c.z;

	x[3] = d.x;
	y[3] = d.z;
	bezierCurve(x, y);
}

void Simulation::bezierCurve(float x[], float y[])
{
	double xu = 0.0, yu = 0.0, u = 0.0;
	for (u = 0.0; u <= 1.0; u += 0.02)
	{
		xu = pow(1.0 - u, 3.0) * x[0] + 3.0 * u * pow(1.0 - u, 2.0) * x[1] + 3.0 * pow(u, 2.0) * (1.0 - u) * x[2]
			+ pow(u, 3.0f) * x[3];
		yu = pow(1 - u, 3.0) * y[0] + 3.0 * u * pow(1.0 - u, 2.0) * y[1] + 3.0 * pow(u, 2.0) * (1.0 - u) * y[2]
			+ pow(u, 3.0) * y[3];
		centerPoints.push_back(glm::vec3(xu, z, yu));
	}

	float x1 = centerPoints[0].x;
	float y1 = centerPoints[0].z;
	float x2 = centerPoints[1].x;
	float y2 = centerPoints[1].z;

	float shift = 2.0f;
	float xnorm = -1 * (y2 - y1);
	float ynorm = (x2 - x1);
	float l = sqrt(xnorm * xnorm + ynorm * ynorm);
	float xa = x1 + ((xnorm / l) * shift);
	float ya = y1 + ((ynorm / l) * shift);

	shiftedPoints.push_back(glm::vec3(xa, z ,ya));

	for (size_t i = 1; i < centerPoints.size(); i++)
	{
		float x1 = centerPoints[i - 1].x;
		float y1 = centerPoints[i - 1].z;
		float x2 = centerPoints[i].x;
		float y2 = centerPoints[i].z;

		float shift = 2.0f;
		float xnorm = -1 * (y2 - y1);
		float ynorm = (x2 - x1);
		float l = sqrt(xnorm * xnorm + ynorm * ynorm);
		float x = x2 + ((xnorm / l) * shift);
		float y = y2 + ((ynorm / l) * shift);

		shiftedPoints.push_back(glm::vec3(x, z, y));
	}
}

//Create the road box modell dinamically by boxSize
void Simulation::roadBuilder()
{

	/*road.Clean();
	roadCoordinates.Clean();
	roadIndices.Clean();
	roadNormals.Clean();
	roadTextures.Clean();*/
	centerPoints.clear();
	shiftedPoints.clear();

	/*bezierCurve(glm::vec3(-500.0f, 0, 0.0f), glm::vec3(-20.0f, 0, 10.0f),
		glm::vec3(20.0f, 0, 10.0f), glm::vec3(500.0f, 0, 0.0f));*/

	bezierCurve(x, y);

	std::vector<glm::vec3> points;
	for (size_t i = 0; i < centerPoints.size(); i++) {
		points.push_back(centerPoints[i]);
		points.push_back(shiftedPoints[i]);
	}
	roadCoordinates.BufferData(points);

	std::vector<glm::vec3> normals;
	for (size_t i = 0; i < points.size(); i++) {
		//std::cout << points[i].x << "," << points[i].z << std::endl;
		normals.push_back(glm::vec3(0, 1, 0));
	}
	roadNormals.BufferData(normals);

	std::vector<glm::vec2> textures;
	for (size_t i = 0; i < points.size(); i = i + 4) {
		textures.push_back(glm::vec2(1, 1));
		textures.push_back(glm::vec2(0, 1));
		textures.push_back(glm::vec2(1, 0));
		textures.push_back(glm::vec2(0, 0));
	}
	roadTextures.BufferData(textures);

	std::vector<int> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(3);

	indices.push_back(0);
	indices.push_back(3);
	indices.push_back(2);

	/*indices.push_back(2);
	indices.push_back(3);
	indices.push_back(5);

	indices.push_back(2);
	indices.push_back(5);
	indices.push_back(4);*/
	for (size_t i = 6; i <= 293; i = i+6) {
		indices.push_back(indices[i-6]+2);
		indices.push_back(indices[i-5]+2);
		indices.push_back(indices[i-4]+2);
		indices.push_back(indices[i-3]+2);
		indices.push_back(indices[i-2]+2);
		indices.push_back(indices[i-1]+2);
	}
	roadIndices.BufferData(indices);
	/*for (size_t i = 280; i < indices.size(); i++)
	{
		std::cout << i << " : " << indices[i] << std::endl;
	}*/

	road.Init(
		{
			{ CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, roadCoordinates },
			{ CreateAttribute<1, glm::vec3, 0, sizeof(glm::vec3)>, roadNormals },
			{ CreateAttribute<2, glm::vec2, 0, sizeof(glm::vec2)>, roadTextures }
		}, roadIndices);
}