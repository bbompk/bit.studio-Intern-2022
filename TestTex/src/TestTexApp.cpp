#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "Loader.h"
#include "cinder/CinderImGui.h"
#include <opencv2/opencv.hpp>
#include <opencv2/core/opengl.hpp>
#include "MyTexture.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace cv;

int NUM_VERTS;

class TestTexApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

  private:
	  CaptureRef mCapture;
	  gl::Texture2dRef mTex;

	  gl::VaoRef mVao;
	  gl::VboRef mVbo;
	  gl::GlslProgRef mCubeShader;

	  gl::FboRef myFbo;
	  void renderToFbo(gl::Texture2dRef &tx);

	  gl::Texture2dRef mObjtex;
	  gl::Texture2dRef mObjtex_blink;
	  Surface8uRef surface_open;
	  Surface8uRef surface_blink;


	  bool        pause = false;
	  CameraPersp mCam;
	  float       zcam = 2.8f;

	  float               lastFrameReportTime = 0;
	  int                 lastFrameRate;
	  int                 framepersec = 0;
	  void                calFrameRate();
	  void                calDT(string s);

	  bool       use_og = true;

	  float      lastTime = 0;
	  float      anim_duration = 3.5f;
	  float      anim_timer = 0;

	  VideoCapture cap;
	  CVTex* frametex = nullptr;
};

void TestTexApp::setup()
{
	cap = VideoCapture("balj.mp4");
	if (!cap.isOpened()) {
		console() << "vid file not found" << endl;
		throw exception("vid vile not found");
	}

	MyBuffer buffer = ObjParser::loadObj("C:/Users/bbom/coding-stuff/cinder/TestTex/assets/shib2.obj");
	NUM_VERTS = buffer.num_vertices;
	mVao = buffer.vao;
	mVbo = buffer.vbo;

	auto img = loadImage(loadAsset("shib_tex.png"));
    mObjtex = gl::Texture2d::create(img);
	img = loadImage(loadAsset("shib_close.png"));
	mObjtex_blink = gl::Texture2d::create(img);

	surface_blink = Surface8u::create(10, 10, true);

	mObjtex->bind(0);
	mObjtex_blink->bind(1);

	gl::Fbo::Format format;
	myFbo = gl::Fbo::create(mObjtex->getWidth(), mObjtex->getHeight(), format);

	console() << mObjtex->getWidth() << " " << mObjtex->getHeight() << endl;

	try {
		mCubeShader = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(CI_GLSL(330,
				layout(location = 0) in vec3 position;
		layout(location = 1) in vec3 normal;
		layout(location = 2) in vec2 iuvcoord;
		uniform mat4 view;
		uniform mat4 projection;
		out vec3 onormal;
		out vec2 uvcoord;

		void main(void) {
			gl_Position = projection * view * vec4(position, 1);
			onormal = normal;
			uvcoord = iuvcoord;
		}
		))

			.fragment(CI_GLSL(330,
				uniform sampler2D tex0;
		uniform sampler2D tex1;
		uniform vec3 collarcol;
		uniform float timer;
		uniform float anim_duration;
		in vec3 onormal;
		in vec2 uvcoord;
		out vec4 oColor;

		void main(void) {
			float ttime = timer;
			if (ttime > anim_duration) ttime = 0.0f;

			vec3 normal = normalize(-onormal);
			float diffuse = max(dot(normal, vec3(0, 0, -1)), 0);
			
			vec3 color;
			if (ttime <= 2.9f || (ttime > 3.1f && ttime <= 3.2f )) color = texture(tex0, uvcoord).rgb;
			else color = texture(tex1, uvcoord).rgb;


			color = texture(tex0, uvcoord).rgb;
			if(uvcoord.x < 0.293 && uvcoord.y < 0.1472) color = collarcol;

			oColor = vec4(color, 1.0);
		}
		))
			);
	}
	catch (gl::GlslProgCompileExc e) {
		console() << e.what() << endl;
	}

	mCubeShader->uniform("tex0", 0);
	mCubeShader->uniform("tex1", 1);
	mCubeShader->uniform("collarcol", vec3(1));
	mCubeShader->uniform("anim_duration", anim_duration);

	try {
		mCapture = Capture::create(2, 2);
		mCapture->start();
	}
	catch (ci::Exception& exc) {
		console() << "Failed to init capture \n" <<  exc.what() <<  endl;
	}

	mCam.lookAt(vec3(4, 2, 3), vec3(0));
#if ! defined( CINDER_GL_ES )
	ImGui::Initialize();
#endif
}

void TestTexApp::mouseDown( MouseEvent event )
{
}

void changeCollarColor(Surface8uRef& surf) {
	int centisec = getElapsedSeconds() * 100;
	float rel = (centisec % 300) / 300.0f;

	for (int i = 0;i < 10;i++) {
		for (int j = 0;j < 10;j++) {
			surf->setPixel(vec2(i, j), ColorA8u(Color(CM_HSV, rel, 1, 1)));
		}
	}
}

vec3 changeCollarColor() {
	int centisec = getElapsedSeconds() * 100;
	float rel = (centisec % 300) / 300.0f;
	return ColorA8u(Color(CM_HSV, rel, 1, 1)).get(ColorModel());
}

void TestTexApp::renderToFbo(gl::Texture2dRef &tx) {


	gl::ScopedFramebuffer fbo(myFbo);
	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), myFbo->getSize());
	gl::clear(Color(0, 0, 0));

	
	gl::setMatricesWindow(myFbo->getSize());
	
	gl::draw(tx);
	
	vec3 c = changeCollarColor();

	gl::color(Color(c.x, c.y, c.z));
	gl::drawSolidCircle(getWindowCenter(), 400);
	gl::drawSolidRect(Rectf(0, 0, 200, 200));

}

void BindCVMat2GLTexture(cv::Mat& image, GLuint& imageTexture1)
{
	if (image.empty()) {
		std::cout << "image empty" << std::endl;
	}
	else {
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glGenTextures(1, &imageTexture1);
		glBindTexture(GL_TEXTURE_2D, imageTexture1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Set texture clamping method
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		cv::cvtColor(image, image, COLOR_RGB2BGR);

		glTexImage2D(GL_TEXTURE_2D,         // Type of texture
			0,                   // Pyramid level (for mip-mapping) - 0 is the top level
			GL_RGB,              // Internal colour format to convert to
			image.cols,          // Image width  i.e. 640 for Kinect in standard mode
			image.rows,          // Image height i.e. 480 for Kinect in standard mode
			0,                   // Border width in pixels (can either be 1 or 0)
			GL_RGB,              // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
			GL_UNSIGNED_BYTE,    // Image data type
			image.ptr());        // The actual image data itself
	}
}

void TestTexApp::update()
{
	float now = getElapsedSeconds();
	float diff = now - lastTime;
	anim_timer += diff;
	if (anim_timer > anim_duration) anim_timer = 0;
	mCubeShader->uniform("timer", anim_timer);
	lastTime = now;


	ImGui::Begin("Params");
	ImGui::Checkbox("Pause", &pause);
	ImGui::Text("FPS: %d", lastFrameRate);
	ImGui::SliderFloat("Zoom", &zcam, 0.0f, 4.5f, "%3f", 1.0f);


	ImGui::End();

	if (mCapture && mCapture->checkNewFrame()) {
		if (!mTex) {
			mTex = gl::Texture2d::create(*(mCapture->getSurface()), gl::Texture::Format().loadTopDown());
		}
		else {
			mTex->update(*(mCapture->getSurface()));
		}
	}


	float rotxrate = 0.01f;
	float rotyrate = 0.01f;
	float rotzrate = 0.01f;
	if (!pause)
	{
		vec3 curr = mCam.getEyePoint();
		mCam.setEyePoint(vec3((curr.x * cos(rotxrate)) + (curr.z * sin(rotzrate)), curr.y, (curr.x * -sin(rotxrate)) + (curr.z * cos(rotzrate))));
	}
	mCam.setEyePoint(normalize(mCam.getEyePoint()) * (5 - zcam));
	mCam.lookAt(vec3(0));

	mCubeShader->uniform("collarcol", changeCollarColor());
	
	//renderToFbo(mObjtex);
	//myFbo->getColorTexture()->bind(0);
	//renderToFbo(mObjtex_blink);
	//myFbo->getColorTexture()->bind(1);

	{
		Mat frame;
		// Capture frame-by-frame
		cap >> frame;

		// If the frame is empty, break immediately
		if (frame.empty()) {
			cap.set(CAP_PROP_POS_FRAMES, 0);
			cap >> frame;
		}
		try {

			if (!frametex) {
				frametex = new CVTex(frame);
			}
			else {
				frametex->update(frame);
			}
		}
		catch (exception e) {
			//console() << e.what() << endl;
			throw e;
		}

		frametex->bind(0);
	}

}

void TestTexApp::draw()
{
	gl::clear(Color(0, 0, 0));

	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::setMatrices(mCam);
	mat4 viewMatrix = gl::getViewMatrix();
	mat4 projectMatrix = gl::getProjectionMatrix();
	mCubeShader->uniform("view", viewMatrix);
	mCubeShader->uniform("projection", projectMatrix);

	gl::ScopedGlslProg render(mCubeShader);
	gl::ScopedVao mvao(mVao);

	gl::drawArrays(GL_TRIANGLES, 0, NUM_VERTS);

	gl::Texture2dRef vid = gl::Texture2d::create(0, frametex->getID(), (int) frametex->getWidth(), (int) frametex->getHeight(), true);
	gl::setMatricesWindow(getWindowSize());
	gl::draw(vid, Rectf(0, getWindowHeight() - 100, 100, getWindowHeight()));


	calFrameRate();
}

void TestTexApp::calFrameRate() {
	double now = getElapsedSeconds();
	++framepersec;
	if (now - lastFrameReportTime > 1.0) {
		lastFrameReportTime = now;
		lastFrameRate = framepersec;
		framepersec = 0;
	}
}

void TestTexApp::calDT(string s) {
	static int state = 0;
	static double dt1, dt2;
	if (state == 0) {
		dt1 = getElapsedSeconds();
		state = 1;
	}
	else if (state == 1) {
		dt2 = getElapsedSeconds();
		console() << s << ": " << (dt2 - dt1) * 1000 << "ms" << endl;
		state = 0;
	}
}

CINDER_APP( TestTexApp, RendererGl )
