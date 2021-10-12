#pragma once

// project
#include "cgra/cgra_geometry.hpp"


using namespace std;
using namespace cgra;
using namespace glm;

struct waveParticle {
	glm::vec2 origin;
	glm::vec2 position;
	glm::vec2 direction;
	float amplitude;
	float displacement(glm::vec2 x);
	float rf(float x);
	float radius = 11;
	float dispAng;
	float speed = 1;
};


struct water_plane {
	GLuint shader = 0;
	cgra::gl_mesh mesh;
	glm::vec3 color{ 0.7 };
	glm::mat4 modelTransform{ 1.0 };
	GLuint texture;
	std::vector<std::vector<waveParticle>> waveFronts{};
	float speed;
	cgra::mesh_builder createSurface();
	float lastTick = 0;
	float rate = 0.01;
	const static int n = 100;
	float heightMap[n][n] = { 0 };
	std::vector<waveParticle> cellMap[n][n];
	float width = 60;
	float threshold = 0.01;

	void draw(const glm::mat4& view, const glm::mat4 proj);
	//Simulates the water at a given time
	void simulate();
	//Runs the math to get the heightMap for the water.
	void getHMap();
	float eta(glm::vec2 x);
	void  iterate();
	void visualize(const glm::mat4& view, const glm::mat4 proj);
	std::vector<waveParticle> getAdjacent(glm::vec2 p, int n, int rad);
	void randWave();
	void generateWaveParticles();
};

water_plane water;
//======================================================================= METHODS FOR WATER ============================================================================

/*
* Draws Mesh
*/
void water_plane::draw(const glm::mat4& view, const glm::mat4 proj) {
	mat4 modelview = view * modelTransform;

	glUseProgram(shader); // load shader and variables
	glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, false, value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(shader, "uModelViewMatrix"), 1, false, value_ptr(modelview));
	glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, value_ptr(color));

	mesh.draw(); // draw
}

/*
Draws the plane based in the heightmap.
*/
mesh_builder water_plane::createSurface() {
	float step = (2 * width) / n;
	vector<vec3> positions;
	vector<vec3> normals;
	vector<unsigned int> indices;
	vector<vec2> uvs;
	int x = 0;
	vec3 normal = vec3(0, 1, 0);
	for (int i = 0; i <= n; i++) {
		for (int j = 0; j <= n; j++) {
			int hi = i;
			int hj = j;
			if (i == n) {
				hi = n - 1;
			}
			if (j == n) {
				hj = n - 1;
			}
			positions.push_back(vec3((j * step - width), heightMap[hi][hj], (i * step - width)));
			normals.push_back(normal);
		}
	}
	for (int row = 0; row < n; row++) {
		for (int col = 0; col < n; col++) {
			indices.push_back(n * row + col);
			indices.push_back(n * row + col + n);
			indices.push_back(n * row + col + n + 1);

			indices.push_back(n * row + col);
			indices.push_back(n * row + col + n + 1);
			indices.push_back(n * row + col + 1);
		}
	}
	uvs.resize(positions.size(), vec3(0));
	mesh_builder mb;
	mb.indices = indices;
	for (int i = 0; i < positions.size(); i++) {
		mb.push_vertex(mesh_vertex{
			positions[i],
			normals[i],
			uvs[i]
			});
	}
	return mb;
}


/*
* VISUALIZATION METHOD FOR WATER
*/
void water_plane::visualize(const glm::mat4& view, const glm::mat4 proj) {
	for (int i = 0; i < waveFronts.size(); i++) {
		for (int j = 0; j < waveFronts[i].size(); j++) {
			waveParticle particle = waveFronts[i][j];
			mat4 pos = translate(view, vec3(particle.position.y, 0, particle.position.x));
			pos = scale(pos, vec3(0.5));
			glUniformMatrix4fv(glGetUniformLocation(shader, "uModelViewMatrix"), 1, false, value_ptr(pos));
			glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, value_ptr(vec3(0, 1, 0)));
			drawSphere();
		}
	}
}

/*
Method to recalculate the water every certain time period.
*/
void water_plane::simulate() {
	clock_t currentTime = clock();
	float elapsedMs = double(currentTime - lastTick) / CLOCKS_PER_SEC;

	if (elapsedMs > rate) {
		lastTick = currentTime;
		iterate();
		generateWaveParticles();
		getHMap();
		mesh = createSurface().build();
	}
}

/*
Moves the particles around and calculates which cell they belong to.
*/
void water_plane::iterate() {
	std::vector<waveParticle> cM[n][n];
	vector<vector<waveParticle>> WF2;
	for (int i = 0; i < waveFronts.size(); i++) {
		vector<waveParticle> wf;
		for (int a = 0; a < waveFronts[i].size(); a++) {
			auto part = waveFronts[i][a];
			part.position += part.direction;
			if (part.position.x < width && part.position.y < width && part.position.x > -width && part.position.y > -width && part.amplitude > threshold) {
				float stepsize = 2 * width / n;
				int j = (int)(((part.position.x + width) / (2 * width)) * n);
				int k = (int)(((part.position.y + width) / (2 * width)) * n);
				wf.push_back(part);
				cM[j][k].push_back(part);
			}
		}
		if (wf.size() > 0) {
			WF2.push_back(wf);
		}
	}
	waveFronts = WF2;

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			cellMap[i][j] = cM[i][j];
		}
	}
}

/*
Finds the height of every vertex in the mesh.
*/
void water_plane::getHMap() {
	float baseHeight = 0;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			float stepSize = 2 * width / n;
			glm::vec2 x = vec2(i, j);
			heightMap[i][j] = baseHeight + eta(x);
		}
	}
}

/*
Sum of displacements of particles
*/
float water_plane::eta(glm::vec2 x) {
	float _sum = 0;

	vector<waveParticle> neighbors = getAdjacent(x, n, 4);
	for (int i = 0; i < neighbors.size(); i++) {
		waveParticle p = neighbors[i];
		float stepSize = 2 * width / n;
		vec2 x2 = vec2(-width, -width) + x * stepSize;
		_sum += p.displacement(x2);
	}
	return _sum;

}

/*
Rectangle function for waveform calculation
*/
float waveParticle::rf(float x) {
	if (abs(x) < 0.5) return 1;
	if (abs(x) < 0.6) return 0.5;
	if (abs(x) < 0.8) return 0.2;
	return 0;
}

/*
Calculates the waveform at a certain point relative to this particle's point
*/
float waveParticle::displacement(glm::vec2 x) {
	float pi = 3.141592;
	float p1 = cosf((pi * distance(x, position)) / radius) + 1;
	float rect = rf(distance(x, this->position) / (2 * this->radius));
	float disp = (this->amplitude / 2) * p1 * rect;

	return disp;
}


//Gets a simple square area around the point of size radius.
vector<waveParticle> water_plane::getAdjacent(vec2 p, int n, int rad) {
	vector<waveParticle> ne = vector<waveParticle>();

	for (int i = fmax(p.x - rad, 0); i < fmin(p.x + rad, n); i++) {
		for (int j = fmax(p.y - rad, 0); j < fmin(p.y + rad, n); j++) {
			for (auto p : cellMap[i][j]) {
				ne.push_back(p);
			}
		}
	}
	return ne;

}

void water_plane::randWave() {
	float ri = (((float)rand() / RAND_MAX) * (2 * width));
	float rj = (((float)rand() / RAND_MAX) * (2 * width));

	waveParticle p1, p2, p3, p4, p5, p6;
	vec2 o = vec2(-width, -width);
	p1.position = o + vec2(ri, rj);
	p2.position = o + vec2(ri, rj);
	p3.position = o + vec2(ri, rj);
	p4.position = o + vec2(ri, rj);
	p1.direction = normalize(vec2(0, 1));
	p2.direction = normalize(vec2(1, 0));
	p3.direction = normalize(vec2(0, -1));
	p4.direction = normalize(vec2(-1, 0));
	p1.dispAng = 0;
	p2.dispAng = 180;
	p3.dispAng = 90;
	p4.dispAng = 270;
	vector<waveParticle> wf;
	wf.push_back(p1);
	wf.push_back(p2);
	wf.push_back(p3);
	wf.push_back(p4);

	for (int i = 0; i < wf.size(); i++) {
		wf[i].origin = wf[i].position;
		wf[i].amplitude = 20;
	}

	waveFronts.push_back(wf);
}

void water_plane::generateWaveParticles() {
	for (int i = 0; i < waveFronts.size(); i++) {

		vector<waveParticle> wf;
		waveParticle p1 = waveFronts[i][waveFronts[i].size() - 1];
		wf.push_back(p1);
		for (int j = 0; j < waveFronts[i].size() - 1; j++) {
			p1 = wf[wf.size() - 1];

			waveParticle p2 = waveFronts[i][j];


			if (distance(p1.position, p2.position) > p1.radius / 2) {
				float d = 1.6;
				waveParticle mid;
				wf[wf.size() - 1].amplitude /= d;
				p2.amplitude /= d;
				mid.amplitude = p2.amplitude;
				vec2 m = (p2.position + p1.position) / 2.f;
				vec2 dir = normalize((p1.direction + p2.direction) / 2.f);
				vec2 midP = p2.origin + (dir * distance(p2.origin, p2.position));
				mid.origin = p2.origin;
				mid.direction = dir;
				mid.position = midP;
				mid.dispAng = p1.dispAng + 0.5f * (p2.dispAng - p1.dispAng);

				wf.push_back(mid);
			}
			wf.push_back(p2);
		}
		waveFronts[i] = wf;
	}
}