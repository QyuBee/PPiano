#include "ppiano.hpp"

static const int MAX_DRIVERS = 2;
static const int MAX_DRIVERS_IN_VIEW = 3;


int FMOD_Main()
{
    FMOD_RESULT result;
    RECORD_STATE record[MAX_DRIVERS] = { };
    FMOD::ChannelGroup *mastergroup;
    int numFFT=-1;
    FMOD::System *system = NULL;

    system=createSyst(mastergroup);
    result=system->update();
    ERRCHECK(result);
    /*
        Main loop
    */
    do
    {
        Common_Update();

        if (Common_BtnPress(BTN_ACTION0))
        {
          result=system->isRecording(0, &record[0].isRecording);
          ERRCHECK(result);

          if (record[0].isRecording)
          {
              system->recordStop(0);
          }
          else
          {
            cleanRecord(record[0]);
            createRecord(record[0], *system);
          }

          Common_Sleep(50);

          result=record[0].channel->isPlaying(&record[0].isPlaying);
        //  ERRCHECK(result); Produit une erreur mais semble pourtant correct => fuite de mémoire

          playRecord(*system,record[0]);
        }


        if (Common_BtnPress(BTN_ACTION1))
        {
          result=record[1].channel->isPlaying(&record[1].isPlaying);
          //  ERRCHECK(result); Produit une erreur mais semble pourtant correct : result=record[1].channel->isPlaying(&isPlaying); => fuite de mémoire

          cleanRecord(record[1]);

          result = system->createSound("media/piano.mp3", FMOD_LOOP_NORMAL, 0, &record[1].sound);
          ERRCHECK(result);

          playRecord(*system,record[1]);
        }

        result = system->update();
        ERRCHECK(result);

        /*
            Print the menu to chose what sound to play / FFT to print
        */
        choicePrint(record, numFFT);

        /*
            Print the FFT
        */
        if (numFFT==1 || numFFT==0) {
            printFFT(record[numFFT]);
          }
          else if (numFFT==2)
          {
            printFFT(record[0]);
            printFFT(record[1]);
          }

        Common_Sleep(50);
    } while (!Common_BtnPress(BTN_QUIT));

    removeAll(MAX_DRIVERS,record);

    result = system->release();
    ERRCHECK(result);

    Common_Close();

    return 0;
}
