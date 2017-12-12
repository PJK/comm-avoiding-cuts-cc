/*
 * testP12DElement.cpp
 * DG++
 *
 * Created by Adrian Lew on 9/22/06.
 *  
 * Copyright (c) 2006 Adrian Lew
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */ 


#include "P12DElement.h"
#include <iostream>


int main()
{ 
  double Vertices[] = {1,0,0,1,0,0};
  std::vector<double> Vertices0(Vertices, Vertices+6);

  Triangle<2>::SetGlobalCoordinatesArray(Vertices0);

  P12DElement<2> TestElement(1,2,3);
  Element * VirtualElement;
  
  sleep(2);

  std::cout << "Number of fields: " << TestElement.GetFields() << " should be 2\n";
  std::cout << "Number of dof field(0): " << TestElement.getDof(0) << " should be 3\n";
  std::cout << "Number of dof field(1): " << TestElement.getDof(1) << " should be 3\n";
  for(int a=0; a<2; a++)
    {
      std::cout << "Shape function values at quad points field("<< a<< "):\n";
      for(unsigned int q=0;q<TestElement.getShapes(a).size();q++)
	std::cout << TestElement.getShapes(a)[q] << " ";
      std::cout << "\n";
    }
 
  for(int a=0; a<2; a++)
    {
      std::cout << "Shape function derivatives values at quad points field("<< a<< "):\n";
      for(unsigned int q=0;q<TestElement.getDShapes(a).size();q++)
	std::cout << TestElement.getDShapes(a)[q] << " ";
      std::cout << "\n";
    }
 
  for(int a=0; a<2; a++)
    {
      std::cout << "Integration weight values at quad points field("<< a<< "):\n";
      for(unsigned int q=0;q<TestElement.getIntegrationWeights(a).size();q++)
	std::cout << TestElement.getIntegrationWeights(a)[q] << " ";
      std::cout << "\n";
    }

  for(int a=0; a<2; a++)
    {
      std::cout << "Quad points coordinates for field("<< a<< "):\n";
      for(unsigned int q=0;q<TestElement.getIntegrationPtCoords(a).size();q++)
	std::cout << TestElement.getIntegrationPtCoords(a)[q] << " ";
      std::cout << "\n";
    }

  {
    P12DElement<2> CopyElement(TestElement);
    std::cout <<  "Test Copy Constructor\n";
    
    std::cout << "Number of fields: " << CopyElement.GetFields() << " should be 2\n";
    std::cout << "Number of dof field(0): " << CopyElement.getDof(0) << " should be 3\n";
    std::cout << "Number of dof field(1): " << CopyElement.getDof(1) << " should be 3\n";
    for(int a=0; a<2; a++)
      {
	std::cout << "Shape function values at quad points field("<< a<< "):\n";
	for(unsigned int q=0;q<CopyElement.getShapes(a).size();q++)
	  std::cout << CopyElement.getShapes(a)[q] << " ";
	std::cout << "\n";
      }
    
    for(int a=0; a<2; a++)
      {
	std::cout << "Shape function derivatives values at quad points field("<< a<< "):\n";
	for(unsigned int q=0;q<CopyElement.getDShapes(a).size();q++)
	  std::cout << CopyElement.getDShapes(a)[q] << " ";
	std::cout << "\n";
      }
    
    for(int a=0; a<2; a++)
      {
	std::cout << "Integration weight values at quad points field("<< a<< "):\n";
	for(unsigned int q=0;q<CopyElement.getIntegrationWeights(a).size();q++)
	  std::cout << CopyElement.getIntegrationWeights(a)[q] << " ";
	std::cout << "\n";
      }
    
    for(int a=0; a<2; a++)
      {
	std::cout << "Quad points coordinates for field("<< a<< "):\n";
	for(unsigned int q=0;q<CopyElement.getIntegrationPtCoords(a).size();q++)
	  std::cout << CopyElement.getIntegrationPtCoords(a)[q] << " ";
	std::cout << "\n";
      }
    
    
    
    VirtualElement = CopyElement.clone();
    std::cout << "Cloned element before destruction. Test cloning mechanism\n";
  }
  
  std::cout << "Number of fields: " << VirtualElement->getNumFields() << " should be 2\n";
  std::cout << "Number of dof field(0): " << VirtualElement->getDof(0) << " should be 3\n";
  std::cout << "Number of dof field(1): " << VirtualElement->getDof(1) << " should be 3\n";
  for(int a=0; a<2; a++)
    {
      std::cout << "Shape function values at quad points field("<< a<< "):\n";
      for(unsigned int q=0;q<VirtualElement->getShapes(a).size();q++)
	std::cout << VirtualElement->getShapes(a)[q] << " ";
      std::cout << "\n";
    }
  
  for(int a=0; a<2; a++)
    {
      std::cout << "Shape function derivatives values at quad points field("<< a<< "):\n";
      for(unsigned int q=0;q<VirtualElement->getDShapes(a).size();q++)
	std::cout << VirtualElement->getDShapes(a)[q] << " ";
      std::cout << "\n";
    }
  
  for(int a=0; a<2; a++)
    {
      std::cout << "Integration weight values at quad points field("<< a<< "):\n";
      for(unsigned int q=0;q<VirtualElement->getIntegrationWeights(a).size();q++)
	std::cout << VirtualElement->getIntegrationWeights(a)[q] << " ";
      std::cout << "\n";
    }
  
  for(int a=0; a<2; a++)
    {
      std::cout << "Quad points coordinates for field("<< a<< "):\n";
      for(unsigned int q=0;q<VirtualElement->getIntegrationPtCoords(a).size();q++)
	std::cout << VirtualElement->getIntegrationPtCoords(a)[q] << " ";
      std::cout << "\n";
    }
  
  delete VirtualElement;
}

