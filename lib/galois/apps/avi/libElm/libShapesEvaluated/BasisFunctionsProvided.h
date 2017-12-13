/**
 * BasisFunctionsProvided.h
 * DG++
 *
 * Created by Adrian Lew on 10/21/06.
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

#ifndef BASISFUNCTIONSPROVIDED
#define BASISFUNCTIONSPROVIDED

#include "Shape.h"
#include "Quadrature.h"
#include "ElementGeometry.h"
#include "BasisFunctions.h"
#include <iostream>

/**
 \brief
 BasisFunctionsProvidedExternalQuad:  set of basis functions and derivatives at
 the quadrature points provided directly at construction. The quadrature points are
 referenced externally, they are not kept as a copy inside the object.
 */

class BasisFunctionsProvidedExternalQuad: public BasisFunctions {
public:
  //! Constructor
  //!
  //! In the following,\n
  //! NQuad = QuadratureWeights.size(), the total number of quadrature points\n
  //! NShapes = ShapesInput.size()/QuadratureWeights.size(), total number of shape
  //! functions provided\n
  //! spd = QuadratureCoords().size/QuadratureWeights.size(), number of spatial dimensions
  //!
  //! @param ShapesInput: Values of each shape function at each quadrature point.\n
  //! ShapesInput[ q*NShapes + a] = value of shape function "a" at quadrature point "q"
  //! @param DShapesInput: Values of each shape function derivative at each quadrature point.\n
  //! DShapesInput[ q*NShapes*spd + a*spd + i] = value of shape function "a" derivative in the i-th
  //! direction at quadrature point "q"
  //! @param QuadratureWeights: QuadratureWeights[q] contains the value of the quadrature weight at quad point "q"
  //! @param QuadratureCoords: QuadratureCoords[q*spd+i] contains the i-th coordinate of the position of quadrature
  //! point "q"
  //!
  //! If not derivatives of shape functions are available, just provide and empty vector as 
  //! DShapesInput
  inline BasisFunctionsProvidedExternalQuad (const std::vector<double> &ShapesInput,
      const std::vector<double> &DShapesInput, const std::vector<double> &QuadratureWeights,
      const std::vector<double> &QuadratureCoords) :
    LocalShapes (ShapesInput), LocalDShapes (DShapesInput),
    LocalWeights (QuadratureWeights), LocalCoordinates (QuadratureCoords) {


    // Check that the dimensions are correct
    if (LocalShapes.size () % LocalWeights.size () != 0
        || LocalCoordinates.size () % LocalWeights.size () != 0
        || LocalDShapes.size () % LocalWeights.size () != 0
        || LocalDShapes.size () % LocalShapes.size () != 0) {
      std::cerr << "BasisFunctionsProvidedExternalQuad::Constructor. Error\n"
        " Inconsistent length of some of the vectors provided\n";
      exit (1);
    }
    NumberOfShapes = LocalShapes.size () / LocalWeights.size ();
  }

  //! Constructor
  //!
  //! In the following no shape functions are provided. An empty vector will be\n
  //! place in its place. \n
  //! NQuad = QuadratureWeights.size(), the total number of quadrature points\n
  //! spd = QuadratureCoords().size/QuadratureWeights.size(), number of spatial dimensions
  //! NDerivatives = LocalDShapes.size()/(NumberOfShapes*LocalWeights.size())
  //!
  //! @param NShapes Number of shape functions for which derivatives are offered
  //! @param DShapesInput: Values of each shape function derivative at each quadrature point.\n
  //! DShapesInput[ q*NShapes*spd + a*spd + i] = value of shape function "a" derivative in the i-th
  //! direction at quadrature point "q"
  //! @param QuadratureWeights: QuadratureWeights[q] contains the value of the quadrature weight at quad point "q"
  //! @param QuadratureCoords: QuadratureCoords[q*spd+i] contains the i-th coordinate of the position of quadrature
  //! point "q"
  inline BasisFunctionsProvidedExternalQuad (size_t NShapes,
      const std::vector<double> &DShapesInput, const std::vector<double> &QuadratureWeights,
      const std::vector<double> &QuadratureCoords) :
    LocalShapes (ZeroSizeVector), LocalDShapes (DShapesInput), NumberOfShapes (NShapes),
    LocalWeights (QuadratureWeights), LocalCoordinates (QuadratureCoords) {

    // Check that the dimensions are correct
    if (LocalCoordinates.size () % LocalWeights.size () != 0 || LocalDShapes.size ()
        % (LocalWeights.size () * NShapes) != 0 || LocalDShapes.size () % NShapes != 0) {
      std::cerr << "BasisFunctionsProvidedExternalQuad::Constructor. Error\n"
        " Inconsistent length of some of the vectors provided\n";
      exit (1);
    }
  }

  inline virtual ~BasisFunctionsProvidedExternalQuad () {
  }

  inline BasisFunctionsProvidedExternalQuad (const BasisFunctionsProvidedExternalQuad &NewBas) :
    LocalShapes (NewBas.LocalShapes), LocalDShapes (NewBas.LocalDShapes),
        NumberOfShapes (NewBas.NumberOfShapes) , LocalWeights (NewBas.LocalWeights), LocalCoordinates (NewBas.LocalCoordinates) {
  }

  virtual BasisFunctionsProvidedExternalQuad * clone () const {
    return new BasisFunctionsProvidedExternalQuad (*this);
  }

  //!  Shape functions at quadrature points
  //!  getShapes()[q*Shape::getNumFunctions()+a]
  //!  gives the value of shape function a at quadrature point q
  inline const std::vector<double> & getShapes () const {
    return LocalShapes;
  }

  //! Derivatives of shape functions at quadrature points 
  //! getDShapes()[q*Shape::getNumFunctions()*ElementGeometry::getEmbeddingDimensions()+a*ElementGeometry::getEmbeddingDimensions()+i] 
  //! gives the
  //! derivative in the i-th direction of degree of freedom a at quadrature point q
  inline const std::vector<double> & getDShapes () const {
    return LocalDShapes;
  }

  //!< Integration weights 
  inline const std::vector<double> & getIntegrationWeights () const {
    return LocalWeights;
  }

  //! Coordinates of quadrature points in the real configuration
  //! getQuadraturePointCoordinates()
  //! [q*ElementGeometry::getEmbeddingDimension()+i]
  //! returns the i-th coordinate in real space of quadrature point q 
  inline const std::vector<double> & getQuadraturePointCoordinates () const {
    return LocalCoordinates;
  }

  //! returns the number of shape functions provided
  inline size_t getBasisDimension () const {
    return NumberOfShapes;
  }

  //! returns the number of directional derivative for each shape function
  inline size_t getNumberOfDerivativesPerFunction () const {
    return LocalDShapes.size () / (NumberOfShapes * LocalWeights.size ());
  }

  //! returns the number of  number of coordinates for each Gauss point
  inline size_t getSpatialDimensions () const {
    return LocalCoordinates.size () / LocalWeights.size ();
  }

private:
  const std::vector<double>& LocalShapes;
  const std::vector<double>& LocalDShapes;
  size_t NumberOfShapes;

protected:
  const std::vector<double>& LocalWeights;
  const std::vector<double>& LocalCoordinates;
  static const std::vector<double> ZeroSizeVector;
};

/**
 \brief
 BasisFunctionsProvided:  set of basis functions and derivatives at
 the quadrature points provided directly at construction. The quadrature points are
 provided and stored inside the object
 */

class BasisFunctionsProvided: public BasisFunctionsProvidedExternalQuad {
public:
  //! Constructor
  //!
  //! In the following,\n
  //! NQuad = QuadratureWeights.size(), the total number of quadrature points\n
  //! NShapes = ShapesInput.size()/QuadratureWeights.size(), total number of shape
  //! functions provided\n
  //! spd = QuadratureCoords().size/QuadratureWeights.size(), number of spatial dimensions
  //!
  //! @param ShapesInput: Values of each shape function at each quadrature point.\n
  //! ShapesInput[ q*NShapes + a] = value of shape function "a" at quadrature point "q"
  //! @param DShapesInput: Values of each shape function derivative at each quadrature point.\n
  //! DShapesInput[ q*NShapes*spd + a*spd + i] = value of shape function "a" derivative in the i-th
  //! direction at quadrature point "q"
  //! @param QuadratureWeights: QuadratureWeights[q] contains the value of the quadrature weight at quad point "q"
  //! @param QuadratureCoords: QuadratureCoords[q*spd+i] contains the i-th coordinate of the position of quadrature
  //! point "q"
  inline BasisFunctionsProvided (const std::vector<double> &ShapesInput,
      const std::vector<double> &DShapesInput, const std::vector<double> &QuadratureWeights,
      const std::vector<double> &QuadratureCoords) :
        BasisFunctionsProvidedExternalQuad (ShapesInput, DShapesInput, QuadratureWeights, QuadratureCoords) {
  }

  inline virtual ~BasisFunctionsProvided () {
  }
  inline BasisFunctionsProvided (const BasisFunctionsProvided &NewBas) :
    BasisFunctionsProvidedExternalQuad (NewBas) {
  }

  virtual BasisFunctionsProvided * clone () const {
    return new BasisFunctionsProvided (*this);
  }

};

#endif

