#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/CinderImGui.h"


using namespace ci;
using namespace ci::app;
using namespace std;

template <typename T>
std::string formatString(T t);

template<typename T, typename... Args>
std::string formatString(T t, Args... args);

struct FrameCounter {
	int frame;
	int frame_amt;
	void inc() {
		frame++;
		if (frame >= frame_amt) {
			frame = 0;
		}
	}
};

struct ParticleVertex
{
	vec3 center;
	vec3 offset;
	float colorid;
	vec3 pcenter;
	float counter;
};

const int MAX_PARTICLE = 15000;
const int SUB_DIV = 4;
int NUM_DOT;
int NUM_PIX;


class TestVidApp : public App {
	public:
		void setup() override;
		void update() override;
		void draw() override;
		string intToLen2Str(int x);
	private:
		// Descriptions of particle data layout.
		gl::VaoRef		mAttributes[2];
		// Buffers holding raw particle data on GPU.
		gl::VboRef		mParticleBuffer[2];

		// Current source and destination buffers for transform feedback.
		// Source and destination are swapped each frame after update.
		std::uint32_t	mSourceIndex = 0;
		std::uint32_t	mDestinationIndex = 1;
		gl::VboRef      mIndexBuffer;

		gl::GlslProgRef       mShader;
		gl::GlslProgRef       mUpdateShader;

		bool                useCam = false;
		CaptureRef			mCapture;
		Surface8uRef        mSurface[16];
		gl::TextureRef		mTexture[16];
		gl::TextureRef      currentTex;
		Surface8uRef        currentSurface;

		vector<ParticleVertex>  Vertices;
		vector<unsigned int>    Indices;
		unsigned int*           mIndicesPtr;
		ParticleVertex*			mVerticesPtr;

		int                 rescaler = 3;
		int                 canvas_w;
		int                 canvas_h;
		float               crad = 1.0f;
		float               border_w_mult = 1.0f;
		float               border_h_mult = 1.0f;
		int                 mCounter = 0;
		bool                render_bin = true;
		float               binThreshold = 0.38f;
		bool                binReverse = true;
		bool                mUseNoise;
		float               noise_multiplier;
		float               noise_teps;
		float               mSpeedMultiplier = 256.0f;

		Color8u             colorBuffer[140][140];
		vec4*               mColorBuffer;
		float*              mPosBuffer;

		CameraPersp		    mCam;
		float               baseZcam = 1.0f;
		float               zcam = 3.5f;

		float               mSparseMultiplier = 1.0f;

		// Mouse state suitable for passing as uniforms to update program
		bool				mMouseDown = false;
		float				mMouseForce = 0.0f;
		vec3				mMousePos = vec3(0, 0, 0);

		FrameCounter        frameCounter;
		float               frameDelay = 0.06f;
		float               lastTime = 0;
		int                 currentFrame = 0;
		int                 lastFrame = 0;
		int                 cframeWidth = 0;
		int                 cframeHeight = 0;

		float               lastFrameReportTime = 0;
		int                 lastFrameRate;
		int                 framepersec = 0;
		void                calFrameRate();
};

void TestVidApp::setup()
{
	console() << "Start Setup" << endl;

	frameCounter.frame = 0;
	frameCounter.frame_amt = 16;

	try {
		for (int i = 0;i < 16;i++) {
			fs::path path = formatString("C:/Users/bbom/coding-stuff/cinder/TestVid/assets/frame_", intToLen2Str(i), "_delay.gif");
			if (!path.empty()) {
				mSurface[i] = Surface8uRef(new Surface8u(loadImage(path)));
				mTexture[i] = gl::Texture::create(loadImage(path));
			}
		}
		
	}
	catch (Exception& exc) {
		CI_LOG_EXCEPTION("failed to load image.", exc);
	}

	currentTex = mTexture[0];
	
	console() << "Load Image Complete" << endl;
	//Prepare Particles;
	Vertices.reserve(MAX_PARTICLE * (SUB_DIV + 1));
	Indices.reserve(MAX_PARTICLE * 3 * SUB_DIV);
	

	canvas_w = currentTex->getBounds().getWidth() / rescaler;
	canvas_h = currentTex->getBounds().getHeight() / rescaler;

	if (useCam) {
		rescaler = 8;
		canvas_w = 640 / rescaler;
		canvas_h = 480 / rescaler;
	}

	NUM_PIX = canvas_w * canvas_h;
	NUM_DOT = NUM_PIX * (SUB_DIV + 1);
	Indices.resize(NUM_PIX * 3 * SUB_DIV);
	Vertices.resize(NUM_DOT);
	mColorBuffer = new vec4[NUM_PIX];
	mPosBuffer = new float[NUM_PIX];

	//use normalized gap;
	float gap_w = 1.0f / (float) canvas_w;
	float gap_h = 1.0f / (float) canvas_h;
	float side = std::min(gap_w, gap_h) / 2.75;
	try {
		for (int i = 0; i < canvas_w; i++) {
			for (int j = 0; j < canvas_h; j++) {
				vec3 cent = vec3(0, 0, 0);
				int idx = (j * canvas_w) + i;
				int og = idx;
				int idxx = idx * 3 * SUB_DIV;
				idx *= (SUB_DIV + 1);
				auto& v = Vertices.at(idx);
				vec3 posi = cent + vec3((-(canvas_w / 2) + i) * gap_w, ((canvas_h / 2) -j) * gap_h, 0.0f) + vec3(0, 0, 0);
				v.center = posi;
				v.offset = vec3(0, 0, 0);
				v.colorid = (float)og;
				v.pcenter = posi;
				v.counter = 0.0f;


				for (int k = 0;k < SUB_DIV;k++) {
					float rel = k / (float)SUB_DIV;
					float angle = rel * M_PI * 2;
					vec2 offset(cos(angle), sin(angle));
					//vec3 rad_pos = posi + (vec3(offset, 0) * side);
					auto& vv = Vertices.at(idx + k + 1);
					vv.center = posi;
					vv.offset = vec3(offset, 0);
					vv.colorid = (float)og;
					vv.pcenter = posi;
					vv.counter = 0.0f;
				}

				for (int k = 1;k < SUB_DIV;k++) {
					Indices[idxx] = idx;
					Indices[idxx + 1] = idx + k;
					Indices[idxx + 2] = idx + k + 1;
					idxx += 3;
				}

				Indices[idxx] = idx;
				Indices[idxx + 1] = idx + SUB_DIV;
				Indices[idxx + 2] = idx + 1;
			}
		}
	}
	catch (exception e) {
		console() << "Setup Error\n" << e.what() << endl;
	}
	mVerticesPtr = Vertices.data();
	mIndicesPtr = Indices.data();

	console() << "Prepare Vertices Data Complete" << endl;

	// Create particle buffers on GPU and copy data into the first buffer.
	// Mark as static since we only write from the CPU once.
	mParticleBuffer[mSourceIndex] = gl::Vbo::create(GL_ARRAY_BUFFER, Vertices.size() * sizeof(ParticleVertex), mVerticesPtr, GL_STATIC_DRAW);
	mParticleBuffer[mDestinationIndex] = gl::Vbo::create(GL_ARRAY_BUFFER, Vertices.size() * sizeof(ParticleVertex), nullptr, GL_STATIC_DRAW);
	mIndexBuffer = gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NUM_PIX * 3 * SUB_DIV, mIndicesPtr, GL_STATIC_DRAW);

	for (int i = 0; i < 2; ++i)
	{	// Describe the particle layout for OpenGL.
		mAttributes[i] = gl::Vao::create();
		gl::ScopedVao vao(mAttributes[i]);

		// Define attributes as offsets into the bound particle buffer
		gl::ScopedBuffer buffer(mParticleBuffer[i]);
		gl::enableVertexAttribArray(0);
		gl::enableVertexAttribArray(1);
		gl::enableVertexAttribArray(2);
		gl::enableVertexAttribArray(3);
		gl::enableVertexAttribArray(4);
		gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (const GLvoid*)offsetof(ParticleVertex, center));
		gl::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (const GLvoid*)offsetof(ParticleVertex, offset));
		gl::vertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (const GLvoid*)offsetof(ParticleVertex, colorid));
		gl::vertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (const GLvoid*)offsetof(ParticleVertex, pcenter));
		gl::vertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (const GLvoid*)offsetof(ParticleVertex, counter));
		

		//gl::ScopedBuffer buff(mIndexBuffer);
		//gl::bindBuffer(mIndexBuffer);
	}

	// Create Custom Shader
	try {

		mShader = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(CI_GLSL(330,
				uniform mat4 ciModelViewProjection;
		uniform mat4 view;
		uniform mat4 projection;
		uniform vec4 colorMap[12288];
		uniform float[12288] ispresent;
		uniform float cradius;
		uniform float w_mult;
		uniform float h_mult;
		uniform float rad_mult;
		layout(location = 0) in vec4 center;
		layout(location = 1) in vec4 roffset;
		layout(location = 2) in float iid;
		layout(location = 3) in vec3 pcenter;
		layout(location = 4) in float counter;
		out vec4 color;




		void main(void) {
			vec4 cent = center;
			cent = vec4(center.x * w_mult, center.y * h_mult, center.z, center.w);
			vec4 pos = cent + (roffset * cradius * rad_mult);
			pos = projection * view * pos;
			gl_Position = pos;
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
		uniform float[12288] ispresent;
		uniform int presentfound;
		uniform int unicounter;
		uniform int pix_w;
		uniform int pix_h;
		uniform int num_pix;
		uniform float gap_w;
		uniform float gap_h;
		uniform float speed_m;
		in vec3 icenter;
		in vec3 iroffset;
		in float icid;
		in vec3 ipcenter;
		in float icounter;

		out vec3 center;
		out vec3 offset;
		out float colorid;
		out vec3 pcenter;
		out float counter;

		const float dt2 = 1.0 / (60.0 * 60.0);
		const float min_speed = 0.001;

		vec4 permute(vec4 x) { return mod(((x * 34.0) + 1.0) * x, 289.0); }
		vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }
		vec2 fade(vec2 t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
		vec3 fade(vec3 t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

		float cnoise(vec2 P) {
			vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
			vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
			Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
			vec4 ix = Pi.xzxz;
			vec4 iy = Pi.yyww;
			vec4 fx = Pf.xzxz;
			vec4 fy = Pf.yyww;
			vec4 i = permute(permute(ix) + iy);
			vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
			vec4 gy = abs(gx) - 0.5;
			vec4 tx = floor(gx + 0.5);
			gx = gx - tx;
			vec2 g00 = vec2(gx.x, gy.x);
			vec2 g10 = vec2(gx.y, gy.y);
			vec2 g01 = vec2(gx.z, gy.z);
			vec2 g11 = vec2(gx.w, gy.w);
			vec4 norm = 1.79284291400159 - 0.85373472095314 *
				vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
			g00 *= norm.x;
			g01 *= norm.y;
			g10 *= norm.z;
			g11 *= norm.w;
			float n00 = dot(g00, vec2(fx.x, fy.x));
			float n10 = dot(g10, vec2(fx.y, fy.y));
			float n01 = dot(g01, vec2(fx.z, fy.z));
			float n11 = dot(g11, vec2(fx.w, fy.w));
			vec2 fade_xy = fade(Pf.xy);
			vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
			float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
			return 2.3 * n_xy;
		}

		float cnoise(vec3 P) {
			vec3 Pi0 = floor(P); // Integer part for indexing
			vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
			Pi0 = mod(Pi0, 289.0);
			Pi1 = mod(Pi1, 289.0);
			vec3 Pf0 = fract(P); // Fractional part for interpolation
			vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
			vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
			vec4 iy = vec4(Pi0.yy, Pi1.yy);
			vec4 iz0 = Pi0.zzzz;
			vec4 iz1 = Pi1.zzzz;

			vec4 ixy = permute(permute(ix) + iy);
			vec4 ixy0 = permute(ixy + iz0);
			vec4 ixy1 = permute(ixy + iz1);

			vec4 gx0 = ixy0 / 7.0;
			vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
			gx0 = fract(gx0);
			vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
			vec4 sz0 = step(gz0, vec4(0.0));
			gx0 -= sz0 * (step(0.0, gx0) - 0.5);
			gy0 -= sz0 * (step(0.0, gy0) - 0.5);

			vec4 gx1 = ixy1 / 7.0;
			vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
			gx1 = fract(gx1);
			vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
			vec4 sz1 = step(gz1, vec4(0.0));
			gx1 -= sz1 * (step(0.0, gx1) - 0.5);
			gy1 -= sz1 * (step(0.0, gy1) - 0.5);

			vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
			vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
			vec3 g010 = vec3(gx0.z, gy0.z, gz0.z);
			vec3 g110 = vec3(gx0.w, gy0.w, gz0.w);
			vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
			vec3 g101 = vec3(gx1.y, gy1.y, gz1.y);
			vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
			vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);

			vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
			g000 *= norm0.x;
			g010 *= norm0.y;
			g100 *= norm0.z;
			g110 *= norm0.w;
			vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
			g001 *= norm1.x;
			g011 *= norm1.y;
			g101 *= norm1.z;
			g111 *= norm1.w;

			float n000 = dot(g000, Pf0);
			float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
			float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
			float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
			float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
			float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
			float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
			float n111 = dot(g111, Pf1);

			vec3 fade_xyz = fade(Pf0);
			vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
			vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
			float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
			return 2.2 * n_xyz;
		}

		float rand(vec2 co) {
			return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
		}


		float findPos() {
			int count = 0;
			float mini = -1.0f;
			float minid = 0.0f;
			for (int i = 0;i < presentfound;i++) {
				int iid = int(ispresent[i]);
				int targi = iid % pix_w;
				int targj = int(int(iid - targi) / pix_w);
				vec3 targ = vec3(0) + vec3((-(pix_w / 2) + targi) * gap_w, ((pix_h / 2) - targj) * gap_h, 0.0f) + vec3(0, 0, 0);

				iid = int(icid);
				int posi = iid % pix_w;
				int posj = int(int(icid - posi) / pix_w);
				float dis = length(vec2(posi, posj) - vec2(targi, targj));
				if (dis < mini || mini < 0) {
					mini = dis;
					minid = ispresent[i];
				}
			}
			return minid;
		}

		void main(void) {
			center = icenter;
			offset = iroffset;
			pcenter = ipcenter;
			colorid = icid;
			counter = icounter;

			int iid = int(icid);
			int posi = iid % pix_w;
			int posj = int(int(icid - posi) / pix_w);
			vec3 cent = vec3(0, 0, 0);
			vec3 home = cent + vec3((-(pix_w / 2) + posi) * gap_w, ((pix_h / 2) + 6) * gap_h, 0.0f) + vec3(0, 0, 0);
			
			//vec2 result = findPos();
			vec2 result = vec2(findPos(), 0.0f);
			if (result.x > 0.0f) result.y = 1.0f;
			iid = int(result.x);
			posi = iid % pix_w;
			posj = int(int(iid - posi) / pix_w);
			vec3 newpos = cent + vec3((-(pix_w / 2) + posi) * gap_w, ((pix_h / 2) - posj) * gap_h, 0.0f) + vec3(0, 0, 0);

			vec3 target = home;
			if (result.y) target = newpos;

			float speed_mult = speed_m;
			//if (result.y == 0.0f) speed_mult = 1.0f;

			float steps = 10.0f;
			float noisex = cnoise(vec2(center.x * steps, center.y * steps));
			float noisey = cnoise(vec2(-center.y * steps, -center.x * steps));

			vec3 rando = vec3(noisex, noisey, 0.0f);
			float noise_mult = 12.0f;
			rando *= noise_mult;

			pcenter = center;
			vec3 acc = (target - center) * speed_mult;
			vec3 go = acc * dt2;
			go += (rando * 0.01 * length(target - center) * length(target - center));
			//if (length(go) < min_speed) go = go * (min_speed / length(go));
			center += go;
			//if (result.y == 0.0f) center = home;
			//center = target;

		}
		))
			.feedbackFormat(GL_INTERLEAVED_ATTRIBS)
			.feedbackVaryings({ "center", "offset", "colorid", "pcenter", "counter" })
			.attribLocation("icenter", 0)
			.attribLocation("iroffset", 1)
			.attribLocation("icid", 2)
			.attribLocation("ipcenter", 3)
			.attribLocation("icounter", 4)
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
	mUpdateShader->uniform("unicounter", (int)0);
	mUpdateShader->uniform("pix_w", canvas_w);
	mUpdateShader->uniform("pix_h", canvas_h);
	mUpdateShader->uniform("num_pix", NUM_PIX);
	mUpdateShader->uniform("gap_w", gap_w);
	mUpdateShader->uniform("gap_h", gap_h);
	mUpdateShader->uniform("ispresent", mPosBuffer, NUM_PIX);
	mUpdateShader->uniform("presentfound", (int)0);

	mShader->uniform("colorMap", mColorBuffer, NUM_PIX);
	mShader->uniform("ispresent", mPosBuffer, NUM_PIX);
	border_h_mult = 1.2;
	border_w_mult = 1.6;
	mShader->uniform("w_mult", border_w_mult);
	mShader->uniform("h_mult", border_h_mult);
	mShader->uniform("cradius", side);
	mShader->uniform("rad_mult", 1.0f);

	try {
		mCapture = Capture::create(2, 2);
		mCapture->start();
	}
	catch (ci::Exception& exc) {
		CI_LOG_EXCEPTION("Failed to init capture ", exc);
	}


	// Listen to mouse events so we can send data as uniforms.
	getWindow()->getSignalMouseDown().connect([this](MouseEvent event)
		{
			mMouseDown = true;
			mMouseForce = 500.0f;
			mMousePos = vec3(event.getX(), event.getY(), 0.0f);
		});
	getWindow()->getSignalMouseDrag().connect([this](MouseEvent event)
		{
			mMousePos = vec3(event.getX(), event.getY(), 0.0f);
		});
	getWindow()->getSignalMouseUp().connect([this](MouseEvent event)
		{
			mMouseForce = 0.0f;
			mMouseDown = false;
		});


#if ! defined( CINDER_GL_ES )
	ImGui::Initialize();
#endif

	console() << "Setup Complete" << endl;
}



void TestVidApp::update()
{
	ImGui::Options options;
	ImGui::Begin("Frame Control");
	ImGui::Text("FPS: %d", lastFrameRate);
	ImGui::Text("Frame Size: %d x %d", cframeWidth, cframeHeight);
	ImGui::SliderFloat("Set Delay", &frameDelay, 0.01667f, 1.0f, "%3f", 10.0f);
	ImGui::SliderFloat("Scale Circle Radius", &crad, 1.0f, 10.0f, "%3f", 1.0f);
	ImGui::SliderFloat("Sparse Multiplier", &mSparseMultiplier, 1.0f, 15.0f, "%3f", 1.0f);
	ImGui::SliderFloat("Zoom", &zcam, 0.1f, 35.0f, "%3f", 1.0f);
	ImGui::SliderFloat("Scale Width", &border_w_mult, 0.5f, 3.0f, "%3f", 1.0f);
	ImGui::SliderFloat("Scale Height", &border_h_mult, 0.5f, 3.0f, "%3f", 1.0f);
	ImGui::SliderFloat("Speed Multiplier", &mSpeedMultiplier, 64.0f, 1288.0f, "%3f", 1.0f);
	ImGui::SliderFloat("Binary Threshold", &binThreshold, 0.0f, 1.0f, "%3f", 1.0f);
	ImGui::Checkbox("Render as Binary", &render_bin);
	ImGui::Checkbox("Reverse Binary", &binReverse);
	ImGui::End();

	if (true) {
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


	double now = getElapsedSeconds();
	if (now - lastTime > frameDelay) {
		lastTime = now;
		frameCounter.inc();
		currentFrame = frameCounter.frame;
		currentTex = mTexture[frameCounter.frame];
		currentSurface = mSurface[frameCounter.frame];
	}
	cframeWidth = currentTex->getBounds().getWidth();
	cframeHeight = currentTex->getBounds().getHeight();

	int pres_count = 0;
	try {
		delete[] mPosBuffer;
		mPosBuffer = new float[NUM_PIX];
		for (int i = 0; i < canvas_w;i++) {
			for (int j = 0;j < canvas_h;j++) {
				int ii = i * rescaler;
				int jj = j * rescaler;
				if (useCam) currentSurface = mCapture->getSurface();
				colorBuffer[i][j] = currentSurface->areaAverage(Area(ii, jj, ii + rescaler - 1, jj + rescaler - 1));
				
				int idx = (j * canvas_w) + i;
				mColorBuffer[idx] = vec4(colorBuffer[i][j].get(ColorModel()), 1.0);
				vec4 c = mColorBuffer[idx];
				float ave = 0.299 * c.x + 0.587 * c.y + 0.114 * c.z;
				float bw;
				if (binReverse) bw = ave >= binThreshold ? 0.0f : 1.0f;
				else bw = ave >= binThreshold ? 1.0f : 0.0f;
				mColorBuffer[idx] = vec4(colorBuffer[i][j].get(ColorModel()), 1.0);
				if (render_bin) mColorBuffer[idx] = vec4(1.0f);
				//mPosBuffer[idx] = bw;
				if (bw) {
					mPosBuffer[pres_count] = idx;
					pres_count++;
				}
			}
		}
	} 
	catch (exception e) {
		console() << "update error" << endl;
	}
	//update uniform values
	mUpdateShader->uniform("ispresent", mPosBuffer, NUM_PIX);
	mUpdateShader->uniform("unicounter", mCounter);
	mUpdateShader->uniform("presentfound", pres_count);
	mUpdateShader->uniform("speed_m", mSpeedMultiplier);
	mShader->uniform("ispresent", mPosBuffer, NUM_PIX);
	mShader->uniform("rad_mult", crad);
	mShader->uniform("w_mult", border_w_mult * mSparseMultiplier);
	mShader->uniform("h_mult", border_h_mult * mSparseMultiplier);

	//zcam = baseZcam * mSparseMultiplier;

}

void TestVidApp::draw()
{
	gl::clear(Color(0, 0, 0));

	mCam.lookAt(vec3(0, 0, zcam), vec3(0));
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
		
		//gl::clear(Color(0, 0, 0));
		//gl::draw(currentTex);

		gl::ScopedGlslProg render(mShader);
		mShader->uniform("colorMap", mColorBuffer, NUM_PIX);
		gl::ScopedVao vao(mAttributes[mSourceIndex]);

		////gl::drawArrays(GL_POINTS, 0, NUM_DOT);
		gl::drawElements(GL_TRIANGLES, NUM_PIX * 3 * SUB_DIV, GL_UNSIGNED_INT, mIndicesPtr);


		//console() << "drawtime: " << drawtime << " ms" << endl;
#endif
	}

	calFrameRate();
}


void TestVidApp::calFrameRate() {
	if (!useCam && currentFrame != lastFrame) {
		float now = getElapsedSeconds();
		framepersec++;
		if (now - lastFrameReportTime > 1.0f) {
			lastFrameReportTime = now;
			lastFrameRate = framepersec;
			framepersec = 0;
		}
		lastFrame = currentFrame;

		return;
	}
	else {
		float now = getElapsedSeconds();
		framepersec++;
		if (now - lastFrameReportTime > 1.0f) {
			lastFrameReportTime = now;
			lastFrameRate = framepersec;
			framepersec = 0;
		}
		lastFrame = currentFrame;
	}
}

string TestVidApp::intToLen2Str(int x) {
	if (x < 10) return formatString("0", x);
	else return formatString(x);
}

template <typename T>
std::string formatString(T t)
{
	ostringstream ss;
	ss << t;
	return ss.str();
}


template<typename T, typename... Args>
std::string formatString(T t, Args... args) {
	ostringstream ss;
	ss << t << formatString(args...);
	return ss.str();
}

CINDER_APP( TestVidApp, RendererGl )
