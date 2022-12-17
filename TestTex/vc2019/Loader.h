
#ifndef  LOADER_H
#define  LOADER_H
#pragma once

#include<fstream>
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include<vector>


using namespace ci;
using namespace std;

namespace MyParser {
    static bool compare_header(string s1, string s2) {
        if (s1.length() < 4) return false;
        if (s2.length() == 1) {
            return (s1.substr(0, 1) == s2) && (s1[1] == ' ');
        }
        else if (s2.length() == 2) {
            return (s1.substr(0, 2) == s2) && (s1[2] == ' ');
        }
        return false;
    }
}



class MyBuffer {
public:
    gl::VaoRef vao;
    gl::VboRef vbo;
    int num_vertices;
    int num_normals;
    int num_uvs;
};


class ObjParser {
private:
public:
    static void loadObj(char* path, std::vector <vec3>& out_vertices, std::vector <vec2>& out_uvs, std::vector <vec3>& out_normals);

    static MyBuffer loadObj(char* path);
};


#endif //! LOADER_H