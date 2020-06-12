#include "ppiano.hpp"
#include <string>
#include <charconv>
#include <cstddef>

#include <array>
#include <memory>

#include <stdlib.h>     /* atoi */

//#include <algorithm>

#include <filesystem>

//#include <gtkmm.h>

double coefHz = 2.93; // 2.93
double coefNote= 1.; // 3 * ...
double coefError=1.5;
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
        if ((float)bin*coefHz>=1000.) {
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
        if ((float)bin*coefHz>=1000.) {
          v[i].append(" ");
        }
      }
    }
  }
  for (int i = 0; i < line; i++)  {
    std::cout << v[line-i] << '\n';
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

    //elem.domFreq=getFreq(elem);
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
getFreq(RECORD_STATE record)
{
  FMOD_RESULT result;
  int max=0;
  std::vector<int> freq;
  FMOD_DSP_PARAMETER_FFT  *data;

  result = record.dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
  ERRCHECK(result);

  for (int bin = 0; bin < data->length/2; bin++)
  {
    if ((data->spectrum[0][max] <= data->spectrum[0][bin]) && (bin>1)) {        //Je recherche le pic de freq
      max=bin;
    }
    else if(data->spectrum[0][max]>0.01 && max>1 ){                              //J'ajoute la freqDom
      if (freq.empty()) {
        freq.emplace_back((int)max*coefHz);
      }
      else if((freq.back() !=(int)(max*coefHz)))
      {
        freq.emplace_back((int)max*coefHz);
      }
      else{
        max=0;
      }
    }
    else{
      max=0;
    }
  }
  return freq;
}

std::vector<int>
getDomFreq(RECORD_STATE record)
{
  std::vector<int> freq;
  freq=getFreq(record);
  freq=removeHarmonic(freq);
  return freq;
}

std::vector<int>
removeHarmonic(std::vector<int> freq)
{
  std::vector<int> domFreq;

  std::cout << "************************" <<'\n';
  std::cout << "HARMONIQUE  : ";

  if (!freq.empty()) {
    domFreq = freq;
    for (auto elem : freq) {
      auto it=std::begin(domFreq);
      while ( it!=std::end(domFreq) ) {
        int dom=*it;

        if (dom/elem>1.5) {
          std::cout << "domfreq :" << dom << ", elem :"<< elem << ", % :"<< (int)(dom % elem) << " result :" <<((dom/elem>1.5) && (((int)(dom+coefError*floor(dom/elem))%elem < 10) || ((int)(dom-coefError*floor(dom/elem))%elem <10) )) << '\n';
          std::cout << (int)(dom+coefError*floor(dom/elem))%elem << " " << (int)(dom-coefError*floor(dom/elem))%elem << '\n';
        }

        if (
          (dom/elem>1.5) &&
          (
            ((int)(dom+coefError*floor(dom/elem))%elem < 10)
            ||
            ((int)(dom-coefError*floor(dom/elem))%elem <10)
          )
        )
        {
          domFreq.erase(it);
        }
        else it++;
      }
    }
  }
  std::cout << "\n" <<"************************" <<'\n';
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

  std::cout  << "\n\n"<< desc->name << " : " << d << "   " << atoi(d) << '\n';
}



double
getCoef(double d, std::vector<int> domFreq)
{
  /*
      Get the coefHz
  */

  std::cout<< "\n\n" << "************************" <<'\n';
  double c;
  if (!domFreq.empty()) {
    coef.emplace_back((double)(d/domFreq[0]));
    for (int i = 0; i < (int)coef.size(); i++) {
      c=c+coef[i];
    }
    c=c/((int)coef.size());
    std::cout << domFreq[0] << '\n' << "Coef " << c << '\n';
    std::cout << c * domFreq[0]<< '\n';
  }

  std::cout << "************************" <<'\n';
  return c;
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

  int window = 16384;
  result=record.dsp->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, window);
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

    result = record.channel->setVolume((float)1);
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
  file.emplace_back("Télécharger sur YT");
  for (const auto & entry : fs::directory_iterator(path))
  {
      file.emplace_back((std::string)entry.path() );
  }

  int cursor=0;

  do {
    Common_Update();
    int i=0;

    std::cout << "Choisissez la vidéo à jouer en appuyant sur "<< Common_BtnStr(BTN_UP) << " et "<<  Common_BtnStr(BTN_DOWN) << '\n';
    for (const auto & song : file)
    {
        std::cout << "(" << i << ") " << song << '\n';
        i++;
    }
    std::cout << "\n\n" <<"Appuyez sur "<< Common_BtnStr(BTN_ENTER) << " pour jouer la vidéo : "<< file[cursor] << '\n';


    if (Common_BtnPress(BTN_UP) && cursor<(int)file.size()-1)
    {
      cursor=cursor+1;
    }
    else if(Common_BtnPress(BTN_DOWN) && cursor>0)
    {
      cursor=cursor-1;
    }

    Common_Sleep(50);
  } while(!Common_BtnPress(BTN_ENTER));

  if(cursor==0)
  {
    Common_Update();

    std::cout << "Entrez l'URL de la vidéo à jouer" << '\n';
    std::cin >> ytUrl; //Traitement à prévoir

    ytName="media/"+ytUrl+".mp3";

    FILE * fichier = fopen(ytName.c_str(), "r+");

    if (fichier == NULL)
    {
      Common_Update();
      std::cout << "~~~~Téléchargement de "<< ytUrl << " en cours~~~~" << '\n';
      downloadYt(ytUrl.c_str());

      FILE * fichier = fopen(ytName.c_str(), "r+");
      if (fichier==NULL) {
        throw std::string("~~~~Le téléchargement de la viéo a échoué !~~~~");
      }
    }
    else
    {
      fclose(fichier);
    }
  }
  else
  {
    ytName=file[cursor];
  }


  return ytName;
}

/*void
showWindow(std::vector<std::string> fft)
{
  for (auto elem:fft) {
    Gtk::Label etiquette(elem+'\n');
  }
}
*/
