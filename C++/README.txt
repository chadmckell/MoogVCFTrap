This code is designed for beginners to C++ and/or physical modeling. For this reason, it does not include more complicated features such as sidechain modulation, parameter interpolation, pausing, or any computation of nonlinear functions. In addition, I chose not to use the Eigen C++ library because I didn't want this to be a hurdle to beginners (this is why I computed the matrix inverse by hand).

Below are instructions for compiling the source code and running the real-time VST plugin in a DAW. 


1. Open Projucer v5.2.0 or later (see juce.com/projucer).

2. Create a new audio plug-in project.

3. Name the project “MoogVCFTrap”.

4. Open your JUCE project settings (click on the gear icon located near the top of the window).

5. Make sure the following are enabled: Build AudioUnit v3, Plugin Midi Input, Plugin Midi Output, and Key Focus.

6. Save and open the file in your selected exporter.

7. Using your computer’s file manager, navigate to your JUCE project folder. Remove all the files located in the “Source” folder. Now add the files from this GitHub repository.

8. Compile the new source code using the exporter.

9. Find and run the VST plugin in your chosen DAW. If you’re using Ableton Live 9, make sure to enable “Use VST Plug-in System Folders” under Preferences->File Folder.

