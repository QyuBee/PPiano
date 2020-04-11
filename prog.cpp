/*==============================================================================
Custom DSP Example
Copyright (c), Firelight Technologies Pty, Ltd 2004-2020.

This example shows how to add a user created DSP callback to process audio
data. The read callback is executed at runtime, and can be added anywhere in
the DSP network.
==============================================================================*/
#include <fmod.h>
#include <fmod.hpp>
#include <fmod_errors.h>
#include "common.h"
#include <iostream>

#include <vector>

typedef struct
{
    float *buffer;
    float volume_linear;
    int   length_samples;
    int   channels;
} mydsp_data_t;

FMOD_RESULT F_CALLBACK myDSPCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels)
{
    mydsp_data_t *data = (mydsp_data_t *)dsp_state->plugindata;

    /*
        This loop assumes inchannels = outchannels, which it will be if the DSP is created with '0'
        as the number of channels in FMOD_DSP_DESCRIPTION.
        Specifying an actual channel count will mean you have to take care of any number of channels coming in,
        but outputting the number of channels specified. Generally it is best to keep the channel
        count at 0 for maximum compatibility.
    */
    for (unsigned int samp = 0; samp < length; samp++)
    {
        /*
            Feel free to unroll this.
        */
        for (int chan = 0; chan < *outchannels; chan++)
        {
            /*
                This DSP filter just halves the volume!
                Input is modified, and sent to output.
            */
            data->buffer[(samp * *outchannels) + chan] = outbuffer[(samp * inchannels) + chan] = inbuffer[(samp * inchannels) + chan] * data->volume_linear;
        }
    }

    data->channels = inchannels;

    return FMOD_OK;
}

/*
    Callback called when DSP is created.   This implementation creates a structure which is attached to the dsp state's 'plugindata' member.
*/
FMOD_RESULT F_CALLBACK myDSPCreateCallback(FMOD_DSP_STATE *dsp_state)
{
    unsigned int blocksize;
    FMOD_RESULT result;

    result = dsp_state->functions->getblocksize(dsp_state, &blocksize);
    ERRCHECK(result);

    mydsp_data_t *data = (mydsp_data_t *)calloc(sizeof(mydsp_data_t), 1);
    if (!data)
    {
        return FMOD_ERR_MEMORY;
    }
    dsp_state->plugindata = data;
    data->volume_linear = 1.0f;
    data->length_samples = blocksize;

    data->buffer = (float *)malloc(blocksize * 8 * sizeof(float));      // *8 = maximum size allowing room for 7.1.   Could ask dsp_state->functions->getspeakermode for the right speakermode to get real speaker count.
    if (!data->buffer)
    {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

/*
    Callback called when DSP is destroyed.   The memory allocated in the create callback can be freed here.
*/
FMOD_RESULT F_CALLBACK myDSPReleaseCallback(FMOD_DSP_STATE *dsp_state)
{
    if (dsp_state->plugindata)
    {
        mydsp_data_t *data = (mydsp_data_t *)dsp_state->plugindata;

        if (data->buffer)
        {
            free(data->buffer);
        }

        free(data);
    }

    return FMOD_OK;
}

/*
    Callback called when DSP::getParameterData is called.   This returns a pointer to the raw floating point PCM data.
    We have set up 'parameter 0' to be the data parameter, so it checks to make sure the passed in index is 0, and nothing else.
*/
FMOD_RESULT F_CALLBACK myDSPGetParameterDataCallback(FMOD_DSP_STATE *dsp_state, int index, void **data, unsigned int *length, char *)
{
    if (index == 0)
    {
        unsigned int blocksize;
        FMOD_RESULT result;
        mydsp_data_t *mydata = (mydsp_data_t *)dsp_state->plugindata;

        result = dsp_state->functions->getblocksize(dsp_state, &blocksize);
        ERRCHECK(result);

        *data = (void *)mydata;
        *length = blocksize * 2 * (int)sizeof(float);

        return FMOD_OK;
    }

    return FMOD_ERR_INVALID_PARAM;
}

/*
    Callback called when DSP::setParameterFloat is called.   This accepts a floating point 0 to 1 volume value, and stores it.
    We have set up 'parameter 1' to be the volume parameter, so it checks to make sure the passed in index is 1, and nothing else.
*/
FMOD_RESULT F_CALLBACK myDSPSetParameterFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float value)
{
    if (index == 1)
    {
        mydsp_data_t *mydata = (mydsp_data_t *)dsp_state->plugindata;

        mydata->volume_linear = value;

        return FMOD_OK;
    }

    return FMOD_ERR_INVALID_PARAM;
}

/*
    Callback called when DSP::getParameterFloat is called.   This returns a floating point 0 to 1 volume value.
    We have set up 'parameter 1' to be the volume parameter, so it checks to make sure the passed in index is 1, and nothing else.
    An alternate way of displaying the data is provided, as a string, so the main app can use it.
*/
FMOD_RESULT F_CALLBACK myDSPGetParameterFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float *value, char *valstr)
{
    if (index == 1)
    {
        mydsp_data_t *mydata = (mydsp_data_t *)dsp_state->plugindata;

        *value = mydata->volume_linear;
        if (valstr)
        {
            sprintf(valstr, "%d", (int)((*value * 100.0f)+0.5f));
        }

        return FMOD_OK;
    }

    return FMOD_ERR_INVALID_PARAM;
}

int FMOD_Main()
{
    FMOD::System       *system;
    FMOD::Sound        *sound;
    FMOD::Sound        *record;
    FMOD::Channel      *channel;
    FMOD::DSP          *mydsp;
    FMOD::ChannelGroup *mastergroup;
    FMOD_RESULT         result;
    unsigned int        version;
    void               *extradriverdata = NULL;

    #define LATENCY_MS      (50) /* Some devices will require higher latency to avoid glitches */
    #define DRIFT_MS        (1)
    #define DEVICE_INDEX    (0)
    unsigned int samplesRecorded = 0;
    unsigned int samplesPlayed = 0;


    Common_Init(&extradriverdata);

    /*
        Create a System object and initialize.
    */
    result = FMOD::System_Create(&system);
    ERRCHECK(result);

    result = system->getVersion(&version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        Common_Fatal("FMOD lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
    }

    result = system->init(32, FMOD_INIT_NORMAL, extradriverdata);
    ERRCHECK(result);

    /* RECORD */
    {
      int numDrivers = 0;
      result = system->getRecordNumDrivers(NULL, &numDrivers);
      ERRCHECK(result);

      if (numDrivers == 0)
      {
          Common_Fatal("No recording devices found/plugged in!  Aborting.");
      }

      /*
          Determine latency in samples.
      */
      int nativeRate = 0;
      int nativeChannels = 0;
      result = system->getRecordDriverInfo(DEVICE_INDEX, NULL, 0, NULL, &nativeRate, NULL, &nativeChannels, NULL);
      ERRCHECK(result);

      unsigned int driftThreshold = (nativeRate * DRIFT_MS) / 1000;       /* The point where we start compensating for drift */
      unsigned int desiredLatency = (nativeRate * LATENCY_MS) / 1000;     /* User specified latency */
      unsigned int adjustedLatency = desiredLatency;                      /* User specified latency adjusted for driver update granularity */
      int actualLatency = desiredLatency;                                 /* Latency measured once playback begins (smoothened for jitter) */

      /*
          Create user sound to record into, then start recording.
      */
      FMOD_CREATESOUNDEXINFO exinfo = {};
      exinfo.cbsize           = sizeof(FMOD_CREATESOUNDEXINFO);
      exinfo.numchannels      = nativeChannels;
      exinfo.format           = FMOD_SOUND_FORMAT_PCM16;
      exinfo.defaultfrequency = nativeRate;
      exinfo.length           = nativeRate * (int)sizeof(short) * nativeChannels; /* 1 second buffer, size here doesn't change latency */

      result = system->createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &record);
      //result = system->createSound("media/piano.mp3", FMOD_LOOP_NORMAL, 0, &sound);
      ERRCHECK(result);

      result = system->recordStart(DEVICE_INDEX, record, true);
      ERRCHECK(result);

      unsigned int soundLength = 0;
      result = record->getLength(&soundLength, FMOD_TIMEUNIT_PCM);
      ERRCHECK(result);

    } /* RECORD */


    result = system->createSound("media/piano.mp3", FMOD_LOOP_NORMAL, 0, &sound);
    ERRCHECK(result);

    result = system->playSound(sound, 0, false, &channel);
    result = system->playSound(record, 0, false, &channel);
    ERRCHECK(result);

    /*
        Create the DSP effect.
    */
    result= system->createDSPByType(FMOD_DSP_TYPE_FFT,&mydsp);
    ERRCHECK(result);

    result = system->getMasterChannelGroup(&mastergroup);
    ERRCHECK(result);

    result = mastergroup->addDSP(0, mydsp);
    ERRCHECK(result);

    /*
        Main loop.
    */
    do
    {

        Common_Update();


        result = system->update();
        ERRCHECK(result);

        {
            FMOD_DSP_PARAMETER_DESC *desc;
            FMOD_DSP_PARAMETER_FFT  *data;

            result = mydsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
            ERRCHECK(result);

            /*Common_Draw("==================================================");
            Common_Draw("QYUBEE");
            Common_Draw("==================================================");
            Common_Draw("");
            Common_Draw("Press %s to quit", Common_BtnStr(BTN_QUIT));
            Common_Draw("");
            Common_Draw("Test : %s", desc->name);
            */
            const int a=(const int)data->length/10;
            const int b=60;
            std::vector<std::vector<char> > v( a , std::vector<char> (b, 0));

            for (int bin = 0; bin < a; bin++)
            {
              for (int i = 0; i < b; i++) {
                if ((data->spectrum[0][bin]*1000000.-5000.*(float)i)/1.>5000.) {
                  v[bin][i] = '#';
                }
                else {
                  v[bin][i] = ' ';
                }
              }
            }
            for (int i = b-1; i > 0; i--) {
              for (int bin = 0; bin < a; bin++)  {
                std::cout << v[bin][i];
              }
              std::cout  <<"\n";
            }

            char d[32]={};

            result = mydsp->getParameterInfo(3, &desc);
            ERRCHECK(result);

            result = mydsp->getParameterFloat(3,0,d,32);
            ERRCHECK(result);

            Common_Draw(desc->name);
            Common_Draw(d);
          }
        Common_Sleep(50);
    } while (!Common_BtnPress(BTN_QUIT));

    /*
        Shut down
    */
    result = sound->release();
    ERRCHECK(result);

    result = mastergroup->removeDSP(mydsp);
    ERRCHECK(result);
    result = mydsp->release();
    ERRCHECK(result);

    result = system->close();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);

    Common_Close();

    return 0;
}
