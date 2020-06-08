#include "ppiano.hpp"
#include <string>
#include <charconv>
#include <cstddef>

#include <array>
#include <memory>

#include <stdlib.h>     /* atoi */


#include <filesystem>

double coefHz = 1; //
double coefNote= 1;
std::vector<int> coef;

FMOD_RESULT F_CALLBACK SystemCallback(FMOD_SYSTEM* /*system*/, FMOD_SYSTEM_CALLBACK_TYPE /*type*/, void *, void *, void *userData)
{
    int *recordListChangedCount = (int *)userData;
    *recordListChangedCount = *recordListChangedCount + 1;

    return FMOD_OK;
}

void
showFFT(FMOD_DSP_PARAMETER_FFT data, const int column, const int line)
{
  /* SHOW the FFT*/
  std::vector<std::string> v(column);
  for (int bin = 0; bin < column; bin++)
  {
    for (int i = 0; i < line; i++) {
      if ((data.spectrum[0][bin]-0.01*(float)i)>0.01) {
        v[i].append("# ");
        if ((float)bin*coefHz>=10.) {
          v[i].append(" ");
        }
        if ((float)bin*coefHz>=100.) {
          v[i].append(" ");
        }
      }
      else {
        v[i].append("  ");
        if ((float)bin*coefHz>=10.) {
          v[i].append(" ");
        }
        if ((float)bin*coefHz>=100.) {
          v[i].append(" ");
        }
      }
    }
  }
  for (int i = 0; i < line; i++)  {
    std::cout << v[line-i] << '\n'; //PB ?
  }
  for (float i = 0.; i < 51.; i++) {
    std::cout << (int)(i*coefHz) << " ";
  }
}

void
compareFFT(RECORD_STATE *record)
{
  FMOD_RESULT result;

  for (int i = 0; i < MAX_DRIVERS; i++) {
    RECORD_STATE elem = record[i];

    elem.domFreq=getFreq(elem);
  }

  std::cout << "************************" <<'\n';
  for (int i = 0; i < (int)record[0].domFreq.size(); i++) {
    for (int j = 0; j < (int)record[1].domFreq.size(); j++) {
      if (abs(record[0].domFreq[i] - record[1].domFreq[j])<((record[0].domFreq[i] + record[1].domFreq[j])/2) * 0.05) {
        std::cout << (record[0].domFreq[i] + record[1].domFreq[j])/2 << '\n';
      }
    }
  }

  std::cout << "************************" <<'\n';
}

std::vector<int>
getFreq(RECORD_STATE record) {
  FMOD_RESULT result;
  int max=0;
  std::vector<int> domFreq;
  FMOD_DSP_PARAMETER_FFT  *data;

  result = record.dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
  ERRCHECK(result);

  for (int bin = 0; bin < data->length/20; bin++) {
    if ((data->spectrum[0][max] <= data->spectrum[0][bin]) && (bin>1)) {
      max=bin;
    }
    else if(data->spectrum[0][max]>0.01 && max>1){
      int i = 0;
      for (auto elem : domFreq) {
        i++;
        if ( !((int) (max) % (int) (elem) < (int) (i*coefNote) || (int) (max) % (int) (elem) > (int) ((elem-i*coefNote))) ) {
          domFreq.emplace_back((int)max*coefHz);
          break;
        }
      }

      if (domFreq.empty()) {
        domFreq.emplace_back((int)max*coefHz);
      }

      max=0;
    }
    else{
      max=0;
    }
  }

  return domFreq;
}

void
printFFT(RECORD_STATE record)
{
  FMOD_RESULT result;

  FMOD_DSP_PARAMETER_DESC *desc;
  FMOD_DSP_PARAMETER_FFT  *data;

  result = record.dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
  ERRCHECK(result);

  //Lenght of the FFT data
  const int column=(const int)data->length/40;
  const int line=10;

  showFFT(*data,column,line);

  char d[32]={};

  /*
      Get the Info (Dominant Frequency)
  */
  result = record.dsp->getParameterInfo(3, &desc);
  ERRCHECK(result);

  result = record.dsp->getParameterFloat(3,0,d,32);
  ERRCHECK(result);

  std::vector<int> domFreq = getFreq(record);

  int c=0;
  std::cout << '\n'<< desc->name << " : " << d << "   " << atoi(d) << '\n';
  if (!domFreq.empty()) {
    coef.emplace_back(atoi(d)/domFreq[0]);
    for (int i = 0; i < (int)coef.size(); i++) {
      c=c+coef[i];
    }
    c=c/((int)coef.size());
    std::cout << domFreq[0] << '\n' << "Coef " << c << '\n';
    std::cout << c * domFreq[0]<< '\n';
  }

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

  result = system->init(2, FMOD_INIT_NORMAL, extraDriverData);
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

  return system;
}

void
cleanRecord(RECORD_STATE &record)
{
  FMOD_RESULT result;
  /* Clean up previous record sound */
  if (record.sound && record.isPlaying)
  {
      result = record.sound->release();
      record.sound=NULL;
      ERRCHECK(result);
  }
  if (record.dsp && record.changrp)
  {
    result = record.changrp->removeDSP(record.dsp);
    ERRCHECK(result);
  }
  if (record.dsp)
  {
    result = record.dsp->release();
    record.dsp=NULL;
    ERRCHECK(result);
  }
  if (record.changrp) {
    result=record.changrp->release();
    record.changrp=NULL;
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
removeAll(int count, RECORD_STATE *record, FMOD::System &system)
{
  FMOD_RESULT result;

  for (int i = 0; i < count; i++)
  {
    cleanRecord(record[i]);
  }

    result = system.close();
    ERRCHECK(result);

    result = system.release();
    ERRCHECK(result);

    Common_Close();
}

void
choicePrint(RECORD_STATE *record,int &numFFT, const char* ytName)
{
  Common_Draw("Press %s to quit", Common_BtnStr(BTN_QUIT));

  Common_Draw("%s Press %s start / stop recording",(record[0].isPlaying ? "(*)" : "( )"), Common_BtnStr(BTN_ACTION0));
  Common_Draw("%s Press %s start / stop %s", (record[1].isPlaying ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION1), ytName);
  Common_Draw("%s Press %s FFT de 0", (numFFT==0 ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION2));
  Common_Draw("%s Press %s FFT de 1", (numFFT==1 ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION3));
  Common_Draw("%s Press %s les 2 FFT", (numFFT==2 ? "(*)" : "( )") ,Common_BtnStr(BTN_ACTION4));
  Common_Draw("");


  if(Common_BtnPress(BTN_ACTION2) && record[0].isPlaying)
  {
    (numFFT==0) ? numFFT=-1 : numFFT=0;
  }
  else if(Common_BtnPress(BTN_ACTION3)&& record[1].isPlaying)
  {
    {(numFFT==1) ? numFFT=-1 : numFFT=1;}
  }
  else if(Common_BtnPress(BTN_ACTION4)&& record[0].isPlaying && record[1].isPlaying)
  {
    (numFFT==2) ? numFFT=-1 : numFFT=2;
  }

}


void
playRecord(FMOD::System &system, RECORD_STATE &record)
{
  FMOD_RESULT result;

  if (record.sound)
  {
    result = system.playSound(record.sound, NULL, false, &record.channel);
    ERRCHECK(result);

    createDSP(record, system);
  }
}


std::string
downloadYt(const char *urlVideo)
{
  std::array<char, 128> buffer;
  std::string result;
  char cmd[200];
  const char* cmdYt = "youtube-dl -o 'media/%(id)s.%(ext)s' --extract-audio --audio-format mp3 https://youtu.be/";
  strcpy(cmd,cmdYt);
  strcat(cmd,urlVideo);
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
      if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
          result += buffer.data();
  }
  return result;
}

std::string
choseSound()
{
  std::string ytName;
  std::string ytUrl;

  namespace fs = std::filesystem;
  std::string path = "./media/";

  std::vector<std::string> file;
  for (const auto & entry : fs::directory_iterator(path))
  {
      std::cout << entry.path() << '\n';
      file.emplace_back((std::string)entry.path() );
  }

  int cursor=0;

  do {
    Common_Update();
    int i=0;

    std::cout << "(0) Download on YT"  << '\n';
    for (const auto & song : file)
    {
        i++;
        std::cout << "(" << i << ") " << song << '\n';
    }


    if (Common_BtnPress(BTN_UP))
    {
      cursor=cursor+1;
    }
    else if(Common_BtnPress(BTN_DOWN) && cursor>0)
    {
      cursor=cursor-1;
    }
    std::cout <<  cursor << '\n';
    Common_Sleep(50);
  } while(!Common_BtnPress(BTN_MORE));

  if(cursor==0)
  {
    std::cout << "Entrez l'URL de la vidéo à jouer" << '\n';
    std::cin >> ytUrl; //Traitement à prévoir

    ytName="media/"+ytUrl+".mp3";

    FILE * fichier = fopen(ytName.c_str(), "r+");

    if (fichier == NULL)
    {
      Common_Update();
      std::cout << "Téléchargement de "<< ytUrl << " en cours" << '\n';
      downloadYt(ytUrl.c_str());

      FILE * fichier = fopen(ytName.c_str(), "r+");
      if (fichier==NULL) {
        std::cerr << '\n' <<"~~~~Le téléchargement de la viéo a échoué !~~~~"<< '\n';
      }
    }
    else
    {
      fclose(fichier);
    }
  }
  else
  {
    ytName=file[cursor-1];
  }


  return ytName;
}
