#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/text.h";
#include "cinder/font.h";
#include "cinder/CinderImGui.h"
#include "iostream";

using namespace ci;
using namespace ci::app;
using namespace std;

template <typename T>
std::string formatString(T t);

template<typename T, typename... Args>
std::string formatString(T t, Args... args);

struct Dot
{
	vec3 pos;
	float color;
};

struct CircleVert
{
	vec3 center;
	vec3 pos;
	float color;
};



int NUM_DOT;
int NUM_PIX;
int SUB_DIV;
float MAX_RAD = 0.01;
float MIN_RAD = 0.005;

class TestCamApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

  private:
	  gl::VaoRef	mAttribute;
	  gl::VboRef	mBuffer;
	  gl::VboRef    mIndexBuffer;
	  CaptureRef			mCapture;
	  gl::TextureRef		mTexture;
	  Surface8uRef          mSurface;
	  CameraPersp		    mCam;
	  gl::BatchRef	        mBox[4];
	  gl::GlslProgRef       mShader;
	  gl::GlslProgRef       mUpdateShader;
	  bool                  firstOutput;
	  Color8u               colorBuffer[128][72];
	  int                   heartMap[24][32] = { {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,        255, 255, 255, 255, 255, 255, 255,   0,   0,   0,   0, 255, 255,        255, 255, 255, 255, 255, 255},       {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,        255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,          0, 255, 255, 255, 255, 255},       {255, 255, 255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,          0,   0, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0, 255, 255, 255},       {255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0, 255},       {255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0},       {255, 255, 255, 255, 255, 255, 255,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0, 255},       {255, 255, 255, 255, 255, 255, 255, 255,   0,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0, 255, 255},       {255, 255, 255, 255, 255, 255, 255, 255, 255,   0,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0,   0,   0, 255, 255},       {255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   0,   0,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,          0,   0, 255, 255, 255, 255},       {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   0,          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        255, 255, 255, 255, 255, 255},       {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 255,        255, 255, 255, 255, 255, 255},       {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,        255,   0,   0,   0,   0,   0,   0,   0,   0,   0, 255, 255, 255,        255, 255, 255, 255, 255, 255} };
	  double                lastTime = 0;
	  int                   framepersec = 0;
	  void                  calAndDisplayFPS(bool logToConsole = true);
	  float                 rescaler = 8;
	  float                 maxFPS = 144;
	  int                   lastFramerate = 0;
	  double                lastCalColorTime = 0;
	  vec4*                 mColorBuffer;
	  float                 crad;
	  float					rad_change_rate = 0.00005;
	  uint32_t*				mIndptr;
	  vector<unsigned int>  indices;
	  // Descriptions of particle data layout.
	  gl::VaoRef		mAttributes[2];
	  // Buffers holding raw particle data on GPU.
	  gl::VboRef		mParticleBuffer[2];

	 // Current source and destination buffers for transform feedback.
	// Source and destination are swapped each frame after update.
	  std::uint32_t	mSourceIndex = 0;
	  std::uint32_t	mDestinationIndex = 1;


};

void TestCamApp::setup()
{

	setFrameRate(maxFPS);
	firstOutput = false;
	
	// Create Buffer
	
	vector<Dot> dots;
	vector<CircleVert> verts;
	int pix_width = 640 / rescaler;
	int pix_height = 480 / rescaler;
	NUM_PIX = pix_width * pix_height;
	NUM_DOT = NUM_PIX * 4;
	mColorBuffer = new vec4[NUM_PIX];
	for (int i = 0; i < NUM_PIX; i++) mColorBuffer[i] = vec4(1, 1, 1, 1);

	SUB_DIV = 10;

	indices.assign(NUM_PIX * 3 * SUB_DIV, 0);
	NUM_DOT = NUM_PIX * (SUB_DIV + 1);
	verts.assign(NUM_DOT, CircleVert());
	vec3 cent = vec3(0);
	float gap = 0.02f;
	float side = gap / 4;
	for (int i = 0; i < pix_width; i++) {
		for (int j = 0; j < pix_height; j++) {
			int idx = (j * pix_width) + i;
			int og = idx;
			int idxx = idx * 3 * SUB_DIV;
			idx *= 11;
			auto& v = verts.at(idx);

			vec3 posi = cent + (vec3(-(pix_width / 2) + i, (pix_height / 2) - j, 0.00f) * gap) + vec3(0, 0 , 0);
			v.center = posi;
			v.pos = vec3(0, 0, 0);
			v.color = (float) og;
			for (int k = 0;k < SUB_DIV;k++) {
				float rel = k / (float) SUB_DIV;
				float angle = rel * M_PI * 2;
				vec2 offset(cos(angle), sin(angle));
				vec3 rad_pos = posi + (vec3(offset, 0) * side);
				auto& v = verts.at(idx + k + 1);
				v.center = posi;
				v.pos = vec3(offset, 0);
				v.color = (float)og;
			}
			
			for (int k = 1;k < SUB_DIV;k++) {
				indices[idxx] = idx;
				indices[idxx + 1] = idx + k;
				indices[idxx + 2] = idx + k + 1;
				idxx += 3;
			}

			indices[idxx] = idx;
			indices[idxx + 1] = idx + SUB_DIV;
			indices[idxx + 2] = idx + 1;
		}
	}
	mIndptr = indices.data();

	/*
	mBuffer = gl::Vbo::create(GL_ARRAY_BUFFER, verts.size() * sizeof(CircleVert), verts.data(), GL_STATIC_DRAW);
	gl::bindBuffer(mBuffer);
	gl::enableVertexAttribArray(0);
	gl::enableVertexAttribArray(1);
	gl::enableVertexAttribArray(2);
	gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CircleVert), (const GLvoid*)offsetof(CircleVert, center));
	gl::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CircleVert), (const GLvoid*)offsetof(CircleVert, pos));
	gl::vertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(CircleVert), (const GLvoid*)offsetof(CircleVert, color));
	*/


	// Create particle buffers on GPU and copy data into the first buffer.
	// Mark as static since we only write from the CPU once.
	mParticleBuffer[mSourceIndex] = gl::Vbo::create(GL_ARRAY_BUFFER, verts.size() * sizeof(CircleVert), verts.data(), GL_STATIC_DRAW);
	mParticleBuffer[mDestinationIndex] = gl::Vbo::create(GL_ARRAY_BUFFER, verts.size() * sizeof(CircleVert), nullptr, GL_STATIC_DRAW);
	mIndexBuffer = gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NUM_PIX * 3 * SUB_DIV, indices.data(), GL_STATIC_DRAW);

	for (int i = 0; i < 2; ++i)
	{	// Describe the particle layout for OpenGL.
		mAttributes[i] = gl::Vao::create();
		gl::ScopedVao vao(mAttributes[i]);

		// Define attributes as offsets into the bound particle buffer
		gl::ScopedBuffer buffer(mParticleBuffer[i]);
		gl::enableVertexAttribArray(0);
		gl::enableVertexAttribArray(1);
		gl::enableVertexAttribArray(2);
		gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CircleVert), (const GLvoid*)offsetof(CircleVert, center));
		gl::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CircleVert), (const GLvoid*)offsetof(CircleVert, pos));
		gl::vertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(CircleVert), (const GLvoid*)offsetof(CircleVert, color));

		//gl::ScopedBuffer buff(mIndexBuffer);
		//gl::bindBuffer(mIndexBuffer);
	}


	//gl::bindBuffer(mIndexBuffer);

	// Create Custom Shader
	try {

		mShader = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(CI_GLSL(330,
		uniform mat4 ciModelViewProjection;
		uniform mat4 view;
		uniform mat4 projection;
		uniform int   numDot;
		uniform vec4 colorMap[4800];
		uniform float cradius;
		layout(location = 0) in vec4 center;
		layout(location = 1) in vec4 roffset;
		layout(location = 2) in float iid;
		out vec4 color;

		void main(void) {
			vec4 pos = center + (roffset * cradius);
			gl_Position = projection * view * pos;
			int idx = int(iid);

			color = colorMap[idx];
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

	//create custom update shader
	try 
	{
		mUpdateShader = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(CI_GLSL(330,

		uniform mat4 zrot;
		in vec3 icenter;
		in vec3 iroffset;
		in float icid;

		out vec3 center;
		out vec3 offset;
		out float colorid;

		void main(void) {
			center = vec3(zrot * vec4(icenter, 1.0f));
			offset = iroffset;
			colorid = icid;
		}
		))
			.feedbackFormat(GL_INTERLEAVED_ATTRIBS)
			.feedbackVaryings({ "center", "offset", "colorid" })
			.attribLocation("icenter", 0)
			.attribLocation("iroffset", 1)
			.attribLocation("icid", 2)
			);
	}
	catch (gl::GlslProgCompileExc e) {
		console() << e.what() << endl;
	}

	float rotz_rate = 0.01;
	float rotz_matrix[16] = {
		cos(rotz_rate), -sin(rotz_rate), 0,              0,
		sin(rotz_rate),  cos(rotz_rate), 0,              0,
		0,               0,              1,              0,
		0,               0,              0,              1,
	};
	mat4 Zrot_Matrix = glm::make_mat4(rotz_matrix);
	mUpdateShader->uniform("zrot", Zrot_Matrix);

	mShader->uniform("numDot", NUM_PIX);
	mShader->uniform("colorMap", mColorBuffer, NUM_PIX);

	crad = side;
	mShader->uniform("cradius", crad);

	try {
		mCapture = Capture::create(2, 2);
		mCapture->start();
	}
	catch (ci::Exception& exc) {
		CI_LOG_EXCEPTION("Failed to init capture ", exc);
	}

 	console() << formatString("hello", 123) << std::endl;

#if ! defined( CINDER_GL_ES )
	ImGui::Initialize();
#endif
}

void TestCamApp::mouseDown( MouseEvent event )
{
}

void TestCamApp::update()
{
#if defined( USE_HW_TEXTURE )
	if (mCapture && mCapture->checkNewFrame()) {
		mTexture = mCapture->getTexture();
	}
#else
	

	float rotxrate = 0.01f;
	float rotyrate = 0.01f;
	float rotzrate = 0.01f;
	vec3 curr = mCam.getEyePoint();
	//mCam.setEyePoint(vec3((curr.x * cos(rotxrate)) + (curr.z * sin(rotzrate)), curr.y, (curr.x * -sin(rotxrate)) + (curr.z * cos(rotzrate))));

	double dt1 = getElapsedSeconds();
	
	{
	// Update particles on the GPU
	gl::ScopedGlslProg prog(mUpdateShader);
	gl::ScopedState rasterizer(GL_RASTERIZER_DISCARD, true);	// turn off fragment stage

	// Bind the source data (Attributes refer to specific buffers).
	gl::ScopedVao source(mAttributes[mSourceIndex]);
	// Bind destination as buffer base.
	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mParticleBuffer[mDestinationIndex]);
	gl::beginTransformFeedback(GL_POINTS);

	// Draw source into destination, performing our vertex transformations.
	gl::drawArrays(GL_POINTS, 0, NUM_DOT);

	gl::endTransformFeedback();


	// Swap source and destination for next loop
	uint32_t tmp = mDestinationIndex;
	mDestinationIndex = mSourceIndex;
	mSourceIndex = tmp;
	}
	
	
	ImGui::Begin("Particle Properties");
	ImGui::SliderFloat("", &crad, MIN_RAD, MAX_RAD, "%3f", 1.0f);
	ImGui::End();

	double dt2 = getElapsedSeconds();
	//console() << "update particles time: "(dt2 - dt1) * 1000 << " ms" << endl;


	if (mCapture && mCapture->checkNewFrame()) {
		if (!mTexture) {
			// Capture images come back as top-down, and it's more efficient to keep them that way
			mTexture = gl::Texture::create(*mCapture->getSurface(), gl::Texture::Format().loadTopDown());
		}
		else {
			
			mTexture->update(*mCapture->getSurface());
			
			Surface8uRef bitmap = mCapture->getSurface();
			Area area(0, 0, 640, 480);
			Surface* ptr = &*bitmap;
			//Twirl(ptr, area, 2);
			mTexture->update(*bitmap);

			for (int i = 0;i < (640 / rescaler) ;i++)
			{
				for (int j = 0;j < (480 / rescaler) ;j++)
				{
					int ii = i * rescaler;
					int jj = j * rescaler;
					colorBuffer[i][j] = mCapture->getSurface()->areaAverage(Area(ii, jj, ii + rescaler - 1, jj + rescaler - 1));
					
					int idx = (j * (640 / rescaler)) + i;
					mColorBuffer[idx] = vec4(colorBuffer[i][j].get(ColorModel()), 1.0);
					
				}

			}

		}
		//console() << mTexture->getWidth() << " " << mTexture->getHeight() << endl;
		
	}

	
	/*
	crad += rad_change_rate;
	if (crad >= MAX_RAD)
	{
		rad_change_rate = -0.00005;
	}
	if (crad <= MIN_RAD)
	{
		rad_change_rate = 0.00005;
	}
	*/

	mShader->uniform("cradius", crad);
	
#endif

}

void TestCamApp::draw()
{
	gl::clear();
	float boxSize = 0.25f;
	float gridSize = 1.0f;
	double drawtime = 0;
	
	mCam.lookAt(vec3(0, 0, 3), vec3(0));
	gl::setMatrices(mCam);
	mat4 viewMatrix = gl::getViewMatrix();
	mat4 projectMatrix = gl::getProjectionMatrix();
	mShader->uniform("view", viewMatrix);
	mShader->uniform("projection", projectMatrix);
	
	//gl::setMatricesWindowPersp(getWindowSize(), 60.0f, 1.0f, 10000.0f);
	//gl::setMatricesWindow(getWindowCenter());
	gl::enableDepthRead();
	gl::enableDepthWrite();

	// Enable additive blending.
	gl::ScopedBlendAdditive blend;

	calAndDisplayFPS(true);

	if (mTexture) {
		//gl::ScopedModelMatrix modelScope;
#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
		// change iphone to landscape orientation
		gl::rotate(M_PI / 2);
		gl::translate(0, -getWindowWidth());

		Rectf flippedBounds(0, 0, getWindowHeight(), getWindowWidth());
#if defined( CINDER_ANDROID )
		std::swap(flippedBounds.y1, flippedBounds.y2);
#endif
		gl::draw(mTexture, flippedBounds);
#else
		// gl::draw(mTexture);
		double dt1 = getElapsedSeconds();
		
		gl::ScopedGlslProg render(mShader);
		mShader->uniform("colorMap", mColorBuffer, NUM_PIX);
		gl::ScopedVao vao(mAttributes[mSourceIndex]);
		
		//gl::drawArrays(GL_POINTS, 0, NUM_DOT);
		gl::drawElements(GL_TRIANGLES, NUM_PIX * 3 * SUB_DIV, GL_UNSIGNED_INT, indices.data());

		double dt2 = getElapsedSeconds();
		drawtime = (dt2 - dt1) * 1000;

		//console() << "drawtime: " << drawtime << " ms" << endl;
#endif
	}


	// log data to monitor
	/*
	gl::setMatricesWindow(getWindowSize());
	TextBox tb;

	tb.setText(formatString("fps: ", lastFramerate, "\n"));
	tb.appendText(formatString("drawtime (ms): ", drawtime, "\n"));
	tb.appendText(formatString("color cal time (ms): ", lastCalColorTime, "\n"));
	
	tb.setSize(ivec2(1000, 1000));
	tb.setBackgroundColor(cinder::ColorA(1, 1, 1, 0));
	tb.setColor(cinder::ColorA(255,255,255,1));
	tb.setFont(cinder::Font("Arial", 40));
	Surface8u textSurface = tb.render(vec2(10, 10));
	gl::Texture2dRef texRef = gl::Texture2d::create(textSurface);
	Rectf drawRect(0, 0, texRef->getSize().x,
		texRef->getSize().y);
	gl::draw(texRef, drawRect);
	*/
	
}

void TestCamApp::calAndDisplayFPS(bool logToConsole) {
	double now = getElapsedSeconds();
	++framepersec;
	if (now - lastTime > 1.0) {
		lastTime = now;
		lastFramerate = framepersec;
		if(logToConsole) console() << framepersec << endl;
		framepersec = 0;
	}
}

template <typename T>
std::string formatString(T t)
{
	ostringstream ss;
	ss << t ;
	return ss.str();
}


template<typename T, typename... Args>
std::string formatString(T t, Args... args) {
	ostringstream ss;
	ss << t << formatString(args...);
	return ss.str();
}



CINDER_APP( TestCamApp, RendererGl )
