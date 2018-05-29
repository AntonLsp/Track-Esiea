#include "Tracker.h"

using namespace std;
using namespace cv;
using namespace libconfig;

/*
Constructeurs
*/

Tracker::Tracker() : m_focalLength(PSEYE_FOCAL), m_ballRadius(BALL_RADIUS), m_filteringType(multi_channel_lowpass), m_exposure(8),
m_gain(8), initialized(false), resolutionX(640), resolutionY(480) // Constructeur par d�faut
{
	set_filter_range(Vec3i(25, 80, 80));
	set_filter_color(Vec3i(255, 255, 255));
	m_lastPosition = new Point3f(1, 1, 1);
}

Tracker::Tracker(float ballRadius, float cameraFocalLength) : m_focalLength(cameraFocalLength), m_ballRadius(ballRadius) // Constructeur
, m_exposure(10), m_gain(10), m_filteringType(simple_lowpass), initialized(false), resolutionX(640), resolutionY(480)
{
	set_filter_range(Vec3i(25,80,80) );
	set_filter_color(Vec3i(255,255,255) );
	m_lastPosition = new Point3f(1, 1, 1);
}

Tracker::~Tracker()
{
	free(m_currentTick); free(m_lastTick);
}

/*
FONCTIONS PUBLIQUES
*/

#ifdef USE_PS3EYEDRIVER
ps3eye_context* ctx;
#endif

void Tracker::init_tracker(int cameraIndex, bool stereo) // Initialisation du tracker
{
	load_params();
#ifdef USE_PS3EYEDRIVER
	ctx = new ps3eye_context(resolutionX, resolutionY, 60);
	if (!ctx->hasDevices()) {printf("Aucune PS3Eye connect�e !\n"); return;}
	ctx->eye->setFlip(true); /* miroir left-right */
	ctx->eye->setExposure(m_exposure);
	ctx->eye->setGain(m_gain);
	ctx->eye->setAutoWhiteBalance(false);
	ctx->eye->setBlueBalance(127); ctx->eye->setRedBalance(127); ctx->eye->setGreenBalance(127);
	ctx->eye->setFrameRate(60);
	m_videoFrame = Mat(480, 640, CV_8UC3, Scalar(0, 0, 0)); // Allocation d'un mat (8U : unsigned int 8bit), (C3 3 canaux) et initialis� au noir
	ctx->eye->start();
#else
	m_videoCap = VideoCapture(cameraIndex);
	if (!m_videoCap.isOpened()) { printf("Aucune cam�ra connect�e !\n"); return; }
	m_videoCap.set(CAP_PROP_FRAME_WIDTH, 640);
	m_videoCap.set(CAP_PROP_FRAME_HEIGHT, 480);
#endif
	m_currentTick = (int64*)malloc(sizeof(int64));
	m_lastTick = (int64*)malloc(sizeof(int64));
	initialized = true;
	track();
}

void Tracker::track()
{
#ifdef USE_PS3EYEDRIVER
	ctx->eye->getFrame(m_videoFrame.data); // Lecture du flux camera
#else
	m_videoCap.read(m_videoFrame); // Lecture du flux camera
#endif

	if (!m_videoFrame.empty())
	{
		// Calculs de positions
		Point3f raw_position = Point3f(0, 0, 0);
		color_filtering(m_videoFrame, m_hsvRange, m_filterColor, m_filteredFrame); // Filtrage de la couleur
		circle_fitting(m_2Dposition,m_filteredFrame); // D�tection de la couleur
		mono_position_estimation(m_focalLength, m_2Dposition, raw_position); // Estimation
		set_delta_time(m_currentTick,m_lastTick);
		
		if (m_isTrackingValid)
		{
			switch (m_filteringType)
			{
			case simple_lowpass:
				m_position = LOWPASS_ALPHA * raw_position + (1.0 - LOWPASS_ALPHA) * (*m_lastPosition);
				break;
			case multi_channel_lowpass:
				m_position.x = LOWPASS_ALPHA * raw_position.x + (1.0 - LOWPASS_ALPHA) * (*m_lastPosition).x;
				m_position.y = LOWPASS_ALPHA * raw_position.y + (1.0 - LOWPASS_ALPHA) * (*m_lastPosition).y;
				m_position.z = LOWPASS_ALPHA * Z_LOWPASS_SMOOTHING * raw_position.z + (1.0 - LOWPASS_ALPHA * Z_LOWPASS_SMOOTHING) * (*m_lastPosition).z;
				break;
			case noFiltering:
				m_position = raw_position;
				break;
			}
			
			m_speed = (*m_lastPosition - m_position) / m_deltaTime;
		}

		// Assignations de fin
		*m_lastPosition = m_position;
	}
}

void Tracker::set_filter_color(Vec3i color) // R�gle la couleur du filtre
{
	m_filterColor = color;
}

void Tracker::set_filter_range(Vec3i hsvRange) // R�gle la tol�rance du filtre
{
	m_hsvRange = hsvRange;
}

void Tracker::set_gain(int gain)
{
#ifdef USE_PS3EYEDRIVER
	m_gain = gain;
	ctx->eye->setGain(m_gain);
#endif
}

void Tracker::set_exposure(int exposure)
{
#ifdef USE_PS3EYEDRIVER
	m_exposure = exposure;
	ctx->eye->setGain(m_exposure);
#endif
}

Vec3i Tracker::get_hsv_color(Point2i coordinates)
{
	Vec3i col = m_hsvFrame.at<Vec3b>(Point(coordinates.x, coordinates.y));
	col += m_hsvFrame.at<Vec3b>(Point(coordinates.x - 0, coordinates.y - 1));
	col += m_hsvFrame.at<Vec3b>(Point(coordinates.x - 0, coordinates.y + 1));
	col += m_hsvFrame.at<Vec3b>(Point(coordinates.x - 1, coordinates.y - 0));
	col += m_hsvFrame.at<Vec3b>(Point(coordinates.x + 1, coordinates.y - 0));
	return col / 5;
}

Vec3i Tracker::get_fullscreen_average_hsv_color()
{
	Vec3i color = Vec3i(0,0,0);
	int divider = 0;
	for (int i = 0; i < 640; i++)
	{
		for (int j = 0; j < 480; j++)
		{
			Vec3i col = m_hsvFrame.at<Vec3b>(Point(i, j));
			if(col.val[2] > 10 ){
			color += col;
			divider++;}
			
		}
	}
	color /= divider;
	return color;
}

Vec3i Tracker::get_filter_color()
{
	return m_filterColor;
}

Vec3i Tracker::get_filter_range()
{
	return m_hsvRange;
}

Mat& Tracker::get_video_frame()
{
	return m_videoFrame;
}

Mat& Tracker::get_hsv_frame()
{
	return m_hsvFrame;
}

Mat& Tracker::get_binary_frame()
{
	return m_filteredFrame;
}

void Tracker::set_delta_time(int64* cur, int64* las)
{
	(*las) = (*cur);
	(*cur) = (int64)getTickCount();
	double delta = (double)( (*cur) - (*las) ) ;
	delta = delta / getTickFrequency();
	m_deltaTime = delta;
}

Point3f Tracker::get_2D_position()
{
	return m_2Dposition;
}

Point3f Tracker::get_position()
{
	return m_position;
}

Point3f Tracker::get_speed()
{
	return m_speed;
}

float Tracker::get_tracking_rate()
{
	return 1.0 / m_deltaTime;
}

bool Tracker::is_tracking_valid()
{
	return m_isTrackingValid;
}

/*
FONCTIONS DE TRACKING PRIVEES
*/

void Tracker::color_filtering(Mat& videoFrame, Vec3i hsvRange, Scalar filterColor,Mat& filteredFrame) // Filtre la couleur
{
	if (!videoFrame.empty())
	{
		Mat binaryFrame;
		Mat erodedFrame;
		cvtColor(videoFrame, m_hsvFrame, CV_BGR2HSV);
		inRange(m_hsvFrame, Scalar(filterColor.val[0] - m_hsvRange[0], filterColor.val[1] - hsvRange[1], filterColor.val[2] - hsvRange[2]),
		Scalar(filterColor.val[0] + hsvRange[0], filterColor.val[1] + hsvRange[1], filterColor.val[2] + hsvRange[2]), binaryFrame);

		erode(binaryFrame, erodedFrame, Mat(), Point(-1, -1), 2);
		dilate(erodedFrame, filteredFrame, Mat(), Point(-1, -1), 2);
	}
}

vector<Point> Tracker::get_largest_contour(vector<vector<Point> > contours) // Retourne le contour le plus grand
{
	int largestContourIndex = 0;
	double largestContourArea = 0;
	for (int i = 0; i < (int)contours.size(); i++)
	{
		double c = contourArea(contours[i]);
		if (c > largestContourArea) { largestContourIndex = i; largestContourArea = c; }
	}
	return contours[largestContourIndex];
}

void Tracker::circle_fitting(Point3f& circleCoord,Mat& filteredFrame) // Donne la position 2D et le rayon du cercle dans circleCoord
{
	Point2f position;
	float radius;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	// Recherche des contours dans l'image binaire
	findContours(filteredFrame, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));
	// Si aucun contour, ne rien faire
	if (contours.size() <= 0) { return; }
	// Sinon trouver le contour le plus grand et calculer le cercle englobant
	vector<Point> largestContour = get_largest_contour(contours);
	minEnclosingCircle(largestContour, position, radius);

	circleCoord.x = position.x;
	circleCoord.y = position.y;
	circleCoord.z = radius;
}

void Tracker::mono_position_estimation(float focal, Point3f circleCoord, Point3f& outPosition)
{
	float radius = circleCoord.z;

	// V�rification si le rayon est valide (sup�rieur � 1 pixel et inf�rieur � 240 pixels)
	if (radius < 1 || radius > 240) { m_isTrackingValid = false; return; }
	else { m_isTrackingValid = true; }

	// Centrage de la position
	float x_px = circleCoord.x - 320; // R�solution hardcod�e -> � changer
	float y_px = 240 - circleCoord.y;

	// Calcul de la distance l_px entre le centre et le cercle
	float l_px = sqrt(x_px * x_px + y_px * y_px);

	// Calcul des rapports
	float K = l_px / focal;
	float J = (l_px + radius) / focal;
	float L = (J - K) / (1 + J * K);

	// Calcul de la distance relle D_cm entre le centre et la sphere
	float D_cm = (BALL_RADIUS * sqrt(1 + L * L)) / L;

	// Calcul de Z_cm la profondeur
	float fl = focal / l_px;
	float Z_cm = (D_cm*fl) / sqrt(1 + fl * fl);

	// Calcul de X_cm et Y_cm
	float L_cm = Z_cm * K;
	float X_cm = (L_cm*x_px) / l_px;
	float Y_cm = (L_cm*y_px) / l_px;

	// Remplissage de la variable de sortie
	outPosition.x = X_cm;
	outPosition.y = Y_cm;
	outPosition.z = Z_cm;
}

void Tracker::new_file_params()
{
	// Cr�ation de l'objet Configuration et ajout des groupes de param�tres
	Config cfg;
	Setting& root = cfg.getRoot();
	Setting& camParams = root.add("camera", Setting::TypeGroup);
	Setting& trackingParams = root.add("tracking", Setting::TypeGroup);
	Setting& filterParams = trackingParams.add("filter", Setting::TypeGroup);

	// Ajout des param�tres par d�faut pour la partie cam�ra
	camParams.add("resolutionX", Setting::TypeInt) = 640;
	camParams.add("resolutionY", Setting::TypeInt) = 480;
	camParams.add("gain", Setting::TypeInt) = 10;
	camParams.add("exposure", Setting::TypeInt) = 10;

	// Ajout des param�tres par d�faut pour la partie tracking
	trackingParams.add("filterType", Setting::TypeString) = "multi_channel_lowpass";

	filterParams.add("colorHue", Setting::TypeInt) = 0;
	filterParams.add("colorSat", Setting::TypeInt) = 100;
	filterParams.add("colorVal", Setting::TypeInt) = 100;

	filterParams.add("toleranceHue", Setting::TypeInt) = 25;
	filterParams.add("toleranceSat", Setting::TypeInt) = 80;
	filterParams.add("toleranceVal", Setting::TypeInt) = 80;

	try
	{
		cfg.writeFile(CONFIG_FILE_PATH);
		cout << "Nouveau fichier configuration : " << CONFIG_FILE_PATH << endl;

	}
	catch (const FileIOException &fioex)
	{
		cout << "Erreur d'ecriture de : " << CONFIG_FILE_PATH << endl;
	}

}

void Tracker::load_params()
{
	Config cfg;

	try
	{
		cfg.readFile(CONFIG_FILE_PATH);
		cout << "Fichier configuration ouvert : " << CONFIG_FILE_PATH << endl;
	}
	catch (const FileIOException &fioex)
	{
		cout << "Erreur de lecture... creation d'un nouveau fichier... " << CONFIG_FILE_PATH << endl;
		new_file_params();
		return;
	}
	Setting& root = cfg.getRoot();
	try 
	{
		Setting& camParams = root["camera"];
		Setting& trackingParams = root["tracking"];
		Setting& filterParams = trackingParams["filter"];

		// Assignation des variables camera
		camParams.lookupValue("resolutionX", resolutionX);
		camParams.lookupValue("resolutionY", resolutionY);
		camParams.lookupValue("gain", m_gain);
		camParams.lookupValue("exposure", m_exposure);


		// Assignation des variables filtre
		string filtertype;
		trackingParams.lookupValue("filterType", filtertype); // simple_lowpass,multi_channel_lowpass,noFiltering
		if (filtertype == "simple_lowpass") { m_filteringType = simple_lowpass; }
		else if (filtertype == "multi_channel_lowpass") { m_filteringType = multi_channel_lowpass; }
		else if (filtertype == "noFiltering") { m_filteringType = noFiltering; }
	
		int h, s, v;
		filterParams.lookupValue("colorHue", h);
		filterParams.lookupValue("colorSat", s);
		filterParams.lookupValue("colorVal", v);
		set_filter_color( Vec3i(h, s, v) );

		filterParams.lookupValue("toleranceHue", h);
		filterParams.lookupValue("toleranceSat", s);
		filterParams.lookupValue("toleranceVal", v);
		set_filter_range( Vec3i(h, s, v) );

		cout << "Values correctely set : " << "H:" << m_filterColor.val[0] << " S:" << m_filterColor[1] << " V:" << m_filterColor[2] << endl;
	}
	catch (const SettingNotFoundException &nfex)
	{
		cout << "Erreur de param�tres dans le fichier." << endl;
	}
}

void Tracker::save_params()
{
	// Cr�ation de l'objet Configuration et ajout des groupes de param�tres
	Config cfg;
	Setting& root = cfg.getRoot();
	Setting& camParams = root.add("camera", Setting::TypeGroup);
	Setting& trackingParams = root.add("tracking", Setting::TypeGroup);
	Setting& filterParams = trackingParams.add("filter", Setting::TypeGroup);

	// Ajout des param�tres par d�faut pour la partie cam�ra
	camParams.add("resolutionX", Setting::TypeInt) = resolutionX;
	camParams.add("resolutionY", Setting::TypeInt) = resolutionY;
	camParams.add("gain", Setting::TypeInt) = m_gain;
	camParams.add("exposure", Setting::TypeInt) = m_exposure;

	// Ajout des param�tres par d�faut pour la partie tracking
	if (m_filteringType == simple_lowpass){ trackingParams.add("filterType", Setting::TypeString) = "simple_lowpass"; }
	else if (m_filteringType == multi_channel_lowpass) { trackingParams.add("filterType", Setting::TypeString) = "multi_channel_lowpass"; }
	else if (m_filteringType == noFiltering) { trackingParams.add("filterType", Setting::TypeString) = "noFiltering"; }

	filterParams.add("colorHue", Setting::TypeInt) = m_filterColor.val[0];
	filterParams.add("colorSat", Setting::TypeInt) = m_filterColor.val[1];
	filterParams.add("colorVal", Setting::TypeInt) = m_filterColor.val[2];

	filterParams.add("toleranceHue", Setting::TypeInt) = m_hsvRange.val[0];
	filterParams.add("toleranceSat", Setting::TypeInt) = m_hsvRange.val[1];
	filterParams.add("toleranceVal", Setting::TypeInt) = m_hsvRange.val[2];

	try
	{
		cfg.writeFile(CONFIG_FILE_PATH);
		cout << "Fichier configuration remplace : " << CONFIG_FILE_PATH << endl;

	}
	catch (const FileIOException &fioex)
	{
		cout << "Erreur d'ecriture de : " << CONFIG_FILE_PATH << endl;
	}
}

// Maths : https://fr.wikipedia.org/wiki/Matrice_de_rotation
Matx33f Tracker::rotation_matrix(Point3f axis, float angle)
{
	float c = cos(angle); // en radians
	float s = sin(angle);

	//Matx33f mat = 
	Matx33f m(axis.x * axis.x * (1 - c) + c         , axis.x * axis.y * (1 - c) - axis.z * s , axis.x * axis.z * (1 - c) + axis.y * s,
			  axis.x * axis.y * (1 - c) + axis.z * s, axis.y * axis.y * (1 - c) + c			 , axis.y * axis.z * (1 - c) - axis.x * s,
			  axis.x * axis.z * (1 - c) - axis.y * s, axis.y * axis.z * (1 - c) + axis.x * s , axis.z * axis.z * (1 - c) + c			);

	return m;
}

void Tracker::compute_camToWorld_rotation_matrix(Vec3f z_world_axis)
{
	Point3f nwz = normalize(z_world_axis); // Normalized World Z - Axe Z absolu normalis�
	Point3f cza = Point3f(0, 0, 1); // Camera Z Axis - Axe Z cam�ra
	Point3f rotationAxis =
	Point3f(nwz.y * cza.z - nwz.z * cza.y,
		    nwz.z * cza.x - nwz.x * cza.z,
		    nwz.x * cza.y - nwz.y * cza.x); // Produit vectoriel

	Matx33f tempRotMatrix = rotation_matrix(rotationAxis,0.0);
	Point3f rCza = tempRotMatrix * cza; // rotated Cza
	float dotProduct = nwz.x * rCza.x + nwz.y * rCza.y + nwz.z * rCza.z;

	const float dotAccuracy = 0.1; float epsilon = 0.1; int counter = 0; float theta = PI / 2;
	while (dotProduct < 1 - dotAccuracy && counter < 1000 ) // Tant que l'on a pas atteint une certaine pr�cision
	{
		tempRotMatrix = rotation_matrix(rotationAxis, theta); // On calcule une nouvelle matrice de rotation avec theta
		rCza = tempRotMatrix * cza; // On fait tourner le vecteur de theta
		float newDotProduct = nwz.x * rCza.x + nwz.y * rCza.y + nwz.z * rCza.z; // On calcule le nouveau produit vectoriel obtenu
		if(newDotProduct > dotProduct) // Si on s'approche du vecteur recherch�
		{
			theta = theta + epsilon;
		}
		else 
		{
			epsilon = epsilon / 2;
			theta = theta - epsilon;
		}

	}
	

}