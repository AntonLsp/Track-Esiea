#pragma once
#include<iostream>
#include<cstdio>
#include<math.h>
#include<opencv2\opencv.hpp>
#include<opencv2\world.hpp>

#define PI 3.1415926536
#define BALL_RADIUS 2.0
#define PSEYE_FOCAL 550.0

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
	Vec3i get_hsv_color(Point2i coordinates);
	Vec3i get_filter_color();
	Vec3i get_filter_range();
	Point3f get_2D_position();
	Point3f get_position();
	Point3f get_speed();

	Mat& get_video_frame();
	Mat& get_binary_frame();

	void get_delta_time(int& cur, int& las);

private :

	// Variables accessibles via les fonctions d'acc�s
	VideoCapture m_videoCap;
	Mat m_videoFrame;
	Mat m_hsvFrame;
	Mat m_filteredFrame;
	Point3f m_2Dposition;
	Point3f m_position;
	Point3f m_speed;

	// R�glages
	float m_focalLength;
	float m_ballRadius;
	Vec3i m_hsvRange;
	Vec3i m_filterColor;

	// Variables priv�es
	Point3f m_lastPosition;
	int64 m_currentTick;
	int64 m_lastTick;
	float m_deltaTime;
	int last;
	int current;

	// Fonctions priv�es
	void color_filtering(Mat& videoFrame, Vec3i hsvRange, Scalar filterColor, Mat& filteredFrame);
	void circle_fitting(Point3f& circleCoord, Mat& filteredFrame);
	void mono_position_estimation(float focal, Point3f circleCoord, Point3f& outPosition);
	std::vector<Point> get_largest_contour(std::vector<std::vector<Point> > contours);
	
};

