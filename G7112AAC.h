#pragma once

#include "includes.h"

#define PCM_BUFFER_SIZE		20480

class CG7112AAC
{
public:
	CG7112AAC() = delete;
	CG7112AAC(int sample_rate, int channels, AVSampleFormat sample_fmt, AVCodecID codec_id);
	virtual ~CG7112AAC();

	void AddData(uint8_t *pData, int iSize);
	bool GetData(uint8_t* &pData, int *iSize);

private:
	bool InitDecoder();
	bool InitEncoder();
	void AddADTS(uint8_t* &pData, int packLen);
private:
	//pcm para
	int					m_sampleRate;
	AVSampleFormat		m_sampleFmt;
	int					m_channels;

	//g711 decoder
	AVCodecID			m_codecId;
	const AVCodec		*m_decoder;
	AVCodecContext		*m_deCtx;
	AVPacket			*m_dePkt;
	AVFrame				*m_deFrame;
	bool				m_initDecoder;
	bool				m_initDecoderOK;

	//aac encoder
	AVCodec				*m_encoder;
	AVCodecContext		*m_enCtx;
	AVPacket			*m_enPkt;
	AVFrame				*m_enFrame;
	bool				m_initEncoder;
	bool				m_initEncoderOK;
	SwrContext			*m_resampleCtx;
	int64_t				m_pts;
	uint8_t				*m_pcmPointer[8];

	//buffer
	uint8_t				*m_pcmBuffer;
	int					m_pcmSize;

	

	//uint8_t				m_pOutData[1024 * 10] = { 0 };

};

