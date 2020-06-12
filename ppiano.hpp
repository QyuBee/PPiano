#ifndef PPIANO_H
#define PPIANO_H

#include "fmod.hpp"
#include "common.h"
#include <vector>
#include <iostream>

static const int MAX_DRIVERS = 2;

struct RECORD_STATE
{
    FMOD::Sound         *sound;
    FMOD::Channel       *channel;
    FMOD::DSP           *dsp;
    FMOD::ChannelGroup  *changrp;
    bool                isPlaying;      // A MODIFiER ET FaiRE UN TRUC AUTO COMME DANS ENIBOOK MAIS J'AI PAS COMPRIS
    bool                isRecording;

    std::vector<int> domFreq;
};

FMOD_RESULT F_CALLBACK SystemCallback(FMOD_SYSTEM* /*system*/, FMOD_SYSTEM_CALLBACK_TYPE /*type*/, void *, void *, void *userData);

void
compareFFT(RECORD_STATE *record);

std::vector<int>
getFreq(RECORD_STATE record);

std::vector<int>
getDomFreq(RECORD_STATE record);

std::vector<int>
removeHarmonic(std::vector<int> freq);

void
showFFT(FMOD_DSP_PARAMETER_FFT data, const int column, const int line);

void
printFFT(RECORD_STATE record);

double
getCoef(double d, std::vector<int> domFreq);

FMOD::System*
createSyst(FMOD::ChannelGroup *mastergroup);

void
cleanRecord(RECORD_STATE &record);

void
createDSP(RECORD_STATE &record, FMOD::System &system);

void
createRecord(RECORD_STATE &record, FMOD::System &system);

void
removeAll(int count, RECORD_STATE *record, FMOD::System &system);

void
choicePrint(RECORD_STATE *record,int &numFFT, const char* ytName);

void
playRecord(FMOD::System &system, RECORD_STATE &record);

std::string
downloadYt(const char* urlVideo);

std::string
choseSound();

#endif
