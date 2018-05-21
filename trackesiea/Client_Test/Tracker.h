#pragma once
#include<iostream>
#include<cstdio>
#include<math.h>
#include<opencv2\opencv.hpp>
#include<opencv2\world.hpp>
#include"ps3eye.h"

#define PI 3.1415926536
#define BALL_RADIUS 2.0
#define PSEYE_FOCAL 550.0
#define USE_PS3EYEDRIVER

#define LOWPASS_ALPHA 0.5
#define Z_LOWPASS_SMOOTHING 0.5

// PS3Eye Driver
struct ps3eye_context {
	ps3eye_context(int width, int height, int fps) :
		eye(0)
		, devices(ps3eye::PS3EYECam::getDevices())
		, running(true)
		, last_ticks(0)
		, last_frames(0)
	{
		if (hasDevices()) {
			eye = devices[0];
			eye->init(width, height, (uint16_t)fps);
		}
	}

	bool hasDevices()
	{
		return (devices.size() > 0);
	}

	std::vector<ps3eye::PS3EYECam::PS3EYERef> devices;
	ps3eye::PS3EYECam::PS3EYERef eye;

	bool running;
	unsigned int last_ticks;
	unsigned int last_frames;
};

/*
Cette classe a pour r�le d'effectuer le tracking 3D de la sph�re.
*/

using namespace cv;

//Membres de la classe Tracker
class Tracker
{
public:
	Tracker::Tracker();
	Tracker::Tracker(float ballRadius, float cameraFocalLength);
	Tracker::~Tracker();
	void init_tracker(int cameraIndex,bool stereo);
	void track();
	void set_filter_color(Vec3i color);
	void set_filter_range(Vec3i hsvrange);
	void set_gain(int gain);
	void set_exposure(int exposure);
	Vec3i get_hsv_color(Point2i coordinates);
	Vec3i get_fullscreen_average_hsv_color();
	Vec3i get_filter_color();
	Vec3i get_filter_range();
	Point3f get_2D_position();
	Point3f get_position();
	Point3f get_speed();
	bool is_tracking_valid();

	Mat& get_video_frame();
	Mat& get_binary_frame();
	Mat& get_hsv_frame();

	enum filterType{simple_lowpass,multi_channel_lowpass,noFiltering};
	filterType m_filteringType;

	void set_delta_time(int64* m_currentTick,int64* las);

private :

	// Variables accessibles via les fonctions d'acc�s
	bool initialized;
	VideoCapture m_videoCap;
	Mat m_videoFrame;
	Mat m_hsvFrame;
	Mat m_filteredFrame;
	Point3f m_2Dposition;
	Point3f m_position;
	Point3f m_speed;
	bool m_isTrackingValid;

	int m_exposure;
	int m_gain;

	// R�glages
	float m_focalLength;
	float m_ballRadius;
	Vec3i m_hsvRange;
	Vec3i m_filterColor;

	// Variables priv�es
	Point3f* m_lastPosition;
	double m_deltaTime;
	int64* m_currentTick;
	int64* m_lastTick;

	// Fonctions priv�es
	void color_filtering(Mat& videoFrame, Vec3i hsvRange, Scalar filterColor, Mat& filteredFrame);
	void circle_fitting(Point3f& circleCoord, Mat& filteredFrame);
	void mono_position_estimation(float focal, Point3f circleCoord, Point3f& outPosition);
	std::vector<Point> get_largest_contour(std::vector<std::vector<Point> > contours);

};

