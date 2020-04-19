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
#include <string>


static const int MAX_DRIVERS = 2;
static const int MAX_DRIVERS_IN_VIEW = 3;

struct RECORD_STATE
{
    FMOD::Sound         *sound;
    FMOD::Channel       *channel;
    FMOD::DSP           *dsp;
    FMOD::ChannelGroup  *changrp;
};

FMOD_RESULT F_CALLBACK SystemCallback(FMOD_SYSTEM* /*system*/, FMOD_SYSTEM_CALLBACK_TYPE /*type*/, void *, void *, void *userData)
{
    int *recordListChangedCount = (int *)userData;
    *recordListChangedCount = *recordListChangedCount + 1;

    return FMOD_OK;
}

int FMOD_Main()
{
    FMOD_RESULT result;
    RECORD_STATE record[MAX_DRIVERS] = { };
    FMOD::ChannelGroup *mastergroup;
    int numFFT;


    void *extraDriverData = NULL;
    Common_Init(&extraDriverData);

    /*
        Create a System object and initialize.
    */
    FMOD::System *system = NULL;
    result = FMOD::System_Create(&system);
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


    result = system->getMasterChannelGroup(&mastergroup);
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

        if (Common_BtnPress(BTN_ACTION0))
        {
            bool isRecording = false;
            result=system->isRecording(0, &isRecording);
            ERRCHECK(result);

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

                /*
                    Create the DSP effect.
                */
                {
                  result = system->createChannelGroup("Channel", &record[0].changrp);
                  ERRCHECK(result);

                  result=record[0].channel->setChannelGroup(record[0].changrp);
                  ERRCHECK(result);

                  result= system->createDSPByType(FMOD_DSP_TYPE_FFT,&record[0].dsp);
                  ERRCHECK(result);

                  FMOD::ChannelGroup *group;
                  result=record[0].channel->getChannelGroup(&group);
                  ERRCHECK(result);

                  result = group->addDSP(0, record[0].dsp);
                  ERRCHECK(result);
                }

            }
        }
        if (Common_BtnPress(BTN_ACTION1))
        {
          bool isPlaying = false;
          record[1].channel->isPlaying(&isPlaying);

          if (record[1].sound)
          {
              result = record[1].sound->release();
              ERRCHECK(result);
          }
          result = system->createSound("media/piano.mp3", FMOD_LOOP_NORMAL, 0, &record[1].sound);
          ERRCHECK(result);


          if (isPlaying)
          {
            record[1].channel->stop();
          }
          else if (record[1].sound)
          {
            result = system->playSound(record[1].sound, 0, false, &record[1].channel);
            ERRCHECK(result);
            /*
                Create the DSP effect.
            */
            {
              result = system->createChannelGroup("Channel", &record[1].changrp);
              ERRCHECK(result);

              result=record[1].channel->setChannelGroup(record[1].changrp);
              ERRCHECK(result);

              result= system->createDSPByType(FMOD_DSP_TYPE_FFT,&record[1].dsp);
              ERRCHECK(result);

              FMOD::ChannelGroup *group;
              result=record[1].channel->getChannelGroup(&group);
              ERRCHECK(result);

              result = group->addDSP(0, record[1].dsp);
              ERRCHECK(result);
            }
          }
        }

        result = system->update();
        ERRCHECK(result);

        Common_Draw("Press %s to quit", Common_BtnStr(BTN_QUIT));
        bool isPlaying0;
        bool isPlaying1;
        record[0].channel->isPlaying(&isPlaying0);
        record[1].channel->isPlaying(&isPlaying1);
        Common_Draw("%s Press %s start / stop recording",(isPlaying0 ? "(*)" : "( )"), Common_BtnStr(BTN_ACTION0));
        Common_Draw("%s Press %s start / stop playback", (isPlaying1 ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION1));
        Common_Draw("%s Press %s FFT de 0", (numFFT==0 ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION2));
        Common_Draw("%s Press %s FFT de 1", (numFFT==1 ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION3));
        Common_Draw("%s Press %s les 2 FFT", (numFFT==2 ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION4));
        Common_Draw("");
        int numDisplay = Common_Min(numDrivers, MAX_DRIVERS_IN_VIEW);

        char name[256];
        int sampleRate;
        int channels;
        FMOD_GUID guid;
        FMOD_DRIVER_STATE state;

        result = system->getRecordDriverInfo(0, name, sizeof(name), &guid, &sampleRate, NULL, &channels, &state);
        ERRCHECK(result);

        if(Common_BtnPress(BTN_ACTION2))
        {
          if(numFFT==0)
          {
            numFFT=-1;
          }
          else{
            numFFT=0;
          }
        }
        else if(Common_BtnPress(BTN_ACTION3))
        {
          if(numFFT==1)
          {
            numFFT=-1;
          }
          else{
            numFFT=1;
          }
        }
        else if(Common_BtnPress(BTN_ACTION4))
        {
          if(numFFT==2)
          {
            numFFT=-1;
          }
          else{
            numFFT=2;
          }
        }

        if (numFFT==1 || numFFT==0) {
            FMOD_DSP_PARAMETER_DESC *desc;
            FMOD_DSP_PARAMETER_FFT  *data;

            result = record[numFFT].dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
            ERRCHECK(result);

            /* AFFICHE FFT*/
            const int a=(const int)data->length/10;
            const int b=59;
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

            result = record[numFFT].dsp->getParameterInfo(3, &desc);
            ERRCHECK(result);

            result = record[numFFT].dsp->getParameterFloat(3,0,d,32);
            ERRCHECK(result);

            Common_Draw("%s : %s",desc->name,d);
          }
          else if (numFFT==2)
          {
            {
                FMOD_DSP_PARAMETER_DESC *desc;
                FMOD_DSP_PARAMETER_FFT  *data;

                result = record[0].dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
                ERRCHECK(result);

                /* AFFICHE FFT*/
                const int a=(const int)data->length/10;
                const int b=29;
                std::vector<std::vector<char> > v( a , std::vector<char> (b, 0));

                for (int bin = 0; bin < a; bin++)
                {
                  for (int i = 0; i < b; i++) {
                    if ((data->spectrum[0][bin]*1000000.-10000.*(float)i)/1.>10000.) {
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

                result = record[0].dsp->getParameterInfo(3, &desc);
                ERRCHECK(result);

                result = record[0].dsp->getParameterFloat(3,0,d,32);
                ERRCHECK(result);

                Common_Draw("%s 0 : %s",desc->name,d);
              }

              {
                  FMOD_DSP_PARAMETER_DESC *desc;
                  FMOD_DSP_PARAMETER_FFT  *data;

                  result = record[1].dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
                  ERRCHECK(result);

                  /* AFFICHE FFT*/
                  const int a=(const int)data->length/10;
                  const int b=29;
                  std::vector<std::vector<char> > v( a , std::vector<char> (b, 0));

                  for (int bin = 0; bin < a; bin++)
                  {
                    for (int i = 0; i < b; i++) {
                      if ((data->spectrum[0][bin]*1000000.-10000.*(float)i)/1.>10000.) {
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

                  result = record[1].dsp->getParameterInfo(3, &desc);
                  ERRCHECK(result);

                  result = record[1].dsp->getParameterFloat(3,0,d,32);
                  ERRCHECK(result);

                  Common_Draw("%s 1 : %s",desc->name,d);
                }
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
        if (record[i].dsp)
        {
          result = record[i].changrp->removeDSP(record[i].dsp);
          ERRCHECK(result);

          result = record[i].dsp->release();
          ERRCHECK(result);
        }
    }

    result = system->release();
    ERRCHECK(result);

    Common_Close();

    return 0;
}
