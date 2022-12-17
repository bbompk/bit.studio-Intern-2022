#pragma once
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include <opencv2/opencv.hpp>

class CVTex {

	private :
		unsigned int m_RendererID;
		size_t m_width;
		size_t m_height;
		bool empty;

	public :

		

		CVTex(cv::Mat& image) : m_RendererID(0), m_width(0), m_height(0), empty(true) {
			createTex(image);
		}

		void createTex(cv::Mat& image) {
			if (image.empty()) {
				empty = true;
			}
			else {
				//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				glGenTextures(1, &m_RendererID);
				glBindTexture(GL_TEXTURE_2D, m_RendererID);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				// Set texture clamping method
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

				cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

				glTexImage2D(GL_TEXTURE_2D,         // Type of texture
					0,                   // Pyramid level (for mip-mapping) - 0 is the top level
					GL_RGB,              // Internal colour format to convert to
					image.cols,          // Image width  i.e. 640 for Kinect in standard mode
					image.rows,          // Image height i.e. 480 for Kinect in standard mode
					0,                   // Border width in pixels (can either be 1 or 0)
					GL_RGB,              // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
					GL_UNSIGNED_BYTE,    // Image data type
					image.ptr());        // The actual image data itself

				empty = false;
				m_width = image.cols;
				m_height = image.rows;

				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

		size_t getWidth() {
			return m_width;
		}

		size_t getHeight() {
			return m_height;
		}

		unsigned int getID() {
			return m_RendererID;
		}

		bool isEmpty() {
			return empty;
		}

		void bind(unsigned int slot = 0) {
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, m_RendererID);
		}

		void unbind() {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void deleteTex() {
			if (!empty) glDeleteTextures(1, &m_RendererID);
			empty = true;
		}

		void update(cv::Mat& image) {
			if (!empty) deleteTex();
			createTex(image);
		}

		~CVTex() {
			if(!empty) deleteTex();
		}
};