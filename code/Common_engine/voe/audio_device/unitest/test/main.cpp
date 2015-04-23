#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include "webrtc/modules/audio_device/include/audio_device.h"


#include "audio_device.h"

//#pragma comment(lib,"audio_device.lib")



#define SAMPLE_RATE			32000
#define SAMPLES_PER_10MS	(SAMPLE_RATE/100)



int main(int argc, char **argv){
	webrtc::AudioDeviceModule *audio;
	audio = webrtc::CreateAudioDeviceModule(0, webrtc::AudioDeviceModule::kPlatformDefaultAudio);
	audio = CreateAudioDeviceModule(0, webrtc::AudioDeviceModule::kPlatformDefaultAudio);

	//assert(audio);
	//audio->Init();

	int num;
	int ret;

	// Recording device enumeration
	num = audio->RecordingDevices();
	printf("Input devices: %d\n", num);
	for (int i = 0; i<num; i++){
		char name[webrtc::kAdmMaxDeviceNameSize];
		char guid[webrtc::kAdmMaxGuidSize];
		int ret = audio->RecordingDeviceName(i, name, guid);
		if (ret != -1){
			printf("	%d %s %s\n", i, name, guid);
		}
	}


	// Playout device enumeration
	num = audio->PlayoutDevices();
	printf("Output devices: %d\n", num);
	for (int i = 0; i<num; i++){
		char name[webrtc::kAdmMaxDeviceNameSize];
		char guid[webrtc::kAdmMaxGuidSize];
		int ret = audio->PlayoutDeviceName(i, name, guid);
		if (ret != -1){
			printf("	%d %s %s\n", i, name, guid);
		}
	}

	// Playout device selection
	ret = audio->SetPlayoutDevice(0);

	// Native sample rate controls (samples/sec)
	ret = audio->SetPlayoutSampleRate(16000);
	if (ret == -1){
		uint32_t rate = 0;
		audio->PlayoutSampleRate(&rate);
		//log_debug("use resampler for playout, device samplerate: %u", rate);
	}
	
	// Audio transport initialization
	ret = audio->InitPlayout();

	ret = audio->SetRecordingDevice(0);
	ret = audio->SetRecordingSampleRate(16000);
	if (ret == -1){
		uint32_t rate = 0;
		audio->RecordingSampleRate(&rate);
		//log_debug("use resampler for recording, device samplerate: %u", rate);
	}
	ret = audio->InitRecording();


	//AudioTransportImpl callback(audio);
	//ret = audio->RegisterAudioCallback(&callback);

	ret = audio->StartPlayout();

	ret = audio->StartRecording();


	//sleep(2);
	getchar();
	return 0;
}