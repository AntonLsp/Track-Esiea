#include "client_test.h"
#include<iostream>

using namespace std;

client_test::client_test()
{
}

client_test::~client_test()
{
}

int main(int argc, char** argv)
{
	// cout est le flux de texte du terminal, '<<' permet d'injecter un �l�ment dans cout, endl correspond � la fin de ligne
	cout << "Track'ESIEA est un pst de 2A qui vise a effectuer le tracking 3D d'un objet dans un espace delimite." << endl; 
	cout << "Librairie utilis�e : OpenCV." << endl;
	cout << "Un grand merci � Antoine pour avoir cr�� le projet." << endl;
	system("pause");
	return 0;
}
