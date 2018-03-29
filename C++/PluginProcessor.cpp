/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SinewaveSynth.h"
#include <iostream>
#include <math.h>

using namespace std;

AudioProcessor* JUCE_CALLTYPE createPluginFilter();


//==============================================================================
MoogVcftrapAudioProcessor::MoogVcftrapAudioProcessor()
     : AudioProcessor (getBusesProperties())
{
    lastPosInfo.resetToDefault();
    
    // This creates our parameters. We'll keep some raw pointers to them in this class,
    // so that we can easily access them later, but the base class will take care of
    // deleting them for us.
    addParameter (gainParam  = new AudioParameterFloat ("gain",  "Overall Gain", 0.0f, 1.0f, 0.5f));
    addParameter (resParam  = new AudioParameterFloat ("cutoff",  "Cutoff", 0.0f, 3000.0f, 0.0f));

    initialiseSynth();
}

MoogVcftrapAudioProcessor::~MoogVcftrapAudioProcessor()
{
}

void MoogVcftrapAudioProcessor::initialiseSynth()
{
    const int numVoices = 8;
    
    // Add some voices...
    for (int i = numVoices; --i >= 0;)
        synth.addVoice (new SineWaveVoice());
    
    // ..and give the synth a sound to play
    synth.addSound (new SineWaveSound());
}

//==============================================================================
bool MoogVcftrapAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only mono/stereo and input/output must have same layout
    const AudioChannelSet& mainOutput = layouts.getMainOutputChannelSet();
    const AudioChannelSet& mainInput  = layouts.getMainInputChannelSet();
    
    // input and output layout must either be the same or the input must be disabled altogether
    if (! mainInput.isDisabled() && mainInput != mainOutput)
        return false;
    
    // do not allow disabling the main buses
    if (mainOutput.isDisabled())
        return false;
    
    // only allow stereo and mono
    if (mainOutput.size() > 2)
        return false;
    
    return true;
}

AudioProcessor::BusesProperties MoogVcftrapAudioProcessor::getBusesProperties()
{
    return BusesProperties().withInput  ("Input",  AudioChannelSet::stereo(), true)
    .withOutput ("Output", AudioChannelSet::stereo(), true);
}

//==============================================================================
void MoogVcftrapAudioProcessor::prepareToPlay (double newSampleRate, int /*samplesPerBlock*/)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    synth.setCurrentPlaybackSampleRate (newSampleRate);
    keyboardState.reset();
    
    reset();
}

void MoogVcftrapAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    keyboardState.reset();
}

void MoogVcftrapAudioProcessor::reset()
{
    // Use this method as the place to clear any delay lines, buffers, etc, as it
    // means there's been a break in the audio's continuity.
}

template <typename FloatType>
void MoogVcftrapAudioProcessor::process (AudioBuffer<FloatType>& buffer,
                                           MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    
    // Now pass any incoming midi messages to our keyboard state object, and let it
    // add messages to the buffer if the user is clicking on the on-screen keys
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);
    
    // and now get our synth to process these midi events and generate its output.
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);
    
    // Apply the Moog effect to the new output
    applyMoog (buffer);
    
    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);
    
    // apply the gain-change to the outgoing data
    applyGain (buffer);
    
    // Now ask the host for the current time so we can store it to be displayed later.
    updateCurrentTimeInfoFromHost();
}

template <typename FloatType>
void MoogVcftrapAudioProcessor::applyGain (AudioBuffer<FloatType>& buffer)
{
    const float gainLevel = *gainParam;
    
    for (int channel = 0; channel < getTotalNumOutputChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gainLevel);
}

template <typename FloatType>
void MoogVcftrapAudioProcessor::applyMoog (AudioBuffer<FloatType>& buffer)
{
    float f0 = *resParam; // cut off frequency
    float r = 0.15f; // tuning parameter, i.e. "resonance" (a number between 0 and 1)
    const float pi = MathConstants<double>::pi; // constant value of pi
    float SR = getSampleRate(); // sample rate [samples/sec]
    float k = 1/SR; // time step [sec]
    float w0 = 2*pi*f0; // angular frequency [rad/sec]
    
    float A[4][4] = { { -w0,  0,   0, -4*r*w0 },
        {  w0, -w0,  0,       0 },
        {   0,  w0, -w0,      0 },
        {   0,   0,  w0,    -w0 } }; // system matrix
    
    float b[4] = {w0,0,0,0}; // forcing vector
    float eye[4][4] = { { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 } }; // identity matrix
    
    // Compute coefficients
    float e[4][4];
    float f[4][4];
    
    for (int n = 0; n < 4; n++)
    {
        for (int m = 0; m < 4; m++)
        {
            e[n][m] = eye[n][m] + k*A[n][m]/2;
            f[n][m] = eye[n][m] - k*A[n][m]/2;
        }
    }
    
    // Find the determinant of f
    float det = f[0][0]*(f[1][1]*f[2][2]*f[3][3] + f[1][2]*f[2][3]*f[3][1]
                         + f[1][3]*f[2][1]*f[3][2] - f[1][1]*f[2][3]*f[3][2]
                         - f[1][2]*f[2][1]*f[3][3] - f[1][3]*f[2][2]*f[3][1])
    + f[0][1]*(f[1][0]*f[2][3]*f[3][2] + f[1][2]*f[2][0]*f[3][3]
               + f[1][3]*f[2][2]*f[3][0] - f[1][0]*f[2][2]*f[3][3]
               - f[1][2]*f[2][3]*f[3][0] - f[1][3]*f[2][0]*f[3][2])
    + f[0][2]*(f[1][0]*f[2][1]*f[3][3] + f[1][1]*f[2][3]*f[3][0]
               + f[1][3]*f[2][0]*f[3][1] - f[1][0]*f[2][3]*f[3][1]
               - f[1][1]*f[2][0]*f[3][3] - f[1][3]*f[2][1]*f[3][0])
    + f[0][3]*(f[1][0]*f[2][2]*f[3][1] + f[1][1]*f[2][0]*f[3][2]
               + f[1][2]*f[2][1]*f[3][0] - f[1][0]*f[2][1]*f[3][2]
               - f[1][1]*f[2][2]*f[3][0] - f[1][2]*f[2][0]*f[3][1]);

    // Compute the inverse of f
    float invDET = 1.0f/det;
    float invF[4][4];

    invF[0][0] = invDET*(f[1][1]*f[2][2]*f[3][3] + f[1][2]*f[2][3]*f[3][1] +
                         f[1][3]*f[2][1]*f[3][2] - f[1][1]*f[2][3]*f[3][2] -
                         f[1][2]*f[2][1]*f[3][3] - f[1][3]*f[2][2]*f[3][1]);

    invF[0][1] = invDET*(f[0][1]*f[2][3]*f[3][2] + f[0][2]*f[2][1]*f[3][3] +
                         f[0][3]*f[2][2]*f[3][1] - f[0][1]*f[2][2]*f[3][3] -
                         f[0][2]*f[2][3]*f[3][1] - f[0][3]*f[2][1]*f[3][2]);

    invF[0][2] = invDET*(f[0][2]*f[1][3]*f[3][1] - f[0][3]*f[1][2]*f[3][1] +
                         f[0][3]*f[1][1]*f[3][2] - f[0][1]*f[1][3]*f[3][2] -
                         f[0][2]*f[1][1]*f[3][3] + f[0][1]*f[1][2]*f[3][3]);

    invF[0][3] = invDET*(f[0][3]*f[1][2]*f[2][1] - f[0][2]*f[1][3]*f[2][1] -
                         f[0][3]*f[1][1]*f[2][2] + f[0][1]*f[1][3]*f[2][2] +
                         f[0][2]*f[1][1]*f[2][3] - f[0][1]*f[1][2]*f[2][3]);

    invF[1][0] = invDET*(f[1][0]*f[2][3]*f[3][2] + f[1][2]*f[2][0]*f[3][3] +
                         f[1][3]*f[2][2]*f[3][0] - f[1][0]*f[2][2]*f[3][3] -
                         f[1][2]*f[2][3]*f[3][0] - f[1][3]*f[2][0]*f[3][2]);

    invF[1][1] = invDET*(f[0][2]*f[2][3]*f[3][0] - f[0][3]*f[2][2]*f[3][0] +
                         f[0][3]*f[2][0]*f[3][2] - f[0][0]*f[2][3]*f[3][2] -
                         f[0][2]*f[2][0]*f[3][3] + f[0][0]*f[2][2]*f[3][3]);

    invF[1][2] = invDET*(f[0][3]*f[1][2]*f[3][0] - f[0][2]*f[1][3]*f[3][0] -
                         f[0][3]*f[1][0]*f[3][2] + f[0][0]*f[1][3]*f[3][2] +
                         f[0][2]*f[1][0]*f[3][3] - f[0][0]*f[1][2]*f[3][3]);

    invF[1][3] = invDET*(f[0][2]*f[1][3]*f[2][0] - f[0][3]*f[1][2]*f[2][0] +
                         f[0][3]*f[1][0]*f[2][2] - f[0][0]*f[1][3]*f[2][2] -
                         f[0][2]*f[1][0]*f[2][3] + f[0][0]*f[1][2]*f[2][3]);

    invF[2][0] = invDET*(f[1][0]*f[2][1]*f[3][3] + f[1][1]*f[2][3]*f[3][0] +
                         f[1][3]*f[2][0]*f[3][1] - f[1][0]*f[2][3]*f[3][1] -
                         f[1][1]*f[2][0]*f[3][3] - f[1][3]*f[2][1]*f[3][0]);

    invF[2][1] = invDET*(f[0][3]*f[2][1]*f[3][0] - f[0][1]*f[2][3]*f[3][0] -
                         f[0][3]*f[2][0]*f[3][1] + f[0][0]*f[2][3]*f[3][1] +
                         f[0][1]*f[2][0]*f[3][3] - f[0][0]*f[2][1]*f[3][3]);

    invF[2][2] = invDET*(f[0][1]*f[1][3]*f[3][0] - f[0][3]*f[1][1]*f[3][0] +
                         f[0][3]*f[1][0]*f[3][1] - f[0][0]*f[1][3]*f[3][1] -
                         f[0][1]*f[1][0]*f[3][3] + f[0][0]*f[1][1]*f[3][3]);

    invF[2][3] = invDET*(f[0][3]*f[1][1]*f[2][0] - f[0][1]*f[1][3]*f[2][0] -
                         f[0][3]*f[1][0]*f[2][1] + f[0][0]*f[1][3]*f[2][1] +
                         f[0][1]*f[1][0]*f[2][3] - f[0][0]*f[1][1]*f[2][3]);

    invF[3][0] = invDET*(f[1][0]*f[2][2]*f[3][1] + f[1][1]*f[2][0]*f[3][2] +
                         f[1][2]*f[2][1]*f[3][0] - f[1][0]*f[2][1]*f[3][2] -
                         f[1][1]*f[2][2]*f[3][0] - f[1][2]*f[2][0]*f[3][1]);

    invF[3][1] = invDET*(f[0][1]*f[2][2]*f[3][0] - f[0][2]*f[2][1]*f[3][0] +
                         f[0][2]*f[2][0]*f[3][1] - f[0][0]*f[2][2]*f[3][1] -
                         f[0][1]*f[2][0]*f[3][2] + f[0][0]*f[2][1]*f[3][2]);

    invF[3][2] = invDET*(f[0][2]*f[1][1]*f[3][0] - f[0][1]*f[1][2]*f[3][0] -
                         f[0][2]*f[1][0]*f[3][1] + f[0][0]*f[1][2]*f[3][1] +
                         f[0][1]*f[1][0]*f[3][2] - f[0][0]*f[1][1]*f[3][2]);

    invF[3][3] = invDET*(f[0][1]*f[1][2]*f[2][0] - f[0][2]*f[1][1]*f[2][0] +
                         f[0][2]*f[1][0]*f[2][1] - f[0][0]*f[1][2]*f[2][1] -
                         f[0][1]*f[1][0]*f[2][2] + f[0][0]*f[1][1]*f[2][2]);
    
    const int numSamples = buffer.getNumSamples();
        
    for (int i = 0; i < numSamples; ++i)
    {
        // signal coming in
        auto channelData = buffer.getReadPointer (0);
        auto leftBuffer  = buffer.getWritePointer (0);
        auto rightBuffer = buffer.getWritePointer (1);
        auto in = channelData[i];

        // Initialize variables
        float d1[4];
        float d2[4];
        float d[4];
        
        // Compute output signal
        for (int j = 0; j < 4; j++)
        {
            d1[j] = e[j][0]*x[0] + e[j][1]*x[1] + e[j][2]*x[2] + e[j][3]*x[3];
            d2[j] = k*in*(e[j][0]*b[0] + e[j][1]*b[1] + e[j][2]*b[2] + e[j][3]*b[3]);
            d[j] = d1[j] + d2[j];
        }
            
        for (int j = 0; j < 4; j++)
        {
            x[j] = invF[j][0]*d[0] + invF[j][1]*d[1] + invF[j][2]*d[2] + invF[j][3]*d[3];
        }
        
        // Write output signal to buffer
        leftBuffer[i]  = x[3];
        rightBuffer[i] = x[3];
    }
}

void MoogVcftrapAudioProcessor::updateCurrentTimeInfoFromHost()
{
    if (AudioPlayHead* ph = getPlayHead())
    {
        AudioPlayHead::CurrentPositionInfo newTime;
            
        if (ph->getCurrentPosition (newTime))
        {
            lastPosInfo = newTime;  // Successfully got the current time from the host..
            return;
        }
    }
        
    // If the host fails to provide the current time, we'll just reset our copy to a default..
    lastPosInfo.resetToDefault();
}
    
//==============================================================================
AudioProcessorEditor* MoogVcftrapAudioProcessor::createEditor()
{
    return new MoogVcftrapAudioProcessorEditor (*this);
}
    
//==============================================================================
void MoogVcftrapAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // Here's an example of how you can use XML to make it easy and more robust:
        
    // Create an outer XML element..
    XmlElement xml ("MYPLUGINSETTINGS");
        
    // add some attributes to it..
    xml.setAttribute ("uiWidth", lastUIWidth);
    xml.setAttribute ("uiHeight", lastUIHeight);
        
    // Store the values of all our parameters, using their param ID as the XML attribute
    for (auto* param : getParameters())
        if (auto* p = dynamic_cast<AudioProcessorParameterWithID*> (param))
            xml.setAttribute (p->paramID, p->getValue());
        
    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xml, destData);
}
    
void MoogVcftrapAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
        
    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
        
    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our last window size..
            lastUIWidth  = jmax (xmlState->getIntAttribute ("uiWidth", lastUIWidth), 400);
            lastUIHeight = jmax (xmlState->getIntAttribute ("uiHeight", lastUIHeight), 200);
                
            // Now reload our parameters..
            for (auto* param : getParameters())
                if (auto* p = dynamic_cast<AudioProcessorParameterWithID*> (param))
                    p->setValue ((float) xmlState->getDoubleAttribute (p->paramID, p->getValue()));
        }
    }
}
    
void MoogVcftrapAudioProcessor::updateTrackProperties (const TrackProperties& properties)
{
    trackProperties = properties;
        
    if (auto* editor = dynamic_cast<MoogVcftrapAudioProcessorEditor*> (getActiveEditor()))
        editor->updateTrackProperties ();
}
    
//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoogVcftrapAudioProcessor();
}
    
    

