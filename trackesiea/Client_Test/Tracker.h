#pragma once
#include<iostream>
#include<cstdio>
#include<stack>
#include<math.h>
#include<opencv2\opencv.hpp>
#include<opencv2\world.hpp>
#include"ps3eye.h"
#include"libconfig.h++"

#define PI 3.1415926536
#define BALL_RADIUS 2.0
#define PSEYE_FOCAL 550.0
#define USE_PS3EYEDRIVER

// Valeur plus basse -> Filtrage plus important
#define LOWPASS_ALPHA 0.5
#define Z_LOWPASS_SMOOTHING 0.33

#define CONFIG_FILE_PATH "configuration.cfg"

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


struct TrackedBall {

	Vec3i hsvRange;
	Vec3i filterColor;

	Point3f position2D;
	Point3f local_position;
	Point3f world_position;
	Point3f speed;
	bool isTrackingValid;

	Point3f last_world_position;
	Point3f last_local_position;

	Mat filteredFrame;
	std::deque<Point3f> last_world_positions;

};

//Membres de la classe Tracker
class Tracker
{
public:
	Tracker::Tracker();
	Tracker::Tracker(float ballRadius, float cameraFocalLength);
	Tracker::~Tracker();
	void init_tracker(int cameraIndex,bool stereo);
	void track();
	void set_filter_color(int index,Vec3i color);
	void set_filter_range(int index,Vec3i hsvrange);
	void set_gain(int gain);
	void set_exposure(int exposure);
	void set_hq_tracking(bool hqTracking);
	Vec3i get_hsv_color(Point2i coordinates);
	Vec3i get_fullscreen_average_hsv_color();
	Vec3i get_filter_color(int index);
	Vec3i get_filter_range(int index);
	Point3f get_2D_position(int index);
	Point3f get_cam_position(int index);
	Point3f get_world_position(int index);
	Point3f get_speed(int index);
	int get_gain();
	int get_exposure();
	// Retourne la position de la cam�ra en coordonn�es absolues
	Point3f get_camera_world_position();
	void set_world_origin();
	void set_world_zaxis();
	void set_world_xaxis();
	void calibrate_camera_pose();
	float get_tracking_rate();
	Point3f get_variance();
	bool is_hq_tracking();
	bool is_tracking_valid(int index);

	Mat& get_video_frame();
	Mat& get_binary_frame(int index);
	Mat& get_hsv_frame();

	enum filterType{simple_lowpass,multi_channel_lowpass,noFiltering};
	filterType m_filteringType;

	void set_delta_time(int64& m_currentTick,int64& las);

	int resolutionX;
	int resolutionY;

	// Sauvegarde les valeurs actuelles dans un fichier de configuration
	void save_params();

private :

	// Variables accessibles via les fonctions d'acc�s
	bool initialized;
	VideoCapture m_videoCap;
	Mat m_videoFrame;
	Mat m_hsvFrame;
	TrackedBall m_trackedBall[2];

	Matx33f m_camToWorld_rotation;
	Point3f m_cam_world_position;
	Point3f m_world_x_axis;
	Point3f m_world_z_axis;
	Point3f m_world_origin;

	float m_trackingRate;
	int m_exposure;
	int m_gain;
	bool m_hqTracking;

	// R�glages
	float m_focalLength;
	float m_ballRadius;

	// Variables priv�es
	double m_deltaTime;
	int64 m_currentTick;
	int64 m_lastTick;

	/* Fonctions priv�es */
	void color_filtering(Mat& videoFrame, Vec3i hsvRange, Scalar filterColor, Mat& filteredFrame);
	void circle_fitting(Point3f& circleCoord, Mat& filteredFrame, bool& trackingValid);
	void circle_refining(Point3f& circleCoord, Mat& hsvFrame, Vec3i color, Vec3i colorRange, bool& trackingValid);
	
	void mono_position_estimation(float focal, Point3f circleCoord, Point3f& outPosition);
	std::vector<Point> get_largest_contour(std::vector<std::vector<Point> > contours);
	// Enfile les derni�res positions dans la file
	void stackPositions(Point3f pos, std::deque<Point3f>& q, int size);
	// Cr�e un nouveau fichier de configuration avec les param�tres par d�faut
	void new_file_params();
	// Ouvre un fichier de configuration et assigne les variables du programme en cons�quence
	void load_params();
	// Retourne une matrice de rotation autour d'un axe et d'un angle theta
	Matx33f rotation_matrix(Point3f axis, float angle);
	// Produit vectoriel
	Vec3f crossProduct(Point3f a, Point3f b);
	// D�termine la matrice de rotation
	void compute_camToWorld_rotation_matrix(Vec3f z_world_axis, Vec3f x_world_axis, Vec3f world_origin);
	// Transforme la position locale (cam�ra) en position absolue
	Point3f get_world_position(Point3f localPosition);
};

