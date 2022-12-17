#include "Loader.h"


#include<fstream>
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include<vector>

using namespace std;
using namespace MyParser;

void ObjParser::loadObj(char* path, std::vector <vec3>& out_vertices, std::vector <vec2>& out_uvs, std::vector <vec3>& out_normals) {
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< vec3 > temp_vertices;
	std::vector< vec2 > temp_uvs;
	std::vector< vec3 > temp_normals;

	ifstream file;

	try {
		file.open(path);
	}
	catch (exception e) {
		throw exception("open file error");
	}

	string line;

	while (getline(file, line)) {

		// else : parse lineHeader
		if (compare_header(line, "v")) {
			vec3 vertex;
			sscanf(line.substr(2).c_str(), "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (compare_header(line, "vt")) {
			vec2 uv;
			sscanf(line.substr(3).c_str(), "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (compare_header(line, "vn")) {
			vec3 normal;
			sscanf(line.substr(3).c_str(), "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (compare_header(line, "f")) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
			bool have_uv = false;
			bool is_quad = false;
			int matches = sscanf(line.substr(2).c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2], &vertexIndex[3], &uvIndex[3], &normalIndex[3]);
			if (matches != 12) {
				matches = sscanf(line.substr(2).c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches != 9) {
					matches = sscanf(line.substr(2).c_str(), "%d//%d %d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2], &vertexIndex[3], &normalIndex[3]);
					if (matches != 8) {
						matches = sscanf(line.substr(2).c_str(), "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2]);
						if (matches != 6) {
							throw exception("File can't be read by our simple parser : ( Try exporting with other options )\n");
						}
					}
					else {
						is_quad = true;
					}
				}
				else {
					have_uv = true;
				}
			}
			else {
				have_uv = true;
				is_quad = true;
			}


			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			if (have_uv) {
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
			}
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
			if (is_quad) {
				vertexIndices.push_back(vertexIndex[2]);
				vertexIndices.push_back(vertexIndex[3]);
				vertexIndices.push_back(vertexIndex[0]);
				normalIndices.push_back(normalIndex[2]);
				normalIndices.push_back(normalIndex[3]);
				normalIndices.push_back(normalIndex[0]);
				if (have_uv) {
					uvIndices.push_back(uvIndex[2]);
					uvIndices.push_back(uvIndex[3]);
					uvIndices.push_back(uvIndex[0]);
				}

			}

		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		vec3 vertex = temp_vertices[vertexIndex - 1];
		out_vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		vec2  uv = temp_uvs[uvIndex - 1];
		out_uvs.push_back(uv);
	}

	for (unsigned int i = 0; i < normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		vec3  normal = normalize(temp_normals[normalIndex - 1]);
		out_normals.push_back(normal);
	}

}

MyBuffer ObjParser::loadObj(char* path) {
	int NUM_VERTS;
	struct TestVertex {
		vec3 pos;
		vec3 normal;
		vec2 uv;
	};

	vector<vec3> vertices;
	vector<vec2> uvs;
	vector<vec3> normals;
	try {
		loadObj(path, vertices, uvs, normals);
	}
	catch (exception e) {
		throw exception("load error");
	}

	NUM_VERTS = vertices.size();


	vector<TestVertex> verts;
	for (int i = 0;i < vertices.size();i++) {
		TestVertex v;
		v.pos = vertices[i];
		v.normal = normals[i];
		if (i < uvs.size()) v.uv = uvs[i];
		verts.push_back(v);

	}

	MyBuffer buffer;

	buffer.num_vertices = vertices.size();
	buffer.num_normals = normals.size();
	buffer.num_uvs = uvs.size();

	buffer.vbo = gl::Vbo::create(GL_ARRAY_BUFFER, verts.size() * sizeof(TestVertex), verts.data(), GL_STATIC_DRAW);
	{
		buffer.vao = gl::Vao::create();
		gl::ScopedVao mvao(buffer.vao);

		gl::ScopedBuffer buffer(buffer.vbo);
		gl::enableVertexAttribArray(0);
		gl::enableVertexAttribArray(1);
		gl::enableVertexAttribArray(2);
		gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TestVertex), (const GLvoid*)offsetof(TestVertex, pos));
		gl::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TestVertex), (const GLvoid*)offsetof(TestVertex, normal));
		gl::vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TestVertex), (const GLvoid*)offsetof(TestVertex, uv));
	}

	return buffer;
}