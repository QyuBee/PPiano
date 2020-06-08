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

  Common_Update();

  std::string ytUrl;

  std::cout << "Entrez l'URL de la vidéo à jouer" << '\n';
  std::cin >> ytUrl; //Traitement à prévoir

  std::string ytName="media/"+ytUrl+".mp3";

  FILE * fichier = fopen(ytName.c_str(), "r+");

  if (fichier == NULL)
  {
    Common_Update();
    std::cout << "Téléchargement de "<< ytUrl << " en cours" << '\n';
    downloadYt(ytUrl.c_str());

    FILE * fichier = fopen(ytName.c_str(), "r+");
    if (fichier==NULL) {
      std::cerr << '\n' <<"~~~~Le téléchargement de la viéo a échoué !~~~~"<< '\n';
      return 1;
    }
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
        result = system->createSound(ytName.c_str(), FMOD_LOOP_NORMAL, 0, &record[1].sound);
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


        std::vector<int> domFreq = getFreq(record[numFFT]);
      /*  for (int i = 0; i < (int) domFreq.size(); i++) {
          if (domFreq[i] != (record[numFFT].domFreq.empty() ? 0 : record[numFFT].domFreq.back())) {
            record[numFFT].domFreq.emplace_back(domFreq[i]);
          }

      }*/
        record[numFFT].domFreq=domFreq;


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

  std::cout << "\n\n\n DomFreq :\n";
  for (int i = 0; i < (int) record[1].domFreq.size(); i++) {
    std::cout << (float) record[1].domFreq[i] << '\n';
  }

  removeAll(MAX_DRIVERS,record, *system);

  return 0;
}
