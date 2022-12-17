#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct Dot
{
	vec3 pos;
	vec4 color;
};


int NUM_DOT = 200;
int NUM_PIX = 30;

class TestBufferApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	
  private:
	  gl::VaoRef	mAttribute;
	  gl::VboRef	mBuffer;
	  gl::GlslProgRef mShader;
	  gl::GlslProgRef  mStockShader;
	  gl::VboRef    mIndexBuffer;
	  CameraPersp	mCam;
	  
	  gl::Texture2dRef mTex;
};

void TestBufferApp::setup()
{
	
	mTex = gl::Texture2d::create(loadImage(loadAsset("shib_tex.png")));
	setWindowSize(mTex->getWidth(), mTex->getHeight());
	/*
	vector<Dot> dots;
	int pix_width = 160;
	int pix_height = 120;
	NUM_PIX = pix_width * pix_height;
	NUM_DOT = pix_width * pix_height * 4;
	dots.assign(NUM_DOT, Dot());
	vector<unsigned int> indices(NUM_PIX * 6);
	vec3 center = vec3();
	const float radius = 0.5f;
	float gap = 0.02f;
	float side = gap / 4;
	vec3 edgeOffset[4] = { vec3( -gap,  gap, 0.0f ),
						   vec3(  gap,  gap, 0.0f),
						   vec3(  gap, -gap, 0.0f),
						   vec3( -gap, -gap, 0.0f), };
	for (int i = 0; i < pix_width; i++) {
		for (int j = 0; j < pix_height; j++) {
			int idx = (j * pix_width) + i;
			int idxx = idx * 6;
			idx *= 4;
			auto& d = dots.at(idx);

			vec3 posi = center + (vec3(-(pix_width / 2) + i, (pix_height / 2) - j, 0.0f) * 0.01f);
			for (int k = 0;k < 4;k++) {
				auto& d = dots.at(idx + k);
				d.pos = posi + edgeOffset[k];
				d.color = vec4(1, 1, 1, 1);
			}
			
			for (int k = 0;k < 3;k++) {
				indices[idxx + k] = idx + k;
			}
			indices[idxx + 3] = idx + 2;
			indices[idxx + 4] = idx + 3;
			indices[idxx + 5] = idx + 0;
			
		}
	}
	*/

	vec4 colorRange[4] = { vec4(1.0f, 0.0f, 0.0f, 1.0f),
						  vec4(0.0f, 1.0f, 0.0f, 1.0f),
						  vec4(0.0f, 0.0f, 1.0f, 1.0f),
						  vec4(1.0f, 1.0f, 1.0f, 1.0f) };
	
	
	float vertices[28] = {
		-1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 
		 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f
	};

	unsigned int indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	mBuffer = gl::Vbo::create(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	gl::bindBuffer(mBuffer);
	

	mIndexBuffer = gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	gl::bindBuffer(mIndexBuffer);


	gl::enableVertexAttribArray(0);
	gl::vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 7, 0);
	gl::enableVertexAttribArray(1);
	gl::vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (const GLvoid*)(sizeof(float) * 3));
	


	/*
	mBuffer = gl::Vbo::create(GL_ARRAY_BUFFER, dots.size() * sizeof(Dot), dots.data(), GL_STATIC_DRAW);
	gl::bindBuffer(mBuffer);
	gl::enableVertexAttribArray(0);
	gl::enableVertexAttribArray(1);
	gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Dot), (const GLvoid*)offsetof(Dot, pos));
	gl::vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Dot), (const GLvoid*)offsetof(Dot, color));
	
	mIndexBuffer = gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NUM_PIX * 6, indices.data(), GL_STATIC_DRAW);
	gl::bindBuffer(mIndexBuffer);
	*/


	mStockShader = gl::getStockShader(gl::ShaderDef().color());

	try {

		mShader = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(CI_GLSL(330,
				uniform mat4 ciModelViewProjection;
		uniform vec4[4] colorRange;
		uniform mat4    view;
		uniform mat4    projection;
		layout(location = 0) in vec4 position;
		layout(location = 1) in vec4 icolor;
		out vec4 color;

		void main(void) {
			gl_Position = projection * view * position;

			int idx = int((position.x + position.y + position.z) * 100);

			color = icolor;
		}
		))
			.fragment(CI_GLSL(330,

				in vec4 color;
		out vec4 oColor;

		void main(void) {

			oColor = color;
		}
		))

			);

	}
	catch (gl::GlslProgCompileExc e) {
		console() << e.what() << endl;
	}
	mShader->uniform("colorRange", colorRange, 4);
	
//	mShader = gl::getStockShader(gl::ShaderDef().color());
}

void TestBufferApp::mouseDown( MouseEvent event )
{
}

void TestBufferApp::update()
{
}

void TestBufferApp::draw()
{
	
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow(getWindowSize());
	gl::draw(mTex);
	gl::pushMatrices();
	gl::color(Color(0, 1, 0));
	gl::drawSolidCircle(getWindowCenter(), 400);
	gl::drawSolidRect(Rectf(0, 0, 200, 200));
	gl::popMatrices();
	
}

CINDER_APP( TestBufferApp, RendererGl )
