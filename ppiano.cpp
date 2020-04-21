#include "ppiano.hpp"


FMOD_RESULT F_CALLBACK SystemCallback(FMOD_SYSTEM* /*system*/, FMOD_SYSTEM_CALLBACK_TYPE /*type*/, void *, void *, void *userData)
{
    int *recordListChangedCount = (int *)userData;
    *recordListChangedCount = *recordListChangedCount + 1;

    return FMOD_OK;
}


int
printFFT(RECORD_STATE record)
{
  FMOD_RESULT result;

  FMOD_DSP_PARAMETER_DESC *desc;
  FMOD_DSP_PARAMETER_FFT  *data;

  result = record.dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
  ERRCHECK(result);

  const int a=(const int)data->length/10;
  const int b=29;

  /* AFFICHE FFT*/

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

  result = record.dsp->getParameterInfo(3, &desc);
  ERRCHECK(result);

  result = record.dsp->getParameterFloat(3,0,d,32);
  ERRCHECK(result);

  Common_Draw("%s 0 : %s",desc->name,d);
  return 1;
}

FMOD::System*
createSyst(FMOD::ChannelGroup *mastergroup)
{
  /*
      Create a System object and initialize.
  */
  FMOD_RESULT result;
  FMOD::System *system = NULL;
  void *extraDriverData = NULL;
  Common_Init(&extraDriverData);

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

  return std::move(system);
}


void
cleanRecord(RECORD_STATE &record)
{
  FMOD_RESULT result;
  /* Clean up previous record sound */
  if (record.sound)
  {
      result = record.sound->release();
      ERRCHECK(result);
  }
}

void
createDSP(RECORD_STATE &record, FMOD::System &system)
{
  FMOD_RESULT result;

  /*
      Create the DSP effect.
  */
  result = system.createChannelGroup("Channel", &record.changrp);
  ERRCHECK(result);

  result=record.channel->setChannelGroup(record.changrp);
  ERRCHECK(result);

  result= system.createDSPByType(FMOD_DSP_TYPE_FFT,&record.dsp);
  ERRCHECK(result);

  result = record.changrp->addDSP(0, record.dsp);
  ERRCHECK(result);
}

void
createRecord(RECORD_STATE &record, FMOD::System &system)
{
  FMOD_RESULT result;

  /* Query device native settings and start a recording */
  int nativeRate = 0;
  int nativeChannels = 0;
  result = system.getRecordDriverInfo(0, NULL, 0, NULL, &nativeRate, NULL, &nativeChannels, NULL);
  ERRCHECK(result);

  FMOD_CREATESOUNDEXINFO exinfo = {};
  exinfo.cbsize           = sizeof(FMOD_CREATESOUNDEXINFO);
  exinfo.numchannels      = nativeChannels;
  exinfo.format           = FMOD_SOUND_FORMAT_PCM16;
  exinfo.defaultfrequency = nativeRate;
  exinfo.length           = nativeRate * (int) sizeof(short) * nativeChannels; /* 1 second buffer, size here doesn't change latency */

  result = system.createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &record.sound);
  ERRCHECK(result);

  result = system.recordStart(0, record.sound, true);


  if (result != FMOD_ERR_RECORD_DISCONNECTED)
  {
      ERRCHECK(result);
  }
}

void
removeAll(int count, RECORD_STATE *record)
{
  FMOD_RESULT result;

  for (int i = 0; i < count; i++)
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
}

void
choicePrint(RECORD_STATE *record,int &numFFT)
{
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


  if(Common_BtnPress(BTN_ACTION2) && isPlaying0)
  {
    (numFFT==0) ? numFFT=-1 : numFFT=0;
  }
  else if(Common_BtnPress(BTN_ACTION3)&& isPlaying1)
  {
    {(numFFT==1) ? numFFT=-1 : numFFT=1;}
  }
  else if(Common_BtnPress(BTN_ACTION4)&& isPlaying0 && isPlaying1)
  {
    (numFFT==2) ? numFFT=-1 : numFFT=2;
  }

}


void
playRecord(FMOD::System &system, RECORD_STATE &record)
{
  FMOD_RESULT result;
  
  if (record.isPlaying)
  {
      record.channel->stop();
  }
  else if (record.sound)
  {
    result = system.playSound(record.sound, NULL, false, &record.channel);
    ERRCHECK(result);

    createDSP(record, system);
  }
}
