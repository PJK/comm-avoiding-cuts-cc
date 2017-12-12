/*
 * NeoHookean.cpp
 * DG++
 *
 * Created by Adrian Lew on 10/24/06.
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

#include "Material.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <iostream>

const double SimpleMaterial::I_MAT[]  = { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
const int IsotropicLinearElastic::DELTA_MAT[][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
const double SimpleMaterial::EPS = 1.e-6;
const double SimpleMaterial::PERT = 1.e-1;
const double SimpleMaterial::DET_MIN = 1.e-10;

const size_t SimpleMaterial::NDF = 3;
const size_t SimpleMaterial::NDM = 3;

bool SimpleMaterial::consistencyTest(const SimpleMaterial &SMat) {
  std::vector<double> strain(MAT_SIZE);
  std::vector<double> stress(MAT_SIZE);
  std::vector<double> tangents(MAT_SIZE * MAT_SIZE);

  std::vector<double> stressplus(MAT_SIZE);
  std::vector<double> stressminus(MAT_SIZE);
  std::vector<double> tangentsnum(MAT_SIZE * MAT_SIZE);

  srand(time(0));

  strain[0] = 1 + double(rand()) / double(RAND_MAX) * PERT;
  strain[1] = double(rand()) / double(RAND_MAX) * PERT;
  strain[2] = double(rand()) / double(RAND_MAX) * PERT;
  strain[3] = double(rand()) / double(RAND_MAX) * PERT;
  strain[4] = 1 + double(rand()) / double(RAND_MAX) * PERT;
  strain[5] = double(rand()) / double(RAND_MAX) * PERT;
  strain[6] = double(rand()) / double(RAND_MAX) * PERT;
  strain[7] = double(rand()) / double(RAND_MAX) * PERT;
  strain[8] = 1 + double(rand()) / double(RAND_MAX) * PERT;

  for (unsigned int i = 0; i < MAT_SIZE; i++) {
    std::vector<double> t;
    double Forig = strain[i];

    strain[i] = Forig + EPS;
    SMat.getConstitutiveResponse(strain, stressplus, t, SKIP_TANGENTS);

    strain[i] = Forig - EPS;
    SMat.getConstitutiveResponse(strain, stressminus, t, SKIP_TANGENTS);

    for (unsigned j = 0; j < MAT_SIZE; j++) {
      tangentsnum[j * MAT_SIZE + i] = (stressplus[j] - stressminus[j]) / (2 * EPS);
    }

    strain[i] = Forig;
  }

  SMat.getConstitutiveResponse(strain, stress, tangents, COMPUTE_TANGENTS);

  double error = 0;
  double norm = 0;
  for (size_t i = 0; i < MAT_SIZE * MAT_SIZE; i++) {
    error += pow(tangents[i] - tangentsnum[i], 2);
    norm += pow(tangents[i], 2);
  }
  error = sqrt(error);
  norm = sqrt(norm);

  if (error / norm > EPS * 100) {
    std::cerr << "SimpleMaterial::ConsistencyTest. Material not consistent\n";
    return false;
  }
  return true;
}
