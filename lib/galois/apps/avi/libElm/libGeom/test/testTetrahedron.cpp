// Sriramajayam

// Purpose : To check class Tetrahedron.

#include <iostream>
#include <cstdlib>
#include "Tetrahedron.h"

int main()
{
 
  double coord[] = {2,0,0, 
		    0,2,0, 
		    0,0,0, 
		    0,0,2};

  std::vector<double> dummycoordinates(coord, coord+12);

  
  int c[] = {0, 1, 2, 3};
  std::vector<GlobalNodalIndex> conn(c, c+4);
  Tetrahedron MyTet(dummycoordinates, conn);
  
  
  std::cout << "\nNumber of vertices: " << MyTet.getNumVertices();

  std::cout << "\nParametricDimension: " << MyTet.getParametricDimension(); 

  std::cout << "\nEmbeddingDimension: " << MyTet.getEmbeddingDimension(); 
  
  std::cout<<"\n";
  const double X[3] = {0.25, 0.25, 0.25}; // Barycentric coordinates.
 
  if(MyTet.consistencyTest(X,1.e-6))
    std::cout << "Consistency test successful" << "\n";
  else
    std::cout << "Consistency test failed" << "\n";

  double DY[9], Jac;
  MyTet.dMap(X, DY, Jac);
  std::cout<<"\n Jacobian: "<<Jac;
  
  // Faces
  std::cout<<"\n Testing Face 1: \n";
  ElementGeometry *face = MyTet.getFaceGeometry(1);
  std::cout << "\nNumber of vertices: " << face->getNumVertices();
  
  std::cout << "\nParametricDimension: " << face->getParametricDimension(); 

  std::cout << "\nEmbeddingDimension: " << face->getEmbeddingDimension(); 

  std::cout << "\nConnectivity: " << face->getConnectivity()[0] << " " 
	    << face->getConnectivity()[1] << " "<<face->getConnectivity()[2]
	    <<" should be 3 1 4\n";

  std::cout << "\nIn Radius: " << MyTet.getInRadius() << "\nshould be 0.42264973\n";   
  std::cout << "\nOut Radius: " << MyTet.getOutRadius() << "\nshould be 1.7320508075688772\n";  
  std::cout << "\n";
  delete face;

  // Test virtual mechanism and copy and clone constructors
  ElementGeometry *MyElmGeo = &MyTet;
  
  std::cout << "Testing virtual mechanism: ";
  std::cout << "\nPolytope name: " << MyElmGeo->getPolytopeName() 
	    << " should be Tetrahedron\n";
  
  const  std::vector<GlobalNodalIndex>  &Conn = MyElmGeo->getConnectivity();
  std::cout << "\nConnectivity: " << Conn[0] << " " << Conn[1] << " " << Conn[2] 
	    <<" "<<Conn[3]<< " should be 1 2 3 4\n"; 

  ElementGeometry *MyElmGeoCloned = MyTet.clone();
  std::cout << "Testing cloning mechanism: ";
  std::cout << "\nPolytope name: " << MyElmGeoCloned->getPolytopeName() 
	    << " should be Tetrahedron\n";
  const std::vector<GlobalNodalIndex>  &Conn2 = MyElmGeoCloned->getConnectivity();
  std::cout << "\nConnectivity: " << Conn2[0] << " " << Conn2[1] << " " 
	    << Conn2[2] <<" "<<Conn2[3]<< " should be 1 2 3 4\n"; 
  
  return 1;
}

