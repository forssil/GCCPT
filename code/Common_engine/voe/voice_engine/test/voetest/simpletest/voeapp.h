#ifndef _VOICE_ENGINE_TEST_WIN_TEST_WINTESTDLG_H_
#define _VOICE_ENGINE_TEST_WIN_TEST_WINTESTDLG_H_

#include "stdafx.h"
#include <string>
#include "voe/voice_engine/include/voe_base.h"
#include "voe/voice_engine/include/voe_codec.h"
#include "voe/voice_engine/include/voe_dtmf.h"
#include "voe/voice_engine/include/voe_encryption.h"
#include "voe/voice_engine/include/voe_external_media.h"
#include "voe/voice_engine/include/voe_file.h"
#include "voe/voice_engine/include/voe_hardware.h"
#include "voe/voice_engine/include/voe_network.h"
#include "voe/voice_engine/include/voe_rtp_rtcp.h"
#include "voe/voice_engine/include/voe_video_sync.h"
#include "voe/voice_engine/include/voe_volume_control.h"

#include "voe/voice_engine/include/voe_audio_processing.h"
#include "voe/voice_engine/include/voe_errors.h"
#include "voe/voice_engine/include/voe_rtp_rtcp.h"
using namespace webrtc;
class MediaProcessImpl;
class ConnectionObserver;
class MyEncryption;
class RxCallback;
class MyTransport;

#define MAX_NUM_OF_CHANNELS    10
class CVOETestApp :
	public VoiceEngineObserver,
	public VoERTPObserver
{
	// Construction
public:
	CVOETestApp();    // standard constructor
	virtual ~CVOETestApp();
protected:  // VoiceEngineObserver
	virtual void CallbackOnError(int channel, int errCode);
protected:    // VoERTPObserver
	virtual void OnIncomingCSRCChanged(
		int channel, unsigned int CSRC, bool added);
	virtual void OnIncomingSSRCChanged(
		int channel, unsigned int SSRC);


public:
	int Init();
	int Terminate();
	int Start();
	int Stop();
private:
	int _channel;
	VoiceEngine*    _vePtr;

	VoECodec*               _veCodecPtr;
	VoEExternalMedia*       _veExternalMediaPtr;
	VoEVolumeControl*       _veVolumeControlPtr;
	VoEEncryption*          _veEncryptionPtr;
	VoEHardware*            _veHardwarePtr;
	VoEVideoSync*           _veVideoSyncPtr;
	VoENetwork*             _veNetworkPtr;
	//VoEFile*                _veFilePtr;
	VoEAudioProcessing*     _veApmPtr;
	VoEBase*                _veBasePtr;
	VoERTP_RTCP*            _veRtpRtcpPtr;

	MyTransport*            _transportPtr;
	MediaProcessImpl*       _externalMediaPtr;
	ConnectionObserver*     _connectionObserverPtr;
	MyEncryption*           _encryptionPtr;
	RxCallback*             _rxVadObserverPtr;

	bool                    _delayEstimate1;
	bool                    _delayEstimate2;
	bool                    _rxVad;
	int                     _nErrorCallbacks;
	int                     _timerTicks;

	bool _externalTransport;
		bool _externalTransportBuild;
};

class ConnectionObserver : public  VoEConnectionObserver
{
public:
	ConnectionObserver();
	virtual void OnPeriodicDeadOrAlive(int channel, bool alive);
};


// ----------------------------------------------------------------------------
//    Transport
// ----------------------------------------------------------------------------

class MyTransport : public Transport
{
public:
	MyTransport(VoENetwork* veNetwork);
	virtual int SendPacket(int channel, const void *data, int len);
	virtual int SendRTCPPacket(int channel, const void *data, int len);
private:
	VoENetwork* _veNetworkPtr;
};


// ----------------------------------------------------------------------------
//    VoEMediaProcess
// ----------------------------------------------------------------------------

class MediaProcessImpl : public VoEMediaProcess
{
public:
	MediaProcessImpl();
	virtual void Process(int channel,
		ProcessingTypes type,
		int16_t audio_10ms[],
		int length,
		int samplingFreqHz,
		bool stereo);
};

// ----------------------------------------------------------------------------
//    Encryptionen
// ----------------------------------------------------------------------------
// 
// class MyEncryption : public Encryption
// {
// public:
// 	void encrypt(int channel_no, unsigned char * in_data, unsigned char * out_data, int bytes_in, int* bytes_out);
// 	void decrypt(int channel_no, unsigned char * in_data, unsigned char * out_data, int bytes_in, int* bytes_out);
// 	void encrypt_rtcp(int channel_no, unsigned char * in_data, unsigned char * out_data, int bytes_in, int* bytes_out);
// 	void decrypt_rtcp(int channel_no, unsigned char * in_data, unsigned char * out_data, int bytes_in, int* bytes_out);
// };

// ----------------------------------------------------------------------------
//    RxVadCallback
// ----
class RxCallback : public VoERxVadCallback
{
public:
	RxCallback() : vad_decision(-1) {};

	virtual void OnRxVad(int , int vadDecision)
	{
		vad_decision = vadDecision;
	}

	int vad_decision;
};
#endif //_VOICE_ENGINE_TEST_WIN_TEST_WINTESTDLG_H_