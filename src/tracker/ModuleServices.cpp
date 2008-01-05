/*
 *  ModuleServices.cpp
 *  milkytracker_universal
 *
 *  Created by Peter Barth on 08.12.07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "ModuleServices.h"
#include "SongLengthEstimator.h"
#include "PlayerGeneric.h"
#include "AudioDriver_NULL.h"
#include "XModule.h"

void ModuleServices::estimateSongLength()
{
	SongLengthEstimator estimator(&module);
	estimatedSongLength = estimator.estimateSongLengthInSeconds();
}

pp_int32 ModuleServices::estimateMixerVolume(WAVWriterParameters& parameters, 
											 pp_int32* numSamplesProgressed/* = NULL*/)
{
	PlayerGeneric* player = new PlayerGeneric(parameters.sampleRate);

	player->setBufferSize(1024);
	player->setPlayMode((PlayerGeneric::PlayModes)parameters.playMode);
	player->setResamplerType((ChannelMixer::ResamplerTypes)parameters.resamplerType);
	player->setSampleShift(parameters.mixerShift);
	player->setMasterVolume(256);
	player->setPeakAutoAdjust(true);

	AudioDriver_NULL* audioDriver = new AudioDriver_NULL;

	pp_int32 res = player->exportToWAV(NULL, &module, 
									   parameters.fromOrder, parameters.toOrder, 
									   parameters.muting, 
									   module.header.channum, 
									   parameters.panning, 
									   audioDriver);
	
	if (numSamplesProgressed)
		*numSamplesProgressed = res;
	
	delete audioDriver;

	pp_int32 mixerVolume = player->getMasterVolume();
	
	delete player;
	
	return mixerVolume;
}

pp_int32 ModuleServices::estimateWaveLengthInSamples(WAVWriterParameters& parameters)
{
	PlayerGeneric* player = new PlayerGeneric(parameters.sampleRate);

	player->setBufferSize(1024);
	player->setPlayMode((PlayerGeneric::PlayModes)parameters.playMode);
	player->setResamplerType((ChannelMixer::ResamplerTypes)parameters.resamplerType);
	player->setSampleShift(parameters.mixerShift);
	player->setMasterVolume(256);
	player->setPeakAutoAdjust(false);

	AudioDriver_NULL* audioDriver = new AudioDriver_NULL;

	pp_int32 res = player->exportToWAV(NULL, &module, 
									   parameters.fromOrder, parameters.toOrder, 
									   parameters.muting, 
									   module.header.channum, 
									   parameters.panning, 
									   audioDriver);
	
	delete audioDriver;
	delete player;
	
	return res;
}

pp_int32 ModuleServices::exportToWAV(const SYSCHAR* fileName, WAVWriterParameters& parameters)
{
	PlayerGeneric* player = new PlayerGeneric(parameters.sampleRate);

	player->setBufferSize(1024);
	player->setPlayMode((PlayerGeneric::PlayModes)parameters.playMode);
	player->setResamplerType((ChannelMixer::ResamplerTypes)parameters.resamplerType);
	player->setSampleShift(parameters.mixerShift);
	player->setMasterVolume(parameters.mixerVolume);

	pp_int32 res = player->exportToWAV(fileName, &module, 
									   parameters.fromOrder, parameters.toOrder, 
									   parameters.muting, 
									   module.header.channum, 
									   parameters.panning);
	
	delete player;	
	return res;
}

class BufferWriter : public AudioDriver_NULL
{
private:
	pp_int16*   destBuffer;
	pp_uint32	destBufferSize;
	bool		mono;
	pp_uint32   index;
	
public:
	BufferWriter(pp_int16* buffer, pp_uint32 bufferSize, bool mono) :
		destBuffer(buffer),
		destBufferSize(bufferSize),
		mono(mono)
	{
	}

	virtual mp_sint32 initDevice(mp_sint32 bufferSizeInWords, mp_uint32 mixFrequency, MasterMixer* mixer)
	{
		mp_sint32 res = AudioDriver_NULL::initDevice(bufferSizeInWords, mixFrequency, mixer);
		if (res < 0)
			return res;
			
		index = 0;
		return 0;
	}
	
	virtual void start()
	{
		index = 0;
	}
	
	virtual	const char* getDriverID() { return "BufferWriter"; }

	virtual void advance()
	{
		AudioDriver_NULL::advance();
		
		if (mono)
		{
			for (pp_int32 i = 0; i < bufferSize / MP_NUMCHANNELS; i++)
			{
				pp_int32 res = ((pp_int32)compensateBuffer[i*2] + (pp_int32)compensateBuffer[i*2+1]) >> 1;
				if (res < -32768) res = -32768;
				if (res > 32767) res = 32767;
				if (index < destBufferSize)
					destBuffer[index] = (pp_int16)res;
				index++;
			}
		}
		else
		{
			for (pp_int32 i = 0; i < bufferSize; i++)
			{
				if (index < destBufferSize)
					destBuffer[index] = compensateBuffer[i];
				index++;
			}
		}
		//f->writeWords((mp_uword*)compensateBuffer, bufferSize);
	}
};

pp_int32 ModuleServices::exportToBuffer16Bit(WAVWriterParameters& parameters, pp_int16* buffer, 
											 pp_uint32 bufferSize, bool mono)
{
	PlayerGeneric* player = new PlayerGeneric(parameters.sampleRate);

	player->setBufferSize(1024);
	player->setPlayMode((PlayerGeneric::PlayModes)parameters.playMode);
	player->setResamplerType((ChannelMixer::ResamplerTypes)parameters.resamplerType);
	player->setSampleShift(parameters.mixerShift);
	player->setMasterVolume(parameters.mixerVolume);

	BufferWriter* audioDriver = new BufferWriter(buffer, bufferSize, mono);

	pp_int32 res = player->exportToWAV(NULL, &module, 
									   parameters.fromOrder, parameters.toOrder, 
									   parameters.muting, 
									   module.header.channum, 
									   parameters.panning,
									   audioDriver);
	
	delete player;	
	return res;
}
