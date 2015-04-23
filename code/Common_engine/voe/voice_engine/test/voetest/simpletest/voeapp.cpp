#include "stdafx.h"
#include "voeapp.h"


CVOETestApp::CVOETestApp()
	: 
	_vePtr(NULL),
	_veBasePtr(NULL),
	_veCodecPtr(NULL),
	_veNetworkPtr(NULL),
	// _veFilePtr(NULL),
	_veHardwarePtr(NULL),
	_veExternalMediaPtr(NULL),
	_veApmPtr(NULL),
	_veEncryptionPtr(NULL),
	_veRtpRtcpPtr(NULL),
	_transportPtr(NULL),
	_encryptionPtr(NULL),
	_externalMediaPtr(NULL),
	_externalTransport(false),
	_externalTransportBuild(false),
	_delayEstimate1(false),
	_delayEstimate2(false),
	_rxVad(false),
	_nErrorCallbacks(0),
	_timerTicks(0),
	_channel(0)
{
	

	_vePtr = VoiceEngine::Create();

	VoiceEngine::SetTraceFilter(kTraceNone);
	// VoiceEngine::SetTraceFilter(kTraceAll);
	// VoiceEngine::SetTraceFilter(kTraceStream | kTraceStateInfo | kTraceWarning | kTraceError | kTraceCritical | kTraceApiCall | kTraceModuleCall | kTraceMemory | kTraceDebug | kTraceInfo);
	// VoiceEngine::SetTraceFilter(kTraceStateInfo | kTraceWarning | kTraceError | kTraceCritical | kTraceApiCall | kTraceModuleCall | kTraceMemory | kTraceInfo);

	VoiceEngine::SetTraceFile("ve_win_test.txt");
	VoiceEngine::SetTraceCallback(NULL);

	if (_vePtr)
	{
		_veExternalMediaPtr = VoEExternalMedia::GetInterface(_vePtr);
		_veVolumeControlPtr = VoEVolumeControl::GetInterface(_vePtr);
		_veEncryptionPtr = VoEEncryption::GetInterface(_vePtr);
		_veVideoSyncPtr = VoEVideoSync::GetInterface(_vePtr);
		_veNetworkPtr = VoENetwork::GetInterface(_vePtr);
		// _veFilePtr = VoEFile::GetInterface(_vePtr);
		_veApmPtr = VoEAudioProcessing::GetInterface(_vePtr);

		_veBasePtr = VoEBase::GetInterface(_vePtr);
		_veCodecPtr = VoECodec::GetInterface(_vePtr);
		_veHardwarePtr = VoEHardware::GetInterface(_vePtr);
		_veRtpRtcpPtr = VoERTP_RTCP::GetInterface(_vePtr);
		_transportPtr = new MyTransport(_veNetworkPtr);
		//_encryptionPtr = new MyEncryption();
		_externalMediaPtr = new MediaProcessImpl();
		_connectionObserverPtr = new ConnectionObserver();
		_rxVadObserverPtr = new RxCallback();
	}

	_veBasePtr->RegisterVoiceEngineObserver(*this);

// 	std::string resource_path = webrtc::test::ProjectRootPath();
// 	if (resource_path == webrtc::test::kCannotFindProjectRootDir) {
// 		_long_audio_file_path = "./";
// 	} else {
// 		_long_audio_file_path = resource_path + "data\\voice_engine\\";
// 	}
}

CVOETestApp::~CVOETestApp()
{
	Terminate();
	
	if (_connectionObserverPtr) delete _connectionObserverPtr;
	if (_externalMediaPtr) delete _externalMediaPtr;
	if (_transportPtr) delete _transportPtr;
//	if (_encryptionPtr) delete _encryptionPtr;
	if (_rxVadObserverPtr) delete _rxVadObserverPtr;

	if (_veExternalMediaPtr) _veExternalMediaPtr->Release();
	if (_veEncryptionPtr) _veEncryptionPtr->Release();
	if (_veVideoSyncPtr) _veVideoSyncPtr->Release();
	if (_veVolumeControlPtr) _veVolumeControlPtr->Release();

	if (_veBasePtr) _veBasePtr->Terminate();
	if (_veBasePtr) _veBasePtr->Release();

	if (_veCodecPtr) _veCodecPtr->Release();
	if (_veNetworkPtr) _veNetworkPtr->Release();
	// if (_veFilePtr) _veFilePtr->Release();
	if (_veHardwarePtr) _veHardwarePtr->Release();
	if (_veApmPtr) _veApmPtr->Release();
	if (_veRtpRtcpPtr) _veRtpRtcpPtr->Release();
	if (_vePtr)
	{
		VoiceEngine::Delete(_vePtr);
	}
	VoiceEngine::SetTraceFilter(kTraceNone);
}

int CVOETestApp::Init()
{
	_veBasePtr->Init() ;
	_channel = _veBasePtr->CreateChannel();
	_veRtpRtcpPtr->RegisterRTPObserver(_channel, *this);
	CodecInst codec;
	_veCodecPtr->GetCodec(0, codec);
	_veCodecPtr->SetSendCodec(_channel, codec);
	_veNetworkPtr->RegisterExternalTransport(_channel, *_transportPtr);
	return 0;
}

int CVOETestApp::Terminate()
{

	_rxVad = false;
	Stop();
	_veNetworkPtr->DeRegisterExternalTransport(_channel);
	_veRtpRtcpPtr->DeRegisterRTPObserver(_channel);
	_veBasePtr->DeleteChannel(_channel); 
	return 0;
}
int CVOETestApp::Start()
{
	 _veBasePtr->StartReceive(_channel);
	 _veBasePtr->StartPlayout(_channel);
	 _veBasePtr->StartSend(_channel);
	 return 0;
}
int CVOETestApp::Stop()
{
	_veBasePtr->StopPlayout(_channel);
	_veBasePtr->StopReceive(_channel);
	
	_veBasePtr->StopSend(_channel);
	return 0;
}
///

MyTransport::MyTransport(VoENetwork* veNetwork) :
_veNetworkPtr(veNetwork)
{
}

int
	MyTransport::SendPacket(int channel, const void *data, int len)
{
	_veNetworkPtr->ReceivedRTPPacket(channel, data, len);
	return len;
}

int
	MyTransport::SendRTCPPacket(int channel, const void *data, int len)
{
	_veNetworkPtr->ReceivedRTCPPacket(channel, data, len);
	return len;
}

///

MediaProcessImpl::MediaProcessImpl()
{
}

void MediaProcessImpl::Process(int channel,
	ProcessingTypes type,
	int16_t audio_10ms[],
	int length,
	int samplingFreqHz,
	bool stereo)
{
	int x = rand() % 100;

	for (int i = 0; i < length; i++)
	{
		if (channel == -1)
		{
			if (type == kPlaybackAllChannelsMixed)
			{
				// playout: scale up
				if (!stereo)
				{
					audio_10ms[i] = (audio_10ms[i] << 2);
				}
				else
				{
					audio_10ms[2*i] = (audio_10ms[2*i] << 2);
					audio_10ms[2*i+1] = (audio_10ms[2*i+1] << 2);
				}
			}
			else
			{
				// recording: emulate packet loss by "dropping" 10% of the packets
				if (x >= 0 && x < 10)
				{
					if (!stereo)
					{
						audio_10ms[i] = 0;
					}
					else
					{
						audio_10ms[2*i] = 0;
						audio_10ms[2*i+1] = 0;
					}
				}
			}
		}
		else
		{
			if (type == kPlaybackPerChannel)
			{
				// playout: mute
				if (!stereo)
				{
					audio_10ms[i] = 0;
				}
				else
				{
					audio_10ms[2*i] = 0;
					audio_10ms[2*i+1] = 0;
				}
			}
			else
			{
				// recording: emulate packet loss by "dropping" 50% of the packets
				if (x >= 0 && x < 50)
				{
					if (!stereo)
					{
						audio_10ms[i] = 0;
					}
					else
					{
						audio_10ms[2*i] = 0;
						audio_10ms[2*i+1] = 0;
					}
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------
//    VoEConnectionObserver
// ----------------------------------------------------------------------------



ConnectionObserver::ConnectionObserver()
{
};

void ConnectionObserver::OnPeriodicDeadOrAlive(int channel, bool alive)
{
	
};

// ----------------------------------------------------------------------------
//    VoiceEngineObserver
// ----------------------------------------------------------------------------

void CVOETestApp::CallbackOnError(int channel, int errCode)
{
	
}

// ----------------------------------------------------------------------------
//    VoERTPObserver
// ----------------------------------------------------------------------------

void CVOETestApp::OnIncomingCSRCChanged(int channel, unsigned int CSRC, bool added)
{
	
}

void CVOETestApp::OnIncomingSSRCChanged(int channel, unsigned int SSRC)
{
	
}