/*==============================================================================
Record enumeration example
Copyright (c), Firelight Technologies Pty, Ltd 2004-2020.

This example shows how to enumerate the available recording drivers on this
device. It demonstrates how the enumerated list changes as microphones are
attached and detached. It also shows that you can record from multi mics at
the same time.

Please note to minimize latency care should be taken to control the number
of samples between the record position and the play position. Check the record
example for details on this process.
==============================================================================*/
#include "fmod.hpp"
#include "common.h"
#include <vector>
#include <iostream>


static const int MAX_DRIVERS = 2;
static const int MAX_DRIVERS_IN_VIEW = 3;

struct RECORD_STATE
{
    FMOD::Sound *sound;
    FMOD::Channel *channel;
};

FMOD_RESULT F_CALLBACK SystemCallback(FMOD_SYSTEM* /*system*/, FMOD_SYSTEM_CALLBACK_TYPE /*type*/, void *, void *, void *userData)
{
    int *recordListChangedCount = (int *)userData;
    *recordListChangedCount = *recordListChangedCount + 1;

    return FMOD_OK;
}

int FMOD_Main()
{
    RECORD_STATE record[MAX_DRIVERS] = { };
    FMOD::DSP          *mydsp;
    FMOD::ChannelGroup *mastergroup;


    void *extraDriverData = NULL;
    Common_Init(&extraDriverData);

    /*
        Create a System object and initialize.
    */
    FMOD::System *system = NULL;
    FMOD_RESULT result = FMOD::System_Create(&system);
    ERRCHECK(result);

    unsigned int version = 0;
    result = system->getVersion(&version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        Common_Fatal("FMOD lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
    }

    result = system->init(100, FMOD_INIT_NORMAL, extraDriverData);
    ERRCHECK(result);

    /*
        Setup a callback so we can be notified if the record list has changed.
    */
    int recordListChangedCount = 0;
    result = system->setUserData(&recordListChangedCount);
    ERRCHECK(result);

    result = system->setCallback(&SystemCallback, FMOD_SYSTEM_CALLBACK_RECORDLISTCHANGED);
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
        Main loop
    */
    do
    {
        Common_Update();

        int numDrivers = 0;
        int numConnected = 0;
        result = system->getRecordNumDrivers(&numDrivers, &numConnected);
        ERRCHECK(result);

        numDrivers = Common_Min(numDrivers, MAX_DRIVERS); /* Clamp the reported number of drivers to simplify this example */

        if (Common_BtnPress(BTN_ACTION1))
        {
            bool isRecording = false;
            system->isRecording(0, &isRecording);

            if (isRecording)
            {
                system->recordStop(0);
            }
            else
            {
                /* Clean up previous record sound */
                if (record[0].sound)
                {
                    result = record[0].sound->release();
                    ERRCHECK(result);
                }

                /* Query device native settings and start a recording */
                int nativeRate = 0;
                int nativeChannels = 0;
                result = system->getRecordDriverInfo(0, NULL, 0, NULL, &nativeRate, NULL, &nativeChannels, NULL);
                ERRCHECK(result);

                FMOD_CREATESOUNDEXINFO exinfo = {};
                exinfo.cbsize           = sizeof(FMOD_CREATESOUNDEXINFO);
                exinfo.numchannels      = nativeChannels;
                exinfo.format           = FMOD_SOUND_FORMAT_PCM16;
                exinfo.defaultfrequency = nativeRate;
                exinfo.length           = nativeRate * (int) sizeof(short) * nativeChannels; /* 1 second buffer, size here doesn't change latency */

                result = system->createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &record[0].sound);
                ERRCHECK(result);

                result = system->recordStart(0, record[0].sound, true);
                if (result != FMOD_ERR_RECORD_DISCONNECTED)
                {
                    ERRCHECK(result);
                }
            }
            Common_Sleep(50);

            bool isPlaying = false;
            record[0].channel->isPlaying(&isPlaying);

            if (isPlaying)
            {
                record[0].channel->stop();
            }
            else if (record[0].sound)
            {
                result = system->playSound(record[0].sound, NULL, false, &record[0].channel);
                ERRCHECK(result);
            }
        }


        result = system->update();
        ERRCHECK(result);

        Common_Draw("Record list has updated %d time(s).", recordListChangedCount);
        Common_Draw("Currently %d device(s) plugged in.", numConnected);
        Common_Draw("");
        Common_Draw("Press %s to quit", Common_BtnStr(BTN_QUIT));
        Common_Draw("Press %s start / stop recording", Common_BtnStr(BTN_ACTION1));
        Common_Draw("Press %s start / stop playback", Common_BtnStr(BTN_ACTION2));
        Common_Draw("");
        int numDisplay = Common_Min(numDrivers, MAX_DRIVERS_IN_VIEW);

        char name[256];
        int sampleRate;
        int channels;
        FMOD_GUID guid;
        FMOD_DRIVER_STATE state;

        result = system->getRecordDriverInfo(0, name, sizeof(name), &guid, &sampleRate, NULL, &channels, &state);
        ERRCHECK(result);

        bool isRecording = false;
        system->isRecording(0, &isRecording);

        bool isPlaying = false;
        record[0].channel->isPlaying(&isPlaying);

        {
            FMOD_DSP_PARAMETER_DESC *desc;
            FMOD_DSP_PARAMETER_FFT  *data;

            result = mydsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
            ERRCHECK(result);

            /* AFFICHE FFT*/
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

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (record[i].sound)
        {
            result = record[i].sound->release();
            ERRCHECK(result);
        }
    }

    result = system->release();
    ERRCHECK(result);

    Common_Close();

    return 0;
}
