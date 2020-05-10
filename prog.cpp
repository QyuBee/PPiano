#include "ppiano.hpp"

int FMOD_Main()
{
  /*
      Initialize variables
  */
  FMOD_RESULT result;

  RECORD_STATE record[MAX_DRIVERS] = { };
  FMOD::ChannelGroup *mastergroup;
  int numFFT=-1;
  FMOD::System *system;

  const char* ytUrl="u9pQFMPAL_s";
  char ytName[200];
  strcpy(ytName,"media/");
  strcat(ytName,ytUrl);
  strcat(ytName,".mp3");

  FILE * fichier = fopen(ytName, "r+");

  if (fichier == NULL)
  {
    downloadYt(ytUrl);
  }
  else
  {
    fclose(fichier);
  }

  /*
    Create the system
  */
  system=createSyst(mastergroup);

  result=system->update();
  ERRCHECK(result);

  /*
      Main loop
  */
  do
  {
    Common_Update();
    /*
        Create/Remove the Piano sound
    */
    if (Common_BtnPress(BTN_ACTION0))
    {
      result=system->isRecording(0, &record[0].isRecording);
      ERRCHECK(result);

      if (record[0].isPlaying)
      {
        result=system->recordStop(0);
        ERRCHECK(result);

        cleanRecord(record[0]);
      }
      else
      {
        createRecord(record[0], *system);
      }

      Common_Sleep(50);

      playRecord(*system,record[0]);

      numFFT=-1;
    }

    /*
        Create/Remove the Record
    */
    if (Common_BtnPress(BTN_ACTION1))
    {
      if (record[1].isPlaying)
      {
        cleanRecord(record[1]);
      }
      else
      {
        result = system->createSound(ytName, FMOD_LOOP_NORMAL, 0, &record[1].sound);
        ERRCHECK(result);
      }

      playRecord(*system,record[1]);
      numFFT=-1;
    }

    result = system->update();
    ERRCHECK(result);

    /*
        Print the menu to chose what sound to play / FFT to print
    */
    if (record[0].sound){record[0].channel->isPlaying(&record[0].isPlaying);}
    else{record[0].isPlaying=0;}
    if (record[1].sound){record[1].channel->isPlaying(&record[1].isPlaying);}
    else{record[1].isPlaying=0;}
    choicePrint(record, numFFT);

    /*
        Print the FFT
    */
    if (numFFT==1 || numFFT==0) {
        printFFT(record[numFFT]);

        FMOD_DSP_PARAMETER_FFT  *data;

        result = record[numFFT].dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&data, 0, 0, 0);
        ERRCHECK(result);

        record[numFFT].domFreq = getFreq(data);
        std::cout << "Fréquence.s propre.s : ";
        for (auto elem : record[numFFT].domFreq) {
          std::cout << elem << " " ;
        }
        std::cout << '\n';
    }
    else if (numFFT==2)
    {
      printFFT(record[0]);

      printFFT(record[1]);

      compareFFT(record);
    }


    Common_Sleep(50);
  } while (!Common_BtnPress(BTN_QUIT));

  for (int i = 0; i < (int) record[1].domFreq.size(); i++) {
    std::cout << "\n\n\n" << (float) record[1].domFreq[i] *24.4  << '\n';
  }

  removeAll(MAX_DRIVERS,record);

  result = system->close();
  ERRCHECK(result);

  result = system->release();
  ERRCHECK(result);

  Common_Close();

  return 0;
}
