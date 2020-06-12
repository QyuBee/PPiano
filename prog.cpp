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

  /*
    Create the system
  */
  system=createSyst(mastergroup);

  result=system->update();
  ERRCHECK(result);


  /*
    Prepare the video to compare
  */

  std::string ytNameStr;

  try
  {
    ytNameStr = choseSound();
  }
  catch(std::string const& erreur)
  {
    std::cerr << '\n' << erreur << '\n';
    return 1;
  }

  const char* ytName= ytNameStr.c_str();

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
    choicePrint(record, numFFT, ytName);

    /*
        Print the FFT
    */
    if (numFFT==1 || numFFT==0) {
        printFFT(record[numFFT]);

        std::vector<int> domFreq = getDomFreq(record[numFFT]);
      /*  for (int i = 0; i < (int) domFreq.size(); i++) {
          if (domFreq[i] != (record[numFFT].domFreq.empty() ? 0 : record[numFFT].domFreq.back())) {
            record[numFFT].domFreq.emplace_back(domFreq[i]);
          }

      }*/
        record[numFFT].domFreq=domFreq;


        std::cout << '\n' << "************************" <<'\n';
        std::cout << "Fréquence.s propre.s : ";
        for (auto elem : record[numFFT].domFreq) {
          std::cout << elem << " " ;
        }
        std::cout << '\n';
        std::cout << "************************" <<'\n';
    }
    else if (numFFT==2)
    {
      printFFT(record[0]);

      printFFT(record[1]);

      compareFFT(record);
    }


    Common_Sleep(50);
  } while (!Common_BtnPress(BTN_QUIT));

  std::cout << "\n\n\n DomFreq :\n";
  for (int i = 0; i < (int) record[1].domFreq.size(); i++) {
    std::cout << (float) record[1].domFreq[i] << '\n';
  }

  removeAll(MAX_DRIVERS,record, *system);

  return 0;
}
