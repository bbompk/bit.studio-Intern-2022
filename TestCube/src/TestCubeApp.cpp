#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TestCubeApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

  private:
	  CameraPersp	        mCam;
	  gl::BatchRef          mBox;
	  gl::GlslProgRef		mGlsl;
	  vec3                  mLightPos;
};

void TestCubeApp::setup()
{

	
	mLightPos = vec3(0, 2, 2);
	mCam.lookAt(vec3(3, 3, 3), vec3(0));
	auto lambert = gl::ShaderDef().lambert();
	auto shader = gl::getStockShader(lambert);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	try {

		mGlsl = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(CI_GLSL(330,

				uniform mat4	ciModelViewProjection;
				uniform mat4    ciModel;
				in vec4			ciPosition;
				out vec4        mPos;
				in vec4         ciNormal;
				out vec3 FragPos;
				out vec4 Normal;


		void main(void) {
			gl_Position = ciModelViewProjection * ciPosition;
			mPos = gl_Position;
			FragPos = vec3(ciModel * ciPosition);
			Normal = ciNormal;
		}
		))
			.fragment(CI_GLSL(330,
				uniform vec3        lightPos;
				in  vec4            Normal;
				in  vec3            FragPos;
				in  vec4            mPos;
				out vec4			oColor;



		void main(void) {

			vec3 norm = normalize(vec3(Normal));
			vec3 lightDir = normalize(lightPos - FragPos);
			float diff = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diff * vec3(1, 1, 1);
			vec3 result = diffuse * vec3(Normal);
			float av = (Normal.x + Normal.y + Normal.z) / 3;
			
			oColor = vec4(result, 0.5);
		}
		)));
	}
	catch (gl::GlslProgCompileExc exc) {
		console() << exc.what() << endl;
	}

	

	mBox = gl::Batch::create(geom::Teapot(), mGlsl);
	mBox->getGlslProg()->uniform("lightPos", mLightPos);
	
}

void TestCubeApp::mouseDown( MouseEvent event )
{
}

void TestCubeApp::update()
{
	float roty = 0.01;
	vec3 curr = mLightPos;
	mLightPos = vec3((curr.x * cos(roty)) + (curr.z * sin(roty)), curr.y, (curr.x * -sin(roty) + (curr.z * cos(roty))));
	mBox->getGlslProg()->uniform("lightPos", mLightPos);
}

void TestCubeApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	// turn on z-buffering
	gl::enableDepthRead();
	gl::enableDepthWrite();
	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	
	

	gl::setMatrices(mCam);

	mBox->draw();

}

CINDER_APP( TestCubeApp, RendererGl )
