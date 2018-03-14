Moog VCF Model (Trapezoidal Approximation)

Authors: Chad McKell + Roli, Ltd.
Date: 14 Mar 2018

This is a virtual analog model of the Moog voltage-controlled filter (VCF). The state-space system is solved using the trapezoidal integration rule. In the MATLAB file, the approximated transfer function is compared with the exact calculation of the Moog VCF transfer function. 

A preliminary C++ implemenation of the MATLAB algorithm is included in the file "PluginProcessor.cpp". An accompanying JUCE project will be uploaded soon that will allow you to use the C++ code to build your own real-time VST plugin of the Moog VCF. Stay tuned!


