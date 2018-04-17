Moog VCF Model (Trapezoidal Approximation)

Authors: Chad McKell + Roli, Ltd.
Date: 14 Mar 2018

This repository contains a virtual analog model of the Moog voltage-controlled filter (VCF) based on the trapezoidal integration rule. In the MATLAB file, the approximated transfer function is compared with the exact calculation of the Moog VCF transfer function. 

A real-time JUCE implemenation of the MATLAB algorithm is included in the C++ folder. The code is designed for beginners to C++ and/or physical modeling. For this reason, it does not include more complicated features such as sidechain modulation, parameter interpolation, pausing, or any computation of nonlinear functions. In addition, I chose not to use the Eigen C++ library because I didn't want this to be a hurdle to beginners (this is why I computed the matrix inverse by hand).
